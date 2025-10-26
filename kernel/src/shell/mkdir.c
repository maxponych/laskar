#include "fs.h"
#include "shell.h"

void cmd_mkdir(char *args) {
  char name[11];
  str_to_fat83(args, name);
  fs_write_file(0, name, NULL, 1, current_dir);
}
