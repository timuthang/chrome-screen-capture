#include "screen_capture_plugin.h"

#include <string.h>

#include "log.h"
#include "screen_capture_script_object.h"
#include "script_object_factory.h"

#ifdef _WINDOWS
#include <atlenc.h>
#include <windowsx.h>
#include <tchar.h>

#include "capture_window.h"
#include "resource.h"
#elif GTK
#include "capture_linux.h"
#include "key_binder.h"
#endif

extern Log g_logger;

int ScreenCapturePlugin::keycode_ = 0;

NPError ScreenCapturePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                   char* argn[],char* argv[], 
                                   NPSavedData* saved) {
  g_logger.WriteLog("msg", "ScreenCapturePlugin Init");
  script_object_ = NULL;

#ifdef _WINDOWS
  int bWindowed = 1;
  old_proc_ = NULL;
#else
  int bWindowed = 0;
#endif
  
#ifdef MAC
  // Select the right drawing model if necessary.
  NPBool support_core_graphics = false;
  if (NPN_GetValue(instance, NPNVsupportsCoreGraphicsBool,
                   &support_core_graphics) == NPERR_NO_ERROR && 
      support_core_graphics)
    NPN_SetValue(instance, NPPVpluginDrawingModel,
                 (void*)NPDrawingModelCoreGraphics);
  else
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  
  // Select the Cocoa event model.
  NPBool support_cocoa_events = false;
  if (NPN_GetValue(instance, NPNVsupportsCocoaBool,
                   &support_cocoa_events) == NPERR_NO_ERROR &&
      support_cocoa_events)
    NPN_SetValue(instance, NPPVpluginEventModel, 
                 (void*)NPEventModelCocoa);
  else
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
  
  NPN_SetValue(instance, NPPVpluginWindowBool, (void *)bWindowed);
  instance->pdata = this;
  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError ScreenCapturePlugin::UnInit(NPSavedData** save) {
  g_logger.WriteLog("msg", "ScreenCapturePlugin UnInit");
  PluginBase::UnInit(save);
  script_object_ = NULL;
  return NPERR_NO_ERROR;
}

NPError ScreenCapturePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (script_object_ == NULL)
        script_object_ = ScriptObjectFactory::CreateObject(
            get_npp(), ScreenCaptureScriptObject::Allocate);
      if (script_object_ != NULL)
        *(NPObject**)value = script_object_;
      else
        return NPERR_OUT_OF_MEMORY_ERROR;
      break;
    case NPPVpluginNeedsXEmbed:
      *(bool*)value = 1;
      break;
    default:
      return NPERR_GENERIC_ERROR;
  }
  return NPERR_NO_ERROR;
}

#ifdef _WINDOWS
INT_PTR CALLBACK Preview(HWND hDlg, UINT message, 
                         WPARAM wParam, LPARAM lParam) {
  POINT pt;
  CaptureWindow* capture = (CaptureWindow*)GetWindowLong(hDlg, GWL_USERDATA);
  switch (message) {
    case WM_INITDIALOG: {
      RECT rect;
      GetWindowRect(GetDesktopWindow(), &rect);
      SetWindowPos(hDlg, HWND_TOPMOST, 
                   rect.left, rect.top, rect.right - rect.left, 
                   rect.bottom - rect.top, SWP_SHOWWINDOW|SWP_NOREDRAW);
      CaptureWindow* capture = (CaptureWindow*)lParam;
      if (capture) {
        capture->SetWindowHandle(hDlg);
        SetWindowLong(hDlg, GWL_USERDATA, (LONG)capture);
        capture->OnPaint();
        SetTimer(hDlg, 1001, 3000, NULL);
      }
      return TRUE;
    }
    case WM_TIMER:
      if (wParam == 1001) {
        SetTimer(hDlg, 1001, 10, NULL);
        capture->OnPaint();
      }
      break;
    case WM_SETCURSOR:
      if (capture)
        capture->OnSetCursor();
      return TRUE;
    case WM_LBUTTONDOWN:
      pt.x = GET_X_LPARAM(lParam); 
      pt.y = GET_Y_LPARAM(lParam); 
      if (capture)
        capture->OnMouseDown(pt);
      break;
    case WM_LBUTTONUP:
      pt.x = GET_X_LPARAM(lParam); 
      pt.y = GET_Y_LPARAM(lParam); 
      if (capture)
        capture->OnMouseUp(pt);
      break;
    case WM_MOUSEMOVE:
      pt.x = GET_X_LPARAM(lParam); 
      pt.y = GET_Y_LPARAM(lParam); 
      if (capture)
        capture->OnMouseMove(pt);
      break;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
      }
      break;
  }
  return (INT_PTR)FALSE;
}

