#include "shell.h"

void cmd_cat(char *args) {
  char fatName[11];
  str_to_fat83(args, fatName);

  DirEntry *file = fs_find_file(fatName, current_dir);
  char buff[512];
  u32 size = 0;
  if (file) {
    size = file->FileSize;
    fs_read_file(file, (u8 *)buff);
  } else {
    println("File not found");
    return;
  }

  if (size == 0) {
    println("");
    return;
  }

  buff[size] = '\0';

  println(buff);
}
