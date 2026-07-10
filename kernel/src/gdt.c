#include "gdt.h"

gdt_entry gdt[3];
gdt_ptr gdt_pointer;

void gdt_set_gate(int num, u64 base, u32 limit, u8 access, u8 gran) {
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;
  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = (limit >> 16) & 0x0F;
  gdt[num].granularity |= gran & 0xF0;
  gdt[num].access = access;
}

void gdt_init() {
  gdt_pointer.limit = (sizeof(gdt_entry) * 3) - 1;
  gdt_pointer.base = (u64)&gdt;

  gdt_set_gate(0, 0, 0, 0, 0);

  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);

  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xC0);

  asm volatile("lgdt %0" : : "m"(gdt_pointer));

  asm volatile("mov $0x10, %%ax\n"
               "mov %%ax, %%ds\n"
               "mov %%ax, %%es\n"
               "mov %%ax, %%fs\n"
               "mov %%ax, %%gs\n"
               "mov %%ax, %%ss\n"
               "pushq $0x08\n"
               "lea 1f(%%rip), %%rax\n"
               "pushq %%rax\n"
               "lretq\n"
               "1:\n"
               :
               :
               : "rax");
}
