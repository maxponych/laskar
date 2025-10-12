#include "print.h"

u16 cursor = 0;

void print(const char *str) {
  volatile u8 *vga = (u8 *)0xB8000;

  while (*str) {
    if (*str == '\n') {
      cursor = ((cursor / 80) + 1) * 80;
    } else {
      int col = cursor % 80;
      int row = cursor / 80;
      int offset = (row * 80 + col) * 2;

      vga[offset] = *str;
      vga[offset + 1] = 0x07;
      cursor++;
    }
    str++;
  }
}

void println(const char *str) {
  print(str);
  cursor = ((cursor / 80) + 1) * 80;
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
