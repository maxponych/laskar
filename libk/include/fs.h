#pragma once
#include "heap.h"
#include "pio.h"
#include "print.h"
#include "string.h"
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

typedef struct {
  u8 used;
  u32 size;
  u8 attr;
  u32 pos;
  u32 *cluster_chain;
  u32 chain_cap;
  u32 cluster_count;
  u32 entry_cluster;
  u32 entry_offset;
  char name[11];
} File;

#define MAX_FILES 256

typedef struct {
  char name[13];
  u32 size;
  u8 attr;
} Stat;

typedef enum {
  END,
  CUR,
  SET,
} Whence;

typedef enum {
  CREATE = 0x01,
  TRUNCATE = 0x02,
  APPEND = 0x04,
  EXCLUSIVE = 0x08,
  // TEMPORARY = 0x10, -- might be added later, but for now it is not supported
} OpenFlags;

void fs_init(void);
u32 fs_open(char *path, OpenFlags flags);
void fs_close(u32 fd);
Stat *fs_stat(char *path);
u32 fs_read(u32 fd, void *buffer, u32 cnt);
u32 fs_write(u32 fd, void *buffer, u32 cnt);
u32 fs_seek(u32 fd, i32 offset, Whence whence);
i8 fs_mkdir(char *path);
Stat *fs_readdir(u32 fd);
i8 fs_remove(char *path);
i8 fs_rmdir(char *path);
