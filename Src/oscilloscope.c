#include "oscilloscope.h"
#include "tim.h"
#include "adc.h"
#include "lcd.h"
#include "zlg7290.h"
#include "pattern.h"
#include <stdio.h>
#include <string.h>
#include <arm_math.h>

//绘图相关
static uint8_t str_buffer[32];
static uint16_t display_values[GRID_WIDTH];
static Graph_TypeDef graph;
extern TIM_HandleTypeDef htim6;

//采样
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
static int16_t adc_samples_buffer[ADC_SAMPLE_COUNT];

//示波器参数结构体
static OscArgs_TypeDef osc_args;

void Oscilloscope_Init(void)
{
	TIM6_Init();
	ADC1_Init();
	ZLG7290_Init();

	//GUI 初始化
	graph.X = GRID_X;
	graph.Y = GRID_Y;
	graph.Width = GRID_WIDTH;
	graph.Height = GRID_HEIGHT;
	graph.BorderColor = WHITE;
	graph.BackgroudColor = BLACK;
	graph.RoughGridColor = GRAY;
	graph.FineGridColor = DARKGRAY;
	Graph_Init(&graph);

	osc_args.Coupling = DC_Coupling;
	osc_args.ProbeAttenuation = X1;
	osc_args.TimeBase = DIV_100us;
	osc_args.VoltBase = DIV_100mV;
	osc_args.TriggerVolt = 125;
	
	LCD_DrawRect(TIMEBOX_X, TIMEBOX_Y, TIMEBOX_WIDTH, TIMEBOX_HEIGHT, WHITE);
	LCD_DrawString(TIMEBOX_X + 12, TIMEBOX_Y + 6, 24, "水平时基档位", WHITE);
	LCD_DrawString(TIMEBOX_X + (TIMEBOX_WIDTH - strlen(time_base_tag[osc_args.TimeBase]) * 12) / 2, TIMEBOX_Y + 36, 24, time_base_tag[osc_args.TimeBase], YELLOW);
	UpdateSamplingRate();

	LCD_DrawString(TIMEBOX_X + 36, TIMEBOX_Y + 66, 24, "水平偏移", WHITE);
	LCD_DrawPicture_Stream(GRID_X + GRID_WIDTH / 2 + osc_args.TimeOffset - 5, GRID_Y - 12, 11, 11, down_arrow_pattern);
	UpdateHorizontalPosInfo();

	LCD_DrawRect(VOLTBOX_X, VOLTBOX_Y, VOLTBOX_WIDTH, VOLTBOX_HEIGHT, WHITE);
	LCD_DrawString(VOLTBOX_X + 12, VOLTBOX_Y + 6, 24, "垂直电压档位", WHITE);
	LCD_DrawString(VOLTBOX_X + (VOLTBOX_WIDTH - strlen(volt_base_tag[osc_args.VoltBase]) * 12) / 2, VOLTBOX_Y + 36, 24, volt_base_tag[osc_args.VoltBase], YELLOW);

	LCD_DrawString(VOLTBOX_X + 36, VOLTBOX_Y + 66, 24, "垂直偏移", WHITE);
	LCD_DrawPicture_Stream(GRID_X - 12, GRID_Y + GRID_HEIGHT / 2 - osc_args.VoltOffset - 5, 11, 11, right_arrow_pattern);
	UpdateVerticalPosInfo();

	LCD_DrawRect(TRIGBOX_X, TRIGBOX_Y, TRIGBOX_WIDTH, TRIGBOX_HEIGHT, WHITE);
	LCD_DrawString(TRIGBOX_X + 36, TRIGBOX_Y + 6, 24, "触发电平", WHITE);

	LCD_DrawRect(INPUTBOX_X, INPUTBOX_Y, INPUTBOX_WIDTH, INPUTBOX_HEIGHT, WHITE);
	LCD_DrawString(INPUTBOX_X + 8, INPUTBOX_Y + 8, 32, "耦合", WHITE);
	LCD_DrawString(INPUTBOX_X + 8, INPUTBOX_Y + 56, 32, "倍率", WHITE);

	LCD_DrawString(INPUTBOX_X + 96, INPUTBOX_Y + 8, 32, "DC", CYAN);
	LCD_DrawString(INPUTBOX_X + 96, INPUTBOX_Y + 56, 32, "1x", CYAN);

	//LCD_BackBuffer_Update();
	HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, ADC_SAMPLE_COUNT);
	Oscilloscope_Test();
}

