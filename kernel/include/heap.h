#pragma once
#include "pmm.h"
#include "types.h"

void heap_init(void);
void *kmalloc(u64 size);
void kfree(void *ptr);
