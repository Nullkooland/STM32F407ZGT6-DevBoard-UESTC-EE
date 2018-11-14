#include "lcd.h"
#include "sram.h"
#include "colors.h"
#include "ascii.h" 
#include "big_number.h"
#include "fatfs.h"

#include <stdlib.h>
#include <string.h>

/* LCD驱动程序 */

//根据驱动ic型号选择对应的底层操作实现驱动
#if LCD_DRIVER_IC == NT35510
#include "nt35510.h"
#elif LCD_DRIVER_IC == ILI9341
#include "ili9341.h"
#endif

//////////////////////////////////////////////////////////////////////////////////

extern DMA_HandleTypeDef hdma_m2m;

//分辨率 默认横屏
static s_orientation;
static uint16_t s_width;
static uint16_t s_height;

//双缓冲区结构体
static LCD_BackBuffer_TypeDef s_back_buffer;

//GBK字库
static FIL s_gbk_font_16x16, s_gbk_font_24x24, s_gbk_font_32x32;
static _Bool s_is_gbk_fontlib_ready;

static inline void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    // Get diagonal point position
    width += x - 1;
    height += y - 1;

#if LCD_DRIVER_IC == NT35510
    // Set x span
    WRITE_REG(0x2A00, x >> 8);
    WRITE_REG(0x2A01, x & 0xFF);
    WRITE_REG(0x2A02, width >> 8);
    WRITE_REG(0x2A03, width & 0xFF);
    // Set y span
    WRITE_REG(0x2B00, y >> 8);
    WRITE_REG(0x2B01, y & 0xFF);
    WRITE_REG(0x2B02, height >> 8);
    WRITE_REG(0x2B03, height & 0xFF);
#elif LCD_DRIVER_IC == ILI9341
    // Set x span
    WRITE_CMD(0x2A);
    WRITE_DATA(x >> 8);
    WRITE_DATA(x & 0xFF);
    WRITE_DATA(width >> 8);
    WRITE_DATA(width & 0xFF);
    // Set y span
    WRITE_CMD(0x2B);
    WRITE_DATA(y >> 8);
    WRITE_DATA(y & 0xFF);
    WRITE_DATA(height >> 8);
    WRITE_DATA(height & 0xFF);
#endif
}

static inline void LCD_SetCursor(uint16_t x, uint16_t y)
{
#if LCD_DRIVER_IC == NT35510
    // Set x pos
    WRITE_REG(0x2A00, x >> 8);
    WRITE_REG(0x2A01, x & 0xFF);
    // Set y pos
    WRITE_REG(0x2B00, y >> 8);
    WRITE_REG(0x2B01, y & 0xFF);
#elif LCD_DRIVER_IC == ILI9341
    // Set x pos
    WRITE_CMD(0x2A);
    WRITE_DATA(x >> 8);
    WRITE_DATA(x & 0xFF);
    // Set y pos
    WRITE_CMD(0x2B);
    WRITE_DATA(y >> 8);
    WRITE_DATA(y & 0xFF);
#endif
}

static inline void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    // Set cursor
    LCD_SetCursor(x, y);
    // Write pixel data
    WRITE_CMD(CMD_WRITE_GRAM);
    WRITE_DATA(color);
}

