#include "keyboard.h"

void kb_init() {
  u8 status = inb(0x64);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x64, 0xAD);

  do {
    inb(0x60);
    status = inb(0x64);
  } while (status & 0x01);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x64, 0x20);

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  u8 old_comm = inb(0x60);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x64, 0x60);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  u8 new_comm = old_comm & 0x5E;
  outb(0x60, new_comm);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x64, 0xAE);

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x60, 0xFF);

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  u8 ack = inb(0x60);

  if (ack != 0xFA) {
    if (ack == 0xFE) {
      outb(0x60, 0xFF);
    } else {
      return;
    }
  }

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  ack = inb(0x60);

  if (ack != 0xAA) {
    return;
  }

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x60, 0xF0);

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  ack = inb(0x60);

  if (ack != 0xFA) {
    return;
  }

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x60, 0x02);

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  ack = inb(0x60);

  if (ack != 0xFA) {
    return;
  }

  do {
    status = inb(0x64);
  } while (status & 0x02);

  outb(0x60, 0xF4);

  do {
    status = inb(0x64);
  } while (!(status & 0x01));

  ack = inb(0x60);

  if (ack != 0xFA) {
    return;
  }
}

void kb_listen(u8 *buff, u16 *bytes) {
  u8 status = inb(0x64);
  while (1) {
    status = inb(0x64);
    if (status & 0x01) {
      buff[*bytes] = inb(0x60);
      (*bytes)++;
    }
  }
}

u8 kb_read() {
  u8 status = inb(0x64);
  u8 res = 0;
  if (status & 0x01) {
    res = inb(0x60);
  }
  return res;
}