void Oscilloscope_Test(void)
{
	for (;;)
	{
		uint16_t trigger_pos = ADC_SAMPLE_COUNT / 2 - 128;
		int32_t delta_volt_a, delta_volt_b;

		Graph_RecoverGrid(&graph, display_values);

		while (hdma_adc1.Instance->NDTR != ADC_SAMPLE_COUNT) {
			__NOP();
		}
		HAL_ADC_Stop_DMA(&hadc1);

		for (uint16_t i = 0; i < 512; i++)
		{
			delta_volt_a = adc_samples_buffer[trigger_pos] - osc_args.TriggerVolt;
			delta_volt_b = adc_samples_buffer[trigger_pos + 1] - osc_args.TriggerVolt;

			if (delta_volt_a < 0 && delta_volt_b > 0) {
				break;
			}
			++trigger_pos;
		}
		int16_t *adc_samples_begin = 
		adc_samples_buffer + (uint16_t)(trigger_pos - GRID_WIDTH / 2 * sampling_args[osc_args.TimeBase].InterpFactor) - osc_args.TimeOffset;

		for (uint16_t i = 0; i < GRID_WIDTH; i++)
		{
			uint32_t x = (i << 20) * sampling_args[osc_args.TimeBase].InterpFactor;
			display_values[i] = arm_linear_interp_q15(adc_samples_begin, x, GRID_WIDTH * sampling_args[osc_args.TimeBase].InterpFactor);
			display_values[i] *= DISPLAY_CVT_FACTOR * pow10(2 - osc_args.VoltBase);
			display_values[i] += GRID_HEIGHT / 2 + osc_args.VoltOffset;
		}

		HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, ADC_SAMPLE_COUNT);

		Graph_DrawCurve(&graph, display_values, RED);
#if GRAPH_USE_BACKBUFFER
		LCD_BackBuffer_Update();
#endif // GRAPH_USE_BACKBUFFER

		switch (ZLG7290_ReadKey())
		{
			/* 水平时基选择 */
		case 1:
			osc_args.TimeBase = (osc_args.TimeBase + 1) % 4;
			UpdateSamplingRate();
			LCD_FillRect(TIMEBOX_X + 12, TIMEBOX_Y + 36, 150, 24, BLACK);
			LCD_DrawString(TIMEBOX_X + (TIMEBOX_WIDTH - strlen(time_base_tag[osc_args.TimeBase]) * 12) / 2, TIMEBOX_Y + 36, 24, time_base_tag[osc_args.TimeBase], YELLOW);
			UpdateHorizontalPosInfo();
			break;

			/* 垂直电压档选择 */
		case 2:
			osc_args.VoltBase = (osc_args.VoltBase + 1) % 3;
			LCD_FillRect(VOLTBOX_X + 12, VOLTBOX_Y + 36, 150, 24, BLACK);
			LCD_DrawString(VOLTBOX_X + (VOLTBOX_WIDTH - strlen(volt_base_tag[osc_args.VoltBase]) * 12) / 2, VOLTBOX_Y + 36, 24, volt_base_tag[osc_args.VoltBase], YELLOW);
			UpdateVerticalPosInfo();
			break;

			/* AC/DC耦合选择 */
		case 9:
			osc_args.Coupling = !osc_args.Coupling;
			LCD_FillRect(INPUTBOX_X + 96, INPUTBOX_Y + 8, 32, 32, BLACK);
			LCD_DrawString(INPUTBOX_X + 96, INPUTBOX_Y + 8, 32, (osc_args.Coupling) ? "AC" : "DC", CYAN);
			break;

			/* 探头倍率选择 */
		case 10:
			osc_args.ProbeAttenuation = !osc_args.ProbeAttenuation;
			LCD_FillRect(INPUTBOX_X + 96, INPUTBOX_Y + 56, 48, 32, BLACK);
			LCD_DrawString(INPUTBOX_X + 96, INPUTBOX_Y + 56, 32, (osc_args.ProbeAttenuation) ? "10x" : "1x", CYAN);
			break;

			/* 水平位置调整 */
		case 3:
			AdjustHorizontalPos(1);
			break;

		case 11:
			AdjustHorizontalPos(0);
			break;

			/* 垂直位置调整 */
		case 4:
			AdjustVerticalPos(1);
			break;

		case 12:
			AdjustVerticalPos(0);
			break;

			/* 触发电平调整 */
		case 5:
			AdjustTriggerVoltage(1);
			break;

		case 13:
			AdjustTriggerVoltage(0);
			break;
		default:
			break;
		}
		Delay_ms(24);
	}
}

