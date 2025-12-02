#include "kernel.h"

void green_ok(void) {
  set_color(0x0000FF00, 0x00000000);
  print("OK");
  set_color(0x00FFFFFF, 0x00000000);
}

void print_ok(const char *msg) {
  print("[  ");
  green_ok();
  print("  ] ");
  println(msg);
}

void greet() {
  printc('\n');
  printc('\n');
  set_color(0x0000FF00, 0x00000000);
  println("<<    Welcome back to laskar    >>");
  set_color(0x00FFFFFF, 0x00000000);
  clear_screen();
}

void print_prompt() {
  set_color(0x0000FFFF, 0x00000000);
  print("$ ");
  set_color(0x00FFFFFF, 0x00000000);
}

void kmain(BootArgs *boot) {
  init_screen(&boot->vbe);
  clear_screen();
  println("Laskar booting...");

  pmm_init(&boot->mem, boot->own_size);
  heap_init();
  print_ok("Initialising Memory");

  fs_init();
  print_ok("Initialising Filesystem");

  kb_init();
  print_ok("Initialising Keyboard driver");

  greet();

  char *comm_buff = kmalloc(128);
  u32 comm_cap = 128;
  u32 comm_buff_cnt = 0;
  print_prompt();
  while (1) {
    u8 in = kb_read();
    u8 c = translate(in);
    if (c > 0) {
      if (c == '\n') {
        if (comm_buff_cnt == 0) {
          printc('\n');
          print_prompt();
          continue;
        }
        printc(c);
        comm_buff[comm_buff_cnt] = '\0';

        cmd_parse(comm_buff, &comm_buff_cnt);

        comm_buff_cnt = 0;
        print_prompt();
        continue;
      }
      if (c == '\b') {
        if (comm_buff_cnt > 0) {
          comm_buff[comm_buff_cnt--] = 0;
          printc(c);
        }
        continue;
      }
      if (comm_buff_cnt + 4 >= comm_cap) {
        u32 new_cap = comm_cap * 1;
        char *new_buff = kmalloc(new_cap);

        for (u32 i = 0; i < comm_cap; i++) {
          new_buff[i] = comm_buff[i];
        }

        kfree(comm_buff);
        comm_buff = new_buff;
        comm_cap = new_cap;
      }

      comm_buff[comm_buff_cnt++] = c;
      printc(c);
    }
  }
}
