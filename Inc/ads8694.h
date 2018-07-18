#pragma once
#include <stm32f4xx_hal.h>

#define ADS8694_CS0			GPIOB->BSRR = (uint32_t)GPIO_PIN_11 << 16U
#define ADS8694_CS1			GPIOB->BSRR = GPIO_PIN_11

#define ADS8694_RST0		GPIOG->BSRR = (uint32_t)GPIO_PIN_7 << 16U
#define ADS8694_RST1		GPIOG->BSRR = GPIO_PIN_7

typedef enum {
	NO_OP		= 0x0000,	// Continued Operation
	STDBY		= 0x8200,	// Standby
	PWR_DN		= 0x8300,	// Power Down
	RST			= 0x8500,	// Reset program registers
	AUTO_RST	= 0xA000,	// Auto Channel x Sequence with Reset
	MAN_CH0		= 0xC000,	// Manual Channel 0 Selection
	MAN_CH1		= 0xC400,	// Manual Channel 1 Selection
	MAN_CH2		= 0xC800,	// Manual Channel 2 Selection
	MAN_CH3		= 0xCC00,	// Manual Channel 3 Selection
	MAN_AUX		= 0xE000,	// Manual Aux Selection
} ADS8694_Cmd;

#define ADS8694_CHANNEL_0		0x01
#define ADS8694_CHANNEL_1		0x02
#define ADS8694_CHANNEL_2		0x04
#define ADS8694_CHANNEL_3		0x08
#define ADS8694_CHANNEL_All		0xFF

void ADS8694_Init(void);
void ADS8694_SendCmd(uint16_t cmd);
void ADS8694_SetAutoScanSequence(uint8_t channels);
void ADS8694_SetChannelPowerDown(uint8_t channels);
void ADS8694_SetInputRange(uint8_t channel, uint8_t range);
void ADS8694_Sampling(int32_t *pBuffer, uint32_t count);

static inline void ADS8694_Reset(void);
static void ADS8694_WriteReg(uint8_t addr, uint8_t data);

extern uint8_t SPI2_RW_Byte(uint8_t data);
