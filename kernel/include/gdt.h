#pragma once
#include "types.h"

typedef struct __attribute__((packed)) {
  u16 limit_low;
  u16 base_low;
  u8 base_middle;
  u8 access;
  u8 granularity;
  u8 base_high;
} gdt_entry;

typedef struct __attribute__((packed)) {
  u16 limit;
  u64 base;
} gdt_ptr;

void gdt_init(void);
