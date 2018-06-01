#pragma once
#include <stm32f4xx_hal.h>

#define SRAM_SIZE			0x1024000U		//SRAM¥Û–° (Bytes)
#define FSMC_SRAM_BASE		0x68000000U		//Bank1.Sector3

void SRAM_WriteBytes(uint32_t offset, uint8_t* pBuffer, uint32_t count);
void SRAM_ReadBytes(uint32_t offset, uint8_t* pBuffer, uint32_t count);
