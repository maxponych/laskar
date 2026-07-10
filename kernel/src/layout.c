#include "layout.h"

layout a_layout = dvorak;
u8 shift = 0;
u8 caps = 0;

void set_layout(LayoutType layout) {
  switch (layout) {
  case QWERTY:
    a_layout = qwerty;
    break;
  case DVORAK:
    a_layout = dvorak;
    break;
  }
  return;
}

u8 to_keycode(u8 scancode) {
  u8 released = scancode & 0x80;
  u8 code = scancode & 0x7F;

  switch (code) {
  case 0x2A:
  case 0x36:
    shift = !released;
    return 0;

  case 0x3A:
    if (!released)
      caps ^= 1;
    return 0;
  }

  if (released)
    return 0;

  return code;
}

char translate(u8 keycode) {
  if (shift && caps) {
    return a_layout(keycode, 0);
  } else if (shift || caps) {
    return a_layout(keycode, 1);
  } else {
    return a_layout(keycode, 0);
  }
}

char game_translate(u8 scancode) { return qwerty(scancode, 0); }
