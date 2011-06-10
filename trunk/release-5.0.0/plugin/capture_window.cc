#include "capture_window.h"

#include <CommDlg.h>
#include <stdio.h>
#include <tchar.h>

std::wstring CaptureWindow::ok_caption_ = L"Ok";
std::wstring CaptureWindow::cancel_caption_ = L"Cancel";

CaptureWindow::CaptureWindow() {
  original_picture_dc_ = NULL;
  original_bmp_ = NULL;
  mem_bmp_ = NULL;
  mem_dc_ = NULL;
  memset(&selected_rect_, 0, sizeof(selected_rect_));
  state_ = kNoCapture;
}

CaptureWindow::~CaptureWindow() {
  UnInit();
}

bool CaptureWindow::Init() {
  RECT rect = {0};
  HWND desktop_hwnd = GetDesktopWindow();
  GetWindowRect(desktop_hwnd, &rect);
  window_width_ = rect.right - rect.left;
  window_height_ = rect.bottom - rect.top;

  HDC desktop_dc = GetDC(desktop_hwnd);
  mem_bmp_ = CreateCompatibleBitmap(desktop_dc, window_width_, window_height_);
  original_bmp_ = CreateCompatibleBitmap(desktop_dc, window_width_, 
                                         window_height_);
  mem_dc_ = CreateCompatibleDC(desktop_dc);
  original_picture_dc_ = CreateCompatibleDC(desktop_dc);
  SelectObject(mem_dc_, mem_bmp_);
  SelectObject(original_picture_dc_, original_bmp_);
  ReleaseDC(desktop_hwnd, desktop_dc);

  display_cursor_ = LoadCursor(NULL, IDC_ARROW);

  if (mem_bmp_ && mem_dc_ && original_bmp_ && original_picture_dc_)
    return true;
  else
    return false;
}

void CaptureWindow::UnInit() {
  if (original_bmp_)
    DeleteObject(original_bmp_);
  if (original_picture_dc_)
    DeleteDC(original_picture_dc_);
  if (mem_bmp_)
    DeleteObject(mem_bmp_);
  if (mem_dc_)
    DeleteDC(mem_dc_);
  original_picture_dc_ = NULL;
  original_bmp_ = NULL;
  mem_bmp_ = NULL;
  mem_dc_ = NULL;
  memset(&selected_rect_, 0, sizeof(selected_rect_));
  state_ = kNoCapture;
}

bool CaptureWindow::CaptureScreen() {
  UnInit();
  if (!Init())
    return false;
  HDC screen_dc = GetDC(NULL);
  BOOL ret = BitBlt(original_picture_dc_, 0, 0, window_width_, window_height_,
                    screen_dc, 0, 0, SRCCOPY);
  ReleaseDC(NULL, screen_dc);
  return ret;
}

void CaptureWindow::OnMouseDown(POINT pt) {
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
        start_pt_.x = selected_rect_.right;
        start_pt_.y = selected_rect_.bottom;
      } else if (PtInRect(&right_bottom_corner_, pt)) {
        start_pt_.x = selected_rect_.left;
        start_pt_.y = selected_rect_.top;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&right_top_corner_, pt)) {
        start_pt_.x = selected_rect_.left;
        start_pt_.y = selected_rect_.bottom;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&left_bottom_corner_, pt)) {
        start_pt_.x = selected_rect_.right;
        start_pt_.y = selected_rect_.top;
        state_ = kStartResizeRegion;
      } else if (PtInRect(&ok_button_rect_, pt)) {
        SendMessage(hwnd_, WM_COMMAND, IDOK, 0);
      } else if (PtInRect(&cancel_button_rect_, pt)) {
        SendMessage(hwnd_, WM_COMMAND, IDCANCEL, 0);
      } else if (PtInRect(&selected_rect_, pt)) {
        start_pt_ = pt;
        state_ = kStartMoveRegion;
      } else {
        start_pt_ = pt;
        state_ = kStartSelectRegion;
        SetRectEmpty(&selected_rect_);
        DrawSelectRegion(pt, false);
      }
      break;
    default:
      break;
  }
}

