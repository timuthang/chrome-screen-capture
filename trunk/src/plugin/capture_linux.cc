#include "capture_linux.h"

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>
#include <X11/keysym.h>

std::string CaptureLinux::ok_caption_ = "OK";
std::string CaptureLinux::cancel_caption_ = "Cancel";

bool PtInRect(XRectangle* rect, XPoint pt) {
  if (pt.x >= rect->x && pt.x <= rect->x + rect->width &&
      pt.y >= rect->y && pt.y <= rect->y + rect->height)
    return true;
  return false;
}

typedef struct ImageData {
  unsigned char* image_data;  
  int image_data_len;
  int image_max_length;
};

CaptureLinux::CaptureLinux() {
  original_image_= NULL;
  mask_image_ = NULL;
  window_ = 0;  
  mem_pixmap_ = 0;
  fontset_ = NULL;
  memset(&selected_rect_, 0, sizeof(selected_rect_));
  state_ = kNoCapture;
}

CaptureLinux::~CaptureLinux() {
  UnInit();
}

cairo_format_t CaptureLinux::get_cairo_format(int depth) {
  cairo_format_t format = (cairo_format_t)-1;
  switch (depth) {
    case 1:
      format = CAIRO_FORMAT_A1;
      break;
    case 8:
      format = CAIRO_FORMAT_A8;
      break;
    case 24:
      format = CAIRO_FORMAT_RGB24;
      break;
    case 32:
      format = CAIRO_FORMAT_ARGB32;
      break;
  }
  
  return format;
}

bool CaptureLinux::Init() {
  display_ = XOpenDisplay(0);
  if (!display_)
    return false;

  char **missing_charsets;
  int num_missing_charsets = 0;
  char *default_string;
  int i;
  
  fontset_ = XCreateFontSet(display_,
                           "*",
                           &missing_charsets, &num_missing_charsets,
                           &default_string); 
  
  if (!fontset_)
    return false;  
  
  window_width_ = XWidthOfScreen(DefaultScreenOfDisplay(display_));
  window_height_ = XHeightOfScreen(DefaultScreenOfDisplay(display_));
  
  original_image_ = XGetImage(
      display_, DefaultRootWindow(display_), 0, 0, window_width_, 
      window_height_, AllPlanes, ZPixmap);
  mask_image_ = XGetImage(
      display_, DefaultRootWindow(display_), 0, 0, window_width_, 
      window_height_, AllPlanes, ZPixmap); 
  int blackColor = BlackPixel(display_, DefaultScreen(display_));  
  XSetWindowAttributes attr;  
  unsigned long attrmask = CWEventMask | CWBackPixel | CWOverrideRedirect;  
  attr.override_redirect = True;    
  attr.background_pixel = blackColor;
  attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | 
      ButtonReleaseMask | PointerMotionMask | ExposureMask;
  window_ = XCreateWindow(
      display_, DefaultRootWindow(display_), 0, 0, window_width_, 
      window_height_, 0, DefaultDepth(display_, DefaultScreen(display_)), 
      InputOutput, DefaultVisual(display_, 0), attrmask, &attr);  

  mem_pixmap_ = XCreatePixmap(
      display_, window_, window_width_, window_height_, original_image_->depth);
  cairo_format_t format = get_cairo_format(mask_image_->depth);
  cairo_surface_t* image_surface = cairo_image_surface_create_for_data(
      (unsigned char*)mask_image_->data, format,
      window_width_, window_height_, 
      cairo_format_stride_for_width(format, window_width_));
  cairo_t* cairo = cairo_create(image_surface);
  cairo_paint_with_alpha(cairo, 0.5);
  cairo_destroy(cairo);
  cairo_surface_destroy(image_surface);
  
  if (original_image_ && mask_image_)
    return true;
  else
    return false;
}

void CaptureLinux::UnInit() {    
  if (mem_pixmap_)
    XFreePixmap(display_, mem_pixmap_);

  if (original_image_)
    XDestroyImage(original_image_);
  if (mask_image_)
    XDestroyImage(mask_image_);
  
  if (fontset_)
    XFreeFontSet(display_, fontset_);
  
  if (window_) {
    XUnmapWindow(display_, window_);
    XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
  }  

  window_ = 0;
  mem_pixmap_ = 0;
  original_image_ = NULL;
  mask_image_ = NULL;
  memset(&selected_rect_, 0, sizeof(selected_rect_));
  state_ = kNoCapture;
}

