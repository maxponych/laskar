#pragma once
#include "io.h"
#include "types.h"

extern void kb_stub(void);

typedef struct __attribute__((packed)) {
  u16 offset_low;
  u16 selector;
  u8 ist;
  u8 type_attr;
  u16 offset_mid;
  u32 offset_high;
  u32 zero;
} idt_entry;

typedef struct __attribute__((packed)) {
  u16 limit;
  u64 base;
} idt_ptr;

void idt_init(void);
void pic_init(void);
