#include "sdcard.h"
#include "spi.h"

SD_CARDINFO sdcard_info;
extern SPI_HandleTypeDef hspi2;
//extern DMA_HandleTypeDef hdma_spi2_rx;
//extern DMA_HandleTypeDef hdma_spi2_tx;

const uint8_t dummy_ = 0xFF;

SD_ERROR SD_Init(void)
{
	//if ((sdcard_info.Status & STA_NOINIT) == 0x00) return SD_NO_ERROR;
	SD_ERROR result;
	uint32_t i;

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	SPI2_Init();
	SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_256);
	__HAL_SPI_ENABLE(&hspi2);

	sdcard_info.Status = STA_NOINIT;
	sdcard_info.Type = SD_TYPE_UNKNOW;

	SD_CS1;
	for (i = 0; i < 10; i++)
	{
		SPI2_RW_Byte(dummy_);
	}

	if (SD_SendCmd(0, 0, 0x95) != SD_IN_IDLE_STATE) return SD_RESPONSE_FAILURE;

	SPI2_RW_Byte(dummy_);
	SPI2_RW_Byte(dummy_);

	result = SD_SendCmd(8, 0x01AA, 0x87);

	if (result == SD_IN_IDLE_STATE) // SDv2 Card or later
	{
		uint8_t ocr[4]; //OCR (Operation Conditions Register)
		ocr[0] = SPI2_RW_Byte(dummy_);
		ocr[1] = SPI2_RW_Byte(dummy_);
		ocr[2] = SPI2_RW_Byte(dummy_);
		ocr[3] = SPI2_RW_Byte(dummy_);

		if (ocr[2] == 0x01 && ocr[3] == 0xAA) //电压范围2.7V~3.6V
		{
			i = 40000;
			do
			{
				result = SD_SendCmd(55, 0, dummy_); // CMD55
				if (result == SD_IN_IDLE_STATE)
				{
					result = SD_SendCmd(41, 0x40000000, dummy_); // ACMD41 注意参数
				}
			} while ((result != SD_NO_ERROR) && --i);

			if (i)
			{
				// CMD58 读取 OCR
				result = SD_SendCmd(58, 0, dummy_);
				if (result == SD_NO_ERROR)
				{
					ocr[0] = SPI2_RW_Byte(dummy_);
					ocr[1] = SPI2_RW_Byte(dummy_);
					ocr[2] = SPI2_RW_Byte(dummy_);
					ocr[3] = SPI2_RW_Byte(dummy_);
					if (ocr[0] & 0x40) // CCS bit (Card Capacity Status)
					{
						sdcard_info.Type = SD_TYPE_SDHC; // or SDXC
					}
					else
					{
						sdcard_info.Type = SD_TYPE_SDV2;
					}
				}
			}
		}
	}
	else
	{
		i = 40000;
		do // 初始化
		{
			result = SD_SendCmd(55, 0, dummy_); // CMD55
			if (result == SD_IN_IDLE_STATE)
			{
				result = SD_SendCmd(41, 0x00, dummy_); // ACMD41 参数与 SDv2 不同
			}
		} while ((result != SD_NO_ERROR) && --i);
		if (i) // 有回应是 SDv1, 且初始化完成
		{
			sdcard_info.Type = SD_TYPE_SDV1;
		}
		else // 没有回应是 MMC
		{
			i = 40000;
			do // 初始化
			{
				result = SD_SendCmd(1, 0x00, dummy_); //CMD1 参数与 SDv2 不同
			} while ((result != SD_NO_ERROR) && --i);
			if (i) sdcard_info.Type = SD_TYPE_MMC3;
		}
	}

	if (sdcard_info.Type != SD_TYPE_UNKNOW)
	{
		sdcard_info.Status &= ~STA_NOINIT; //初始化成功
										   // CMD59 关闭 CRC 校验
		SD_SendCmd(59, 0x00, dummy_);
		// CMD16 设置 BLOCK 为 512 Bytes (对 SDHC/SDHX 无效)
		SD_SendCmd(16, 0x200, dummy_);

		result = SD_NO_ERROR;
	}
	else
	{
		result = SD_RESPONSE_FAILURE;
	}
	
	SD_CS1;
	SPI2_SetSpeed(SPI_BAUDRATEPRESCALER_2);
	SD_GetCardInfo();
	return result;
}

