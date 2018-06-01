#pragma once
#include <stm32f4xx_hal.h>
#include "ad9959.h"

#define	OUTPUT_CHANNEL		AD9959_CHANNEL_2

#define ADC_SAMPLE_COUNT	32
#define MAX_SAMPLE_COUNT	2048
#define MIN_SAMPLE_COUNT	128

#define GRID_X				40
#define GRID_Y				10
#define GRID_WIDTH			500
#define GRID_HEIGHT			400

#define FREQBOX_X			GRID_X + GRID_WIDTH + 8
#define FREQBOX_Y			GRID_Y - 1
#define FREQBOX_WIDTH		242
#define FREQBOX_HEIGHT		104

#define AMPBOX_X			FREQBOX_X
#define AMPBOX_Y			FREQBOX_Y + FREQBOX_HEIGHT + 8
#define AMPBOX_WIDTH		FREQBOX_WIDTH
#define AMPBOX_HEIGHT		56

#define CURSORBOX_X			FREQBOX_X
#define CURSORBOX_Y			AMPBOX_Y + AMPBOX_HEIGHT + 8
#define CURSORBOX_WIDTH		FREQBOX_WIDTH
#define CURSORBOX_HEIGHT	225

#define STATUS_LED(X)		GPIOF->BSRR = (uint32_t)GPIO_PIN_10 << ((X) ? 16 : 0)

static const uint16_t amp_table[][2] =
{
	{ 2, 980},		//100mV
	{ 3, 1023},		//98mV
	{ 3, 1010},		//96mV
	{ 3, 995 },		//94mV
	{ 3, 975 },		//92mV
	{ 3, 950 },		//90mV
	{ 3, 930 },		//88mV
	{ 4, 1005 },	//86mV
	{ 4, 983 },		//84mV
	{ 5, 1023 },	//82mV
	{ 5, 1000 },	//80mV
	{ 6, 1023 },	//78mV
	{ 6, 1010 },	//76mV
	{ 6, 981 },		//74mV
	{ 7, 1023 },	//72mV
	{ 7, 996 },		//70mV
	{ 8, 1023 },	//68mV
	{ 8, 1010 },	//66mV
	{ 8, 979 },		//64mV
	{ 9, 1010 },	//62mV
	{ 10, 1023 },	//60mV
	{ 10, 1000 },	//58mV
	{ 11, 1023 },	//56mV
	{ 11, 998 },	//54mV
	{ 12, 1023 },	//52mV
	{ 12, 995 },	//50mV
	{ 13, 1012 },	//48mV
	{ 14, 1023 },	//46mV
	{ 14, 980 },	//44mV
	{ 16, 1023 },	//42mV
	{ 16, 970 },	//40mV
	{ 17, 1023 },	//38mV
	{ 18, 1023 },	//36mV
	{ 19, 1023 },	//34mV
	{ 20, 1023 },	//32mV
	{ 21, 1023 },	//30mV
	{ 22, 1023 },	//28mV
	{ 23, 1008 },	//26mV
	{ 24, 988 },	//24mV
	{ 26, 1023 },	//22mV
	{ 27, 980 },	//20mV
	{ 29, 1000 },	//18mV
	{ 33, 1023 },	//16mV
	{ 35, 1000 },	//14mV
	{ 37, 1000 },	//12mV
	{ 40, 990 },	//10mV
	{ 44, 1008 },	//8mV
	{ 50, 1023 },	//6mV
	{ 57, 1023 },	//4mV
};


void FreqSweep_Init(void);
/*
void FreqSweep_Test(void);
void FreqSweep_Start(void);
void FreqPoint_Output(void);
*/

static void SetFreqParameters(void);
static void UpdateFreqInfoDispaly(void);
static void UpdateOutputAmp(void);

//Inline Functions
static inline void FreqParameterDisplay(uint8_t i, _Bool isSlected);
static inline void CursorDisplay(void);

//ZLG7290 Keyboard Driver
extern void ZLG7290_Init();
extern uint8_t ZLG7290_ReadKey();

//PE4302 Attenuator
extern void PE4302_Init(void);
extern void PE4302_SetLoss(uint8_t twoTimes_dB);
//AD9959 DDS
extern void AD9959_Init(void);
extern void AD9959_SetFreq(uint8_t channel, uint32_t freq);
extern void AD9959_SetAmp(uint8_t channel, uint16_t amp);

extern void ADC1_Init();

extern void Delay_ms(uint16_t ms);