#pragma once

#include "io.h"
#include "types.h"

void init_screen(VBE *vbe);
void clear_screen();
void set_color(u32 nfg, u32 nbg);
u64 screen_height(void);
u64 screen_width(void);
void fillc(u16 x, u16 y, u32 color);
void fillc8x8(u16 x, u16 y, u32 color);
void fillc16x16(u16 x, u16 y, u32 color);
void putc(const char c, u8 x, u8 y, u32 color);
void printc(const char c);
void print(const char *str);
void println(const char *str);
void printx(u8 val);
void printxln(u8 val);
void printnum(u64 n);
