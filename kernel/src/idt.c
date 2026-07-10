#include "idt.h"

idt_entry idt[256];
idt_ptr idtp;

void idt_set_gate(u8 num, u64 handler, u16 selector, u8 flags) {
  idt[num].offset_low = handler & 0xFFFF;
  idt[num].offset_mid = (handler >> 16) & 0xFFFF;
  idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;

  idt[num].selector = selector;
  idt[num].ist = 0;
  idt[num].type_attr = flags;
  idt[num].zero = 0;
}

void idt_init() {
  idtp.limit = (sizeof(idt_entry) * 256) - 1;
  idtp.base = (u64)&idt;

  idt_set_gate(33, (u64)kb_stub, 0x08, 0x8E);
  __asm__ volatile("lidt %0" : : "m"(idtp));
}

void pic_init() {
  outb(0x21, 0xFF);
  outb(0xA1, 0xFF);

  outb(0x20, 0x11);
  outb(0x80, 0);
  outb(0xA0, 0x11);
  outb(0x80, 0);

  outb(0x21, 0x20);
  outb(0x80, 0);
  outb(0xA1, 0x28);
  outb(0x80, 0);

  outb(0x21, 0x04);
  outb(0x80, 0);
  outb(0xA1, 0x02);
  outb(0x80, 0);

  outb(0x21, 0x01);
  outb(0x80, 0);
  outb(0xA1, 0x01);
  outb(0x80, 0);

  outb(0x21, 0xFD);
  outb(0x80, 0);
  outb(0xA1, 0xFF);
  outb(0x80, 0);
}
