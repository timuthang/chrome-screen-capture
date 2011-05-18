#include "screen_capture_plugin.h"

#include "capture_window.h"
#include "log.h"
#include "screen_capture_script_object.h"
#include "script_object_factory.h"

#ifdef _WINDOWS
#include <atlenc.h>
#include <windowsx.h>
#include <tchar.h>

#include "resource.h"
#endif

extern Log g_logger;

#ifdef _WINDOWS
int ScreenCapturePlugin::keycode_ = 0;
NativeWindow ScreenCapturePlugin::hotkey_window_ = NULL;
#endif

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
      }
      return TRUE;
    }
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
      CaptureWindow* capture = new CaptureWindow;
      capture->CaptureScreen();
      HINSTANCE module = GetModuleHandle(_T("screen_capture.dll"));
      INT_PTR ret = DialogBoxParam(module, MAKEINTRESOURCE(IDD_PREVIEW), 
                                   hWnd, Preview, (LPARAM)capture);
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
  if (hotkey_window_ == NULL && window->window != NULL) {
    hotkey_window_ = (NativeWindow)window->window;
  }

  if (window->window == NULL && old_proc_ != NULL) {
    SubclassWindow(get_native_window(), old_proc_);
    old_proc_ = NULL;
    if (get_native_window() == hotkey_window_)
      hotkey_window_ = NULL;
  }

  PluginBase::SetWindow(window);

  if (get_native_window() != NULL && old_proc_ == NULL) {
    old_proc_ = SubclassWindow(get_native_window(), WndProc);
    SetWindowLong(get_native_window(), GWLP_USERDATA, (LONG)this);
  }
#endif

  return NPERR_NO_ERROR;
}

void ScreenCapturePlugin::CaptureScreen() {
#ifdef _WINDOWS
  PostMessage(get_native_window(), WM_CAPTURESCREEN, 0, 0);
#endif
}

#ifdef _WINDOWS
void ScreenCapturePlugin::SetButtonMessage(WCHAR* ok_caption, 
                                           WCHAR* cancel_caption) {
  CaptureWindow::SetButtonMessage(ok_caption, cancel_caption);
}

bool ScreenCapturePlugin::SetHotKey(int keycode) {
  if (hotkey_window_) {
    std::string hotkey = "CTRL+ALT+";
    ATOM atom;
    if (keycode_ != 0) {
      hotkey += keycode_;
      atom = GlobalFindAtomA(hotkey.c_str());
      UnregisterHotKey(hotkey_window_, atom);
    }
    hotkey += keycode;
    atom = GlobalAddAtomA(hotkey.c_str());
    if (RegisterHotKey(hotkey_window_, atom, MOD_CONTROL | MOD_ALT, keycode)) {
      keycode_ = keycode;
      return true;
    }
  }
  return false;
}
#endif

void ScreenCapturePlugin::CaptureScreenCallback(BYTE* image_data, 
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