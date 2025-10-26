#include "print.h"

u16 cursor = 0;

void update_hw_cursor() {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (u8)(cursor & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (u8)((cursor >> 8) & 0xFF));
}

void printc(const char c) {
  volatile u16 *vga = (u16 *)0xB8000;
  int col = cursor % 80;
  int row = cursor / 80;
  switch (c) {
  case '\n':
    cursor = ((cursor / 80) + 1) * 80;
    break;
  case '\r':
    cursor -= col;
    break;

  case '\t':
    cursor += 8 - (col % 8);
    break;

  case '\b':
    if (cursor > 0)
      cursor--;
    vga[cursor] = (u16)' ' | ((u16)0x07 << 8);
    break;

  default:
    vga[cursor++] = (u16)c | ((u16)0x07 << 8);
    break;
  }
  update_hw_cursor();
}

void print(const char *str) {
  while (*str) {
    printc(*str);
    str++;
  }
}

void println(const char *str) {
  print(str);
  cursor = ((cursor / 80) + 1) * 80;
  update_hw_cursor();
}

void printx(u8 val) {
  const char *hex = "0123456789ABCDEF";
  char str[3] = {hex[(val >> 4) & 0xF], hex[val & 0xF], '\0'};
  print(str);
}

void printxln(u8 val) {
  printx(val);
  cursor = ((cursor / 80) + 1) * 80;
}
