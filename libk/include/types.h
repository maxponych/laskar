#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor = 0,
  PixelBlueGreenRedReserved8BitPerColor = 1,
  PixelBitMask = 2,
  PixelBltOnly = 3,
} PixelFormat;

typedef struct {
  u64 width;
  u64 height;
  u64 pitch;
  u32 *framebuffer;
  PixelFormat pixel_format;
} VBE;

typedef struct {
  u64 base;
  u64 page_count;
  u32 type;
} MemoryMapEntry;

typedef struct {
  MemoryMapEntry *entries;
  u64 entry_count;
} MemoryMap;

typedef struct {
  VBE vbe;
  MemoryMap mem;
  u64 own_size;
} BootArgs;

#ifndef NULL
#define NULL ((void *)0)
#endif
