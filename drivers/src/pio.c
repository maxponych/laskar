#include "pio.h"

u8 *read_sector(u32 lba) {

  if (lba >= (1UL << 28)) {
    print("Bad LBA");
    return 0;
  }

  u8 status;
  do {
    status = inb(0x1F7);

  } while ((status & 0x80));

  outb(0x1F2, 1);
  outb(0x1F3, (u8)(lba & 0xFF));
  outb(0x1F4, (u8)((lba >> 8) & 0xFF));
  outb(0x1F5, (u8)((lba >> 16) & 0xFF));
  outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

  outb(0x1F7, 0x20);

  do {
    status = inb(0x1F7);
  } while ((status & 0x88) != 0x08);

  if (status & 0x01) {
    print("ATA ERR: ");
    printx(inb(0x1F1));
    return 0;
  }

  static u16 buffer[256];
  for (int i = 0; i < 256; i++) {
    buffer[i] = inw(0x1F0);
  }

  u8 *bytes = (u8 *)buffer;

  return bytes;
}