LRESULT ScreenCapturePlugin::WndProc(HWND hWnd, UINT message,
                                     WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_HOTKEY:
      if (HIWORD(lParam) == keycode_ && 
          LOWORD(lParam) == (MOD_CONTROL | MOD_ALT))
        PostMessage(hWnd, WM_CAPTURESCREEN, 0, 0);
      break;
    case WM_CAPTURESCREEN: {
      static bool capturing = false;
      if (capturing)
        break;
      capturing = true;
      CaptureWindow* capture = new CaptureWindow;
      capture->CaptureScreen();
      HINSTANCE module = GetModuleHandle(_T("screen_capture.dll"));
      INT_PTR ret = DialogBoxParam(module, MAKEINTRESOURCE(IDD_PREVIEW), 
                                   hWnd, Preview, (LPARAM)capture);
      capturing = false;
      if (ret == IDOK) {
        int image_data_len = 0;
        BYTE* image_data = capture->GetImageData(&image_data_len);
        if (image_data) {
          ScreenCapturePlugin* plugin = (ScreenCapturePlugin*)GetWindowLong(
              hWnd, GWLP_USERDATA);
          if (plugin)
            plugin->CaptureScreenCallback(image_data, image_data_len);
          capture->FreeImageData(image_data);
        }
      }
      delete capture;
      break;
    }
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return TRUE;
}
#endif

NPError ScreenCapturePlugin::SetWindow(NPWindow* window) {
#ifdef _WINDOWS
  if (window->window == NULL && old_proc_ != NULL) {
    SubclassWindow(get_native_window(), old_proc_);
    old_proc_ = NULL;
  }

  PluginBase::SetWindow(window);

  if (get_native_window() != NULL && old_proc_ == NULL) {
    old_proc_ = SubclassWindow(get_native_window(), WndProc);
    SetWindowLong(get_native_window(), GWLP_USERDATA, (LONG)this);
  }
#endif

  return NPERR_NO_ERROR;
}

#ifdef GTK
int MyErrorHandler(Display* display, XErrorEvent* error_event) {
  printf("%ld, type=%ld\n", display, error_event->type);
}

void* CaptureThread(void* param) {
  ScreenCapturePlugin* plugin = (ScreenCapturePlugin*)param;
  plugin->CaptureScreen();
  return 0;
}

void GrabKeyHandler(uint keycode, uint modifiers, void* user_data) {
  pthread_t thread_id = 0;
  pthread_create(&thread_id, NULL, CaptureThread, user_data);
}
#endif

void ScreenCapturePlugin::CaptureScreen() {
#ifdef _WINDOWS
  PostMessage(get_native_window(), WM_CAPTURESCREEN, 0, 0);
#elif GTK
  static bool capturing = false;
  if (capturing)
    return;
  capturing = true;
  XSetErrorHandler(MyErrorHandler);
  CaptureLinux capture;
  if (capture.CaptureScreen()) {
    if (capture.Show() == 1) {
      int image_data_len = 0;
      unsigned char* image_data = capture.GetImageData(&image_data_len);
      CaptureScreenCallback(image_data, image_data_len);
      capture.FreeImageData(image_data);
    }
  }
  capturing = false;
#endif
}

#ifdef _WINDOWS
void ScreenCapturePlugin::SetMessage(WCHAR* ok_caption, WCHAR* cancel_caption,
                                     WCHAR* tip_message) {
  CaptureWindow::SetMessage(ok_caption, cancel_caption, tip_message);
}
#elif GTK
void ScreenCapturePlugin::SetMessage(const char* ok_caption, 
                                     const char* cancel_caption,
                                     const char* tip_message) {
  CaptureLinux::SetMessage(ok_caption, cancel_caption, tip_message);
}
#endif

