#include <stm32f4xx_hal.h>
#include "diskio.h"

#define SD_BLOCK_SIZE		512U
#define RESERVED_MASK		(uint32_t)0x0F7D0F7D

#define SD_CS0				GPIOB->BSRR = (uint32_t)GPIO_PIN_12 << 16U
#define SD_CS1				GPIOB->BSRR = GPIO_PIN_12

typedef enum
{
	SD_TYPE_UNKNOW,
	SD_TYPE_MMC3,
	SD_TYPE_SDV1,
	SD_TYPE_SDV2,
	SD_TYPE_SDHC,
} SD_TYPE;

typedef enum
{
	/**
	* @brief  SD reponses and error flags
	*/
	SD_NO_ERROR = (0x00),
	SD_IN_IDLE_STATE = (0x01),
	SD_ERASE_RESET = (0x02),
	SD_ILLEGAL_COMMAND = (0x04),
	SD_COM_CRC_ERROR = (0x08),
	SD_ERASE_SEQUENCE_ERROR = (0x10),
	SD_ADDRESS_ERROR = (0x20),
	SD_PARAMETER_ERROR = (0x40),
	SD_RESPONSE_FAILURE = (0xFF),

	/**
	* @brief  Data response error
	*/
	SD_DATA_OK = (0x05),
	SD_DATA_CRC_ERROR = (0x0B),
	SD_DATA_WRITE_ERROR = (0x0D),
	SD_DATA_OTHER_ERROR = (0xFF)
} SD_ERROR;

typedef struct               /* Card Specific Data */
{
	__IO uint8_t  CSDStruct;            /* CSD structure */
	__IO uint8_t  SysSpecVersion;       /* System specification version */
	__IO uint8_t  Reserved1;            /* Reserved */
	__IO uint8_t  TAAC;                 /* Data read access-time 1 */
	__IO uint8_t  NSAC;                 /* Data read access-time 2 in CLK cycles */
	__IO uint8_t  MaxBusClkFrec;        /* Max. bus clock frequency */
	__IO uint16_t CardComdClasses;      /* Card command classes */
	__IO uint8_t  RdBlockLen;           /* Max. read data block length */
	__IO uint8_t  PartBlockRead;        /* Partial blocks for read allowed */
	__IO uint8_t  WrBlockMisalign;      /* Write block misalignment */
	__IO uint8_t  RdBlockMisalign;      /* Read block misalignment */
	__IO uint8_t  DSRImpl;              /* DSR implemented */
	__IO uint8_t  Reserved2;            /* Reserved */
	__IO uint32_t DeviceSize;           /* Device Size */
	__IO uint8_t  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
	__IO uint8_t  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
	__IO uint8_t  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
	__IO uint8_t  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
	__IO uint8_t  DeviceSizeMul;        /* Device size multiplier */
	__IO uint8_t  EraseGrSize;          /* Erase group size */
	__IO uint8_t  EraseGrMul;           /* Erase group size multiplier */
	__IO uint8_t  WrProtectGrSize;      /* Write protect group size */
	__IO uint8_t  WrProtectGrEnable;    /* Write protect group enable */
	__IO uint8_t  ManDeflECC;           /* Manufacturer default ECC */
	__IO uint8_t  WrSpeedFact;          /* Write speed factor */
	__IO uint8_t  MaxWrBlockLen;        /* Max. write data block length */
	__IO uint8_t  WriteBlockPaPartial;  /* Partial blocks for write allowed */
	__IO uint8_t  Reserved3;            /* Reserded */
	__IO uint8_t  ContentProtectAppli;  /* Content protection application */
	__IO uint8_t  FileFormatGroup;     /* File format group */
	__IO uint8_t  CopyFlag;             /* Copy flag (OTP) */
	__IO uint8_t  PermWrProtect;        /* Permanent write protection */
	__IO uint8_t  TempWrProtect;        /* Temporary write protection */
	__IO uint8_t  FileFormat;           /* File Format */
	__IO uint8_t  ECC;                  /* ECC code */
	__IO uint8_t  CSD_CRC;              /* CSD CRC */
	__IO uint8_t  Reserved4;            /* always 1*/
} SD_CSD;


typedef struct
{
	SD_CSD CSD;
	uint64_t Capacity;
	uint32_t BlockSize;
	SD_TYPE Type;
	DSTATUS Status;
} SD_CARDINFO;

SD_ERROR SD_Init(void);
SD_ERROR SD_GetCardInfo(void);
SD_ERROR SD_ReadSingleBlock(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize);
SD_ERROR SD_ReadMultiBlocks(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize, uint32_t NumberOfBlocks);
SD_ERROR SD_WriteSingleBlock(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize);
SD_ERROR SD_WriteMultiBlocks(uint32_t Addr, uint8_t* pBuffer, uint16_t BlockSize, uint32_t NumberOfBlocks);
SD_ERROR SD_WaitForReady(void);

static SD_ERROR SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc);
