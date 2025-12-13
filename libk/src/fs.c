#include "fs.h"

static FileSystem fs;

static File fd_table[MAX_FILES];

static u8 *buff;

void fs_init() {
  static u8 data[512];
  read_sector(0, data);
  fs.BytsPerSec = data[11] | (data[12] << 8);
  fs.SecPerClus = data[13];
  fs.RsvdSecCnt = data[14] | (data[15] << 8);
  fs.NumFATs = data[16];
  fs.FATSz32 = data[36] | (data[37] << 8) | (data[38] << 16) | (data[39] << 24);
  fs.RootClus =
      data[44] | (data[45] << 8) | (data[46] << 16) | (data[47] << 24);
  fs.FAT1StartSector = fs.RsvdSecCnt;
  fs.FAT2StartSector = fs.FAT1StartSector + fs.FATSz32;
  fs.DataStartSector = fs.RsvdSecCnt + (fs.NumFATs * fs.FATSz32);
  buff = kmalloc(fs.SecPerClus * fs.BytsPerSec);
}

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

void link_clus(u32 clus, u32 to) {
  u32 clusBytsOff = clus * 4;
  u32 clusSecOff = clusBytsOff / fs.BytsPerSec;
  u32 secBytsOff = clusBytsOff % fs.BytsPerSec;
  read_sector(fs.FAT1StartSector + clusSecOff, buff);
  u32 *fat = (u32 *)buff;
  fat[secBytsOff / 4] = to & 0x0FFFFFFF;
  write_sector(fs.FAT1StartSector + clusSecOff, buff);
  write_sector(fs.FAT2StartSector + clusSecOff, buff);
}

void write_clus(u32 clus, u8 *buff) {
  u32 sector = clus_to_sec(clus);
  for (u8 i = 0; i < fs.SecPerClus; i++) {
    write_sector(sector + i, buff + (i * fs.BytsPerSec));
  }
}

void read_clus(u32 clus, u8 *buff) {
  u32 sector = clus_to_sec(clus);
  for (u8 i = 0; i < fs.SecPerClus; i++) {
    read_sector(sector + i, buff + (i * fs.BytsPerSec));
  }
}

void zero_clus(u32 clus) {
  u64 *buffer = (u64 *)buff;
  u32 count = (fs.SecPerClus * fs.BytsPerSec) / sizeof(u64);
  for (u32 i = 0; i < count; i++) {
    buffer[i] = 0;
  }
  write_clus(clus, buff);
}

u32 find_free_clus() {
  for (u32 i = 0; i < fs.FATSz32; i++) {
    read_sector(fs.FAT1StartSector + i, buff);
    u32 *data = (u32 *)buff;
    for (u32 x = 0; x < (fs.BytsPerSec / 4); x++) {
      if (data[x] == 0x00000000 && (x > 1 || i > 0)) {
        return x + (i * (fs.BytsPerSec / 4));
      }
    }
  }

  return (u32)-1;
}

DirEntry *find_entry(u32 clus, char *name, u32 *entry_clus, u32 *entry_offset) {
  static DirEntry res = {0};
  while (clus > 1 && clus < 0x0FFFFFF8) {
    read_clus(clus, buff);
    for (u32 x = 0; x < fs.SecPerClus * fs.BytsPerSec / sizeof(DirEntry); x++) {
      DirEntry *entry = (DirEntry *)(buff + x * sizeof(DirEntry));
      if (entry->Attr & 0x08 || entry->Name[0] == 0x00 ||
          entry->Name[0] == 0xE5 || entry->Attr == 0x0F)
        continue;
      u8 match = 1;
      for (u8 y = 0; y < 11 && name[y]; y++) {
        if (entry->Name[y] != name[y]) {
          match = 0;
          break;
        }
      }
      if (match) {
        res = *entry;
        *entry_clus = clus;
        *entry_offset = x * sizeof(DirEntry);
        return &res;
      }
    }
    clus = get_next_clus(clus);
  }
  return NULL;
}