SD_ERROR SD_GetCardInfo(void)
{
	SD_ERROR result;
	uint8_t CSD_Tab[16];
	uint8_t CID_Tab[16];
	uint32_t i;

	/* Send CMD9, Read CSD */
	result = SD_SendCmd(9, 0, 0xFF);

	if (result == SD_NO_ERROR)
	{
		i = 10;
		while (SPI2_RW_Byte(dummy_) != 0xFE && --i);

		if (i > 0)
		{
			for (i = 0; i < 16; i++) // Store CSD register value on CSD_Tab
			{
				CSD_Tab[i] = SPI2_RW_Byte(dummy_);
			}
			// Get CRC bytes (not really needed by ME, but required by SD)
			SPI2_RW_Byte(dummy_);
			SPI2_RW_Byte(dummy_);
		}
	}
	SD_CS1;
	SPI2_RW_Byte(dummy_);

	sdcard_info.CSD.CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
	sdcard_info.CSD.SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
	sdcard_info.CSD.Reserved1 = CSD_Tab[0] & 0x03;

	sdcard_info.CSD.TAAC = CSD_Tab[1];

	sdcard_info.CSD.NSAC = CSD_Tab[2];

	sdcard_info.CSD.MaxBusClkFrec = CSD_Tab[3];

	sdcard_info.CSD.CardComdClasses = CSD_Tab[4] << 4;

	sdcard_info.CSD.CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
	sdcard_info.CSD.RdBlockLen = CSD_Tab[5] & 0x0F;

	sdcard_info.CSD.PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
	sdcard_info.CSD.WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
	sdcard_info.CSD.RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
	sdcard_info.CSD.DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
	sdcard_info.CSD.Reserved2 = 0;
	sdcard_info.CSD.DeviceSize = (CSD_Tab[6] & 0x03) << 10;

	sdcard_info.CSD.DeviceSize |= (CSD_Tab[7]) << 2;

	sdcard_info.CSD.DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
	sdcard_info.CSD.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
	sdcard_info.CSD.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);

	sdcard_info.CSD.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
	sdcard_info.CSD.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
	sdcard_info.CSD.DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;


	sdcard_info.CSD.DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
	sdcard_info.CSD.EraseGrSize = (CSD_Tab[10] & 0x40) >> 6;
	sdcard_info.CSD.EraseGrMul = (CSD_Tab[10] & 0x3F) << 1;


	sdcard_info.CSD.EraseGrMul |= (CSD_Tab[11] & 0x80) >> 7;
	sdcard_info.CSD.WrProtectGrSize = (CSD_Tab[11] & 0x7F);

	sdcard_info.CSD.WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
	sdcard_info.CSD.ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
	sdcard_info.CSD.WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
	sdcard_info.CSD.MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;

	sdcard_info.CSD.MaxWrBlockLen |= (CSD_Tab[13] & 0xC0) >> 6;
	sdcard_info.CSD.WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
	sdcard_info.CSD.Reserved3 = 0;
	sdcard_info.CSD.ContentProtectAppli = (CSD_Tab[13] & 0x01);


	sdcard_info.CSD.FileFormatGroup = (CSD_Tab[14] & 0x80) >> 7;
	sdcard_info.CSD.CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
	sdcard_info.CSD.PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
	sdcard_info.CSD.TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
	sdcard_info.CSD.FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
	sdcard_info.CSD.ECC = (CSD_Tab[14] & 0x03);

	/*!< Byte 15 */
	sdcard_info.CSD.CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
	sdcard_info.CSD.Reserved4 = 1;

	sdcard_info.Capacity = (sdcard_info.CSD.DeviceSize + 1);
	sdcard_info.Capacity *= (1 << (sdcard_info.CSD.DeviceSizeMul + 2));
	sdcard_info.BlockSize = 1 << (sdcard_info.CSD.RdBlockLen);
	sdcard_info.Capacity *= sdcard_info.BlockSize;

	return result;
}

SD_ERROR SD_ReadSingleBlock(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize)
{
	SD_ERROR result;

	// Send CMD17 to read single block
	result = SD_SendCmd(17, Addr, dummy_);
	if (result == SD_NO_ERROR)
	{
		// Now look for the data token(0xFE) to signify the start of the data
		uint32_t i = 200000U; //200ms
		while ((SPI2_RW_Byte(dummy_) != 0xFE) && --i);

		if (i > 0)
		{
			for (uint16_t i = 0; i < BlockSize; i++)
			{
				pBuffer[i] = SPI2_RW_Byte(dummy_);
			}

			// Get CRC bytes (not really needed by ME, but required by SD, fuck you)
			SPI2_RW_Byte(dummy_);
			SPI2_RW_Byte(dummy_);
			// Set response value to success
			result = SD_NO_ERROR;
		}
		else
		{
			result = SD_RESPONSE_FAILURE;
		}
	}

	SD_CS1;
	SPI2_RW_Byte(dummy_);
	return result;
}


SD_ERROR SD_ReadMultiBlocks(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize, uint32_t NumberOfBlocks)
{
	SD_ERROR result;

	// Send CMD18 to read multiple blocks
	result = SD_SendCmd(18, Addr, 0xFF);

	if (result == SD_NO_ERROR)
	{
		hspi2.hdmatx->Instance->CR &= ~DMA_MINC_ENABLE;

		for (uint32_t i = 0; i < NumberOfBlocks; i++)
		{
			// Now look for the data token(0xFE) to signify the start of the data
			uint32_t j = 200000U; //200ms
			while ((SPI2_RW_Byte(dummy_) != 0xFE) && --j);

			if (j > 0)
			{
				//HAL_SPI_TransmitReceive_DMA(&hspi2, &dummy_, pBuffer + i * BlockSize, BlockSize);

				for (j = 0; j < BlockSize; j++)
				{
					pBuffer[i*BlockSize + j] = SPI2_RW_Byte(dummy_);
				}

				// Get CRC bytes (not really needed by us, but required by SD)
				SPI2_RW_Byte(dummy_);
				SPI2_RW_Byte(dummy_);
				// Set response value to success
				result = SD_NO_ERROR;
			}
			else
			{
				result = SD_RESPONSE_FAILURE; // Set response value to failure
			}
		}
		// CMD12 停止传输
		result = SD_SendCmd(12, 0, 0xFF);

	}

	SD_CS1;
	SPI2_RW_Byte(dummy_);
	return result;
}

