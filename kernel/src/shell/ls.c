#include "shell.h"

void cmd_ls(char *args) {
  char buffer[64 * 12];
  u8 modes[64];
  u8 count = fs_list(current_dir, buffer, modes, 64);
  char str[11];
  for (u8 i = 0; i < count; i++) {
    for (u8 x = 0; x < 11; x++) {
      str[x] = buffer[x + i * 11];
    }
    char filename[13];
    fat83_to_str(str, filename);
    for (u8 x = 0; x < 13 && filename[x] != '\0'; x++) {
      if (modes[i] == 0x10) {
        set_color(0x00217185, 0x00000000);
      }
      printc(to_lower(filename[x]));
    }
    set_color(0x00FFFFFF, 0x00000000);
    printc(' ');
  }
  printc('\n');
}
