#include "fatfs.h"

char SDPath[4];
FATFS fs;

void FATFS_Init(void)
{
    if (f_mount(&fs, (TCHAR const*)SDPath, 1) != FR_OK)
    {
        while (1);
    }
}