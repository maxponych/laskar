#include "fs.h"
#include "pio.h"
#include "print.h"
#include "types.h"

void fs_parser() {
  fs_init();
  DirEntry *kernel = fs_find_file("KERNEL  BIN");
  if (kernel) {
    DirEntry *kernel = fs_find_file("KERNEL  BIN");
    if (kernel) {
      u32 cluster = fs_get_file_cluster(kernel);

      fs_read_file(cluster, kernel->FileSize, (u8 *)0x20000);

      void (*kernel_main)() = (void (*)())0x20000;
      kernel_main();
    }
  }

  print("Fail");
  for (;;)
    asm("hlt");
}
