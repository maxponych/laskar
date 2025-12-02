#include "fs.h"
#include "heap.h"
#include "keyboard.h"
#include "layout.h"
#include "print.h"
#include "shell.h"
#include "types.h"

void kmain(BootArgs *boot) __attribute__((section(".text.start")));
