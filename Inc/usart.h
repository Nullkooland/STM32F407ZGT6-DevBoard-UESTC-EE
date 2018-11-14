#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>

#define UART_RX_BUFFER_SIZE 2048

typedef struct {
    uint8_t Data[UART_RX_BUFFER_SIZE];
    _Bool RxEnd;
    uint16_t RxCount;
} UART_RxBufferTypeDef;

void USART1_UART_Init(void);
extern void _Error_Handler(char *, int);
