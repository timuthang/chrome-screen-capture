#ifndef CAPTURE_WINDOW_H_
#define CAPTURE_WINDOW_H_

#include <windows.h>
#include <string>

class CaptureWindow {
public:
  CaptureWindow(void);
  ~CaptureWindow(void);

  bool CaptureScreen();
  void SetWindowHandle(HWND hwnd) { hwnd_ = hwnd; }

  enum CaptureState {
    kNoCapture,
    kStartSelectRegion,
    kSelectingRegion,
    kFinishSelectRegion,
    kStartMoveRegion,
    kMovingRegion,
    kFinishMoveRegion,
    kStartResizeRegion,
    kResizingRegion,
    kFinishResizeRegion,
  };

  // Dispose mouse down message.
  void OnMouseDown(POINT pt);
  void OnMouseUp(POINT pt);
  void OnMouseMove(POINT pt);
  void OnSetCursor() { SetCursor(display_cursor_); }
  void OnPaint();

  static void SetButtonMessage(WCHAR* ok_message, WCHAR* cancel_message);

  // Image data functions.
  BYTE* GetImageData(int* len);
  void FreeImageData(BYTE* data) { if (data) free(data); }

private:
  bool Init();
  void UnInit();
  void DrawSelectRegion(POINT pt, bool compulte_select_region = true);
  void MoveSelectRegion(POINT pt);

private:
  HWND hwnd_;
  HDC original_picture_dc_;
  HBITMAP original_bmp_;
  HDC mem_dc_;
  HBITMAP mem_bmp_;
  RECT selected_rect_;
  RECT left_top_corner_;
  RECT right_top_corner_;
  RECT left_bottom_corner_;
  RECT right_bottom_corner_;
  RECT ok_button_rect_;
  RECT cancel_button_rect_;
  HCURSOR display_cursor_;
  POINT start_pt_;
  CaptureState state_;
  int window_width_;
  int window_height_;

  static std::wstring ok_caption_;
  static std::wstring cancel_caption_;
};

#endif