static inline uint16_t LCD_ReadPixel(uint16_t x, uint16_t y)
{
    // Beyond screen range, return black (0x0000)
    if (x >= s_width || y >= s_height) return BLACK;

    uint16_t r = 0, g = 0, b = 0;

    // Set cursor
    LCD_SetCursor(x, y);
    // Read pixel data
    WRITE_CMD(CMD_READ_GRAM);
    READ_DATA();  // Dummy
    r = READ_DATA();
    b = READ_DATA();
    g = r & 0xFF;
    g <<= 8;
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

void LCD_Init(void)
{
#if LCD_DRIVER_IC == NT35510
    NT35510_Init(LCD_ORIENTAION);
#elif LCD_DRIVER_IC == ILI9341
    ILI9341_Init(LCD_ORIENTAION);
#endif

    s_width = LCD_RESOLUTION_X;
    s_height = LCD_RESOLUTION_Y;

    LCD_Clear(BLACK);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = BACKLIGHT_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BACKLIGHT_GPIOX, &GPIO_InitStruct);

    BACKLIGHT_GPIOX->BSRR = BACKLIGHT_GPIO_PIN; //点亮背光

    if (f_open(&s_gbk_font_16x16, "0:更纱黑体_16x16.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
        f_open(&s_gbk_font_24x24, "0:更纱黑体_24x24.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
        f_open(&s_gbk_font_32x32, "0:更纱黑体_32x32.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        s_is_gbk_fontlib_ready = 1;
    }
}

void LCD_GBKFontLib_Init(GBK_FontType type)
{
    switch (type)
    {
        case SANS:
            if (f_open(&s_gbk_font_16x16, "0:更纱黑体_16x16.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
                f_open(&s_gbk_font_24x24, "0:更纱黑体_24x24.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
                f_open(&s_gbk_font_32x32, "0:更纱黑体_32x32.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
                s_is_gbk_fontlib_ready = 1;
            }
            break;
        case SERIF:
            if (f_open(&s_gbk_font_16x16, "0:宋体_16x16.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
                f_open(&s_gbk_font_24x24, "0:宋体_24x24.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK &&
                f_open(&s_gbk_font_32x32, "0:宋体_32x32.fon", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
                s_is_gbk_fontlib_ready = 1;
            }
            break;
        default: return;
    }
}

void LCD_Clear(uint16_t color)
{
    LCD_SetWindow(0, 0, s_width, s_height);

    WRITE_CMD(CMD_WRITE_GRAM);
    for (uint32_t i = 0; i < s_width * s_height; i++) {
        WRITE_DATA(color);
    }
}

void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint16_t temp;
    _Bool is_steep = (abs(y1 - y0) > abs(x1 - x0));

    if (is_steep) {
        temp = x0; x0 = y0; y0 = temp;
        temp = x1; x1 = y1; y1 = temp;
    }
    if (x0 > x1) {
        temp = x0; x0 = x1; x1 = temp;
        temp = y0; y0 = y1; y1 = temp;
    }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t y_step = (y0 < y1) ? 1 : -1;
    int16_t x = x0;
    int16_t y = y0;

    while (x <= x1)
    {
        if (is_steep) {
            LCD_DrawPixel(y, x, color);
        }
        else {
            LCD_DrawPixel(x, y, color);
        }

        err -= dy;
        if (err < 0) {
            y += y_step;
            err += dx;
        }
    }
}

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    width += x;
    height += y;

    for (uint16_t i = x; i <= width; i++)
    {
        LCD_DrawPixel(i, y, color);
        LCD_DrawPixel(i, height, color);
    }

    for (uint16_t i = y; i <= height; i++)
    {
        LCD_DrawPixel(x, i, color);
        LCD_DrawPixel(width, i, color);
    }
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    LCD_SetWindow(x, y, width, height);
    WRITE_CMD(CMD_WRITE_GRAM);
    for (uint32_t i = 0; i < width * height; i++) {
        WRITE_DATA(color);
    }
}

void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint8_t radius, uint16_t color)
{
    int16_t x = 0;
    int16_t y = radius;
    int16_t di = 3 - radius / 2;             //判断下个点位置的标志
    while (x <= y)
    {
        LCD_DrawPixel(x0 + x, y0 - y, color);             //5
        LCD_DrawPixel(x0 + y, y0 - x, color);             //0           
        LCD_DrawPixel(x0 + y, y0 + x, color);             //4               
        LCD_DrawPixel(x0 + x, y0 + y, color);             //6 
        LCD_DrawPixel(x0 - x, y0 + y, color);             //1       
        LCD_DrawPixel(x0 - y, y0 + x, color);
        LCD_DrawPixel(x0 - x, y0 - y, color);             //2             
        LCD_DrawPixel(x0 - y, y0 - x, color);             //7     	         
        x++;
        //使用Bresenham算法画圆     
        if (di < 0) {
            di += 4 * x + 6;
        }
        else {
            di += 10 + 4 * (x - y);
            y--;
        }
    }
}

void LCD_DrawPicture_Stream(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *buffer)
{
    LCD_SetWindow(x, y, width, height);
    WRITE_CMD(CMD_WRITE_GRAM);

    HAL_DMA_Start(&hdma_m2m, buffer, FSMC_LCD_DATA, width * height / 2);
    HAL_DMA_PollForTransfer(&hdma_m2m, HAL_DMA_FULL_TRANSFER, 0xFF);
}

void LCD_DrawPicture_SD(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* file_name)
{
    FIL img_file;
    UINT fnum;
    uint8_t buffer[512];

    if (f_open(&img_file, file_name, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
        LCD_SetWindow(x, y, width, height);
        WRITE_CMD(CMD_WRITE_GRAM);

        do {
            f_read(&img_file, &buffer, 512U, &fnum);
            uint16_t* pixel_buffer = (uint16_t *)buffer;

            for (uint16_t i = 0; i < fnum / 2; i++) {
                WRITE_DATA(pixel_buffer[i]);
            }
        } while (fnum > 0);
    }

    f_close(&img_file);
}

void LCD_DrawNumber(uint16_t x, uint16_t y, uint8_t font_size, int num, uint16_t color)
{
    uint8_t temp[12] = { 0 };
    uint8_t i = 10;
    _Bool sign = 0;

    if (num < 0) {
        num = -num;
        sign = 1;
    }

    do {
        temp[i--] = num % 10 + '0';
        num /= 10;
    } while (num);

    if (sign) {
        temp[i--] = '-';
    }

    LCD_DrawString(x, y, font_size, temp + i + 1, color);
}

void LCD_DrawBigNumber(uint16_t x, uint16_t y, uint8_t num, uint16_t color)
{
    if (num == '.') {
        num = 10;
    }
    uint16_t y0 = y;

    for (uint16_t i = 0; i < 384; i++)
    {
        uint8_t temp = big_number_64x48[num][i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 1) LCD_DrawPixel(x, y, color);

            temp >>= 1;
            ++y;
            if ((y - y0) == 64)
            {
                y = y0;
                ++x;
                break;
            }
        }
    }

}

void LCD_DrawString(uint16_t x, uint16_t y, uint8_t font_size, uint8_t *str, uint16_t color)
{
    while (*str)
    {
        if (*str < 0x80)
        {
            LCD_DrawChar_ASCII(x, y, font_size, *(str++), color);
            x += font_size / 2;
        }
        else
        {
            if (s_is_gbk_fontlib_ready) {
                LCD_DrawChar_GBK(x, y, font_size, str, color);
            }
            str += 2;
            x += font_size;
        }
    }
}

void LCD_DrawChar_ASCII(uint16_t x, uint16_t y, uint8_t font_size, uint8_t ch, uint16_t color)
{
    uint8_t bufferSize = font_size * font_size >> 4;
    uint16_t y0 = y;
    uint8_t* buffer;

    ch -= ' ';
    switch (font_size)
    {
        case 16: buffer = ASCII_16x8[ch]; break;
        case 24: buffer = ASCII_24x12[ch]; break;
        case 32: buffer = ASCII_32x16[ch]; break;
        default: return;
    }

    for (uint8_t i = 0; i < bufferSize; i++)
    {
        uint8_t temp = buffer[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 1) LCD_DrawPixel(x, y, color);

            temp >>= 1;
            ++y;
            if ((y - y0) == font_size)
            {
                y = y0;
                ++x;
                break;
            }
        }
    }
}

void LCD_DrawChar_GBK(uint16_t x, uint16_t y, uint8_t font_size, uint8_t* ptr, uint16_t color)
{
    uint8_t bufferSize = font_size * font_size >> 3;
    uint16_t y0 = y;

    FIL *fontlib;
    UINT fnum;
    uint8_t font_buffer[128];

    switch (font_size)
    {
        case 16: fontlib = &s_gbk_font_16x16; break;
        case 24: fontlib = &s_gbk_font_24x24; break;
        case 32: fontlib = &s_gbk_font_32x32; break;
        default: return;
    }

    f_lseek(fontlib, ((ptr[0] - 0xA1) * 94 + (ptr[1] - 0xA1)) * bufferSize);
    f_read(fontlib, &font_buffer, bufferSize, &fnum);

    for (uint8_t i = 0; i < bufferSize; i++)
    {
        uint8_t temp = font_buffer[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 1) LCD_DrawPixel(x, y, color);

            temp >>= 1;
            ++y;
            if ((y - y0) == font_size)
            {
                y = y0;
                ++x;
                break;
            }
        }
    }
}

void LCD_BackBuffer_Init(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    s_back_buffer.X = x;
    s_back_buffer.Y = y;
    s_back_buffer.Width = width;
    s_back_buffer.Height = height;
    s_back_buffer.Pixels = (__IO uint16_t *)(FSMC_SRAM_BASE + BACKBUFFER_ADDR_OFFSET);
}

void LCD_BackBuffer_Clear(uint16_t color)
{
    for (uint32_t i = 0; i < s_back_buffer.Width * s_back_buffer.Height; i++) {
        s_back_buffer.Pixels[i] = color;
    }
}

void LCD_BackBuffer_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    width += x;
    height += y;

    for (uint16_t i = x; i < width; i++)
    {
        LCD_BackBuffer_DrawPixel(i, y, color);
        LCD_BackBuffer_DrawPixel(i, height, color);
    }

    for (uint16_t i = y; i < height; i++)
    {
        LCD_BackBuffer_DrawPixel(x, i, color);
        LCD_BackBuffer_DrawPixel(width, i, color);
    }
}

void LCD_BackBuffer_Update()
{
    uint32_t src_addr = s_back_buffer.Pixels;
    uint32_t word_count = s_back_buffer.Width * s_back_buffer.Height / 2;

    LCD_SetWindow(s_back_buffer.X, s_back_buffer.Y, s_back_buffer.Width, s_back_buffer.Height);
    WRITE_CMD(CMD_WRITE_GRAM);

    /*
    for (uint32_t i = 0; i < s_back_buffer.Width * s_back_buffer.Height; i++) {
    WRITE_DATA(s_back_buffer.Pixels[i]);
    }*/

    for (uint32_t i = 0; i < word_count >> 15; i++)
    {
        HAL_DMA_Start(&hdma_m2m, src_addr, FSMC_LCD_DATA, 0x8000);
        HAL_DMA_PollForTransfer(&hdma_m2m, HAL_DMA_FULL_TRANSFER, 0xFF);
        src_addr += 0x20000;
    }

    HAL_DMA_Start(&hdma_m2m, src_addr, FSMC_LCD_DATA, word_count % 0x8000);
    HAL_DMA_PollForTransfer(&hdma_m2m, HAL_DMA_FULL_TRANSFER, 0xFF);
}

static inline void LCD_BackBuffer_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    s_back_buffer.Pixels[s_back_buffer.Width * y + x] = color;
}

static inline uint16_t LCD_BackBuffer_ReadPixel(uint16_t x, uint16_t y)
{
    return s_back_buffer.Pixels[s_back_buffer.Width * y + x];
}

void Graph_Init(const Graph_TypeDef *graph)
{
    uint16_t i, j;
    //边界框
    LCD_DrawRect(graph->X - 1, graph->Y - 1, graph->Width + 1, graph->Height + 1, graph->BorderColor);

#if GRAPH_USE_BACKBUFFER
    //初始化双缓冲区
    LCD_BackBuffer_Init(graph->X, graph->Y, graph->Width, graph->Height);
    //填充背景
    LCD_BackBuffer_Clear(graph->BackgroudColor);
    //细网格-水平
    for (i = graph->FineGridHeight; i < graph->Height; i += graph->FineGridHeight) {
        for (j = 0; j < graph->Width; j++) {
            LCD_BackBuffer_DrawPixel(j, i, graph->FineGridColor);
        }
    }
    //细网格-垂直
    for (i = graph->FineGridWidth; i < graph->Width; i += graph->FineGridWidth) {
        for (j = 0; j < graph->Height; j++) {
            LCD_BackBuffer_DrawPixel(i, j, graph->FineGridColor);
        }
    }
    //粗网格-水平
    for (i = 0; i < graph->Height; i += graph->RoughGridHeight) {
        for (j = 0; j < graph->Width; j++) {
            LCD_BackBuffer_DrawPixel(j, i, graph->RoughGridColor);
        }
    }
    //粗网格-垂直
    for (i = 0; i < graph->Width; i += graph->RoughGridWidth) {
        for (j = 0; j < graph->Height; j++) {
            LCD_BackBuffer_DrawPixel(i, j, graph->RoughGridColor);
        }
    }
#else
    //填充背景
    LCD_FillRect(graph->X, graph->Y, graph->Width, graph->Height, graph->BackgroudColor);
    //细网格-水平 
    for (i = graph->FineGridHeight; i < graph->Height; i += graph->FineGridHeight) {
        LCD_DrawLine(graph->X, i + graph->Y, graph->X + graph->Width - 1, i + graph->Y, graph->FineGridColor);
    }
    //细网格-垂直
    for (i = graph->FineGridWidth; i < graph->Width; i += graph->FineGridWidth) {
        LCD_DrawLine(i + graph->X, graph->Y, i + graph->X, graph->Y + graph->Height - 1, graph->FineGridColor);
    }
    //粗网格-水平
    for (i = 0; i < graph->Height; i += graph->RoughGridHeight) {
        LCD_DrawLine(graph->X, i + graph->Y, graph->X + graph->Width - 1, i + graph->Y, graph->RoughGridColor);
    }
    //粗网格-垂直
    for (i = 0; i < graph->Width; i += graph->RoughGridWidth) {
        LCD_DrawLine(i + graph->X, graph->Y, i + graph->X, graph->Y + graph->Height - 1, graph->RoughGridColor);
    }
#endif // GRPAH_USE_BACKBUFFER
}

void Graph_DrawImg(const Graph_TypeDef *graph, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *buffer)
{
    if (y >= graph->Height) return;
    y = graph->Height - y;

    for (uint16_t i = 0; i < height; i++)
    {
        for (uint16_t j = 0; j < width; j++)
        {
            /* 把黑色当作透明 */
            if (buffer[i * width + j] != 0x0000) {
#if GRAPH_USE_BACKBUFFER
                LCD_BackBuffer_DrawPixel(x + j, y + i, buffer[i * width + j]);
#else
                LCD_DrawPixel(graph->X + x + j, graph->Y + y + i, buffer[i * width + j]);
#endif // GRAPH_USE_BACKBUFFER
            }
        }
    }
}

void Grpah_RecoverRect(const Graph_TypeDef *graph, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint16_t pixel_color;
    if (y >= graph->Height) return;
    y = graph->Height - y;

    for (uint16_t i = 0; i < height; i++)
    {
        for (uint16_t j = 0; j < width; j++)
        {
            pixel_color = Graph_GetRecoverPixelColor(graph, x + j, y + i);
#if GRAPH_USE_BACKBUFFER
            LCD_BackBuffer_DrawPixel(x + j, y + i, pixel_color);
#else
            LCD_DrawPixel(graph->X + x + j, graph->Y + y + i, pixel_color);
#endif // GRAPH_USE_BACKBUFFER

        }
    }
}

void Graph_DrawCurve(const Graph_TypeDef *graph, const uint16_t *data, uint16_t color)
{
    uint16_t y0, y1, temp;

    for (uint16_t i = 0; i < graph->Width - 1; i++)
    {
        if (data[i] < data[i + 1]) {
            y0 = data[i];
            y1 = data[i + 1];
        }
        else {
            y1 = data[i];
            y0 = data[i + 1];
        }

        if (y0 >= graph->Height) {
            continue;
        }
        y1 = (y1 < graph->Height) ? y1 : graph->Height - 1;

        y0 = graph->Height - y0 - 1;
        y1 = graph->Height - y1 - 1;

        for (uint16_t j = y1; j <= y0; j++) {
#if GRAPH_USE_BACKBUFFER
            LCD_BackBuffer_DrawPixel(i, j, color);
#else
            LCD_DrawPixel(graph->X + i, graph->Y + j, color);
#endif // GRPAH_USE_BACKBUFFER
        }
    }
}

void Graph_DrawLineX(const Graph_TypeDef *graph, uint16_t x, uint16_t color)
{
    if (x > graph->Width) return;

    for (uint16_t i = 0; i < graph->Height; i++)
    {
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(x, i, color);
#else
        LCD_DrawPixel(graph->X + x, graph->Y + i, color);
#endif // GRAPH_USE_BACKBUFFER
    }
}

void Graph_DrawDashedLineX(const Graph_TypeDef *graph, uint16_t x, uint16_t color)
{
    if (x > graph->Width) return;
    uint8_t pixel_count = 0;

    for (uint16_t i = 0; i < graph->Height; i++)
    {
        ++pixel_count;

        if (pixel_count > 4) {
            i += 2;
            pixel_count = 0;
        }
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(x, i, color);
#else
        LCD_DrawPixel(graph->X + x, graph->Y + i, color);
#endif // GRAPH_USE_BACKBUFFER

    }
}


void Graph_DrawLineY(const Graph_TypeDef *graph, uint16_t y, uint16_t color)
{
    if (y > graph->Height) return;

    for (uint16_t i = 0; i < graph->Width; i++)
    {
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(i, graph->Height - y, color);
#else
        LCD_DrawPixel(graph->X + i, graph->Y + graph->Height - y, color);
#endif // GRAPH_USE_BACKBUFFER
    }
}

void Graph_DrawDashedLineY(const Graph_TypeDef *graph, uint16_t y, uint16_t color)
{
    if (y > graph->Height) return;
    uint8_t pixel_count = 0;

    for (uint16_t i = 0; i < graph->Width; i++)
    {
        ++pixel_count;

        if (pixel_count > 4) {
            i += 2;
            pixel_count = 0;
        }
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(i, graph->Height - y, color);
#else
        LCD_DrawPixel(graph->X + i, graph->Y + graph->Height - y, color);
#endif // GRAPH_USE_BACKBUFFER
    }
}

void Graph_RecoverGrid(const Graph_TypeDef *graph, const uint16_t *data)
{
    uint16_t y0, y1, temp;
    uint16_t pixel_color;

    for (uint16_t i = 0; i < graph->Width - 1; i++)
    {
        if (data[i] < data[i + 1]) {
            y0 = data[i];
            y1 = data[i + 1];
        }
        else {
            y1 = data[i];
            y0 = data[i + 1];
        }

        if (y0 >= graph->Height) {
            continue;
        }
        y1 = (y1 < graph->Height) ? y1 : graph->Height - 1;

        y0 = graph->Height - y0 - 1;
        y1 = graph->Height - y1 - 1;

        for (uint16_t j = y1; j <= y0; j++)
        {
            pixel_color = Graph_GetRecoverPixelColor(graph, i, j);

#if GRAPH_USE_BACKBUFFER
            LCD_BackBuffer_DrawPixel(i, j, pixel_color);
#else
            LCD_DrawPixel(graph->X + i, graph->Y + j, pixel_color);
#endif // GRAPH_USE_BACKBUFFER
        }
    }
}

void Graph_RecoverLineX(const Graph_TypeDef *graph, uint16_t x)
{
    if (x > graph->Width) return;
    uint16_t pixel_color;
    for (uint16_t i = 0; i < graph->Height; i++)
    {
        pixel_color = Graph_GetRecoverPixelColor(graph, x, i);
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(x, i, pixel_color);
#else
        LCD_DrawPixel(graph->X + x, graph->Y + i, pixel_color);
#endif // GRAPH_USE_BACKBUFFER
    }
}

void Graph_RecoverLineY(const Graph_TypeDef *graph, uint16_t y)
{
    if (y > graph->Height) return;
    uint16_t pixel_color;

    for (uint16_t i = 0; i < graph->Width; i++)
    {
        pixel_color = Graph_GetRecoverPixelColor(graph, i, graph->Height - y);
#if GRAPH_USE_BACKBUFFER
        LCD_BackBuffer_DrawPixel(i, graph->Height - y, pixel_color);
#else
        LCD_DrawPixel(graph->X + i, graph->Y + graph->Height - y, pixel_color);
#endif // GRAPH_USE_BACKBUFFER
    }
}

static inline uint16_t Graph_GetRecoverPixelColor(const Graph_TypeDef *graph, uint16_t x0, uint16_t y0)
{
    if ((x0 % graph->RoughGridWidth == 0) || (y0 % graph->RoughGridHeight == 0)) {
        return graph->RoughGridColor;
    }

    if ((x0 % graph->FineGridWidth == 0) || (y0 % graph->FineGridHeight == 0)) {
        return graph->FineGridColor;
    }

    return graph->BackgroudColor;
}