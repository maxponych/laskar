#pragma once
#include "pio.h"
#include "types.h"

typedef struct {
  u16 BytsPerSec;
  u8 SecPerClus;
  u16 RsvdSecCnt;
  u8 NumFATs;
  u32 FATSz32;
  u32 RootClus;
  u32 FATStartSector;
  u32 DataStartSector;
  u32 RootDirSector;
  u64 DataStartOffset;
  u64 RootDirBytsOffset;
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

u32 cluster_to_sector(u32 cluster);
u32 get_next_cluster(u32 cluster);
u32 fs_get_file_cluster(DirEntry *entry);
void fs_init();
DirEntry *fs_find_file(const char *filename);
void fs_read_file(u32 first_cluster, u32 file_size, u8 *buffer);
