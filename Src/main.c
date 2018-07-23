#include <stm32f4xx_hal.h>
#include <stdio.h>
#include <arm_math.h>
/* System Peripherals */
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "fsmc.h"
#include "fatfs.h"
/* External Hardware */
#include "ads8694.h"
#include "zlg7290.h"
#include "lcd.h"
#include "sram.h"
#include "ad9959.h"
/* Applications & GUI */
//#include "frequency_sweep.h"
//#include "number_input.h"
#include "oscilloscope.h"
#include "spectrum.h"

static void SystemClock_Config(void);
void _Error_Handler(char*, int);

extern UART_RxBufferTypeDef uart1_rx_buffer;

_Bool is_extra_gain;

int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* Initialize system peripherals */
	DMA_Init();
	FSMC_Init();
	USART1_UART_Init();
	/* Initialize all external hardware */
	ZLG7290_Init();
	LCD_Init(0);
	/* Initialize Fat FileSystem for SD Card */
	FATFS_Init();
	/* Load GBK Fontlib */
	LCD_GBKFontLib_Init(SANS);
	/* Initialize I/O LED */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	//FreqSweep_Init();
	//FreqSweep_Start();
	
	//Oscilloscope_Init();
	//Oscilloscope_Start();

	//SpectrumDisplay_Init(); 
	//SpectrumDisplay_Start();

	LCD_DrawPicture_SD(0, 0, 800, 480, "0:WBB.RGB16");
	LCD_DrawString(320, 240, 32, "´ýÃüÖÐ£¡", BLACK);

	is_extra_gain = 0;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, !is_extra_gain);

	for (;;)
	{
		switch (ZLG7290_ReadKey())
		{
		case 1: 	
			Oscilloscope_Init();
			Oscilloscope_Start();
			LCD_Clear(BLACK);
			LCD_DrawString(320, 240, 32, "´ýÃüÖÐ£¡", WHITE);
			break;

		case 2:
			SpectrumDisplay_Init();
			SpectrumDisplay_Start();
			LCD_Clear(BLACK);
			LCD_DrawString(320, 240, 32, "´ýÃüÖÐ£¡", WHITE);
			break;

		case 34:
			is_extra_gain = !is_extra_gain;
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, !is_extra_gain);
			break;

		default:
			break;
		}

		Delay_ms(100);
		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_10);

		/*
		Graph_DrawCursorX(&graph, cursorXA, YELLOW, cursorXB, BROWN);

		Graph_DrawCurve(&graph, ch0, RED);
		Graph_DrawCurve(&graph, ch1, BLUE);
		LCD_BackBuffer_Update();
		Delay_ms(10);

		Graph_RecoverCursorX(&graph, cursorXA, cursorXB);

		++cursorXA;
		cursorXB += 2;
		cursorXA %= graph.Width;
		cursorXB %= graph.Width;

		Graph_RecoverGrid(&graph, ch0);
		Graph_RecoverGrid(&graph, ch1);
		*/
	}
}

static void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/* Configure the main internal regulator output voltage */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Initializes the CPU, AHB and APB busses clocks */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Configure the Systick */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 8000);
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK_DIV8);
	//HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
}

void HAL_MspInit(void)
{
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	/* MemoryManagement_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
	/* BusFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
	/* UsageFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
	/* SVCall_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
	/* DebugMonitor_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
}

void _Error_Handler(char * file, int line)
{
	printf("Error at file:%s, line:%d\n", file, line);
	while (1);
}

void HardFault_Handler(void)
{
	puts("HARD FAULT!!!");
	while (1);
}
