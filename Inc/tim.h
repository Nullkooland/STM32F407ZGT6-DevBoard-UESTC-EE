#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>

void Delay_ms(uint16_t ms);
void Delay_us(uint32_t us);

void TIM13_PWM_Output_Init(uint16_t period);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

extern void _Error_Handler(char *, int);

