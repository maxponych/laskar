#include "print.h"
#include "font.h"

static VBE *vbe = NULL;
static u32 fg = 0x00FFFFFF;
static u32 bg = 0x00000000;
static u64 pos_x = 0;
static u64 pos_y = 0;
const u8 char_width = 8;
const u8 char_height = 16;

void init_screen(VBE *pointer) {
  vbe = pointer;
  pos_x = 0;
  pos_y = 0;
}

void clear_screen() {
  for (u64 i = 0; i < vbe->height * vbe->width; i++) {
    vbe->framebuffer[i] = bg;
  }

  pos_x = 0;
  pos_y = 0;
}

void set_color(u32 nfg, u32 nbg) {
  bg = nbg;
  fg = nfg;
}

u64 screen_width() { return vbe->width; }

u64 screen_height() { return vbe->height; }

void putc(const char c, u8 x, u8 y, u32 color) {
  if (x >= vbe->width / char_width || y >= vbe->height / char_height)
    return;

  u64 char_x = x * char_width;
  u64 char_y = y * char_height;

  u8 *glyph = font8x16[c];

  for (u8 row = 0; row < char_height; row++) {
    for (u8 col = 0; col < char_width; col++) {
      u32 pixel = (glyph[row] & (1 << (7 - col))) ? color : bg;
      vbe->framebuffer[(char_y + row) * vbe->pitch + (char_x + col)] = pixel;
    }
  }
}

void fillc(u16 x, u16 y, u32 color) {
  if (x >= vbe->width / char_width || y >= vbe->height / char_height)
    return;

  u64 char_x = x * char_width;
  u64 char_y = y * char_height;

  for (u8 row = 0; row < char_height; row++) {
    for (u8 col = 0; col < char_width; col++) {
      vbe->framebuffer[(char_y + row) * vbe->pitch + (char_x + col)] = color;
    }
  }
}

void fillc16x16(u16 x, u16 y, u32 color) {
  if (x >= vbe->width / 16 || y >= vbe->height / 16)
    return;

  u64 char_x = x * 16;
  u64 char_y = y * 16;

  for (u8 row = 1; row < 15; row++) {
    for (u8 col = 1; col < 15; col++) {
      vbe->framebuffer[(char_y + row) * vbe->pitch + (char_x + col)] = color;
    }
  }
}

void fillc8x8(u16 x, u16 y, u32 color) {
  if (x >= vbe->width / 8 || y >= vbe->height / 8)
    return;

  u64 char_x = x * 8;
  u64 char_y = y * 8;

  for (u8 row = 0; row < 8; row++) {
    for (u8 col = 0; col < 8; col++) {
      vbe->framebuffer[(char_y + row) * vbe->pitch + (char_x + col)] = color;
    }
  }
}

void print_char(char c) {
  if (c > 127)
    c = '?';

  u8 *glyph = font8x16[c];

  for (u8 row = 0; row < char_height; row++) {
    for (u8 col = 0; col < char_width; col++) {
      u32 color = (glyph[row] & (1 << (7 - col))) ? fg : bg;
      vbe->framebuffer[(pos_y * char_height + row) * vbe->pitch +
                       (pos_x * char_width + col)] = color;
    }
  }
}

void scroll_up() {
  u64 lines_to_copy = (vbe->height / char_height) - 1;

  for (u64 y = 0; y < lines_to_copy * char_height; y++) {
    for (u64 x = 0; x < vbe->width; x++) {
      u32 pixel = vbe->framebuffer[(y + char_height) * vbe->pitch + x];
      vbe->framebuffer[y * vbe->pitch + x] = pixel;
    }
  }

  for (u64 y = lines_to_copy * char_height; y < vbe->height; y++) {
    for (u64 x = 0; x < vbe->width; x++) {
      vbe->framebuffer[y * vbe->pitch + x] = bg;
    }
  }
}

void printc(char c) {
  u64 max_cols = vbe->width / char_width;
  u64 max_rows = vbe->height / char_height;

  switch (c) {
  case '\n':
    pos_x = 0;
    pos_y++;
    break;
  case '\r':
    pos_x = 0;
    break;
  case '\t':
    pos_x += 4 - (pos_x % 4);
    break;
  case '\b':
    if (pos_x > 0) {
      pos_x--;
      print_char(' ');
    }
    break;
  default:
    print_char(c);
    pos_x++;
    break;
  }

  if (pos_x >= max_cols) {
    pos_x = 0;
    pos_y++;
  }

  if (pos_y >= max_rows) {
    scroll_up();
    pos_y = max_rows - 1;
  }
}

void printlncount(const char *str, u64 count) {
  for (u64 i = 0; i <= count; i++) {
    printc(str[i]);
  }
  printc('\n');
}

void print(const char *str) {
  while (*str) {
    printc(*str);
    str++;
  }
}

void println(const char *str) {
  print(str);
  printc('\n');
}

void printx(u8 val) {
  const char *hex = "0123456789ABCDEF";
  printc(hex[(val >> 4) & 0xF]);
  printc(hex[val & 0xF]);
}

void printxln(u8 val) {
  printx(val);
  printc('\n');
}

void printnum(u64 n) {
  char buf[21];
  int i = 0;

  if (n == 0) {
    printc('0');
    return;
  }

  while (n > 0) {
    buf[i++] = '0' + (n % 10);
    n /= 10;
  }

  for (int j = i - 1; j >= 0; j--) {
    printc(buf[j]);
  }
}
