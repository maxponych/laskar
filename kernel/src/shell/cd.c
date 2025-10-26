#include "shell.h"

void cmd_cd(char *args) {
  char name[11];
  str_to_fat83(args, name);
  DirEntry *new_dir = fs_find_file(name, current_dir);
  if (new_dir)
    if (new_dir->Attr == 0x10)
      current_dir = new_dir->FstClusLo | (new_dir->FstClusHi << 16);
    else
      println("No such directory");
  else
    println("No such directory");
}
