#pragma once
#include "io.h"
#include "types.h"

void kb_init();
void kb_listen(u8 *buff, u16 *bytes);
u8 kb_read();
