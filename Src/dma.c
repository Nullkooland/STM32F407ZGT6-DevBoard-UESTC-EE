/* Includes ------------------------------------------------------------------*/
#include "dma.h"

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

DMA_HandleTypeDef hdma_m2m;

DMA_HandleTypeDef hdma_adc1;

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_m2m
  */
void DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* Configure DMA request hdma_m2m on DMA2_Stream0 */
	hdma_m2m.Instance = DMA2_Stream0;
	hdma_m2m.Init.Channel = DMA_CHANNEL_0;
	hdma_m2m.Init.Direction = DMA_MEMORY_TO_MEMORY;
	hdma_m2m.Init.PeriphInc = DMA_PINC_ENABLE;
	hdma_m2m.Init.MemInc = DMA_MINC_ENABLE;
	hdma_m2m.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_m2m.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma_m2m.Init.Mode = DMA_NORMAL;
	hdma_m2m.Init.Priority = DMA_PRIORITY_LOW;
	hdma_m2m.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
	hdma_m2m.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
	hdma_m2m.Init.MemBurst = DMA_MBURST_SINGLE;
	hdma_m2m.Init.PeriphBurst = DMA_PBURST_SINGLE;
	if (HAL_DMA_Init(&hdma_m2m) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* DMA interrupt init */
	/* DMA2_Stream2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	/* DMA2_Stream7_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 1, 2);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

void DMA2_Stream2_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void DMA2_Stream7_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

void DMA2_Stream4_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_adc1);
}