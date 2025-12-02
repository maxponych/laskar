#include "pmm.h"

u8 *bitmap = NULL;
u64 bitmap_size = 0;

void print_gb(u64 bytes, const char *label) {
  u64 gb = bytes / (1024ULL * 1024 * 1024);

  u64 mb_total = bytes / (1024ULL * 1024);
  u64 mb_frac = mb_total - gb * 1024;

  u64 hundredths = (mb_frac * 100 + 512) / 1024;

  if (hundredths == 100) {
    gb += 1;
    hundredths = 0;
  }

  print(label);
  printnum(gb);
  printc('.');
  if (hundredths < 10)
    printc('0');
  printnum(hundredths);
  print(" GB\n");
}

void mark_used(u8 *bitmap, u64 page_start, u64 page_end) {
  u64 start_byte = page_start / 8;
  u64 end_byte = page_end / 8;
  u8 start_bit = page_start % 8;
  u8 end_bit = page_end % 8;

  if (start_byte == end_byte) {
    for (u8 bit = start_bit; bit <= end_bit; bit++) {
      bitmap[start_byte] |= (1 << bit);
    }
  } else {
    for (u8 bit = start_bit; bit < 8; bit++) {
      bitmap[start_byte] |= (1 << bit);
    }

    for (u64 b = start_byte + 1; b < end_byte; b++) {
      bitmap[b] = 0xFF;
    }

    for (u8 bit = 0; bit <= end_bit; bit++) {
      bitmap[end_byte] |= (1 << bit);
    }
  }
}

void pmm_init(MemoryMap *mmap, u64 own_size) {
  u64 highest_addr = 0;
  for (u16 i = 0; i < mmap->entry_count; i++) {
    u64 region_end =
        mmap->entries[i].base + (mmap->entries[i].page_count * 4096);
    if (region_end > highest_addr)
      highest_addr = region_end;
  }

  u64 total_pages = highest_addr / 4096;
  bitmap_size = (total_pages + 7) / 8;

  for (u16 i = mmap->entry_count; i > 0; i--) {
    u64 region_size = mmap->entries[i - 1].page_count * 4096;
    if (region_size > bitmap_size) {
      u64 end = mmap->entries[i - 1].base + region_size;
      bitmap = (u8 *)(end - bitmap_size);
      break;
    }
  }

  for (u64 i = 0; i < bitmap_size; i++) {
    bitmap[i] = 0xFF;
  }

  for (u16 i = 0; i < mmap->entry_count; i++) {
    u64 start_addr = mmap->entries[i].base;
    u64 end_addr = start_addr + (mmap->entries[i].page_count * 4096);
    u64 size = end_addr - start_addr;

    u64 start_byte = (start_addr / 4096) / 8;
    u8 start_bit = (start_addr / 4096) % 8;
    u64 end_byte = (end_addr / 4096) / 8;
    u8 end_bit = (end_addr / 4096) % 8;

    if (start_byte == end_byte) {
      for (u8 bit = start_bit; bit <= end_bit; bit++) {
        bitmap[start_byte] &= ~(1 << bit);
      }
    } else {
      for (u8 bit = start_bit; bit < 8; bit++) {
        bitmap[start_byte] &= ~(1 << bit);
      }

      for (u64 b = start_byte + 1; b < end_byte; b++) {
        bitmap[b] = 0x00;
      }

      for (u8 bit = 0; bit <= end_bit; bit++) {
        bitmap[end_byte] &= ~(1 << bit);
      }
    }
  }

  u64 kernel_start = 0x100000 / 4096;
  u64 kernel_end = (0x100000 + own_size - 1) / 4096;
  mark_used(bitmap, kernel_start, kernel_end);

  u64 bitmap_start = (u64)bitmap / 4096;
  u64 bitmap_end = ((u64)bitmap + bitmap_size - 1) / 4096;
  mark_used(bitmap, bitmap_start, bitmap_end);

  u64 mem = bitmap_size * 8 * 4096;

  print_gb(mem, "Total memory: ");
}

void *alloc_page() {
  for (u64 byte = 0; byte < bitmap_size; byte++) {
    if (bitmap[byte] == 0xFF)
      continue;

    for (u8 bit = 0; bit < 8; bit++) {
      if (((bitmap[byte] >> bit) & 1) == 0) {
        bitmap[byte] |= (1 << bit);
        u64 page = byte * 8 + bit;
        return (void *)(page * 4096ULL);
      }
    }
  }

  return NULL;
}

void *alloc_pages(u32 count) {
  u32 consecutive = 0;
  u64 start_page = 0;
  u32 total_pages = bitmap_size * 8;

  for (u64 page = 0; page < total_pages; page++) {
    u64 byte = page / 8;
    u8 bit = page % 8;

    if (((bitmap[byte] >> bit) & 1) == 0) {
      if (consecutive == 0)
        start_page = page;
      consecutive += 1;

      if (consecutive == count) {
        for (u64 p = start_page; p < start_page + count; p++) {
          u64 byte = p / 8;
          u8 bit = p % 8;
          bitmap[byte] |= (1 << bit);
        }
        return (void *)(start_page * 4096ULL);
      }
    } else {
      consecutive = 0;
    }
  }

  return NULL;
}

void free_page(void *addr) {
  if ((u64)addr % 4096 != 0)
    return;

  u64 page = (u64)addr / 4096;
  u64 byte = page / 8;
  u8 bit = page % 8;
  bitmap[byte] &= ~(1 << bit);
}

void free_pages(void *addr, u32 count) {
  if ((u64)addr % 4096 != 0)
    return;

  u64 start_page = (u64)addr / 4096;
  for (u64 page = start_page; page < start_page + count; page++) {
    u64 byte = page / 8;
    u8 bit = page % 8;

    bitmap[byte] &= ~(1 << bit);
  }
}

void free_mem_stat() {
  u64 total_mem = bitmap_size * 8 * 4096;
  u64 free_mem = 0;
  for (u64 page = 0; page < bitmap_size * 8; page++) {
    u64 byte = page / 8;
    u8 bit = page % 8;
    if (((bitmap[byte] >> bit) & 1) == 0) {
      free_mem += 4096;
    }
  }

  print_gb(total_mem, "Total memory: ");
  print_gb(free_mem, "Free memory:  ");
}
