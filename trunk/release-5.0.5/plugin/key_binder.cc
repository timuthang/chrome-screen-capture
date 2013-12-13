#include "key_binder.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

namespace {
  
uint num_lock_mask = Mod2Mask;
uint caps_lock_mask = LockMask;
  
GdkFilterReturn FilterFunc(GdkXEvent *gdk_xevent, 
                           GdkEvent *event, gpointer data) {
  GdkFilterReturn return_val = GDK_FILTER_CONTINUE;
  XEvent* xevent = (XEvent*)gdk_xevent;
  guint event_mods;	
  std::list<KeyBinder::Binding>::iterator iter;	

  switch (xevent->type) {
    case KeyPress:
      event_mods = xevent->xkey.state & ~(num_lock_mask | caps_lock_mask);
      for (iter = KeyBinder::binging_list_.begin(); 
           iter != KeyBinder::binging_list_.end(); 
           iter++) {
        KeyBinder::Binding binding = *iter;
        if (binding.keycode == xevent->xkey.keycode &&
            binding.modifiers == event_mods) {
          (binding.handler)(
              binding.keycode, binding.modifiers, binding.user_data);
        }
      }
      break;
    case KeyRelease:
      break;
  }
  
  return return_val;
}

void GrabOrUngrab(GdkWindow* rootwin, 
                  KeyBinder::Binding* binding, gboolean grab) {
  guint mod_masks [] = {
    0,
    num_lock_mask,
    caps_lock_mask,
    num_lock_mask  | caps_lock_mask,
  };
  int i;
  
  for (i = 0; i < G_N_ELEMENTS (mod_masks); i++) {
    if (grab) {
      XGrabKey(
          GDK_WINDOW_XDISPLAY(rootwin), binding->keycode, 
          binding->modifiers | mod_masks[i], 
          GDK_WINDOW_XWINDOW(rootwin), 
          False, GrabModeAsync, GrabModeAsync);
    } else {
      XUngrabKey(
          GDK_WINDOW_XDISPLAY(rootwin),
          binding->keycode,
          binding->modifiers | mod_masks[i], 
          GDK_WINDOW_XWINDOW(rootwin));
    }
  }
}

}

std::list<KeyBinder::Binding> KeyBinder::binging_list_;

bool KeyBinder::Init() {
  GdkWindow *rootwin = gdk_get_default_root_window();
  gdk_window_add_filter(rootwin, FilterFunc, NULL);  
  return true;
}

bool KeyBinder::UnInit() {
  GdkWindow *rootwin = gdk_get_default_root_window();
  gdk_window_remove_filter(rootwin, FilterFunc, NULL);
  std::list<KeyBinder::Binding>::iterator iter = 
      KeyBinder::binging_list_.begin();
  while (iter != KeyBinder::binging_list_.end()) {
    GrabOrUngrab(rootwin, &*iter, FALSE);
    iter++;
  }
  KeyBinder::binging_list_.clear();
  return true;
}

bool KeyBinder::GrabKey(Binding* bind_data) {
  GdkWindow *rootwin = gdk_get_default_root_window();  
  gdk_error_trap_push ();
  GrabOrUngrab(rootwin, bind_data, TRUE);
  gdk_flush ();
  if (gdk_error_trap_pop ()) {  
    return false;
  }
  binging_list_.push_back(*bind_data);
  return true;
}

bool KeyBinder::UnGrabKey(Binding* bind_data) {
  GdkWindow *rootwin = gdk_get_default_root_window();
  std::list<KeyBinder::Binding>::iterator iter;
  for (iter = KeyBinder::binging_list_.begin(); 
       iter != KeyBinder::binging_list_.end(); 
       iter++) {
    if ((*iter).keycode == bind_data->keycode && 
        (*iter).modifiers == bind_data->modifiers &&
        (*iter).user_data == bind_data->user_data) {
      GrabOrUngrab(rootwin, bind_data, FALSE);
      KeyBinder::binging_list_.erase(iter);
      break;
    }
  }
  return true;
}