#include "shell.h"

void cmd_cat(char *args) {
  char fatName[11];
  str_to_fat83(args, fatName);

  DirEntry *file = fs_find_file(fatName, current_dir);

  static u8 buff[512];
  u32 clus = 0;
  u32 size = 0;
  if (file) {
    size = file->FileSize;
    clus = file->FstClusLo | (file->FstClusHi << 16);
    while (clus > 1 && clus < 0x0FFFFFF8 && size > 0) {
      read_clus(clus, buff);

      u32 bytes_to_print = (size > 512) ? 512 : size;
      for (u32 i = 0; i < bytes_to_print; i++) {
        printc(buff[i]);
      }
      size -= bytes_to_print;

      clus = get_next_clus(clus);
    }
  } else {
    println("File not found");
    return;
  }

  if (size == 0) {
    println("");
    return;
  }

  printc('\n');
}
