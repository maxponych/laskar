#pragma once
#include "print.h"
#include "types.h"

void pmm_init(MemoryMap *mmap, u64 own_size);
void *alloc_page(void);
void *alloc_pages(u32 count);
void free_page(void *addr);
void free_pages(void *addr, u32 count);

void free_mem_stat();
