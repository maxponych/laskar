#include "shell.h"

void cmd_mkdir(char *args) {
  char *path = normalize_path(args);
  i8 res = fs_mkdir(path);
  kfree(path);
  if (res == -1) {
    println("Invalid path");
    return;
  }
  if (res == -2) {
    println("Directory already exists");
    return;
  }
  if (res == -3) {
    println("Failed to create entry");
  }
}