DirEntry *create_entry(u32 clus, char *name, u8 is_dir, u32 *entry_clus,
                       u32 *entry_offset, u32 point_clus) {
  u32 file_cluster = 0;
  if (point_clus == 0) {
    file_cluster = find_free_clus();
    link_clus(file_cluster, 0x0FFFFFF8);
    if (is_dir) {
      zero_clus(file_cluster);
    }
  } else {
    file_cluster = point_clus;
  }
  u32 prev_clus = 0;
  static DirEntry new_entry = {0};
  while (clus > 1 && clus < 0x0FFFFFF8) {
    read_clus(clus, buff);
    for (u32 x = 0; x < (fs.SecPerClus * fs.BytsPerSec) / sizeof(DirEntry);
         x++) {
      DirEntry *entry = (DirEntry *)(buff + x * sizeof(DirEntry));
      if (entry->Name[0] == 0x00) {
        for (u8 y = 0; y < 11; y++) {
          entry->Name[y] = name[y];
        }
        if (is_dir == 1) {
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
        entry->FstClusHi = (file_cluster >> 16) & 0xFFFF;
        entry->FstClusLo = file_cluster & 0xFFFF;
        entry->FileSize = 0;
        write_clus(clus, buff);
        *entry_clus = clus;
        *entry_offset = x * sizeof(DirEntry);
        new_entry = *entry;
        return &new_entry;
      }
    }
    prev_clus = clus;
    clus = get_next_clus(clus);
  }

  if (new_entry.Name[0] == 0x00) {
    u32 new_clus = find_free_clus();
    link_clus(prev_clus, new_clus);
    link_clus(new_clus, 0x0FFFFFF8);
    zero_clus(new_clus);

    read_clus(clus, buff);
    for (u32 x = 0; x < (fs.SecPerClus * fs.BytsPerSec) / sizeof(DirEntry);
         x++) {
      DirEntry *entry = (DirEntry *)(buff + x * sizeof(DirEntry));
      if (entry->Name[0] == 0x00) {
        for (u8 y = 0; y < 11; y++) {
          entry->Name[y] = name[y];
        }
        if (is_dir == 1) {
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
        entry->FstClusHi = (file_cluster >> 16) & 0xFFFF;
        entry->FstClusLo = file_cluster & 0xFFFF;
        entry->FileSize = 1;
        write_clus(clus, buff);
        *entry_clus = clus;
        *entry_offset = x * sizeof(DirEntry);
        new_entry = *entry;
        return &new_entry;
      }
    }
  }

  return NULL;
}

i8 parse_path(char *path, u32 *clus, char *name, u32 *size) {
  if (path[0] != '/')
    return -1;

  path++;

  static char component[13];
  static char fat_name[11];
  u8 comp_cnt = 0;
  u32 current_clus = fs.RootClus;

  while (*path) {
    if (*path == '/') {
      if (comp_cnt == 0)
        return -1;

      component[comp_cnt] = '\0';
      str_to_fat83(component, fat_name);

      DirEntry *entry = find_entry(current_clus, fat_name, NULL, NULL);
      if (entry == NULL || !(entry->Attr & 0x10)) {
        return -1;
      }

      current_clus = (entry->FstClusHi << 16) | entry->FstClusLo;
      comp_cnt = 0;
      path++;
      continue;
    }

    if (comp_cnt > 12)
      return -1;
    component[comp_cnt++] = *path;
    path++;
  }

  if (comp_cnt == 0)
    return -1;

  component[comp_cnt] = '\0';
  str_to_fat83(component, fat_name);

  *clus = current_clus;
  for (u8 i = 0; i < 11; i++) {
    name[i] = fat_name[i];
  }

  return 0;
}

u32 *resolve_chain(u32 clus, u32 *count, u32 *cap) {
  u32 *chain = kmalloc(*cap * sizeof(u32));
  u32 chain_cnt = 0;

  while (clus > 1 && clus < 0x0FFFFFF8) {
    if (chain_cnt >= *cap) {
      u32 new_cap = (*cap < 32768) ? (*cap * 2) : (*cap + 32768);
      u32 *new_chain = kmalloc(new_cap * sizeof(u32));
      memcopy(new_chain, chain, chain_cnt * sizeof(u32));
      kfree(chain);
      chain = new_chain;
      *cap = new_cap;
    }
    chain[chain_cnt++] = clus;
    clus = get_next_clus(clus);
  }

  *count = chain_cnt;
  return chain;
}

i8 is_dir_empty(u32 dir_clus) {
  u32 count = 0;
  u32 cap = 4096;
  u32 *chain = resolve_chain(dir_clus, &count, &cap);
  u32 clus_size = fs.SecPerClus * fs.BytsPerSec;
  u16 entries_per_clus = clus_size / sizeof(DirEntry);

  for (u32 i = 0; i < count; i++) {
    read_clus(chain[i], buff);
    DirEntry *entries = (DirEntry *)buff;

    for (u16 j = 0; j < entries_per_clus; j++) {
      DirEntry *e = &entries[j];

      if (e->Name[0] == 0x00) {
        kfree(chain);
        return 1;
      }

      if (e->Name[0] == 0xE5 || e->Attr & 0x08 || e->Attr == 0x0F) {
        continue;
      }

      kfree(chain);
      return 0;
    }
  }

  kfree(chain);
  return 1;
}

u32 fs_open(char *path, u32 flags) {
  if (path[0] == '/' && path[1] == '\0') {
    u32 root_clus = fs.RootClus;
    u32 clus_count = 0;
    u32 chain_cap = 4096;
    u32 *chain = resolve_chain(root_clus, &clus_count, &chain_cap);
    char root_name[5] = "root";
    for (u32 fd = 0; fd < MAX_FILES; fd++) {
      if (fd_table[fd].used == 0) {
        fd_table[fd].used = 1;
        fd_table[fd].cluster_chain = chain;
        fd_table[fd].cluster_count = clus_count;
        fd_table[fd].chain_cap = chain_cap;
        fd_table[fd].pos = 0;
        fd_table[fd].size = 0;
        fd_table[fd].attr = 0x10;
        fd_table[fd].entry_cluster = 0;
        fd_table[fd].entry_offset = 0;
        for (u8 i = 0; i < 11; i++) {
          fd_table[fd].name[i] = (i < 4) ? "root"[i] : ' ';
        }
        return fd;
      }
    }
    return (u32)-1;
  }

  u32 dir_clus = 0;
  char name[11];
  u32 existing_size = 0;

  if (parse_path(path, &dir_clus, name, &existing_size) < 0)
    return (u32)-1;

  u32 entry_clus = 0;
  u32 entry_off = 0;

  DirEntry *entry = find_entry(dir_clus, name, &entry_clus, &entry_off);

  if (entry == NULL) {
    if (!(flags & CREATE))
      return (u32)-1;

    entry = create_entry(dir_clus, name, 0, &entry_clus, &entry_off, 0);
    if (entry == NULL)
      return (u32)-1;
  } else {
    if ((flags & CREATE) && (flags & EXCLUSIVE))
      return (u32)-1;

    if ((flags & TRUNCATE) && (entry->Attr & 0x20)) {
      u32 clus = (entry->FstClusHi << 16) | entry->FstClusLo;
      if (clus >= 2 && clus < 0x0FFFFFF8) {
        u32 count = 0;
        u32 cap = 4096;
        u32 *chain = resolve_chain(clus, &count, &cap);

        for (u32 i = 1; i < count; i++)
          link_clus(chain[i], 0);

        kfree(chain);
      }

      read_clus(entry_clus, buff);
      DirEntry *e = (DirEntry *)(buff + entry_off);

      e->FileSize = 0;
      write_clus(entry_clus, buff);

      *entry = *e;
    }
  }

  u32 file_clus = (entry->FstClusHi << 16) | entry->FstClusLo;

  u32 clus_count = 0;
  u32 chain_cap = 4096;

  u32 *chain = (file_clus >= 2)
                   ? resolve_chain(file_clus, &clus_count, &chain_cap)
                   : kmalloc(chain_cap * sizeof(u32));

  for (u32 fd = 0; fd < MAX_FILES; fd++) {
    if (fd_table[fd].used == 0) {
      fd_table[fd].used = 1;
      fd_table[fd].cluster_chain = chain;
      fd_table[fd].cluster_count = clus_count;
      fd_table[fd].chain_cap = chain_cap;
      fd_table[fd].pos =
          ((flags & APPEND) && (entry->Attr & 0x20)) ? entry->FileSize : 0;
      fd_table[fd].size = entry->FileSize;
      fd_table[fd].attr = entry->Attr;
      fd_table[fd].entry_cluster = entry_clus;
      fd_table[fd].entry_offset = entry_off;

      for (u8 i = 0; i < 11; i++)
        fd_table[fd].name[i] = entry->Name[i];

      return fd;
    }
  }

  kfree(chain);
  return (u32)-1;
}

void fs_close(u32 fd) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return;
  }

  fd_table[fd].used = 0;
  kfree(fd_table[fd].cluster_chain);
}

