#include "sram.h"

void SRAM_WriteBytes(uint32_t offset, uint8_t* pBuffer, uint32_t count)
{
    __IO uint16_t *addr = (__IO uint16_t*)(FSMC_SRAM_BASE + offset);
    uint16_t *pBuffer16 = (uint16_t *)pBuffer;

    for (uint32_t i = 0; i < count >> 1; i++) {
        *(addr++) = *(pBuffer16++);
    }

    if (count & 1) {
        *addr = (uint16_t)(pBuffer[count - 1] << 4);
    }
}

void SRAM_ReadBytes(uint32_t offset, uint8_t* pBuffer, uint32_t count)
{
    __IO uint16_t *addr = (__IO uint16_t*)(FSMC_SRAM_BASE + offset);
    uint16_t *pBuffer16 = (uint16_t *)pBuffer;

    for (uint32_t i = 0; i < count >> 1; i++) {
        *(pBuffer16++) = *(addr++);
    }

    if (count & 1) {
        pBuffer[count - 1] = (uint8_t)(*addr >> 4);
    }
}


