#include "fs.h"
#include "heap.h"

FileSystem fs;

static u8 buff[512];

u32 clus_to_sec(u32 clus) {
  return fs.DataStartSector + ((clus - 2) * fs.SecPerClus);
}

u32 get_next_clus(u32 clus) {
  u32 clusBytsOff = clus * 4;
  u32 clusSecOff = clusBytsOff / fs.BytsPerSec;
  u32 secBytsOff = clusBytsOff % fs.BytsPerSec;
  read_sector(fs.FAT1StartSector + clusSecOff, buff);
  u32 *fat = (u32 *)buff;
  return fat[secBytsOff / 4] & 0x0FFFFFFF;
}

void read_clus(u32 clus, u8 *buff) {
  u32 sector = clus_to_sec(clus);
  for (u8 i = 0; i < fs.SecPerClus; i++) {
    read_sector(sector + i, buff + (i * fs.BytsPerSec));
  }
}

void write_fat(u32 clus, u32 value) {
  u32 clusBytsOff = clus * 4;
  u32 clusSecOff = clusBytsOff / fs.BytsPerSec;
  u32 secBytsOff = clusBytsOff % fs.BytsPerSec;
  read_sector(fs.FAT1StartSector + clusSecOff, buff);
  u32 *fat = (u32 *)&buff[secBytsOff];
  u32 old = *fat;
  *fat = (old & 0xF0000000) | (value & 0x0FFFFFFF);
  write_sector(fs.FAT1StartSector + clusSecOff, buff);
  write_sector(fs.FAT2StartSector + clusSecOff, buff);
}

u32 alloc_clus(u32 size, u8 dir) {
  u32 clusters = (size + (fs.BytsPerSec * fs.SecPerClus - 1)) /
                 (fs.BytsPerSec * fs.SecPerClus);
  if (dir && clusters == 0)
    clusters = 1;
  u32 *freeClus = (u32 *)kmalloc(clusters * sizeof(FreeBlock));
  u64 freeClusCnt = 0;
  for (u32 i = 0; i < fs.FATSz32 && clusters > 0; i++) {
    read_sector(fs.FAT1StartSector + i, buff);
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

  u32 free = freeClus[0];
  kfree(freeClus);

  return free;
}

u64 get_clus_size() { return fs.SecPerClus * fs.BytsPerSec; }

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
      read_sector(clus_to_sec(cluster) + i, buff);
      for (u8 x = 0; x < 16 && !found; x++) {
        DirEntry *entry = (DirEntry *)(buff + x * 32);
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
          write_sector(clus_to_sec(cluster) + i, buff);
          return entry;
        }
      }
    }
    cluster = get_next_clus(cluster);
  }
  return NULL;
}

void fs_init() {
  read_sector(0, buff);
  fs.BytsPerSec = buff[11] | (buff[12] << 8);
  fs.SecPerClus = buff[13];
  fs.RsvdSecCnt = buff[14] | (buff[15] << 8);
  fs.NumFATs = buff[16];
  fs.FATSz32 = buff[36] | (buff[37] << 8) | (buff[38] << 16) | (buff[39] << 24);
  fs.RootClus =
      buff[44] | (buff[45] << 8) | (buff[46] << 16) | (buff[47] << 24);
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
      read_sector(clus_to_sec(cluster) + i, buff);
      for (u32 x = 0; x < 512 / sizeof(DirEntry); x++) {
        DirEntry *entry = (DirEntry *)(buff + x * 32);
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

void fs_read_file(u32 cluster, u32 size, u8 *dest) {
  u32 readCnt = 0;
  while (cluster > 1 && cluster < 0x0FFFFFF8) {
    for (u32 i = 0; i < fs.SecPerClus; i++) {
      u32 bytsLeft = size - readCnt * fs.BytsPerSec;
      read_sector(clus_to_sec(cluster) + i, buff);
      u32 readByts = 0;
      if (bytsLeft > fs.BytsPerSec) {
        readByts = fs.BytsPerSec;
      } else {
        readByts = bytsLeft;
      }
      for (u16 x = 0; x < readByts; x++) {
        dest[readCnt * fs.BytsPerSec + x] = buff[x];
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

u8 fs_list(u32 dir, char **found, u8 **modes) {
  u32 cluster = (dir == 0) ? fs.RootClus : dir;
  u64 cap = 64;
  *found = (char *)kmalloc(cap * 11);
  *modes = (u8 *)kmalloc(cap);
  u8 foundCnt = 0;
  while (cluster > 1 && cluster < 0x0FFFFFF8) {
    for (u32 i = 0; i < fs.SecPerClus; i++) {
      read_sector(clus_to_sec(cluster) + i, buff);
      for (u8 x = 0; x < 16; x++) {
        DirEntry *entry = (DirEntry *)(buff + x * 32);
        if (entry->Attr & 0x08 || entry->Name[0] == 0x00 ||
            entry->Name[0] == 0xE5 || entry->Attr == 0x0F)
          continue;
        for (u8 j = 0; j < 11; j++) {
          (*found)[foundCnt * 11 + j] = entry->Name[j];
        }
        (*modes)[foundCnt] = entry->Attr;
        foundCnt++;
        if (foundCnt + 2 == cap) {
          u64 new_cap = cap * 2;
          char *new_found = (char *)kmalloc(new_cap * 11);
          u8 *new_modes = (u8 *)kmalloc(new_cap);

          for (u64 i = 0; i < cap * 11; i++) {
            new_found[i] = (*found)[i];
          }

          for (u64 i = 0; i < cap; i++) {
            new_modes[i] = (*modes)[i];
          }

          cap = new_cap;
          kfree(*found);
          kfree(*modes);
          *found = new_found;
          *modes = new_modes;
        }
      }
    }
    cluster = get_next_clus(cluster);
  }
  return foundCnt;
}
