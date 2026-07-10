#include "keyboard.h"

static Ring buff = {0};

void kb_init(void) {
  u8 status;

#define WAIT_INPUT_EMPTY()                                                     \
  do {                                                                         \
    status = inb(0x64);                                                        \
  } while (status & 0x02)

#define WAIT_OUTPUT_FULL()                                                     \
  do {                                                                         \
    status = inb(0x64);                                                        \
  } while (!(status & 0x01))

  WAIT_INPUT_EMPTY();
  outb(0x64, 0xAD);

  while (inb(0x64) & 0x01)
    inb(0x60);

  WAIT_INPUT_EMPTY();
  outb(0x64, 0x20);
  WAIT_OUTPUT_FULL();
  u8 cmd = inb(0x60);

  cmd |= 0x01;
  cmd &= ~0x02;
  cmd &= ~0x20;

  WAIT_INPUT_EMPTY();
  outb(0x64, 0x60);
  WAIT_INPUT_EMPTY();
  outb(0x60, cmd);

  WAIT_INPUT_EMPTY();
  outb(0x64, 0xAE);

  WAIT_INPUT_EMPTY();
  outb(0x60, 0xFF);

  WAIT_OUTPUT_FULL();
  if (inb(0x60) != 0xFA)
    return;

  WAIT_OUTPUT_FULL();
  if (inb(0x60) != 0xAA)
    return;

  WAIT_INPUT_EMPTY();
  outb(0x60, 0xF0);
  WAIT_OUTPUT_FULL();
  if (inb(0x60) != 0xFA)
    return;

  WAIT_INPUT_EMPTY();
  outb(0x60, 0x02);
  WAIT_OUTPUT_FULL();
  if (inb(0x60) != 0xFA)
    return;

  WAIT_INPUT_EMPTY();
  outb(0x60, 0xF4);
  WAIT_OUTPUT_FULL();
  if (inb(0x60) != 0xFA)
    return;
}

u8 kb_read() {
  if (buff.tail == buff.head) {
    return 0x00;
  }
  return buff.buff[buff.tail++];
}

void kb_handler() {
  u8 sc = inb(0x60);
  u8 c = to_keycode(sc);

  u8 next = buff.head + 1;
  if (next != buff.tail && c != 0x00) {
    buff.buff[buff.head] = c;
    buff.head = next;
  }
}