void CaptureWindow::OnMouseUp(POINT pt) {
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

void CaptureWindow::OnPaint() {
  POINT pt = {0};
  DrawSelectRegion(pt, false);
}

void CaptureWindow::SetButtonMessage(WCHAR* ok_message, 
                                     WCHAR* cancel_message) {
  ok_caption_ = ok_message;
  cancel_caption_ = cancel_message;
}

void CaptureWindow::OnMouseMove(POINT pt) {
  switch (state_) {
    case kStartSelectRegion:
    case kSelectingRegion:
      DrawSelectRegion(pt);
      state_ = kSelectingRegion;
      break;
    case kStartMoveRegion:
    case kMovingRegion:
      display_cursor_ = LoadCursor(NULL, IDC_SIZEALL);
      SetCursor(display_cursor_);
      MoveSelectRegion(pt);
      state_ = kMovingRegion;
      break;
    case kStartResizeRegion:
    case kResizingRegion:
      if (PtInRect(&left_top_corner_, pt) || 
          PtInRect(&right_bottom_corner_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_SIZENWSE);
        SetCursor(display_cursor_);
      } else if (PtInRect(&right_top_corner_, pt) || 
                 PtInRect(&left_bottom_corner_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_SIZENESW);
        SetCursor(display_cursor_);
      }
      DrawSelectRegion(pt);
      state_ = kResizingRegion;
      break;
    case kFinishResizeRegion:
    case kFinishMoveRegion:
    case kFinishSelectRegion:
      if (PtInRect(&left_top_corner_, pt) || 
          PtInRect(&right_bottom_corner_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_SIZENWSE);
        SetCursor(display_cursor_);
      } else if (PtInRect(&right_top_corner_, pt) || 
                 PtInRect(&left_bottom_corner_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_SIZENESW);
        SetCursor(display_cursor_);
      } else if (PtInRect(&cancel_button_rect_, pt) || 
                 PtInRect(&ok_button_rect_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_HAND);
        SetCursor(display_cursor_);
      } else if (PtInRect(&selected_rect_, pt)) {
        display_cursor_ = LoadCursor(NULL, IDC_SIZEALL);
        SetCursor(display_cursor_);
      } else {
        display_cursor_ = LoadCursor(NULL, IDC_ARROW);
        SetCursor(display_cursor_);
      }
      break;
  }
}

void CaptureWindow::DrawSelectRegion(POINT pt, bool compulte_select_region) {
  RECT rect = {0, 0, window_width_, window_height_};

  BLENDFUNCTION bm;
  bm.BlendOp = AC_SRC_OVER;
  bm.BlendFlags = 0;
  bm.SourceConstantAlpha = 200;
  bm.AlphaFormat = 0; 
  FillRect(mem_dc_, &rect, CreateSolidBrush(RGB(0, 0, 0)));
  AlphaBlend(mem_dc_, 0, 0, window_width_, window_height_, 
             original_picture_dc_, 0, 0, window_width_, window_height_, bm);

  if (compulte_select_region) {
    selected_rect_.left = (start_pt_.x < pt.x ? start_pt_.x : pt.x);
    selected_rect_.top = (start_pt_.y < pt.y ? start_pt_.y : pt.y);
    selected_rect_.right = selected_rect_.left + abs(start_pt_.x - pt.x);
    selected_rect_.bottom = selected_rect_.top + abs(start_pt_.y - pt.y);
  }

  BitBlt(mem_dc_, selected_rect_.left, selected_rect_.top, 
         selected_rect_.right - selected_rect_.left, 
         selected_rect_.bottom - selected_rect_.top, 
         original_picture_dc_, selected_rect_.left, selected_rect_.top, 
         SRCCOPY);

  HBRUSH blue_brush = CreateSolidBrush(RGB(0, 0, 255));

  // Draw border and corner.
  FrameRect(mem_dc_, &selected_rect_, blue_brush);

  // Draw left top corner.
  left_top_corner_.left = selected_rect_.left - 2;
  left_top_corner_.top = selected_rect_.top - 2;
  left_top_corner_.right = left_top_corner_.left + 5;
  left_top_corner_.bottom = left_top_corner_.top + 5;
  FillRect(mem_dc_, &left_top_corner_, blue_brush);

  // Draw right top corner.
  right_top_corner_.right = selected_rect_.right + 2;
  right_top_corner_.top = selected_rect_.top - 2;
  right_top_corner_.left = right_top_corner_.right - 5;
  right_top_corner_.bottom = right_top_corner_.top + 5;
  FillRect(mem_dc_, &right_top_corner_, blue_brush);

  // Draw left bottom corner.
  left_bottom_corner_.left = selected_rect_.left - 2;
  left_bottom_corner_.bottom = selected_rect_.bottom + 2;
  left_bottom_corner_.top = selected_rect_.bottom - 5;
  left_bottom_corner_.right = left_bottom_corner_.left + 5;
  FillRect(mem_dc_, &left_bottom_corner_, blue_brush);

  // Draw right bottom corner.
  right_bottom_corner_.right = selected_rect_.right + 2;
  right_bottom_corner_.left = right_bottom_corner_.right - 5;
  right_bottom_corner_.bottom = selected_rect_.bottom + 2;
  right_bottom_corner_.top = right_bottom_corner_.bottom - 5;
  FillRect(mem_dc_, &right_bottom_corner_, blue_brush);

  if (!IsRectEmpty(&selected_rect_)) {
    int height = -MulDiv(9, GetDeviceCaps(mem_dc_, LOGPIXELSY), 72);

    HFONT font = CreateFont(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                            DEFAULT_CHARSET, OUT_STROKE_PRECIS, 
                            CLIP_STROKE_PRECIS, PROOF_QUALITY, 
                            VARIABLE_PITCH | FF_SWISS, _T("Arial"));
    if (!font)
      font = CreateFont(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,  
                        DEFAULT_CHARSET, OUT_STROKE_PRECIS, 
                        CLIP_STROKE_PRECIS, PROOF_QUALITY, 
                        VARIABLE_PITCH | FF_SWISS, NULL);
    HFONT old_font = (HFONT)SelectObject(mem_dc_, font);

    // Draw tip message.
    TCHAR message[100];
    _stprintf(message, _T(" %ld x %ld "), 
              selected_rect_.right - selected_rect_.left,
              selected_rect_.bottom - selected_rect_.top);
   
    SIZE size;
    GetTextExtentPoint32(mem_dc_, message, _tcslen(message), &size);
    rect.left = selected_rect_.left;
    rect.right = rect.left + size.cx;
    rect.top = selected_rect_.top - size.cy;
    rect.bottom = selected_rect_.top;

    if (rect.top < 0) {
      rect.top = selected_rect_.top - 2;
      rect.bottom = rect.top + size.cy;
    }
    if (rect.right > window_width_) {
      rect.right = selected_rect_.left - 2;
      rect.left = rect.right - size.cx;
    }
    
    SetBkMode(mem_dc_, OPAQUE);
    SetBkColor(mem_dc_, RGB(0, 0, 0));
    SetTextColor(mem_dc_, RGB(255, 255, 255));
    DrawText(mem_dc_, message, _tcslen(message), &rect, DT_CENTER);

    // Draw button
    int button_width = 0;
    int button_height = 0;
    GetTextExtentPoint32(mem_dc_, ok_caption_.c_str(), 
                         ok_caption_.length(), &size);
    button_width = size.cx;
    button_height = size.cy;
    GetTextExtentPoint32(mem_dc_, cancel_caption_.c_str(), 
                         cancel_caption_.length(), &size);
    if (button_width > size.cx)
      size.cx = button_width;
    if (button_height > size.cy)
      size.cy = button_height;

    size.cx += 8;  // Add 8 pix.
    size.cy += 4;

    ok_button_rect_.right = selected_rect_.right;
    ok_button_rect_.left = ok_button_rect_.right - size.cx;
    if (selected_rect_.bottom + size.cy > window_height_) {
      ok_button_rect_.bottom = selected_rect_.bottom - 2;
      ok_button_rect_.top = ok_button_rect_.bottom - size.cy;
    } else {
      ok_button_rect_.top = selected_rect_.bottom;
      ok_button_rect_.bottom = ok_button_rect_.top + size.cy;
    }

    // Create black brush.
    HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(mem_dc_, &ok_button_rect_, black_brush);
    DrawText(mem_dc_, ok_caption_.c_str(), ok_caption_.length(), 
             &ok_button_rect_, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    cancel_button_rect_.right = ok_button_rect_.left - 10;
    cancel_button_rect_.left = cancel_button_rect_.right - size.cx;
    cancel_button_rect_.top = ok_button_rect_.top;
    cancel_button_rect_.bottom = ok_button_rect_.bottom;
    FillRect(mem_dc_, &cancel_button_rect_, black_brush);
    DrawText(mem_dc_, cancel_caption_.c_str(), cancel_caption_.length(), 
             &cancel_button_rect_, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(mem_dc_, old_font);
    DeleteObject(font);
    DeleteObject(black_brush);
  }

  DeleteObject(blue_brush);
  HDC dlg_dc = GetDC(hwnd_);
  BitBlt(dlg_dc, 0, 0, window_width_, window_height_, mem_dc_, 0, 0, SRCCOPY);
  ReleaseDC(hwnd_, dlg_dc);
}

void CaptureWindow::MoveSelectRegion(POINT pt) {
  POINT offset = {pt.x - start_pt_.x, pt.y - start_pt_.y};
  start_pt_ = pt;
  OffsetRect(&selected_rect_, offset.x, offset.y);
  if (selected_rect_.left < 0) {
    selected_rect_.right -= selected_rect_.left;
    selected_rect_.left = 0;
  }
  if (selected_rect_.right > window_width_) {
    selected_rect_.left -= selected_rect_.right - window_width_;
    selected_rect_.right = window_width_;
  }
  if (selected_rect_.top < 0) {
    selected_rect_.bottom -= selected_rect_.top;
    selected_rect_.top = 0;
  }
  if (selected_rect_.bottom > window_height_) {
    selected_rect_.top -= selected_rect_.bottom - window_height_;
    selected_rect_.bottom = window_height_;
  }

  DrawSelectRegion(pt, false);
}

BYTE* CaptureWindow::GetImageData(int* len) {
  BITMAPFILEHEADER bmp_file_header = {0};
  int bmp_info_size = sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD);
  BITMAPINFO* bmp_info = (BITMAPINFO*)malloc(bmp_info_size);
  if (!bmp_info)
    return NULL;

  memset(bmp_info, 0, bmp_info_size);
  bmp_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  HDC temp_dc = CreateCompatibleDC(original_picture_dc_);
  int width = selected_rect_.right - selected_rect_.left;
  int height = selected_rect_.bottom - selected_rect_.top;
  HBITMAP bitmap = CreateCompatibleBitmap(original_picture_dc_, width, height);

  if (!temp_dc || !bitmap)
    return NULL;

  SelectObject(temp_dc, bitmap);
  BitBlt(temp_dc, 0, 0, width, height, 
         original_picture_dc_, selected_rect_.left,
         selected_rect_.top, SRCCOPY);
  GetDIBits(temp_dc, bitmap, 0, height, NULL, bmp_info, DIB_RGB_COLORS);
  int bmp_data_size = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) +
      bmp_info->bmiHeader.biSizeImage;
  BYTE* bmp_data = (BYTE*)malloc(bmp_data_size);
  if (!bmp_data)
    return NULL;

  bmp_info->bmiHeader.biCompression = BI_RGB;
  GetDIBits(temp_dc, bitmap, 0, height, 
            bmp_data + sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER), 
            bmp_info, DIB_RGB_COLORS);
  if (bmp_info->bmiHeader.biBitCount == 32) {
    int offset = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);
    for (int index = 3; index < bmp_info->bmiHeader.biSizeImage; index+=4) {
      bmp_data[offset + index] = 0xFF;
    }
  }

  bmp_file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + 
      sizeof(BITMAPINFOHEADER);
  bmp_file_header.bfType = 0x4d42;
  bmp_file_header.bfSize = bmp_data_size;
  bmp_file_header.bfReserved1 = bmp_file_header.bfReserved2 = 0;

  memcpy(bmp_data, &bmp_file_header, sizeof(BITMAPFILEHEADER));
  memcpy(bmp_data + sizeof(BITMAPFILEHEADER), 
         bmp_info, sizeof(BITMAPINFOHEADER));
  free(bmp_info);

  *len = bmp_data_size;
  return bmp_data;
}