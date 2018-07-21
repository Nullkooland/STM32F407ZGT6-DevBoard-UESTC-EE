#pragma once
#include <stm32f4xx_hal.h>

//////////////////////////////////////////////////////////////////////////////////	 
//-----------------LCD端口定义----------------  
//使用NOR/SRAM的 Bank1.Sector4,地址位HADDR[27,26]=11   A6作为数据命令区分线 
//注意设置时STM32内部会右移一位对其! 			   
#define FSMC_LCD_CMD				0x6C000000U	    //FSMC_Bank1_NORSRAM4用于LCD命令操作的地址
#define FSMC_LCD_DATA				0x6C000080U     //FSMC_Bank1_NORSRAM4用于LCD数据操作的地址
#define FSMC_SRAM_BACKBUFFER		0x0U			//双缓冲区在SRAM中的偏移地址

#define WRITE_REG(CMD, DATA)		WRITE_CMD(CMD); WRITE_DATA(DATA)

#define WRITE_CMD(X)				*(__IO uint16_t *)FSMC_LCD_CMD  = X 
#define WRITE_DATA(X)				*(__IO uint16_t *)FSMC_LCD_DATA = X
#define READ_DATA()					*(__IO uint16_t *)FSMC_LCD_DATA
//////////////////////////////////////////////////////////////////////////////////

//动态曲线图 结构体

#define GRAPH_MAX_SIZE				512U
#define GRAPH_USE_BACKBUFFER		1

typedef struct
{
	uint16_t X;
	uint16_t Y;

	uint16_t Width;
	uint16_t Height;
	uint16_t RoughGridWidth;
	uint16_t RoughGridHeight;
	uint16_t FineGridWidth;
	uint16_t FineGridHeight;

	uint16_t BorderColor;
	uint16_t BackgroudColor;
	uint16_t RoughGridColor;
	uint16_t FineGridColor;

} Graph_TypeDef;

// 双缓冲区 结构体
typedef struct
{
	uint16_t X;
	uint16_t Y;
	uint16_t Width;
	uint16_t Height;
	__IO uint16_t *Pixels;

} LCD_BackBuffer_TypeDef;

typedef enum {
	SANS, SERIF,
} GBK_FontType;

//////////////////////////////////////////////////////////////////////////////////

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0xF81F
#define GRED 			 0xFFE0
#define GBLUE			 0x07FF
#define RED           	 0xF800	//红色
#define MAGENTA       	 0xF81F	//洋红色
#define GREEN         	 0x07E0	//绿色
#define CYAN          	 0x7FFF	//青色
#define YELLOW        	 0xFFE0	//黄色
#define BROWN 			 0xBC40 //棕色
#define BRRED 			 0xFC07 //棕红色
#define GRAY  			 0x7BEF //灰色
#define DARKGRAY  		 0x2124	//深灰色

#define DARKBLUE      	 0x01CF	//深蓝色
#define LIGHTBLUE      	 0x7D7C	//浅蓝色  
#define GRAYBLUE       	 0x5458 //灰蓝色

#define LIGHTGREEN     	 0x841F //浅绿色
#define LIGHTGRAY        0xEF5B //浅灰色(PANNEL)
#define LGRAY 			 0xC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0xA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0x2B12 //浅棕蓝色(选择条目的反色)

/* 底层IO函数 */
static inline void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
static inline void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
static inline uint16_t LCD_ReadPixel(uint16_t x, uint16_t y);

/* 用户接口函数 */
void LCD_Init(_Bool isVerticalSreen);
void LCD_GBKFontLib_Init(uint8_t fontType);
void LCD_Clear(uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawPicture_Stream(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pBuffer);
void LCD_DrawPicture_SD(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* fileName);
void LCD_DrawNumber(uint16_t x, uint16_t y, uint8_t fontSize, int num, uint16_t color);
void LCD_DrawBigNumber(uint16_t x, uint16_t y, uint8_t num, uint16_t color);
void LCD_DrawString(uint16_t x, uint16_t y, uint8_t fontSize, uint8_t *str, uint16_t color);
void LCD_DrawChar_ASCII(uint16_t x, uint16_t y, uint8_t fontSize, uint8_t ch, uint16_t color);
void LCD_DrawChar_GBK(uint16_t x, uint16_t y, uint8_t fontSize, uint8_t* ptr, uint16_t color);

/* 双缓冲操作函数 */
void LCD_BackBuffer_Init(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_BackBuffer_Update();
void LCD_BackBuffer_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
static inline void LCD_BackBuffer_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
static inline uint16_t LCD_BackBuffer_ReadPixel(uint16_t x, uint16_t y);

/* 实时曲线图操作函数 */
void Graph_Init(const Graph_TypeDef *graph);

void Graph_DrawImg(const Graph_TypeDef *graph, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pBuffer);
void Grpah_RecoverRect(const Graph_TypeDef *graph, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void Graph_DrawCurve(const Graph_TypeDef *graph, const uint16_t *data, uint16_t color);
void Graph_DrawLineX(const Graph_TypeDef *graph, uint16_t x, uint16_t color);
void Graph_DrawDashedLineX(const Graph_TypeDef *graph, uint16_t x, uint16_t color);
void Graph_DrawLineY(const Graph_TypeDef *graph, uint16_t y, uint16_t color);
void Graph_DrawDashedLineY(const Graph_TypeDef *graph, uint16_t y, uint16_t color);
void Graph_RecoverGrid(const Graph_TypeDef *graph, const uint16_t *data);
void Graph_RecoverLineX(const Graph_TypeDef *graph, uint16_t x);
void Graph_RecoverLineY(const Graph_TypeDef *graph, uint16_t y);
static inline uint16_t Graph_GetRecoverPixelColor(const Graph_TypeDef *graph, uint16_t x0, uint16_t y0);

extern void Delay_ms(uint16_t ms);
extern void Delay_us(uint32_t us);

