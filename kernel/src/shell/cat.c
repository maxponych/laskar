#include "shell.h"

void cmd_cat(char *args) {
  static char fatName[11];
  str_to_fat83(args, fatName);

  DirEntry *file = fs_find_file(fatName, current_dir);
  if (!file) {
    println("File not found");
    return;
  }

  u64 clus_size = get_clus_size();
  u8 *buff = (u8 *)kmalloc(clus_size);
  u32 size = file->FileSize;
  u32 clus = file->FstClusLo | (file->FstClusHi << 16);
  while (clus > 1 && clus < 0x0FFFFFF8 && size > 0) {
    read_clus(clus, buff);

    u32 bytes_to_print = (size > clus_size) ? clus_size : size;
    for (u32 i = 0; i < bytes_to_print; i++) {
      printc(buff[i]);
    }
    size -= bytes_to_print;

    clus = get_next_clus(clus);
  }

  kfree(buff);
  printc('\n');
}