Stat *fs_stat(char *path) {
  static Stat out;

  if (path[0] == '/' && path[1] == '\0') {
    u32 root_clus = fs.RootClus;
    fat83_to_str("ROOT       ", out.name);
    out.size = 0;
    out.attr = 0x10;
    return &out;
  }

  u32 clus = 0;
  char name[11];
  u32 size = 0;

  i8 res = parse_path(path, &clus, name, &size);
  if (res == -1) {
    return NULL;
  }

  u32 entry_clus = 0;
  u32 entry_offset = 0;
  DirEntry *entry = find_entry(clus, name, &entry_clus, &entry_offset);
  if (entry == NULL) {
    return NULL;
  }

  fat83_to_str((char *)entry->Name, out.name);
  out.size = entry->FileSize;
  out.attr = entry->Attr;
  return &out;
}

u32 fs_read(u32 fd, void *buffer, u32 cnt) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return (u32)-1;
  }
  if (fd_table[fd].used == 0 || fd_table[fd].attr & 0x10) {
    return (u32)-1;
  }
  if (fd_table[fd].pos == fd_table[fd].size || cnt == 0) {
    return 0;
  }

  u32 clus_size = fs.SecPerClus * fs.BytsPerSec;
  u32 end = (fd_table[fd].pos + cnt < fd_table[fd].size)
                ? fd_table[fd].pos + cnt
                : fd_table[fd].size;
  u32 bytes_read = 0;

  u32 start_clus_idx = fd_table[fd].pos / clus_size;
  u32 end_clus_idx = (end - 1) / clus_size;

  for (u32 i = start_clus_idx; i <= end_clus_idx; i++) {
    u32 start_read_offset =
        (i == start_clus_idx) ? fd_table[fd].pos % clus_size : 0;
    u32 end_read_offset =
        (i == end_clus_idx)
            ? (end % clus_size == 0 ? clus_size : end % clus_size)
            : clus_size;

    u32 to_read = end_read_offset - start_read_offset;

    read_clus(fd_table[fd].cluster_chain[i], buff);

    memcopy(buffer + bytes_read, buff + start_read_offset, to_read);

    bytes_read += to_read;
    fd_table[fd].pos += to_read;
  }

  return bytes_read;
}

