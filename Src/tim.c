/* Includes ------------------------------------------------------------------*/
#include "tim.h"

TIM_HandleTypeDef htim13;

void Delay_ms(uint16_t ms)
{
	uint32_t temp;
	SysTick->LOAD = ms * 21000;
	SysTick->VAL = 0;
	SysTick->CTRL = 1;
	do
	{
		temp = SysTick->CTRL;
	} while (temp & 0x01 && !(temp & (1 << 16)));
	SysTick->CTRL = 0;
	SysTick->VAL = 0;
}

void Delay_us(uint32_t us)
{
	uint32_t temp;
	SysTick->LOAD = us * 21;
	SysTick->VAL = 0;
	SysTick->CTRL = 1;
	do
	{
		temp = SysTick->CTRL;
	} while (temp & 0x01 && !(temp & (1 << 16)));
	SysTick->CTRL = 0;
	SysTick->VAL = 0;
}

/* TIM13 init function */
void TIM13_PWM_Output_Init(uint16_t period)
{
	TIM_OC_InitTypeDef sConfigOC;

	htim13.Instance = TIM13;
	htim13.Init.Prescaler = 83;							//分频系数 84MHz / (83 + 1) => 1MHz
	htim13.Init.CounterMode = TIM_COUNTERMODE_UP;		//向上计数模式
	htim13.Init.Period = period - 1;					//周期

	if (HAL_TIM_PWM_Init(&htim13) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = period / 2 + 1;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

	if (HAL_TIM_PWM_ConfigChannel(&htim13, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	HAL_TIM_MspPostInit(&htim13);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{
	if (tim_pwmHandle->Instance == TIM13)
	{
		__HAL_RCC_TIM13_CLK_ENABLE();
	}
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (timHandle->Instance == TIM13)
	{
		/**TIM13 GPIO Configuration
		PF8     ------> TIM13_CH1
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF9_TIM13;
		HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{
	if (tim_baseHandle->Instance == TIM13)
	{
		/* USER CODE BEGIN TIM13_MspDeInit 0 */

		/* USER CODE END TIM13_MspDeInit 0 */
		  /* Peripheral clock disable */
		__HAL_RCC_TIM13_CLK_DISABLE();
		/* USER CODE BEGIN TIM13_MspDeInit 1 */

		/* USER CODE END TIM13_MspDeInit 1 */
	}
}