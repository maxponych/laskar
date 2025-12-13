#include "shell.h"

void cmd_rm(char *args) {
  char *path = normalize_path(args);
  Stat *entry = fs_stat(path);
  if (entry == NULL) {
    println("No such file or directory");
    return;
  }

  i8 res;
  if (entry->attr & 0x10) {
    res = fs_rmdir(path);
    if (res == -5) {
      println("Cannot remove root directory");
    } else if (res == -6) {
      println("Directory not empty");
    }
  } else {
    res = fs_remove(path);
  }

  kfree(path);

  if (res == -1) {
    println("Failed to remove");
  } else if (res == -2) {
    println("File or directory doesn't exist");
  } else if (res == -3) {
    println("Invalid path");
  } else if (res == -4) {
    println("Invalid type");
  }
}
