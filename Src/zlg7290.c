#include "zlg7290.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c1;
static _Bool is_keydown;

void ZLG7290_Init(void)
{
    I2C1_Init();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 3);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

uint8_t ZLG7290_ReadKey(void)
{
    if (!is_keydown) return 0;
    is_keydown = 0;

    uint8_t data = 0x01;
    HAL_I2C_Master_Transmit(&hi2c1, ZLG7290_WRITE_ADDR, &data, 1, 0xFF);
    HAL_I2C_Master_Receive(&hi2c1, ZLG7290_READ_ADDR, &data, 1, 0xFF);
    return data;
}

void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_FLAG(GPIO_PIN_0)) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
        is_keydown = 1;
    }
}