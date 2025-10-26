// NOTE: Buffer overflow hazard!
// TODO: Rewrite after dynamic allocator

#include "fs.h"

FileSystem fs;

u32 clus_to_sec(u32 clus) {
  return fs.DataStartSector + ((clus - 2) * fs.SecPerClus);
}

u32 get_next_clus(u32 clus) {
  u32 clusBytsOff = clus * 4;
  u32 clusSecOff = clusBytsOff / fs.BytsPerSec;
  u32 secBytsOff = clusBytsOff % fs.BytsPerSec;
  u8 *buff = read_sector(fs.FAT1StartSector + clusSecOff);
  u32 *fat = (u32 *)buff;
  return fat[secBytsOff / 4] & 0x0FFFFFFF;
}

void write_fat(u32 clus, u32 value) {
  u32 clusBytsOff = clus * 4;
  u32 clusSecOff = clusBytsOff / fs.BytsPerSec;
  u32 secBytsOff = clusBytsOff % fs.BytsPerSec;
  u8 *sector = read_sector(fs.FAT1StartSector + clusSecOff);
  u32 *fat = (u32 *)&sector[secBytsOff];
  u32 old = *fat;
  *fat = (old & 0xF0000000) | (value & 0x0FFFFFFF);
  write_sector(fs.FAT1StartSector + clusSecOff, sector);
  write_sector(fs.FAT2StartSector + clusSecOff, sector);
}

u32 alloc_clus(u32 size, u8 dir) {
  u32 clusters = (size + (fs.BytsPerSec * fs.SecPerClus - 1)) /
                 (fs.BytsPerSec * fs.SecPerClus);
  if (dir && clusters == 0)
    clusters = 1;
  u32 freeClus[256];
  u8 freeClusCnt = 0;
  for (u32 i = 0; i < fs.FATSz32 && clusters > 0; i++) {
    u8 *buff = read_sector(fs.FAT1StartSector + i);
    u32 *data = (u32 *)buff;
    for (u32 x = 0; x < (fs.BytsPerSec / 4) && clusters > 0; x++) {
      if (data[x] == 0x00000000 && (x > 1 || i > 0)) {
        freeClus[freeClusCnt++] = x + (i * (fs.BytsPerSec / 4));
        clusters--;
      }
    }
  }
  if (clusters > 0) {
    println("Error: Not enough memory");
    return 0;
  }
  for (u8 i = 0; i < freeClusCnt; i++) {
    if (i == freeClusCnt - 1) {
      write_fat(freeClus[i], 0x0FFFFFF8);
    } else {
      write_fat(freeClus[i], freeClus[i + 1]);
    }
  }

  return freeClus[0];
}

void write_clus_data(u32 cluster, u32 size, u8 *buff) {
  u32 offsetByts = 0;
  while (cluster > 1 && cluster < 0x0FFFFFF8 && offsetByts < size) {
    u32 base = clus_to_sec(cluster);
    for (u32 i = 0; i < fs.SecPerClus && offsetByts < size; i++) {
      if (size - offsetByts >= fs.BytsPerSec) {
        write_sector(base + i, buff + offsetByts);
        offsetByts += fs.BytsPerSec;
      } else {
        u8 temp[512];
        for (u16 y = 0; y < size - offsetByts; y++) {
          temp[y] = buff[offsetByts + y];
        }
        write_sector(base + i, temp);
        offsetByts = size;
      }
    }
    cluster = get_next_clus(cluster);
  }
}

DirEntry *create_dir_entry(const char *name, u32 size, u32 firstClus, u8 dir,
                           u32 in_dir) {
  u32 cluster = 0xFFFFFFF;
  if (!in_dir) {
    cluster = fs.RootClus;
  } else {
    cluster = in_dir;
  }
  u8 found = 0;
  while (cluster > 1 && cluster < 0x0FFFFFF8 && !found) {
    for (u32 i = 0; i < fs.SecPerClus && !found; i++) {
      u8 *data = read_sector(clus_to_sec(cluster) + i);
      for (u8 x = 0; x < 16 && !found; x++) {
        DirEntry *entry = (DirEntry *)(data + x * 32);
        if (entry->Name[0] == 0x00) {
          found = 1;
          for (u8 y = 0; y < 11; y++) {
            entry->Name[y] = name[y];
          }
          if (dir) {
            entry->Attr = 0x10;
          } else {
            entry->Attr = 0x20;
          }
          entry->Reserved = 0;
          entry->CrtTimeTenth = 0;
          entry->CrtTime = 0;
          entry->CrtDate = 0;
          entry->LstAccDate = 0;
          entry->WrtTime = 0;
          entry->WrtDate = 0;
          entry->FstClusHi = (firstClus >> 16) & 0xFFFF;
          entry->FstClusLo = firstClus & 0xFFFF;
          entry->FileSize = size;
          write_sector(clus_to_sec(cluster) + i, data);
          return entry;
        }
      }
    }
    cluster = get_next_clus(cluster);
  }
  return NULL;
}

