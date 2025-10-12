#include "print.h"

void kmain() {
  println("Hello from kernel!");

  for (;;)
    asm("hlt");
}