static void AdjustVerticalPos(_Bool up_down_select)
{
	static _Bool previous_direction;
	static int16_t delta;

	Graph_RecoverLineY(&graph, osc_args.TriggerVolt * DISPLAY_CVT_FACTOR * pow10(2 - osc_args.VoltBase) + osc_args.VoltOffset + GRID_HEIGHT / 2);

	if (previous_direction == up_down_select) {
		delta += (up_down_select) ? 1 : -1;
		delta = CLAMP(delta, -10, 10);
	}
	else {
		delta = (up_down_select) ? 1 : -1;
	}
	previous_direction = up_down_select;

	LCD_FillRect(GRID_X - 12, GRID_Y + GRID_HEIGHT / 2 - osc_args.VoltOffset - 5, 11, 11, BLACK);

	osc_args.VoltOffset += delta;
	osc_args.VoltOffset = CLAMP(osc_args.VoltOffset, -GRID_HEIGHT / 2, GRID_HEIGHT / 2);

	LCD_DrawPicture_Stream(GRID_X - 12, GRID_Y + GRID_HEIGHT / 2 - osc_args.VoltOffset - 5, 11, 11, right_arrow_pattern);
	UpdateVerticalPosInfo();
}

static inline void UpdateVerticalPosInfo(void)
{
	LCD_FillRect(VOLTBOX_X + 44, VOLTBOX_Y + 96, 72, 24, BLACK);
	switch (osc_args.VoltBase)
	{
	case DIV_10mV: sprintf(str_buffer, "%+3.1fmV", osc_args.VoltOffset * 0.2f); break;
	case DIV_100mV: sprintf(str_buffer, "%+3.0fmV", osc_args.VoltOffset * 2.0f); break;
	case DIV_1V: sprintf(str_buffer, "%+3.2fV", osc_args.VoltOffset * 0.02f); break;
	default: return;
	}
	LCD_DrawString(VOLTBOX_X + 44, VOLTBOX_Y + 96, 24, str_buffer, YELLOW);
}

static void AdjustHorizontalPos(_Bool right_left_select)
{
	static _Bool previous_direction;
	static int16_t delta;

	if (previous_direction == right_left_select) {
		delta += (right_left_select) ? 1 : -1;
		delta = CLAMP(delta, -10, 10);
	}
	else {
		delta = (right_left_select) ? 1 : -1;
	}
	previous_direction = right_left_select;

	LCD_FillRect(GRID_X + GRID_WIDTH / 2 + osc_args.TimeOffset - 5, GRID_Y - 12, 11, 11, BLACK);

	osc_args.TimeOffset += delta;
	osc_args.TimeOffset = CLAMP(osc_args.TimeOffset, -GRID_WIDTH / 2, GRID_WIDTH / 2);

	LCD_DrawPicture_Stream(GRID_X + GRID_WIDTH / 2 + osc_args.TimeOffset - 5, GRID_Y - 12, 11, 11, down_arrow_pattern);
	UpdateHorizontalPosInfo();
}

