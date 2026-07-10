#include "fs.h"
#include "gdt.h"
#include "heap.h"
#include "idt.h"
#include "keyboard.h"
#include "layout.h"
#include "pmm.h"
#include "print.h"
#include "shell.h"
#include "types.h"

void kmain(BootArgs *boot) __attribute__((section(".text.start")));