u32 fs_write(u32 fd, void *buffer, u32 cnt) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return (u32)-1;
  }
  if (fd_table[fd].used == 0 || fd_table[fd].attr & 0x10) {
    return (u32)-1;
  }
  if (cnt == 0) {
    return 0;
  }

  u32 cluster_size = fs.SecPerClus * fs.BytsPerSec;
  u32 max_file_size = fd_table[fd].cluster_count * cluster_size;
  u32 write_size = fd_table[fd].pos + cnt;
  u32 new_file_size = fd_table[fd].size;

  if (write_size > max_file_size) {
    u32 diff = write_size - max_file_size;
    u32 clus_need = (diff + cluster_size - 1) / cluster_size;
    while (clus_need--) {
      if (fd_table[fd].cluster_count >= fd_table[fd].chain_cap) {
        u32 new_cap = (fd_table[fd].chain_cap < 32768)
                          ? (fd_table[fd].chain_cap * 2)
                          : (fd_table[fd].chain_cap + 32768);
        u32 *new_chain = kmalloc(new_cap * sizeof(u32));
        for (u32 i = 0; i < fd_table[fd].cluster_count; i++) {
          new_chain[i] = fd_table[fd].cluster_chain[i];
        }
        kfree(fd_table[fd].cluster_chain);
        fd_table[fd].cluster_chain = new_chain;
        fd_table[fd].chain_cap = new_cap;
      }
      u32 clus = find_free_clus();
      link_clus(fd_table[fd].cluster_chain[fd_table[fd].cluster_count - 1],
                clus);
      link_clus(clus, 0x0FFFFFF8);
      fd_table[fd].cluster_chain[fd_table[fd].cluster_count++] = clus;
    }
  }

  if (write_size > fd_table[fd].size) {
    fd_table[fd].size = write_size;
  }

  u32 bytes_written = 0;

  u32 start_clus_idx = fd_table[fd].pos / cluster_size;
  u32 end_clus_idx = (fd_table[fd].pos + cnt - 1) / cluster_size;
  u32 end_pos = fd_table[fd].pos + cnt;

  for (u32 i = start_clus_idx; i <= end_clus_idx; i++) {
    u32 start_write_offset =
        (i == start_clus_idx) ? fd_table[fd].pos % cluster_size : 0;
    u32 end_write_offset = (i == end_clus_idx) ? (end_pos % cluster_size == 0
                                                      ? cluster_size
                                                      : end_pos % cluster_size)
                                               : cluster_size;

    u32 to_write = end_write_offset - start_write_offset;

    if (end_write_offset != cluster_size || start_write_offset != 0) {
      read_clus(fd_table[fd].cluster_chain[i], buff);
    }

    memcopy(buff + start_write_offset, buffer + bytes_written, to_write);

    write_clus(fd_table[fd].cluster_chain[i], buff);

    bytes_written += to_write;
    fd_table[fd].pos += to_write;
  }

  read_clus(fd_table[fd].entry_cluster, buff);
  DirEntry *entry = (DirEntry *)(buff + fd_table[fd].entry_offset);
  entry->FileSize = fd_table[fd].size;
  write_clus(fd_table[fd].entry_cluster, buff);

  return bytes_written;
}