bool CaptureLinux::CaptureScreen() {
  UnInit();
  if (!Init())
    return false;
  return true;
}

int CaptureLinux::Show() {  
  XMapRaised(display_, window_);
  XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
  
  XPoint pt;
  XEvent e;
  while (true) {    
    XNextEvent(display_, &e);    
    switch(e.type) {
      case KeyPress:        
        if (e.xkey.keycode == XKeysymToKeycode(display_, XK_Escape))
          return 0;
        if (e.xkey.keycode == XKeysymToKeycode(display_, XK_Return)) {
          return 1;
        }        
        break;
      case KeyRelease:        
        break;
      case ButtonPress:
        XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
        pt.x = e.xbutton.x;
        pt.y = e.xbutton.y;
        OnMouseDown(pt);
        break;
      case ButtonRelease:
        pt.x = e.xbutton.x;
        pt.y = e.xbutton.y;
        OnMouseUp(pt);
        break;
      case MotionNotify:
        pt.x = e.xmotion.x;
        pt.y = e.xmotion.y;
        if (state_ == kMovingRegion) {
          if (e.xmotion.state & Button1Mask)
            OnMouseMove(pt);
        } else {
          OnMouseMove(pt);
        }
        break;
      case Expose:
        XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
        display_cursor_ = XCreateFontCursor(display_, XC_arrow);
        SetCursor();
        OnPaint();
        break;
      default:
        break;
    }
  }  
}

void CaptureLinux::OnMouseDown(XPoint pt) {
  switch (state_) {
    case kNoCapture:
      start_pt_ = pt;
      state_ = kStartSelectRegion;
      break;
    case kFinishResizeRegion:
    case kFinishSelectRegion:
    case kFinishMoveRegion:
      if (PtInRect(&left_top_corner_, pt)) {
        state_ = kStartResizeRegion;
        start_pt_.x = selected_rect_.x + selected_rect_.width;
        start_pt_.y = selected_rect_.y + selected_rect_.height;
      } else if (PtInRect(&right_bottom_corner_, pt)) {
        start_pt_.x = selected_rect_.x;
        start_pt_.y = selected_rect_.y;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&right_top_corner_, pt)) {
        start_pt_.x = selected_rect_.x;
        start_pt_.y = selected_rect_.y + selected_rect_.height;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&left_bottom_corner_, pt)) {
        start_pt_.x = selected_rect_.x + selected_rect_.width;
        start_pt_.y = selected_rect_.y;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&ok_button_rect_, pt)) {
        XEvent evt;
        evt.type = KeyPress;
        evt.xkey.keycode = XKeysymToKeycode(display_, XK_Return);
        XSendEvent(display_, window_, False, KeyPressMask, &evt);
      } else if (PtInRect(&cancel_button_rect_, pt)) {
        XEvent evt;
        evt.type = KeyPress;
        evt.xkey.keycode = XKeysymToKeycode(display_, XK_Escape);
        XSendEvent(display_, window_, False, KeyPressMask, &evt);
      } else if (PtInRect(&selected_rect_, pt)) {
        start_pt_ = pt;
        state_ = kStartMoveRegion;
      } else {
        start_pt_ = pt;
        state_ = kStartSelectRegion;
        memset(&selected_rect_, 0, sizeof(selected_rect_));
        DrawSelectRegion(pt, false);
      }
      break;
    default:
      break;
  }
}

void CaptureLinux::OnMouseUp(XPoint pt) {
  switch (state_) {
    case kStartSelectRegion:
    case kSelectingRegion:
      if (pt.x != start_pt_.x || pt.y != start_pt_.y)
        state_ = kFinishSelectRegion;
      else
        state_ = kNoCapture;
      break;
    case kStartMoveRegion:
    case kMovingRegion:
      state_ = kFinishMoveRegion;
      break;
    case kStartResizeRegion:
    case kResizingRegion:
      state_ = kFinishResizeRegion;
      break;
  }
}