SD_ERROR SD_WriteSingleBlock(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize)
{
	SD_ERROR result;
	// Send CMD24 to write single block
	result = SD_SendCmd(24, Addr, 0xFF);

	if (result == SD_NO_ERROR)
	{
		if (SD_WaitForReady() == SD_NO_ERROR)
		{
			hspi2.hdmatx->Instance->CR |= DMA_MINC_ENABLE;
			// Send the data token(0xFE) to signify the start of the data
			SPI2_RW_Byte(0xFE);

			for (uint16_t i = 0; i < BlockSize; i++)
			{
				SPI2_RW_Byte(pBuffer[i]);
			}

			// Put CRC bytes (not really needed by us, but required by SD)
			SPI2_RW_Byte(dummy_);
			SPI2_RW_Byte(dummy_);

			// 读取数据响应
			if ((SPI2_RW_Byte(dummy_) & 0x15) == SD_DATA_OK)
			{
				result = SD_NO_ERROR;
			}
			else
			{
				result = SD_RESPONSE_FAILURE;
			}
		}
	}
	else
	{	// 有时 SD 卡已发送响应，但单片机的 SPI 无法正确解析。
		// 此时 SD 卡处于接收数据状态，很可能将未知数据写入 SD 卡。
		// 为保险起见，强制发送 CMD12 停止传输，使 SD 卡重新进入传输状态。
		SD_SendCmd(12, 0x00, 0xFF);
		SPI2_RW_Byte(dummy_);
	}

	SD_CS1;
	SPI2_RW_Byte(dummy_);
	return result;
}

SD_ERROR SD_WriteMultiBlocks(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize, uint32_t NumberOfBlocks)
{
	SD_ERROR result;
	uint32_t i;

	result = SD_SendCmd(55, 0x00, 0xFF);
	if (SD_SendCmd(55, 0x00, 0xFF) == SD_NO_ERROR)
	{
		SD_SendCmd(23, NumberOfBlocks, 0xFF);
	}
	else return SD_ERASE_SEQUENCE_ERROR;

	result = SD_SendCmd(25, Addr, 0xFF);

	if (result == SD_NO_ERROR)
	{
		hspi2.hdmatx->Instance->CR |= DMA_MINC_ENABLE;

		for (i = 0; i < NumberOfBlocks; i++)
		{
			if (SD_WaitForReady() != SD_NO_ERROR) break;

			SPI2_RW_Byte(0xFC);

			for (uint16_t j = 0; j < BlockSize; j++)
			{
				SPI2_RW_Byte(pBuffer[j + i * BlockSize]);
			}

			// Put CRC bytes (not really needed by us, but required by SD)
			SPI2_RW_Byte(dummy_);
			SPI2_RW_Byte(dummy_);

			if ((SPI2_RW_Byte(dummy_) & 0x1F) != SD_DATA_OK) break;
		}
		if (i < NumberOfBlocks)
		{
			result = SD_DATA_WRITE_ERROR;
		}
		else if (SD_WaitForReady() != SD_NO_ERROR)
		{
			result = SD_RESPONSE_FAILURE;
		}
		SPI2_RW_Byte(0xFD);
	}
	else
	{
		SD_WaitForReady();
		SPI2_RW_Byte(0xFD);
		SD_SendCmd(12, 0x00, 0xFF);
	}

	SD_CS1;
	SPI2_RW_Byte(dummy_);
	return result;
}

static SD_ERROR SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	SD_CS0;
	if (SD_WaitForReady() == SD_RESPONSE_FAILURE)
	{
		SD_CS1;
		SPI2_RW_Byte(dummy_);
		return SD_RESPONSE_FAILURE;
	}

	SPI2_RW_Byte(cmd | 0x40);
	SPI2_RW_Byte(arg >> 24);
	SPI2_RW_Byte(arg >> 16);
	SPI2_RW_Byte(arg >> 8);
	SPI2_RW_Byte(arg);
	SPI2_RW_Byte(crc);

	uint8_t retry = 10;
	SD_ERROR result;

	do {
		result = (SD_ERROR)SPI2_RW_Byte(dummy_);
	} while ((result & 0x80) && --retry);

	return result;
}

SD_ERROR SD_WaitForReady(void)
{
	uint32_t i = 50000U;
	while (--i) {
		if(SPI2_RW_Byte(dummy_) == 0xFF) return SD_NO_ERROR;
	}
	return SD_RESPONSE_FAILURE;
}