u32 fs_seek(u32 fd, i32 offset, Whence whence) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return (u32)-1;
  }
  if (fd_table[fd].used == 0 || fd_table[fd].attr & 0x10) {
    return (u32)-1;
  }

  u32 off = 0;
  switch (whence) {
  case CUR:
    off = fd_table[fd].pos + offset;
    break;
  case END:
    off = fd_table[fd].size + offset;
    break;
  case SET:
    off = offset;
    break;
  }

  if (off < 0) {
    fd_table[fd].pos = 0;
  } else if (off > fd_table[fd].size) {
    fd_table[fd].pos = fd_table[fd].size;
  } else {
    fd_table[fd].pos = off;
  }

  return fd_table[fd].pos;
}

u32 fs_tell(u32 fd) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return (u32)-1;
  }
  if (fd_table[fd].used == 0 || fd_table[fd].attr & 0x10) {
    return (u32)-1;
  }

  return fd_table[fd].pos;
}

i8 fs_mkdir(char *path) {
  u32 clus = 0;
  char name[11];
  u32 size = 0;

  i8 res = parse_path(path, &clus, name, &size);
  if (res == -1) {
    return -1;
  }

  u32 entry_clus = 0;
  u32 entry_offset = 0;
  DirEntry *entry = find_entry(clus, name, &entry_clus, &entry_offset);
  if (entry != NULL) {
    return -2;
  }

  entry = create_entry(clus, name, 1, &entry_clus, &entry_offset, 0);
  if (entry == NULL) {
    return -3;
  }

  return 0;
}