void CaptureLinux::OnPaint() {
  XPoint pt = {0};
  DrawSelectRegion(pt, false);
}

void CaptureLinux::SetButtonMessage(const char* ok_message, 
                                    const char* cancel_message) {
  ok_caption_ = ok_message;
  cancel_caption_ = cancel_message;
}

void CaptureLinux::OnMouseMove(XPoint pt) {
  switch (state_) {
    case kStartSelectRegion:
    case kSelectingRegion:
      DrawSelectRegion(pt);
      state_ = kSelectingRegion;
      break;
    case kStartMoveRegion:
    case kMovingRegion:
      XFreeCursor(display_, display_cursor_);
      display_cursor_ = XCreateFontCursor(display_, XC_hand1);
      SetCursor();
      MoveSelectRegion(pt);
      state_ = kMovingRegion;
      break;
    case kStartResizeRegion:
    case kResizingRegion:
      if (PtInRect(&left_top_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_top_left_corner);
        SetCursor();
      } else if (PtInRect(&right_bottom_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_bottom_right_corner);
        SetCursor();
      } else if (PtInRect(&right_top_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_top_right_corner);
        SetCursor();
      } else if (PtInRect(&left_bottom_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_bottom_left_corner);
        SetCursor();
      }
      DrawSelectRegion(pt);
      state_ = kResizingRegion;
      break;
    case kFinishResizeRegion:
    case kFinishMoveRegion:
    case kFinishSelectRegion:
      if (PtInRect(&left_top_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_top_left_corner);
        SetCursor();
      } else if (PtInRect(&right_bottom_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_bottom_right_corner);
        SetCursor();
      } else if (PtInRect(&right_top_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_top_right_corner);
        SetCursor();
      } else if (PtInRect(&left_bottom_corner_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_bottom_left_corner);
        SetCursor();
      } else if (PtInRect(&cancel_button_rect_, pt) || 
                 PtInRect(&ok_button_rect_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_hand2);
        SetCursor();
      } else if (PtInRect(&selected_rect_, pt)) {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_hand1);
        SetCursor();
      } else {
        XFreeCursor(display_, display_cursor_);
        display_cursor_ = XCreateFontCursor(display_, XC_arrow);
        SetCursor();
      }
      break;
  }
}

