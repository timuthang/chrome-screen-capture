#ifndef KEY_BINDER_H
#define	KEY_BINDER_H

#include <list>
#include <sys/types.h>

typedef void (* BindKeyHandler)(uint keycode, uint modifiers, void* user_data);

class KeyBinder {
public:
  KeyBinder();
  ~KeyBinder();
  
public:
  struct Binding {
    uint keycode;
    uint modifiers;
    void* user_data;
    BindKeyHandler handler;
  };
  
public:
  static bool Init();
  static bool UnInit();
  static bool GrabKey(Binding* bind_data);
  static bool UnGrabKey(Binding* bind_data);
  
public:
  static std::list<KeyBinder::Binding> binging_list_;
};

#endif

