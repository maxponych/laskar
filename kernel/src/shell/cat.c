#include "shell.h"

void cmd_cat(char *args) {
  char *path = normalize_path(args);
  u32 file = fs_open(path, 0x00);
  if (file == (u32)-1) {
    println("No such file or directory");
    return;
  }

  Stat *stat = fs_stat(path);
  kfree(path);
  if (stat == NULL) {
    println("Can't read file metadata");
    fs_close(file);
    return;
  }
  if (stat->attr & 0x10) {
    print(stat->name);
    print(": ");
    println("Is a directory");
    fs_close(file);
    return;
  }

  static char buff[4096];
  u32 end = (stat->size + 4095) / 4096;

  u32 read = 0;

  for (u32 i = 0; i <= end; i++) {
    u32 was_read = 0;
    if (i == end) {
      was_read = fs_read(file, buff, stat->size % 4096);
      for (u32 i = 0; i < was_read; i++) {
        printc(buff[i]);
      }
    } else {
      was_read = fs_read(file, buff, 4096);
      for (u32 i = 0; i < was_read; i++) {
        printc(buff[i]);
      }
    }

    if (was_read == (u32)-1) {
      println("An error occured!");
      fs_close(file);
      return;
    }
    read += was_read;
  }

  fs_close(file);
  printc('\n');
}
