#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>

void FSMC_Init(void);
static void HAL_FSMC_MspInit(void);

extern void _Error_Handler(char *, int);