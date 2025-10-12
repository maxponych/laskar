#include "fs.h"

FileSystem fs;

u32 cluster_to_sector(u32 cluster) {
  return fs.DataStartSector + ((cluster - 2) * fs.SecPerClus);
}

u32 get_next_cluster(u32 cluster) {
  u32 fat_offset = cluster * 4;
  u32 fat_sector = fs.FATStartSector + (fat_offset / fs.BytsPerSec);
  u32 entry_offset = fat_offset % fs.BytsPerSec;

  u8 *fat = read_sector(fat_sector);
  u32 next = fat[entry_offset] | (fat[entry_offset + 1] << 8) |
             (fat[entry_offset + 2] << 16) | (fat[entry_offset + 3] << 24);

  return next & 0x0FFFFFFF;
}

u32 fs_get_file_cluster(DirEntry *entry) {
  return entry->FstClusLo | (entry->FstClusHi << 16);
}

void fs_init() {
  u8 *bytes = read_sector(0);

  fs.BytsPerSec = bytes[11] | (bytes[12] << 8);
  fs.SecPerClus = bytes[13];
  fs.RsvdSecCnt = bytes[14] | (bytes[15] << 8);
  fs.NumFATs = bytes[16];
  fs.FATSz32 =
      bytes[36] | (bytes[37] << 8) | (bytes[38] << 16) | (bytes[39] << 24);
  fs.RootClus =
      bytes[44] | (bytes[45] << 8) | (bytes[46] << 16) | (bytes[47] << 24);
  fs.FATStartSector = fs.RsvdSecCnt;
  fs.DataStartSector = fs.RsvdSecCnt + (fs.NumFATs * fs.FATSz32);
  fs.RootDirSector = fs.DataStartSector + ((fs.RootClus - 2) * fs.SecPerClus);
  fs.DataStartOffset = (u64)fs.DataStartSector * fs.BytsPerSec;
  fs.RootDirBytsOffset = (u64)fs.RootDirSector * fs.BytsPerSec;
}

DirEntry *fs_find_file(const char *filename) {
  u8 *root = read_sector(fs.RootDirSector);

  for (int i = 0; i < 16; i++) {
    DirEntry *entry = (DirEntry *)&root[i * 32];

    if (entry->Name[0] == 0x00)
      break;

    if (entry->Name[0] == 0xE5)
      continue;
    if (entry->Attr == 0x0F)
      continue;
    if (entry->Attr & 0x08)
      continue;

    int match = 1;
    for (int j = 0; j < 11 && filename[j]; j++) {
      if (entry->Name[j] != filename[j]) {
        match = 0;
        break;
      }
    }

    if (match)
      return entry;
  }

  return 0;
}

void fs_read_file(u32 first_cluster, u32 file_size, u8 *buffer) {
  u32 cluster = first_cluster;
  u32 offset = 0;

  while (cluster >= 2 && cluster < 0x0FFFFFF8 && offset < file_size) {
    u32 sector = cluster_to_sector(cluster);
    u8 *data = read_sector(sector);

    u32 bytes_in_cluster = fs.SecPerClus * fs.BytsPerSec;
    u32 bytes_to_copy = file_size - offset;
    if (bytes_to_copy > bytes_in_cluster)
      bytes_to_copy = bytes_in_cluster;

    for (u32 i = 0; i < bytes_to_copy; i++) {
      buffer[offset + i] = data[i];
    }

    offset += bytes_to_copy;
    cluster = get_next_cluster(cluster);
  }
}
