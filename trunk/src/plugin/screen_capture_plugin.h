#ifndef SCREEN_CAPTURE_PLUGIN_H_
#define SCREEN_CAPTURE_PLUGIN_H_

#include "npapi.h"
#include "npruntime.h"
#include "npfunctions.h"
#include "plugin_base.h"
#include "script_object_base.h"

#ifdef _WINDOWS
#include <windows.h>
#define WM_CAPTURESCREEN  WM_USER+1001
#endif

class ScreenCapturePlugin : public PluginBase {
public:
  ScreenCapturePlugin() {}
  virtual ~ScreenCapturePlugin() {}
    
  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** saved);
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetWindow(NPWindow* window);

  static PluginBase* CreateObject() { return new ScreenCapturePlugin; }

  void CaptureScreen();
  void CaptureScreenCallback(unsigned char* image_data, int image_data_len);
  bool SetHotKey(int keycode);
  void DisableHotKey();

#ifdef _WINDOWS
  void SetButtonMessage(WCHAR* ok_caption, WCHAR* cancel_caption);  
  static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#elif GTK
  void SetButtonMessage(const char* ok_caption, const char* cancel_caption);  
#endif

private:
  ScriptObjectBase* script_object_;
  static int keycode_;
#ifdef _WINDOWS
  WNDPROC old_proc_;  
#endif
};

#endif // SCREEN_CAPTURE_PLUGIN_H_