static inline void UpdateHorizontalPosInfo(void)
{
	LCD_FillRect(TIMEBOX_X + 44, TIMEBOX_Y + 96, 84, 24, BLACK);
	switch (osc_args.TimeBase)
	{
	case DIV_10us: sprintf(str_buffer, "%+3.1fus", osc_args.TimeOffset * 0.2f); break;
	case DIV_100us: sprintf(str_buffer, "%+3.0fus", osc_args.TimeOffset * 2.0f); break;
	case DIV_1ms: sprintf(str_buffer, "%+3.2fms", osc_args.TimeOffset * 0.02f); break;
	case DIV_10ms: sprintf(str_buffer, "%+3.1fms", osc_args.TimeOffset * 0.2f); break;
	default: return;
	}
	LCD_DrawString(TIMEBOX_X + 44, TIMEBOX_Y + 96, 24, str_buffer, YELLOW);
}

static void AdjustTriggerVoltage(_Bool up_down_select)
{
	static _Bool previous_direction;
	static int16_t delta;

	if (previous_direction == up_down_select) {
		delta += (up_down_select) ? 1 : -1;
		delta = CLAMP(delta, -25, 25);
	}
	else {
		delta = (up_down_select) ? 1 : -1;
	}
	previous_direction = up_down_select;

	uint16_t line_pos_Y = 0;

	Graph_RecoverLineY(&graph, osc_args.TriggerVolt * DISPLAY_CVT_FACTOR * pow10(2 - osc_args.VoltBase) + osc_args.VoltOffset + GRID_HEIGHT / 2);

	osc_args.TriggerVolt += delta;
	osc_args.TriggerVolt = CLAMP(osc_args.TriggerVolt, 0, 4095);

	Graph_DrawDashedLineY(&graph, osc_args.TriggerVolt * DISPLAY_CVT_FACTOR * pow10(2 - osc_args.VoltBase) + osc_args.VoltOffset + GRID_HEIGHT / 2, LIGHTGREEN);

	htim6.Instance->CNT = 0;
	HAL_TIM_Base_Start_IT(&htim6);
}

void TIM6_DAC_IRQHandler(void)
{
	if (__HAL_TIM_GET_IT_SOURCE(&htim6, TIM_IT_UPDATE) != RESET)
	{
		__HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
		HAL_TIM_Base_Stop_IT(&htim6);
		Graph_RecoverLineY(&graph, osc_args.TriggerVolt * DISPLAY_CVT_FACTOR * pow10(2 - osc_args.VoltBase) + osc_args.VoltOffset + GRID_HEIGHT / 2);
	}
}

static inline void UpdateTriggerVoltageInfo(void)
{

}

static inline void UpdateSamplingRate(void)
{
	HAL_ADC_Stop_DMA(&hadc1);

	ADC_Common_TypeDef *tmpADC_Common = ADC_COMMON_REGISTER(&hadc1);
	tmpADC_Common->CCR &= ~(ADC_CCR_ADCPRE);
	tmpADC_Common->CCR |= sampling_args[osc_args.TimeBase].ADCClockPrescaler;

	hadc1.Instance->SMPR2 &= ~ADC_SMPR2(ADC_SMPR2_SMP0, ADC_CHANNEL_5);
	hadc1.Instance->SMPR2 |= ADC_SMPR2(sampling_args[osc_args.TimeBase].ADCSamplingTime, ADC_CHANNEL_5);

	HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, ADC_SAMPLE_COUNT);
}

static inline float pow10(uint8_t n)
{
	float value = 1.0f;
	while (n--) {
		value *= 10.0f;
	}
	return value;
}