bool ScreenCapturePlugin::SetHotKey(int keycode) {
#ifdef _WINDOWS
  if (get_native_window()) {
    std::string hotkey = "CTRL+ALT+";
    ATOM atom;
    if (keycode_ != 0) {
      hotkey += keycode_;
      atom = GlobalFindAtomA(hotkey.c_str());
      UnregisterHotKey(get_native_window(), atom);
    }
    hotkey = "CTRL+ALT+";
    hotkey += keycode;
    atom = GlobalAddAtomA(hotkey.c_str());
    if (RegisterHotKey(get_native_window(), atom, 
                       MOD_CONTROL | MOD_ALT, keycode)) {
      keycode_ = keycode;
      return true;
    }
  }
#elif GTK
  Display* dpy = XOpenDisplay(NULL);
  KeyBinder::Binding binding = {0};
  binding.modifiers = ControlMask | Mod1Mask;
  binding.handler = GrabKeyHandler;
  binding.user_data = this;  
  printf("this = %ld\n", this);
  if (keycode_ != 0) {    
    binding.keycode = XKeysymToKeycode(dpy, keycode_);    
    KeyBinder::UnGrabKey(&binding);
  }
  binding.keycode = XKeysymToKeycode(dpy, keycode);
  XCloseDisplay(dpy);
  if (KeyBinder::GrabKey(&binding)) {
    keycode_ = keycode;
    printf("SetHotKey true!\n");
    return true;
  }
  printf("SetHotKey false!\n");  
#endif
  return false;
}

void ScreenCapturePlugin::DisableHotKey() {
#ifdef _WINDOWS
  if (get_native_window()) {
    std::string hotkey = "CTRL+ALT+";
    ATOM atom;
    if (keycode_ != 0) {
      hotkey += keycode_;
      atom = GlobalFindAtomA(hotkey.c_str());
      UnregisterHotKey(get_native_window(), atom);
      keycode_ = 0;
    }
  }
#elif GTK
  Display* dpy = XOpenDisplay(NULL);
  KeyBinder::Binding binding = {0};
  binding.modifiers = ControlMask | Mod1Mask;
  binding.handler = GrabKeyHandler;
  binding.user_data = this;  
  printf("this = %ld\n", this);
  if (keycode_ != 0) {
    binding.keycode = XKeysymToKeycode(dpy, keycode_);    
    KeyBinder::UnGrabKey(&binding);
    keycode_ = 0;
  }
#endif
}

void ScreenCapturePlugin::CaptureScreenCallback(unsigned char* image_data, 
                                                int image_data_len) {
  NPObject* window;
  NPN_GetValue(get_npp(), NPNVWindowNPObject, &window);

  NPVariant object;
  NPVariant ret;
  std::string image;
#ifdef _WINDOWS
  int dest_len = Base64EncodeGetRequiredLength(image_data_len);
  char* base64 = (char*)malloc(dest_len + 1);
  if (base64) {
    Base64Encode(image_data, image_data_len, base64, 
                 &dest_len, ATL_BASE64_FLAG_NOCRLF);
    base64[dest_len] = 0;
    image = base64;
    free(base64);
  }
#elif GTK
  char* base64 = g_base64_encode(image_data, image_data_len);
  if (base64) {
    image += base64;
    g_free(base64);
  }  
#endif
  NPVariant params;
  STRINGZ_TO_NPVARIANT(image.c_str(), params);

  NPIdentifier id = NPN_GetStringIdentifier("screenshot");
  if (!id)
    return;
  NPN_GetProperty(get_npp(), window, id, &object);
  id = NPN_GetStringIdentifier("captureScreenCallback");
  if (!id)
    return;
  NPN_Invoke(get_npp(), object.value.objectValue, id, &params, 1, &ret);
  NPN_ReleaseVariantValue(&ret);
  NPN_ReleaseVariantValue(&object);
}