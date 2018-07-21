#pragma once
#include <stm32f4xx_hal.h>

#define SAMPLE_COUNT		2048

#define GRID_X				25
#define GRID_Y				25
#define GRID_WIDTH			750
#define GRID_HEIGHT			400

typedef enum {
	RECTANGLE_WINDOW,
	BARTLETT_WINDOW,
	HAMMING_WINDOW,
	BLACKMAN_WINDOW,
	FLATTOP_WINDOW,
} WindowFunc_Type;

typedef enum {
	DIV_50Hz, DIV_100Hz, DIV_500Hz, DIV_1kHz, DIV_5kHz, DIV_10kHz, DIV_50kHz
} FreqBase;

static const uint8_t *freq_base_tag[7] = { "50Hz/div", "100Hz/div", "500Hz/div", "1kHz/div", "5kHz/div", "10kHz/div", "50kHz/div" };

void SpectrumDisplay_Init(void);
void SpectrumDisplay_Start(void);

static void GenerateWindowFunction(float *window, uint16_t length, WindowFunc_Type type);
static void UpdateFrequencyInfo(void);
static void MoveCursor(_Bool right_left_select);

//ZLG7290 KeyBoard Driver
extern void ZLG7290_Init(void);
extern uint8_t ZLG7290_ReadKey(void);

static const uint16_t arrow_pattern[] =
{
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xB596, 0xCE59, 0x2104, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x2965, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0x1082, 0x0000,
	0x5ACB, 0xEF7D, 0x73AE, 0x0000, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x0000, 0x31A6, 0xF79E, 0x94B2,
	0x8430, 0xFFFF, 0xFFFF, 0x738E, 0x0000, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x0000, 0x39E7, 0xFFFF, 0xFFFF, 0xC638,
	0x0000, 0x7BCF, 0xFFFF, 0xFFFF, 0x632C, 0x0000, 0x0000, 0xE73C, 0xFFFF, 0x2945, 0x0000, 0x31A6, 0xFFFF, 0xFFFF, 0xAD75, 0x0000,
	0x0000, 0x0000, 0x7BCF, 0xFFFF, 0xFFFF, 0x632C, 0x0000, 0xDEFB, 0xFFFF, 0x0000, 0x2104, 0xFFFF, 0xFFFF, 0xAD75, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x7BCF, 0xFFFF, 0xFFFF, 0x630C, 0xCE59, 0xFFFF, 0x5ACB, 0xEF7D, 0xFFFF, 0xAD75, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7BCF, 0xFFFF, 0xFFFF, 0xFFDF, 0xFFFF, 0xFFFF, 0xFFFF, 0xAD75, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x73AE, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xAD55, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x73AE, 0xFFFF, 0xFFFF, 0xAD55, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x94B2, 0xBDF7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};