void fs_init() {
  u8 *mem = read_sector(0);
  fs.BytsPerSec = mem[11] | (mem[12] << 8);
  fs.SecPerClus = mem[13];
  fs.RsvdSecCnt = mem[14] | (mem[15] << 8);
  fs.NumFATs = mem[16];
  fs.FATSz32 = mem[36] | (mem[37] << 8) | (mem[38] << 16) | (mem[39] << 24);
  fs.RootClus = mem[44] | (mem[45] << 8) | (mem[46] << 16) | (mem[47] << 24);
  fs.FAT1StartSector = fs.RsvdSecCnt;
  fs.FAT2StartSector = fs.FAT1StartSector + fs.FATSz32;
  fs.DataStartSector = fs.RsvdSecCnt + (fs.NumFATs * fs.FATSz32);
}

DirEntry *fs_find_file(const char *filename, u32 dir) {
  u32 cluster = 0xFFFFFFFF;
  if (dir == 0) {
    cluster = fs.RootClus;
  } else {
    cluster = dir;
  }
  while (cluster > 1 && cluster < 0x0FFFFFF8) {
    for (u32 i = 0; i < fs.SecPerClus; i++) {
      u8 *data = read_sector(clus_to_sec(cluster) + i);
      for (u32 x = 0; x < 512 / sizeof(DirEntry); x++) {
        DirEntry *entry = (DirEntry *)(data + x * 32);
        if (entry->Attr & 0x08 || entry->Name[0] == 0x00 ||
            entry->Name[0] == 0xE5 || entry->Attr == 0x0F)
          continue;
        u8 match = 1;
        for (u8 y = 0; y < 11 && filename[y]; y++) {
          if (entry->Name[y] != filename[y]) {
            match = 0;
            break;
          }
        }
        if (match) {
          return entry;
        }
      }
    }
    cluster = get_next_clus(cluster);
  }
  return NULL;
}

void fs_read_file(DirEntry *file, u8 *buff) {
  u32 cluster = file->FstClusLo | (file->FstClusHi << 16);
  u32 readCnt = 0;
  u32 size = file->FileSize;
  while (cluster > 1 && cluster < 0x0FFFFFF8) {
    for (u32 i = 0; i < fs.SecPerClus; i++) {
      u32 bytsLeft = size - readCnt * fs.BytsPerSec;
      u8 *data = read_sector(clus_to_sec(cluster) + i);
      u32 readByts = 0;
      if (bytsLeft > fs.BytsPerSec) {
        readByts = fs.BytsPerSec;
      } else {
        readByts = bytsLeft;
      }
      for (u16 x = 0; x < readByts; x++) {
        buff[readCnt * fs.BytsPerSec + x] = data[x];
      }
      readCnt++;
    }
    cluster = get_next_clus(cluster);
  }
  return;
}

void fs_write_file(u32 size, const char *name, u8 *buff, u8 dir, u32 in_dir) {
  u32 firstParentCluster = 0;
  if (in_dir == 0)
    firstParentCluster = fs.RootClus;
  else
    firstParentCluster = in_dir;

  u32 firstClus = alloc_clus(size, dir);
  if (firstClus == 0) {
    return;
  }
  write_clus_data(firstClus, size, buff);
  DirEntry *entry =
      create_dir_entry(name, size, firstClus, dir, firstParentCluster);
  if (dir) {
    u32 firstSelfCluster = entry->FstClusLo | (entry->FstClusHi << 16);
    create_dir_entry(".          ", 0, firstSelfCluster, 1, firstSelfCluster);
    create_dir_entry("..         ", 0, firstParentCluster, 1, firstSelfCluster);
  }
}

u8 fs_list(u32 dir, char *found, u8 max_entries) {
  u32 cluster = (dir == 0) ? fs.RootClus : dir;
  u8 foundCnt = 0;
  while (cluster > 1 && cluster < 0x0FFFFFF8 && foundCnt < max_entries) {
    for (u32 i = 0; i < fs.SecPerClus && foundCnt < max_entries; i++) {
      u8 *data = read_sector(clus_to_sec(cluster) + i);
      for (u8 x = 0; x < 16 && foundCnt < max_entries; x++) {
        DirEntry *entry = (DirEntry *)(data + x * 32);
        if (entry->Attr & 0x08 || entry->Name[0] == 0x00 ||
            entry->Name[0] == 0xE5 || entry->Attr == 0x0F)
          continue;
        for (u8 j = 0; j < 11; j++) {
          found[foundCnt * 11 + j] = entry->Name[j];
        }
        foundCnt++;
      }
    }
    cluster = get_next_clus(cluster);
  }
  return foundCnt;
}
