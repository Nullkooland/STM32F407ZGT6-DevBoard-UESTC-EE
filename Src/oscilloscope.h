#pragma once
#include <stm32f4xx_hal.h>

#define ADC_SAMPLE_COUNT	2048

#define GRID_X				20
#define GRID_Y				20
#define GRID_WIDTH			600
#define GRID_HEIGHT			400

#define TIMEBOX_X			GRID_X + GRID_WIDTH + 10
#define TIMEBOX_Y			GRID_Y - 1
#define TIMEBOX_WIDTH		160
#define TIMEBOX_HEIGHT		150

#define VOLTBOX_X			GRID_X + GRID_WIDTH + 10
#define VOLTBOX_Y			TIMEBOX_Y + TIMEBOX_HEIGHT + 10
#define VOLTBOX_WIDTH		160
#define	VOLTBOX_HEIGHT		150

#define TRIGBOX_X			GRID_X + GRID_WIDTH + 10
#define TRIGBOX_Y			VOLTBOX_Y + VOLTBOX_HEIGHT + 10
#define TRIGBOX_WIDTH		160
#define	TRIGBOX_HEIGHT		80

static const uint8_t *timeBase[5] = { "10us/div", "100us/div", "1ms/div", "10ms/div", "100ms/div" };
static const uint8_t *voltBase[3] = { "10mV/div", "100mV/div", "1V/div" };

void Oscilloscope_Init(void);
void Oscilloscope_Test(void);

static void AdjustVerticalPos(_Bool up_down_select);
static inline void UpdateVerticalPosInfo(void);

static void AdjustHorizontalPos(_Bool right_left_select);
static inline void UpdateHorizontalPosInfo(void);

static inline float pow10(uint8_t n);

//ZLG7290 KeyBoard Driver
extern void ZLG7290_Init(void);
extern uint8_t ZLG7290_ReadKey(void);
//On Chip ADC
extern void ADC1_Init(void);
//Delay
extern void Delay_ms(uint16_t ms);