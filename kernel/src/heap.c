#include "heap.h"

static FreeBlock *free_list = NULL;

void heap_init() {
  void *heap = alloc_pages(256);

  free_list = (FreeBlock *)heap;
  free_list->size = 256 * 4096 - sizeof(FreeBlock);
  free_list->next = NULL;
}

void *kmalloc(u64 size) {
  FreeBlock *prev = NULL;
  FreeBlock *curr = free_list;

  size = (size + 7) & ~7;

  while (curr) {
    if (curr->size > size) {
      FreeBlock *new_block =
          (FreeBlock *)((u8 *)curr + sizeof(FreeBlock) + size);

      new_block->size = curr->size - sizeof(FreeBlock) - size;
      new_block->next = curr->next;

      if (prev) {
        prev->next = new_block;
      } else {
        free_list = new_block;
      }

      curr->size = size;
      return (u8 *)curr + sizeof(FreeBlock);
    }

    if (curr->size == size) {
      if (prev) {
        prev->next = curr->next;
      } else {
        free_list = curr->next;
      }

      return (u8 *)curr + sizeof(FreeBlock);
    }

    prev = curr;
    curr = curr->next;
  }

  return NULL;
}

void kfree(void *ptr) {
  FreeBlock *block = ptr - sizeof(FreeBlock);
  FreeBlock *prev = NULL;
  FreeBlock *curr = free_list;

  while (curr && curr < block) {
    prev = curr;
    curr = curr->next;
  }

  block->next = curr;
  if (prev) {
    prev->next = block;
  } else {
    free_list = block;
  }

  if ((u8 *)curr &&
      (u8 *)block + sizeof(FreeBlock) + block->size == (u8 *)curr) {
    block->size += sizeof(FreeBlock) + curr->size;
    block->next = curr->next;
  }

  if ((u8 *)prev &&
      (u8 *)prev + sizeof(FreeBlock) + prev->size == (u8 *)block) {
    prev->size += sizeof(FreeBlock) + block->size;
    prev->next = block->next;
  }
}