void CaptureLinux::DrawSelectRegion(XPoint pt, 
                                          bool compulte_select_region) {
  XRectangle rect = {0, 0, window_width_, window_height_};

  if (compulte_select_region) {
    selected_rect_.x = (start_pt_.x < pt.x ? start_pt_.x : pt.x);
    selected_rect_.y = (start_pt_.y < pt.y ? start_pt_.y : pt.y);
    selected_rect_.width = abs(start_pt_.x - pt.x);
    selected_rect_.height = abs(start_pt_.y - pt.y);
  }

  GC gc = XCreateGC(display_, mem_pixmap_, 0, 0);

  XPutImage(display_, mem_pixmap_, gc, mask_image_, 
            0, 0, 0, 0, window_width_, window_height_);
  XPutImage(display_, mem_pixmap_, gc, original_image_, 
            selected_rect_.x, selected_rect_.y, 
            selected_rect_.x, selected_rect_.y, 
            selected_rect_.width, selected_rect_.height);
  
  XSetForeground(display_, gc, 0x0000FF);

  // Draw border and corner.
  XDrawRectangle(display_, mem_pixmap_, gc, 
                 selected_rect_.x, selected_rect_.y, 
                 selected_rect_.width, selected_rect_.height);

  // Draw left top corner.
  left_top_corner_.x = selected_rect_.x - 2;
  left_top_corner_.y = selected_rect_.y - 2;
  left_top_corner_.width = 5;
  left_top_corner_.height = 5;
  XFillRectangle(display_, mem_pixmap_, gc, 
                 left_top_corner_.x, left_top_corner_.y, 
                 left_top_corner_.width, left_top_corner_.height);

  // Draw right top corner.
  right_top_corner_.width = 5;
  right_top_corner_.height = 5;
  right_top_corner_.y = selected_rect_.y - 2;
  right_top_corner_.x = selected_rect_.x + selected_rect_.width - 2;  
  XFillRectangle(display_, mem_pixmap_, gc, 
                 right_top_corner_.x, right_top_corner_.y, 
                 right_top_corner_.width, right_top_corner_.height);

  // Draw left bottom corner.
  left_bottom_corner_.width = 5;
  left_bottom_corner_.height = 5;
  left_bottom_corner_.x = selected_rect_.x - 2;  
  left_bottom_corner_.y = selected_rect_.y + selected_rect_.height - 2;  
  XFillRectangle(display_, mem_pixmap_, gc, 
                 left_bottom_corner_.x, left_bottom_corner_.y, 
                 left_bottom_corner_.width, left_bottom_corner_.height);

  // Draw right bottom corner.
  right_bottom_corner_.width = 5;
  right_bottom_corner_.height = 5;  
  right_bottom_corner_.x = selected_rect_.x + selected_rect_.width - 2;  
  right_bottom_corner_.y = selected_rect_.y + selected_rect_.height - 2;
  XFillRectangle(display_, mem_pixmap_, gc, 
                 right_bottom_corner_.x, right_bottom_corner_.y, 
                 right_bottom_corner_.width, right_bottom_corner_.height);

  if (selected_rect_.width && selected_rect_.height) {
    XRectangle overall_ink_return, overall_logical_return;
    int ok_width, cancel_width;
    
    // Draw tip message.    
    char message[100];
    sprintf(message, " %ld x %ld ", 
            selected_rect_.width, selected_rect_.height);
    
    Xutf8TextExtents(fontset_, message, strlen(message),
                     &overall_ink_return, &overall_logical_return);
    
    rect.x = selected_rect_.x;
    rect.width = overall_logical_return.width;
    rect.height = overall_logical_return.height;
    rect.y = selected_rect_.y - rect.height;    

    if (rect.y < 0) {
      rect.y = 0;
    }
    if (rect.x + rect.width > window_width_) {      
      rect.x = window_width_ - rect.width;
    }
    
    XSetBackground(display_, gc, 0);    
    XSetForeground(display_, gc, 0);
    XFillRectangle(display_, mem_pixmap_, gc, 
                   rect.x, rect.y, rect.width, rect.height);
    XSetForeground(display_, gc, 0xFFFFFF);
    Xutf8DrawString(display_, mem_pixmap_, fontset_, gc, 
                    rect.x, rect.y + rect.height - 2, message, strlen(message));

    // Draw button
    int button_width = 0;
    int button_height = 0;
    Xutf8TextExtents(fontset_, ok_caption_.c_str(), ok_caption_.length(), 
                     &overall_ink_return, &overall_logical_return);
    ok_width = overall_logical_return.width;
    
    button_width = overall_logical_return.width;
    button_height = overall_logical_return.height;
    Xutf8TextExtents(fontset_, cancel_caption_.c_str(), 
                     cancel_caption_.length(), 
                     &overall_ink_return, &overall_logical_return);
    cancel_width = overall_logical_return.width;
    
    if (button_width < overall_logical_return.width)
      button_width = overall_logical_return.width;
    if (button_height < overall_logical_return.height)
      button_height = overall_logical_return.height;

    button_width += 8;  // Add 8 pix.
    button_height += 4;

    ok_button_rect_.width = button_width;
    ok_button_rect_.height = button_height;
    ok_button_rect_.x = selected_rect_.x + selected_rect_.width - button_width;
    if (selected_rect_.y + selected_rect_.height + 
        button_height > window_height_) {  
      ok_button_rect_.y = window_height_ - button_height;
    } else {
      ok_button_rect_.y = selected_rect_.y + selected_rect_.height;      
    }

    // Create black brush.
    XSetForeground(display_, gc, 0);
    XFillRectangle(display_, mem_pixmap_, gc, 
                   ok_button_rect_.x, ok_button_rect_.y, 
                   ok_button_rect_.width, ok_button_rect_.height);
    XSetForeground(display_, gc, 0xFFFFFF);
    Xutf8DrawString(display_, mem_pixmap_, fontset_, gc, 
                    ok_button_rect_.x + (button_width - ok_width) / 2, 
                    ok_button_rect_.y + button_height - 4, 
                    ok_caption_.c_str(), ok_caption_.length());

    cancel_button_rect_.width = button_width;
    cancel_button_rect_.height = button_height;    
    cancel_button_rect_.y = ok_button_rect_.y;
    cancel_button_rect_.x = ok_button_rect_.x - button_width - 10;
    XSetForeground(display_, gc, 0);
    XFillRectangle(display_, mem_pixmap_, gc, 
                   cancel_button_rect_.x, cancel_button_rect_.y, 
                   cancel_button_rect_.width, cancel_button_rect_.height);
    XSetForeground(display_, gc, 0xFFFFFF);
    Xutf8DrawString(display_, mem_pixmap_, fontset_, gc, 
                    cancel_button_rect_.x + (button_width - cancel_width) / 2, 
                    cancel_button_rect_.y + button_height - 4, 
                    cancel_caption_.c_str(), cancel_caption_.length());
  }
  XCopyArea(display_, mem_pixmap_, window_, gc, 
            0, 0, window_width_, window_height_, 0, 0);
  XFreeGC(display_, gc);
  
  XEvent event_return;
  XFlush(display_);  
  if (!XCheckTypedWindowEvent(display_, window_, ButtonRelease, &event_return))
    XSync(display_, True);
  else {
    XSync(display_, True);
    XSendEvent(display_, window_, False, ButtonReleaseMask, &event_return);    
  }
}

