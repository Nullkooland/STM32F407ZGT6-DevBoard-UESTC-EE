#pragma once
#include <stm32f4xx_hal.h>

//////////////////////////////////////////////////////////////////////////////////	 
//---------------LCD驱动型号选择---------------// 
#define NT35510                     0
#define ILI9341                     1
#define LCD_DRIVER_IC               ILI9341
//-----------------LCD端口定义-----------------// 
/* FSMC地址映射 */
#define FSMC_LCD_BANK               FSMC_NORSRAM_BANK4  //与片选 NE1~NE4 相对应
#define FSMC_LCD_REG_SELECT         6U                  //与寄存器选择 A[25:0] 相对应

#define FSMC_LCD_BASE               0x60000000U | (FSMC_LCD_BANK << 25)
#define FSMC_LCD_CMD				FSMC_LCD_BASE 	//LCD命令操作地址
#define FSMC_LCD_DATA				FSMC_LCD_BASE + (1 << (FSMC_LCD_REG_SELECT + 1)) //LCD数据操作地址

/* 底层I/O操作 */
#define WRITE_CMD(X)				*(__IO uint16_t *)(FSMC_LCD_CMD)  = X 
#define WRITE_DATA(X)				*(__IO uint16_t *)(FSMC_LCD_DATA) = X
#define READ_DATA()					*(__IO uint16_t *)(FSMC_LCD_DATA)
#define WRITE_REG(CMD, DATA)		WRITE_CMD(CMD); WRITE_DATA(DATA)

/* 根据驱动IC型号选取 GRAM I/O 命令 */
#if LCD_DRIVER_IC == NT35510
#define CMD_WRITE_GRAM              0x2C00
#define CMD_READ_GRAM               0x2E00
#elif LCD_DRIVER_IC == ILI9341
#define CMD_WRITE_GRAM              0x002C
#define CMD_READ_GRAM               0x002E
#endif


/* LCD背光 */
#define BACKLIGHT_GPIOX             GPIOA
#define BACKLIGHT_GPIO_PIN          GPIO_PIN_6

/* 双缓冲区 */
#define BACKBUFFER_ADDR_OFFSET	    0x0U

//---------------LCD基本参数---------------//
#define LCD_RESOLUTION_X            800
#define LCD_RESOLUTION_Y            480
#define LCD_ORIENTAION              1
//////////////////////////////////////////////////////////////////////////////////

//-----------------动态曲线绘图-----------------// 
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

/* 像素点读写 */
static inline void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
static inline void LCD_SetCursor(uint16_t x, uint16_t y);
static inline void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
static inline uint16_t LCD_ReadPixel(uint16_t x, uint16_t y);

/* LCD接口函数 */
void LCD_Init(void);
void LCD_GBKFontLib_Init(GBK_FontType type);
void LCD_Clear(uint16_t color);
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawPicture_Stream(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *buffer);
void LCD_DrawPicture_SD(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* file_name);
void LCD_DrawNumber(uint16_t x, uint16_t y, uint8_t font_size, int num, uint16_t color);
void LCD_DrawBigNumber(uint16_t x, uint16_t y, uint8_t num, uint16_t color);
void LCD_DrawString(uint16_t x, uint16_t y, uint8_t font_size, uint8_t *str, uint16_t color);
void LCD_DrawChar_ASCII(uint16_t x, uint16_t y, uint8_t font_size, uint8_t ch, uint16_t color);
void LCD_DrawChar_GBK(uint16_t x, uint16_t y, uint8_t font_size, uint8_t* ptr, uint16_t color);

/* 双缓冲操作函数 */
void LCD_BackBuffer_Init(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void LCD_BackBuffer_Update();
void LCD_BackBuffer_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
static inline void LCD_BackBuffer_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
static inline uint16_t LCD_BackBuffer_ReadPixel(uint16_t x, uint16_t y);

/* 实时曲线图操作函数 */
void Graph_Init(const Graph_TypeDef *graph);
void Graph_DrawImg(const Graph_TypeDef *graph, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *buffer);
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

