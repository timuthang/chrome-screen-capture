#ifndef CAPTURE_LINUX_H_
#define CAPTURE_LINUX_H_

#include <stdlib.h>
#include <string>

#include <cairo/cairo.h>
#include <X11/Xlib.h>

class CaptureLinux {
public:
  CaptureLinux(void);
  ~CaptureLinux(void);

  bool CaptureScreen();
  int Show();

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
  void OnMouseDown(XPoint pt);
  void OnMouseUp(XPoint pt);
  void OnMouseMove(XPoint pt);  
  void OnPaint();

  void SetCursor() { XDefineCursor(display_, window_, display_cursor_); }
  
  static void SetButtonMessage(const char* ok_message, 
                               const char* cancel_message);

  // Image data functions.
  unsigned char* GetImageData(int* len);
  void FreeImageData(unsigned char* data) { if (data) free(data); }

private:
  bool Init();
  void UnInit();
  void DrawSelectRegion(XPoint pt, bool compulte_select_region = true);
  void MoveSelectRegion(XPoint pt);
  cairo_format_t get_cairo_format(int dapth);  

private:
  Display* display_;
  GC gc;
  Window window_;
  Pixmap mem_pixmap_;
  XFontSet fontset_;
  XImage* original_image_;
  XImage* mask_image_;
  XRectangle selected_rect_;
  XRectangle left_top_corner_;
  XRectangle right_top_corner_;
  XRectangle left_bottom_corner_;
  XRectangle right_bottom_corner_;
  XRectangle ok_button_rect_;
  XRectangle cancel_button_rect_;
  Cursor display_cursor_;
  XPoint start_pt_;
  CaptureState state_;
  int window_width_;
  int window_height_;

  static std::string ok_caption_;
  static std::string cancel_caption_;
};

#endif