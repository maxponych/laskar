#include "shell.h"

void cmd_ls(char *args) {
  char *path = normalize_path(args);
  u32 dir = fs_open(path, 0x00);
  kfree(path);
  if (dir == (u32)-1) {
    println("No such file or directory");
    return;
  }

  static Stat *entry;
  while (1) {
    entry = fs_readdir(dir);
    if (entry != NULL) {
      u8 i = 0;
      for (u8 x = 0; entry->name[x] != '\0'; x++) {
        if (entry->attr & 0x10) {
          set_color(0x00217185, 0x00000000);
        }
        printc(to_lower(entry->name[x]));
      }
      set_color(0x00FFFFFF, 0x00000000);
      printc(' ');
    }
    if (entry == NULL) {
      break;
    }
  }

  fs_close(dir);
  printc('\n');
}
