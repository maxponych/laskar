#include "pio.h"

u8 *read_sector(u32 lba) {
  if (lba >= (1UL << 28)) {
    println("Bad LBA");
    return 0;
  }

  u8 status;
  do {
    status = inb(0x1F7);
  } while (inb(0x1F7) & 0x80);

  outb(0x1F2, 1);
  outb(0x1F3, (u8)(lba & 0xFF));
  outb(0x1F4, (u8)((lba >> 8) & 0xFF));
  outb(0x1F5, (u8)((lba >> 16) & 0xFF));
  outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

  outb(0x1F7, 0x20);

  do {
    status = inb(0x1F7);
  } while (status & 0x80);

  if (status & 0x01) {
    println("ATA ERR");
    return 0;
  }

  if (!(status & 0x08)) {
    println("ATA NO DRQ");
    return 0;
  }

  static u16 words[256];
  for (u16 i = 0; i < 256; i++) {
    words[i] = inw(0x1F0);
  }
  u8 *buff = (u8 *)words;
  return buff;
}

void write_sector(u32 lba, u8 *buffer) {
  if (lba >= (1UL << 28)) {
    println("Bad LBA");
    return;
  }

  u8 status;
  do {
    status = inb(0x1F7);
  } while (inb(0x1F7) & 0x80);

  outb(0x1F2, 1);
  outb(0x1F3, (u8)(lba & 0xFF));
  outb(0x1F4, (u8)((lba >> 8) & 0xFF));
  outb(0x1F5, (u8)((lba >> 16) & 0xFF));
  outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

  outb(0x1F7, 0x30);

  do {
    status = inb(0x1F7);
  } while (status & 0x80);

  if (status & 0x01) {
    println("ATA ERR");
    return;
  }

  if (!(status & 0x08)) {
    println("ATA NO DRQ");
    return;
  }
  u16 *buff = (u16 *)buffer;
  for (int i = 0; i < 256; i++) {
    outw(0x1F0, buff[i]);
  }
}