void CaptureLinux::MoveSelectRegion(XPoint pt) {
  XPoint offset = {pt.x - start_pt_.x, pt.y - start_pt_.y};

  start_pt_ = pt;
  selected_rect_.x += offset.x;
  selected_rect_.y += offset.y;
  if (selected_rect_.x < 0) {
    selected_rect_.x = 0;
  }
  if (selected_rect_.x + selected_rect_.width > window_width_) {
    selected_rect_.x = window_width_ - selected_rect_.width;
  }
  if (selected_rect_.y < 0) {
    selected_rect_.y = 0;
  }
  if (selected_rect_.y + selected_rect_.height > window_height_) {
    selected_rect_.y = window_height_ - selected_rect_.height;
  }

  DrawSelectRegion(pt, false);
}

cairo_status_t WritePNGStream(void *closure, const unsigned char *data, 
                              unsigned int length) {
  ImageData* imagedata = (ImageData*)closure;
  if (imagedata->image_max_length < imagedata->image_data_len + length) {
    imagedata->image_max_length += 1024 * 1024;
    imagedata->image_data = (unsigned char*)realloc(
        imagedata->image_data, imagedata->image_max_length);
    if (!imagedata->image_data)
      return CAIRO_STATUS_NO_MEMORY;
  }
  
  memcpy(imagedata->image_data + imagedata->image_data_len,
         data, length);
  imagedata->image_data_len += length;  
  return CAIRO_STATUS_SUCCESS;
}

unsigned char* CaptureLinux::GetImageData(int* len) {
  GC gc = XCreateGC(display_, mem_pixmap_, 0, 0);
  XPutImage(display_, mem_pixmap_, gc, original_image_, 
            selected_rect_.x, selected_rect_.y, 
            selected_rect_.x, selected_rect_.y, 
            selected_rect_.width, selected_rect_.height);  
  XFreeGC(display_, gc);
  XImage* image = XGetImage(
      display_, mem_pixmap_, selected_rect_.x, selected_rect_.y, 
      selected_rect_.width, selected_rect_.height, AllPlanes, ZPixmap);
  cairo_format_t format = get_cairo_format(image->depth);
  cairo_surface_t* image_surface = cairo_image_surface_create_for_data(
      (unsigned char*)image->data, format,
      selected_rect_.width, selected_rect_.height, 
      cairo_format_stride_for_width(format, selected_rect_.width));
  ImageData imagedata = {0};
  imagedata.image_max_length = 1024 * 1024;
  imagedata.image_data = (unsigned char*)malloc(imagedata.image_max_length);
  cairo_status_t status = cairo_surface_write_to_png_stream(
      image_surface, WritePNGStream, &imagedata);
  cairo_surface_destroy(image_surface);
  XDestroyImage(image);
  *len = imagedata.image_data_len;
  if (status == CAIRO_STATUS_SUCCESS)
    return imagedata.image_data;
  else
    return NULL;
}
