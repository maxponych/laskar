#pragma once
#include "heap.h"
#include "io.h"
#include "layout.h"
#include "print.h"
#include "types.h"

typedef struct {
  u8 buff[256];
  u8 head;
  u8 tail;
} Ring;

void kb_init(void);
void kb_handler(void);
u8 kb_read(void);
