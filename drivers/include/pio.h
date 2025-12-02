#pragma once
#include "io.h"
#include "print.h"
#include "types.h"

void write_sector(u32 lba, u8 *buffer);
void read_sector(u32 lba, u8 *buff);
