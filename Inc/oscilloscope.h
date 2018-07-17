#pragma once
#include <stm32f4xx_hal.h>
#include "lmh6518.h"

#define CLAMP(X, LOW, HIGH)  (((X) > (HIGH)) ? (HIGH) : (((X) < (LOW)) ? (LOW) : (X)))
#define ADC_SAMPLE_COUNT	4096
#define DISPLAY_CVT_FACTOR	0.039f

#define GRID_X				15
#define GRID_Y				15
#define GRID_WIDTH			600
#define GRID_HEIGHT			400

#define TIMEBOX_X			GRID_X + GRID_WIDTH + 10
#define TIMEBOX_Y			GRID_Y - 1
#define TIMEBOX_WIDTH		168
#define TIMEBOX_HEIGHT		127

#define VOLTBOX_X			GRID_X + GRID_WIDTH + 10
#define VOLTBOX_Y			TIMEBOX_Y + TIMEBOX_HEIGHT + 10
#define VOLTBOX_WIDTH		168
#define	VOLTBOX_HEIGHT		127

#define TRIGBOX_X			GRID_X + GRID_WIDTH + 10
#define TRIGBOX_Y			VOLTBOX_Y + VOLTBOX_HEIGHT + 10
#define TRIGBOX_WIDTH		168
#define	TRIGBOX_HEIGHT		67

#define INPUTBOX_X			GRID_X + GRID_WIDTH + 10
#define INPUTBOX_Y			TRIGBOX_Y + TRIGBOX_HEIGHT + 10
#define INPUTBOX_WIDTH		168
#define INPUTBOX_HEIGHT		96

typedef enum {
	DIV_10us, DIV_100us, DIV_1ms, DIV_10ms
} TimeBase;

typedef enum {
	DIV_10mV, DIV_100mV, DIV_1V,
} VoltBase;

typedef enum {
	DC_Coupling, AC_Coupling
} Coupling;

typedef enum {
	X1, X10,
} ProbeAttenuation;

typedef struct {
	_Bool Coupling;
	_Bool ProbeAttenuation;
	uint8_t Gain;
	uint8_t TimeBase;
	int16_t TimeOffset;
	uint8_t VoltBase;
	int16_t VoltOffset;
	int16_t TriggerVolt;
} OscArgs_TypeDef;

typedef struct {
	uint32_t ADCClockPrescaler;
	uint32_t ADCSamplingTime;
	float InterpFactor;
} SamplingArgs;


static const SamplingArgs sampling_args[4] = {
	{ ADC_CLOCK_SYNC_PCLK_DIV2, ADC_SAMPLETIME_3CYCLES, 0.567f },
	{ ADC_CLOCK_SYNC_PCLK_DIV4, ADC_SAMPLETIME_28CYCLES, 1.052f },
	{ ADC_CLOCK_SYNC_PCLK_DIV8, ADC_SAMPLETIME_144CYCLES, 1.345f },
	{ ADC_CLOCK_SYNC_PCLK_DIV8, ADC_SAMPLETIME_480CYCLES, 4.254f },
};
static const uint8_t *time_base_tag[5] = { "10us/div", "100us/div", "1ms/div", "10ms/div", "100ms/div" };
static const uint8_t *volt_base_tag[3] = { "10mV/div", "100mV/div", "1V/div" };

void Oscilloscope_Init(void);
void Oscilloscope_Test(void);

static void AdjustVerticalPos(_Bool up_down_select);
static inline void UpdateVerticalPosInfo(void);

static void AdjustHorizontalPos(_Bool right_left_select);
static inline void UpdateHorizontalPosInfo(void);

static void AdjustTriggerVoltage(_Bool up_down_select);
static inline void UpdateTriggerVoltageInfo(void);

static inline void UpdateSamplingRate(void);
static inline float pow10(uint8_t n);

//ZLG7290 KeyBoard Driver
extern void ZLG7290_Init(void);
extern uint8_t ZLG7290_ReadKey(void);
//LMH6518 PGA
extern void LMH6518_SetAuxOutput(_Bool isEnabled);
extern void LMH6518_PreAmp(LMH6518_PreAmpMode mode);
extern void LMH6518_SetBandWidth(LMH6518_FilterBandwith bandwith);
extern void LMH6518_SetAttenuation(LMH6518_LadderAttenuation attenuation);
//On Chip ADC
extern void ADC1_Init(void);
//Delay
extern void Delay_ms(uint16_t ms);