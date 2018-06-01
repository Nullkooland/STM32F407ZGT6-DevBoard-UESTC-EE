
/* Includes ------------------------------------------------------------------*/
#include "diskio.h"
#include "sdcard.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern SD_CARDINFO sdcard_info;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_status(
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	if (pdrv) return STA_NOINIT;		/* Supports only single drive */
}

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_initialize(
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv) return STA_NOINIT;		/* Supports only single drive */

	if (SD_Init() == SD_NO_ERROR)
	{
		return RES_OK;
	}
	else
	{
		return STA_NOINIT;
	}
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT disk_read(
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	        /* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	sector *= SD_BLOCK_SIZE;
	if (count == 1)
	{
		if (SD_ReadSingleBlock(sector, buff, SD_BLOCK_SIZE) == SD_NO_ERROR) return RES_OK;
	}
	else
	{
		if (SD_ReadMultiBlocks(sector, buff, SD_BLOCK_SIZE, count) == SD_NO_ERROR) return RES_OK;
	}
	return RES_ERROR;
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT disk_write(
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count        	/* Number of sectors to write */
)
{
	sector *= SD_BLOCK_SIZE;
	if (count == 1)
	{
		if (SD_WriteSingleBlock(sector, buff, SD_BLOCK_SIZE) == SD_NO_ERROR) return RES_OK;
	}
	else
	{
		if (SD_WriteMultiBlocks(sector, buff, SD_BLOCK_SIZE, count) == SD_NO_ERROR) return RES_OK;
	}
	return RES_ERROR;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT disk_ioctl(
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_OK;

	switch (cmd)
	{
#if !_FS_READONLY
	case CTRL_SYNC:
		SD_CS0;
		if (SD_WaitForReady() == SD_NO_ERROR)
		{
			res = RES_OK;
		}
		else
		{
			res = RES_ERROR;
			SD_CS1;
		}
		break;
#endif

	case GET_SECTOR_COUNT:
		*(DWORD*)buff = sdcard_info.Capacity / sdcard_info.BlockSize;
		res = RES_OK;
		break;
	case GET_BLOCK_SIZE:
		*(DWORD*)buff = sdcard_info.BlockSize;
		res = RES_OK;
		break;

#if _MAX_SS != _MIN_SS
	case GET_SECTOR_SIZE:
		res = RES_OK;
		break;
#endif
	default:
		res = RES_PARERR;
	}
	return res;
}
#endif /* _USE_IOCTL == 1 */

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
__weak DWORD get_fattime(void)
{
	return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

