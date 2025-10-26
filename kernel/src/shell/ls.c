#include "shell.h"

void cmd_ls(char *args) {
  char buffer[64 * 11];
  u8 count = fs_list(current_dir, buffer, 64);
  char str[11];
  for (u8 i = 0; i < count; i++) {
    for (u8 x = 0; x < 11; x++) {
      str[x] = buffer[x + i * 11];
    }
    char filename[13];
    fat83_to_str(str, filename);
    for (u8 x = 0; x < 13 && filename[x] != '\0'; x++) {
      printc(to_lower(filename[x]));
    }
    printc('\n');
  }
}
