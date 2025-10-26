#pragma once
#include "pio.h"
#include "print.h"
#include "types.h"

typedef struct {
  u16 BytsPerSec;
  u8 SecPerClus;
  u16 RsvdSecCnt;
  u8 NumFATs;
  u32 FATSz32;
  u32 RootClus;
  u32 FAT1StartSector;
  u32 FAT2StartSector;
  u32 DataStartSector;
} FileSystem;

typedef struct {
  u8 Name[11];
  u8 Attr;
  u8 Reserved;
  u8 CrtTimeTenth;
  u16 CrtTime;
  u16 CrtDate;
  u16 LstAccDate;
  u16 FstClusHi;
  u16 WrtTime;
  u16 WrtDate;
  u16 FstClusLo;
  u32 FileSize;
} DirEntry;

void fs_init();
DirEntry *fs_find_file(const char *filename, u32 dir);
u8 fs_list(u32 dir, char *found, u8 max_entries);
void fs_read_file(DirEntry *file, u8 *buff);
void fs_write_file(u32 size, const char *name, u8 *buff, u8 dir, u32 in_dir);
