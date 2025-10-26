#include "kernel.h"

void kmain() {
  println("Welcome back to MinorOS");
  fs_init();
  kb_init();
  char comm_buff[80];
  u8 comm_buff_cnt = 0;
  while (1) {
    u8 in = kb_read();
    u8 c = translate(in);
    if (c > 0) {
      if (c == '\n') {
        printc(c);
        comm_buff[comm_buff_cnt] = '\0';

        cmd_parse(comm_buff, &comm_buff_cnt);

        for (u8 i = 79; i > 0; i++) {
          comm_buff[i] = 0;
        }

        comm_buff_cnt = 0;
        continue;
      }
      if (c == '\b') {
        if (comm_buff_cnt > 0) {
          comm_buff[comm_buff_cnt--] = 0;
          printc(c);
        }
        continue;
      }
      if (comm_buff_cnt < 80) {
        comm_buff[comm_buff_cnt++] = c;
        printc(c);
      }
    }
  }
}
