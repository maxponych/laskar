#pragma once
#include "pmm.h"
#include "types.h"

#define PAGES_CNT 2560

typedef struct FreeBlock {
  u64 size;
  struct FreeBlock *next;
} FreeBlock;

void heap_init(void);
void *kmalloc(u64 size);
void kfree(void *ptr);