Stat *fs_readdir(u32 fd) {
  if (fd == (u32)-1 || fd > MAX_FILES) {
    return NULL;
  }
  if (fd_table[fd].used == 0 || fd_table[fd].attr & 0x20) {
    return NULL;
  }

  u32 cluster_size = fs.SecPerClus * fs.BytsPerSec;
  u16 entries_per_clus = cluster_size / sizeof(DirEntry);
  static Stat entry;

  while (1) {
    u32 clus_idx = fd_table[fd].pos / entries_per_clus;

    if (fd_table[fd].cluster_chain[clus_idx] < 2 ||
        fd_table[fd].cluster_chain[clus_idx] >= 0x0FFFFFF8) {
      return NULL;
    }

    u32 bytes_offset = (fd_table[fd].pos % entries_per_clus) * sizeof(DirEntry);
    read_clus(fd_table[fd].cluster_chain[clus_idx], buff);
    DirEntry *e = (DirEntry *)(buff + bytes_offset);

    if (e->Name[0] == 0x00) {
      return NULL;
    }

    fd_table[fd].pos++;

    if (e->Name[0] == 0xE5 || e->Attr & 0x08 || e->Attr == 0x0F) {
      continue;
    }

    entry.size = e->FileSize;
    fat83_to_str((char *)e->Name, entry.name);
    entry.attr = e->Attr;
    return &entry;
  }
}

i8 fs_remove(char *path) {
  u32 clus = 0;
  char name[11];
  u32 size = 0;
  i8 res = parse_path(path, &clus, name, &size);
  if (res == -1) {
    return -3;
  }
  u32 entry_clus = 0;
  u32 entry_offset = 0;
  DirEntry *entry = find_entry(clus, name, &entry_clus, &entry_offset);
  if (entry == NULL) {
    return -2;
  }

  if (entry->Attr & 0x10) {
    return -4;
  }

  read_clus(entry_clus, buff);
  DirEntry *e = (DirEntry *)(buff + entry_offset);
  e->Name[0] = 0xE5;
  write_clus(entry_clus, buff);

  u32 count = 0;
  u32 cap = 4096;
  u32 *chain = resolve_chain(((entry->FstClusHi << 16) | entry->FstClusLo),
                             &count, &cap);
  for (u32 i = 0; i < count; i++) {
    link_clus(chain[i], 0);
  }
  kfree(chain);

  return 0;
}

i8 fs_rmdir(char *path) {
  if (path[0] == '/' && path[1] == '\0') {
    return -5;
  }

  u32 clus = 0;
  char name[11];
  u32 size = 0;
  i8 res = parse_path(path, &clus, name, &size);
  if (res == -1) {
    return -3;
  }

  u32 entry_clus = 0;
  u32 entry_offset = 0;
  DirEntry *entry = find_entry(clus, name, &entry_clus, &entry_offset);
  if (entry == NULL) {
    return -2;
  }

  if (!(entry->Attr & 0x10)) {
    return -4;
  }

  u32 dir_clus = (entry->FstClusHi << 16) | entry->FstClusLo;
  if (!is_dir_empty(dir_clus)) {
    return -6;
  }

  read_clus(entry_clus, buff);
  DirEntry *e = (DirEntry *)(buff + entry_offset);
  e->Name[0] = 0xE5;
  write_clus(entry_clus, buff);

  u32 count = 0;
  u32 cap = 4096;
  u32 *chain = resolve_chain(dir_clus, &count, &cap);
  for (u32 i = 0; i < count; i++) {
    link_clus(chain[i], 0);
  }
  kfree(chain);

  return 0;
}
