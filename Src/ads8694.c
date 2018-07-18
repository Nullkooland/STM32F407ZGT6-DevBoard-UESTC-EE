#include "ads8694.h"
#include "spi.h"

void ADS8694_Init(void)
{
	/* PCLK1 / 4 = 10.5 MHz */
	SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_4);
	/* Chip Select GPIO Init */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Reset GPIO Init */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	ADS8694_Reset();
}

void ADS8694_SetAutoScanSequence(uint8_t channels) {
	ADS8694_WriteReg(0x01, channels);
}

void ADS8694_SetChannelPowerDown(uint8_t channels) {
	ADS8694_WriteReg(0x02, channels);
}

void ADS8694_SetInputRange(uint8_t channel, uint8_t range)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		if (channel & 1) {
			ADS8694_WriteReg(0x05 + i, range);
		}
		channel >>= 1;
	}
}

void ADS8694_SendCmd(uint16_t cmd)
{
	/* Start command frame */
	ADS8694_CS0;
	/* Send command */
	SPI2_RW_Byte(cmd >> 8);
	SPI2_RW_Byte(cmd & 0xFF);
	/* Dummy read to form a complete frame */
	SPI2_RW_Byte(0xFF);
	SPI2_RW_Byte(0xFF);
	SPI2_RW_Byte(0xFF);
	/* Pull CS to HIGH for at least 30ns */
	ADS8694_CS1;
	__NOP();
}

static void ADS8694_WriteReg(uint8_t addr, uint8_t data)
{
	union {
		struct {
			unsigned Data	: 8;
			_Bool WR		: 1;
			unsigned Addr	: 7;
		} Reg;
		uint8_t Bytes[2];
	} write_data;

	write_data.Reg.WR = 1;
	write_data.Reg.Addr = addr;
	write_data.Reg.Data = data;
	
	/* Start write frame */
	ADS8694_CS0;
	/* Write program register */
	SPI2_RW_Byte(write_data.Bytes[0]);
	SPI2_RW_Byte(write_data.Bytes[1]);
	/* Read the data just been written for check */
	uint8_t echo_data = SPI2_RW_Byte(0xFF);
	/* Pull CS to HIGH for at least 30ns */
	ADS8694_CS1;
	__NOP();
}

void ADS8694_Sampling(int32_t *pBuffer, uint32_t count)
{
	union {
		int32_t Value;
		uint8_t Bytes[3];
	} adc_code;

	for (uint32_t i = 0; i < count; i++)
	{
		/* Start data frame */
		ADS8694_CS0;
		/* Send 0x0000 for continuous mode */
		SPI2_RW_Byte(0x00);
		SPI2_RW_Byte(0x00);
		/* Read 18-bit ADC Code */
		adc_code.Bytes[0] = SPI2_RW_Byte(0xFF);
		adc_code.Bytes[1] = SPI2_RW_Byte(0xFF);
		adc_code.Bytes[2] = SPI2_RW_Byte(0xFF);
		/* Shift 6 bit */
		pBuffer[i] = adc_code.Value >> 6;
		/* Pull CS to HIGH for at least 30ns */
		ADS8694_CS1;
		__NOP();
	}
}

static inline void ADS8694_Reset(void)
{
	ADS8694_RST0;
	__NOP();
	ADS8694_RST1;
}
