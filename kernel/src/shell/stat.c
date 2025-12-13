#include "shell.h"

void cmd_stat(char *args) {
  char *path = normalize_path(args);
  Stat *file = fs_stat(path);

  if (file == NULL) {
    println("No such file or directory");
    return;
  }

  print("Stats of path: ");
  println(path);

  kfree(path);

  print("Entry name: ");
  println(file->name);

  print("Entry size: ");
  printnum(file->size);
  printc('\n');

  print("Entry type: ");
  if (file->attr & 0x10) {
    println("directory");
  } else {
    println("file");
  }
}
