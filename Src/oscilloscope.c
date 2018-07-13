#include "oscilloscope.h"
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

//采样
extern ADC_HandleTypeDef hadc1;
static int16_t adc_samples_buffer[ADC_SAMPLE_COUNT];

//水平时基档位
static uint8_t time_div = 4;

//垂直电压档位
static uint8_t volt_div = 2;

//垂直位置
static int16_t vertical_pos = GRID_HEIGHT / 2;

//触发
static int16_t trigger_volt = 256;

void Oscilloscope_Init(void)
{
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

	LCD_DrawRect(TIMEBOX_X, TIMEBOX_Y, TIMEBOX_WIDTH, TIMEBOX_HEIGHT, WHITE);
	LCD_DrawString(TIMEBOX_X + 8, TIMEBOX_Y + 5, 24, "水平时基档位", WHITE);
	LCD_DrawString(TIMEBOX_X + (TIMEBOX_WIDTH - strlen(timeBase[time_div]) * 16) / 2, TIMEBOX_Y + 36, 32, timeBase[time_div], YELLOW);

	LCD_DrawString(TIMEBOX_X + 32, TIMEBOX_Y + 75, 24, "水平位置", WHITE);
	LCD_DrawString(TIMEBOX_X + 20, TIMEBOX_Y + 105, 32, "+5ms", YELLOW);

	LCD_DrawRect(VOLTBOX_X, VOLTBOX_Y, VOLTBOX_WIDTH, VOLTBOX_HEIGHT, WHITE);
	LCD_DrawString(VOLTBOX_X + 8, VOLTBOX_Y + 5, 24, "垂直电压档位", WHITE);
	LCD_DrawString(VOLTBOX_X + (VOLTBOX_WIDTH - strlen(voltBase[volt_div]) * 16) / 2, VOLTBOX_Y + 36, 32, voltBase[volt_div], YELLOW);

	LCD_DrawString(VOLTBOX_X + 32, VOLTBOX_Y + 75, 24, "垂直位置", WHITE);
	UpdateVerticalPosInfo();

	//LCD_DrawPicture_Stream(7, GRID_Y + GRID_HEIGHT / 2 - 6, 12, 12, right_arrow_pattern);
	LCD_DrawPicture_Stream(GRID_X + GRID_WIDTH / 2 - 6, 7, 12, 12, down_arrow_pattern);

	LCD_DrawRect(TRIGBOX_X, TRIGBOX_Y, TRIGBOX_WIDTH, TRIGBOX_HEIGHT, WHITE);
	LCD_DrawString(TRIGBOX_X + 32, TRIGBOX_Y + 5, 24, "触发电平", WHITE);
	
	//LCD_BackBuffer_Update();
	//HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, ADC_SAMPLE_COUNT);

	Oscilloscope_Test();
}

void Oscilloscope_Test(void)
{
	for (;;)
	{
		uint16_t trigger_pos = ADC_SAMPLE_COUNT / 2 - 64;
		int32_t delta_volt_a, delta_volt_b;

		Graph_RecoverGrid(&graph, display_values);
		HAL_ADC_Stop_DMA(&hadc1);

		for (uint16_t i = 0; i < 512; i++)
		{
			delta_volt_a = adc_samples_buffer[trigger_pos] - trigger_volt;
			delta_volt_b = adc_samples_buffer[trigger_pos + 1] - trigger_volt;

			if (delta_volt_a < 0 && delta_volt_b > 0) {
				break;
			}
			++trigger_pos;
		}

		int16_t *adc_samples_start = adc_samples_buffer + trigger_pos - GRID_WIDTH / 2;

		for (uint16_t i = 0; i < GRID_WIDTH; i++)
		{
			display_values[i] = ((adc_samples_start[i]) * 25) >> 8;
			display_values[i] += vertical_pos;
		}

		HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, ADC_SAMPLE_COUNT);

		Graph_DrawCurve(&graph, display_values, RED);
		LCD_BackBuffer_Update();

		switch (ZLG7290_ReadKey())
		{
		case 1:
			time_div = (time_div + 1) % 5;
			LCD_FillRect(TIMEBOX_X + 8, TIMEBOX_Y + 36, 150, 32, BLACK);
			LCD_DrawString(TIMEBOX_X + (TIMEBOX_WIDTH - strlen(timeBase[time_div]) * 16) / 2, TIMEBOX_Y + 36, 32, timeBase[time_div], YELLOW);
			break;

		case 2:
			volt_div = (volt_div + 1) % 3;
			LCD_FillRect(VOLTBOX_X + 8, VOLTBOX_Y + 36, 150, 32, BLACK);
			LCD_DrawString(VOLTBOX_X + (VOLTBOX_WIDTH - strlen(voltBase[volt_div]) * 16) / 2, VOLTBOX_Y + 36, 32, voltBase[volt_div], YELLOW);
			UpdateVerticalPosInfo();
			break;

		case 9:
			break;

		case 10:
			break;

		case 4:
			AdjustVerticalPos(1);
			break;

		case 12:
			AdjustVerticalPos(0);
			break;

		case 5:
			break;

		case 13:
			break;
		default:
			break;
		}
		Delay_ms(32);
	}
}

static void AdjustVerticalPos(_Bool up_down_select)
{
	static _Bool last_direction;
	static int16_t delta;

	if (last_direction == up_down_select) {
		delta += (up_down_select) ? 1 : -1;
		delta = (delta < 10) ? delta : 10;
		delta = (delta > -10) ? delta : -10;
	}
	else {
		delta = (up_down_select) ? 1 : -1;
	}

	LCD_FillRect(7, GRID_Y + GRID_HEIGHT - vertical_pos - 6, 12, 12, BLACK);

	vertical_pos += delta;
	vertical_pos = (vertical_pos > 0) ? vertical_pos : 0;
	vertical_pos = (vertical_pos < GRID_HEIGHT) ? vertical_pos : GRID_HEIGHT;

	LCD_DrawPicture_Stream(7, GRID_Y + GRID_HEIGHT - vertical_pos - 6, 12, 12, right_arrow_pattern);
	UpdateVerticalPosInfo();

	last_direction = up_down_select;
}

static inline void UpdateVerticalPosInfo(void)
{
	LCD_FillRect(VOLTBOX_X + 24, VOLTBOX_Y + 105, 112, 32, BLACK);
	if (volt_div == 0) {
		sprintf(str_buffer, "%+3.1fmV", (float)(vertical_pos - GRID_HEIGHT / 2) * 0.2f);
	}
	else if (volt_div == 1) {
		sprintf(str_buffer, "%+3.0fmV", (float)(vertical_pos - GRID_HEIGHT / 2) * 2.0f);
	}
	else if (volt_div == 2) {
		sprintf(str_buffer, "%+3.2fV", (float)(vertical_pos - GRID_HEIGHT / 2) * 0.02f);
	}
	LCD_DrawString(VOLTBOX_X + 24, VOLTBOX_Y + 105, 32, str_buffer, YELLOW);
}

static inline float pow10(uint8_t n)
{
	float value = 1.0f;
	while (n--) {
		value *= 10.0f;
	}
	return value;
}