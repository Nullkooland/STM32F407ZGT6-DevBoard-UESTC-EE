#pragma once

#include "ff.h"
#include "diskio.h"

extern FATFS fs;
extern char SD_Path[4];

void FATFS_Init(void);
