#include "heap.h"

static FreeBlock *free_list = NULL;

void heap_init() {
  void *heap = alloc_pages(PAGES_CNT);

  if (heap == NULL) {
    println("FATAL: Cannot allocate heap stack");
    while (1) {
      __asm__ volatile("hlt");
    }
  }

  free_list = (FreeBlock *)heap;
  free_list->size = PAGES_CNT * 4096 - sizeof(FreeBlock);
  free_list->next = NULL;
}

void *kmalloc(u64 size) {
  FreeBlock *prev = NULL;
  FreeBlock *curr = free_list;
  size = (size + 7) & ~7;

  if (size < 256) {
    size = 256;
  }

  while (curr) {
    if (curr->size >= size + sizeof(FreeBlock)) {
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

    if (curr->size >= size) {
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
  if (ptr == NULL)
    return;

  FreeBlock *block = (FreeBlock *)((u8 *)ptr - sizeof(FreeBlock));
  FreeBlock *prev = NULL;
  FreeBlock *curr = free_list;

  while (curr != NULL && curr < block) {
    prev = curr;
    curr = curr->next;
  }

  block->next = curr;
  if (prev != NULL) {
    prev->next = block;
  } else {
    free_list = block;
  }

  if (curr != NULL &&
      (u8 *)block + sizeof(FreeBlock) + block->size == (u8 *)curr) {
    block->size += sizeof(FreeBlock) + curr->size;
    block->next = curr->next;
  }

  if (prev != NULL &&
      (u8 *)prev + sizeof(FreeBlock) + prev->size == (u8 *)block) {
    prev->size += sizeof(FreeBlock) + block->size;
    prev->next = block->next;
  }
}
