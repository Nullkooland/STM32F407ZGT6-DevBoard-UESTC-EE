#include "frequency_sweep.h"
#include "number_input.h"
#include "lcd.h"
#include "zlg7290.h"
#include "ad9959.h"

#include <arm_math.h>

//绘图相关
static uint8_t str_buffer[32];
static uint16_t display_values[GRID_WIDTH];
static Graph_TypeDef graph;

static _Bool is_cursor_select_A;
static int16_t cursor_XA, cursor_XB;

//输出幅值控制
static uint8_t output_amp;
static uint8_t amp_step;
//static uint8_t pe4302_2x_loss;	// dB

//扫频控制
static uint32_t sweep_freq[3];		// 0:起始频率, 1:终止频率, 2:频率步进

//检波电平采样
extern ADC_HandleTypeDef hadc1;
static uint16_t adc_sampling_values[ADC_SAMPLE_COUNT];
static uint16_t data_values[MAX_SAMPLE_COUNT];
static uint16_t sample_count;

void FreqSweep_Init(void)
{
	//硬件外设初始化
	PE4302_Init();
	AD9959_Init();
	ADC1_Init();

	AD9959_SetFreq(AD9959_CHANNEL_2, 1000000U);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = GPIO_PIN_10;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

	//设置各项初始参数
	//pe4302_2x_loss = 0;
	output_amp = 50;
	amp_step = 1;

	sweep_freq[0] = 4000;
	sweep_freq[1] = 50000;
	sweep_freq[2] = 100;
	sample_count = (sweep_freq[1] - sweep_freq[0]) / sweep_freq[2];

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

	/* Interp Test */
	/*
	uint16_t size = 128;
	float delta = 0.12f;

	uint16_t sample_data[128];
	uint16_t origin_data[GRID_WIDTH];
	uint16_t interp_data[GRID_WIDTH];

	for (uint16_t i = 0; i < size; i++) {
		sample_data[i] = (2.0f + arm_sin_f32(delta * i)) * 100U;
	}

	for (uint16_t i = 0; i < GRID_WIDTH; i++) {
		//Linear Interpolation
		uint32_t x = (i << 20) / GRID_WIDTH * size;
		interp_data[i] = arm_linear_interp_q15(sample_data, x, size);
		//For Contrast
		origin_data[i] = (2.0f + arm_sin_f32(delta * size / GRID_WIDTH * i)) * 100U;
	}

	Graph_DrawCurve(&graph, origin_data, BLUE);
	Graph_DrawCurve(&graph, interp_data, RED);
	*/
	//LCD_BackBuffer_Update();

	int i;

	//纵坐标-幅度
	LCD_ShowChar_ASCII(GRID_X - 15, GRID_Y + 192, 16, '0', WHITE);
	for (i = 0; i < 4; i++) {
		LCD_ShowNumber(GRID_X - 20, GRID_Y - 8 + i * 50, 16, (4 - i) * 10, WHITE);
	}
	for (i = 0; i < 3; i++) {
		LCD_ShowNumber(GRID_X - 20, GRID_Y + 242 + i * 50, 16, (i + 1) * 10, WHITE);
		LCD_ShowChar_ASCII(GRID_X - 30, GRID_Y + 242 + i * 50, 16, '-', WHITE);
	}
	//单位：dB
	LCD_ShowString(GRID_X - 20, GRID_Y + GRID_HEIGHT - 16, 16, "dB", WHITE);

	//横坐标-频率
	UpdateFreqInfoDispaly();
	//单位：MHz
	LCD_ShowString(GRID_X + GRID_WIDTH - 12, GRID_Y + GRID_HEIGHT + 20, 16, "MHz", WHITE);

	//扫频信息窗    
	LCD_DrawRect(FREQBOX_X, FREQBOX_Y, FREQBOX_WIDTH, FREQBOX_HEIGHT, WHITE);
	LCD_ShowString(FREQBOX_X + 8, FREQBOX_Y + 8, 16, "起始频率:", WHITE);
	LCD_ShowString(FREQBOX_X + 8, FREQBOX_Y + 32, 16, "终止频率:", WHITE);
	LCD_ShowString(FREQBOX_X + 8, FREQBOX_Y + 56, 16, "频率步进:", WHITE);
	LCD_ShowString(FREQBOX_X + 8, FREQBOX_Y + 80, 16, "采样点数:", WHITE);

	//输出幅度信息窗
	LCD_DrawRect(AMPBOX_X, AMPBOX_Y, AMPBOX_WIDTH, AMPBOX_HEIGHT, WHITE);
	LCD_ShowString(AMPBOX_X + 8, AMPBOX_Y + 8, 16, "当前输出幅度:", WHITE);
	LCD_ShowString(AMPBOX_X + 8, AMPBOX_Y + 32, 16, "幅度调节步进:", WHITE);

	sprintf(str_buffer, "%-3u mV", (amp_step + 1) * 2);
	LCD_ShowString(AMPBOX_X + 118, AMPBOX_Y + 32, 16, str_buffer, LIGHTGRAY);
	UpdateOutputAmp();

	//光标读数窗
	LCD_DrawRect(CURSORBOX_X, CURSORBOX_Y, CURSORBOX_WIDTH, CURSORBOX_HEIGHT, WHITE);
	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 8, 16, "光标选择:", WHITE);

	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 40, 16, "A - 频率:", WHITE);
	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 64, 16, "B - 频率:", WHITE);
	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 88, 16, "Δ- 频率:", WHITE);

	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 120, 16, "A - 增益:", WHITE);
	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 144, 16, "B - 增益:", WHITE);
	LCD_ShowString(CURSORBOX_X + 8, CURSORBOX_Y + 168, 16, "Δ- 增益:", WHITE);

	//光标初始化
	is_cursor_select_A = 1;
	cursor_XA = GRID_WIDTH / 3;
	cursor_XB = GRID_WIDTH * 2 / 3;
	CursorParametersDisplay();
}

static void UpdateFreqInfoDispaly(void)
{
	//使用大黑块进行[数据删除]
	LCD_FillRect(GRID_X - 12, GRID_Y + GRID_HEIGHT + 3, GRID_WIDTH + 36, 16, BLACK);
	LCD_FillRect(FREQBOX_X + 85, FREQBOX_Y + 8, 80, 88, BLACK);

	//更新频率轴坐标值
	for (uint8_t i = 0; i <= 10; i++) {

		sprintf(str_buffer, "%-4.1f", (sweep_freq[0] + i * (sweep_freq[1] - sweep_freq[0]) / 10) * 0.001f);
		LCD_ShowString(GRID_X - 12 + i * 50, GRID_Y + GRID_HEIGHT + 2, 16, str_buffer, WHITE);
	}

	//更新扫频信息窗显示数值
	for (uint8_t i = 0; i < 3; i++) {
		sprintf(str_buffer, "%-6.3f MHz", sweep_freq[i] * 0.001f);
		LCD_ShowString(FREQBOX_X + 85, FREQBOX_Y + 8 + 24 * i, 16, str_buffer, LIGHTGRAY);
	}

	sprintf(str_buffer, "%u 点", sample_count);
	LCD_ShowString(FREQBOX_X + 85, FREQBOX_Y + 80, 16, str_buffer, LIGHTGRAY);
}

static inline void UpdateOutputAmp(void)
{
	//查表并设置衰减值
	PE4302_SetLoss(amp_table[50 - output_amp][0]);
	AD9959_SetAmp(OUTPUT_CHANNEL, amp_table[50 - output_amp][1]);

	//使用大黑块进行[数据删除]
	LCD_FillRect(AMPBOX_X + 118, AMPBOX_Y + 8, 48, 16, BLACK);

	//更新输出幅度息窗显示数值
	sprintf(str_buffer, "%-3u mV", output_amp * 2);
	LCD_ShowString(AMPBOX_X + 118, AMPBOX_Y + 8, 16, str_buffer, LIGHTGRAY);
}

static inline void FreqParametersDisplay(uint8_t i, _Bool isSlected)
{
	uint16_t back_color = (isSlected) ? LIGHTGRAY : BLACK;
	uint16_t font_color = (isSlected) ? BLACK : LIGHTGRAY;

	LCD_FillRect(FREQBOX_X + 85, FREQBOX_Y + 8 + 24 * i, 80, 16, back_color);
	sprintf(str_buffer, "%-6.3f MHz", sweep_freq[i] * 0.001f);
	LCD_ShowString(FREQBOX_X + 85, FREQBOX_Y + 8 + 24 * i, 16, str_buffer, font_color);
}

static void SetFreqParameters(void)
{
	uint8_t i = 0;
	float input_val;

	uint16_t backup_sweep_freq[3];
	backup_sweep_freq[0] = sweep_freq[0];
	backup_sweep_freq[1] = sweep_freq[1];
	backup_sweep_freq[2] = sweep_freq[2];

	FreqParametersDisplay(i, 1);

	for (;;)
	{
		switch (ZLG7290_ReadKey())
		{
		case 18:

			FreqParametersDisplay(i, 0);

			if (i == 0) {
				i = 2;
			}
			else {
				--i;
			}

			FreqParametersDisplay(i, 1);
			break;

		case 19:

			FreqParametersDisplay(i, 0);

			if (i == 2) {
				i = 0;
			}
			else {
				++i;
			}

			FreqParametersDisplay(i, 1);
			break;

		case 20:

			if (GetInputFloat(&input_val)) {
				sweep_freq[i] = input_val * 1000U;
				FreqParametersDisplay(i, 1);
			}
			break;

		case 17:
			//输入的扫频范围无效 还原原来的范围
			if (sweep_freq[0] > sweep_freq[1] || sweep_freq[1] < 100U || sweep_freq[1] > 200000U) {
				sweep_freq[0] = backup_sweep_freq[0];
				sweep_freq[1] = backup_sweep_freq[1];
			}

			sample_count = (sweep_freq[1] - sweep_freq[0]) / sweep_freq[2];

			//采样点太多或太少 还原原来的频率步进
			if (sample_count < MIN_SAMPLE_COUNT || sample_count > MAX_SAMPLE_COUNT) {
				sweep_freq[2] = backup_sweep_freq[2];
				sample_count = (sweep_freq[1] - sweep_freq[0]) / sweep_freq[2];
			}

			UpdateFreqInfoDispaly();
			return;
		}

		Delay_ms(33);
	}
}

static void FreqSweepAndSampling()
{
	uint32_t output_freq = sweep_freq[0];

	//ADC开始采样对数检波电平
	HAL_ADC_Start_DMA(&hadc1, adc_sampling_values, ADC_SAMPLE_COUNT);

	for (uint16_t i = 0; i < sample_count; i++)
	{
		AD9959_SetFreq(OUTPUT_CHANNEL, output_freq * 1000U);

		Delay_us(25);
		arm_mean_q15(adc_sampling_values, ADC_SAMPLE_COUNT, &data_values[i]);

		output_freq += sweep_freq[2];
	}

	//ADC停止采样
	HAL_ADC_Stop_DMA(&hadc1);
}

void FreqSweep_Start()
{
	for (;;)
	{
		switch (ZLG7290_ReadKey())
		{
		case 17:
			SetFreqParameters();
			break;

		case 25:
			++amp_step;
			amp_step %= 5;

			sprintf(str_buffer, "%-3u mV", (amp_step + 1) * 2);

			LCD_FillRect(AMPBOX_X + 118, AMPBOX_Y + 32, 48, 16, BLACK);
			LCD_ShowString(AMPBOX_X + 118, AMPBOX_Y + 32, 16, str_buffer, LIGHTGRAY);
			break;

		case 26:
			if (output_amp > amp_step + 2) {
				output_amp -= amp_step + 1;
			}

			UpdateOutputAmp();
			break;

		case 27:
			if (output_amp + amp_step < 50) {
				output_amp += amp_step + 1;
			}

			UpdateOutputAmp();
			break;

		case 33:
			is_cursor_select_A = !is_cursor_select_A;

			Graph_RecoverCursorX(&graph, cursor_XA, cursor_XB);
			CursorParametersDisplay();
			break;

		case 34:
			Graph_RecoverCursorX(&graph, cursor_XA, cursor_XB);

			if (is_cursor_select_A) {
				cursor_XA -= 2;
				cursor_XA = (cursor_XA <= 0) ? GRID_WIDTH - 1 : cursor_XA;
			}
			else {
				cursor_XB -= 2;
				cursor_XB = (cursor_XB <= 0) ? GRID_WIDTH - 1 : cursor_XB;
			}

			CursorParametersDisplay();
			break;

		case 35:
			Graph_RecoverCursorX(&graph, cursor_XA, cursor_XB);

			if (is_cursor_select_A) {
				cursor_XA += 2;
				cursor_XA %= GRID_WIDTH;
			}
			else {
				cursor_XB += 2;
				cursor_XB %= GRID_WIDTH;
			}

			CursorParametersDisplay();
			break;
		}
		
		FreqSweepAndSampling();

		Graph_RecoverCursorX(&graph, cursor_XA, cursor_XB);
		Graph_RecoverGrid(&graph, display_values);

		//将采样数据线性插值到图表区相同的宽度以便于显示
		for (uint16_t i = 0; i < GRID_WIDTH; i++) {
			uint32_t x = (i << 20) / GRID_WIDTH * sample_count;
			display_values[i] = arm_linear_interp_q15(data_values, x, sample_count) + 2048U;
		}
		arm_shift_q15(display_values, -4, display_values, GRID_WIDTH);

		if (is_cursor_select_A) {
			Graph_DrawCursorX(&graph, cursor_XA, YELLOW, cursor_XB, BROWN);
		}
		else {
			Graph_DrawCursorX(&graph, cursor_XA, BROWN, cursor_XB, YELLOW);
		}

		Graph_DrawCurve(&graph, display_values, RED);

#if GRAPH_USE_BACKBUFFER
		LCD_BackBuffer_Update();
#endif // GRAPH_USE_BACKBUFFER

		Delay_ms(33);
	}
}

static inline void CursorParametersDisplay(void)
{
	//用大黑块进行[数据删除]
	LCD_FillRect(CURSORBOX_X + 85, CURSORBOX_Y + 8, 88, 180, BLACK);

	uint8_t mark = (is_cursor_select_A) ? 'A' : 'B';
	LCD_ShowChar_ASCII(CURSORBOX_X + 85, CURSORBOX_Y + 8, 16, mark, YELLOW);

	float freq_A = (sweep_freq[0] + (sweep_freq[1] - sweep_freq[0]) * cursor_XA / GRID_WIDTH) * 0.001f;
	float freq_B = (sweep_freq[0] + (sweep_freq[1] - sweep_freq[0]) * cursor_XB / GRID_WIDTH) * 0.001f;

	sprintf(str_buffer, "%-6.3f MHz", freq_A);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 40, 16, str_buffer, LIGHTGRAY);

	sprintf(str_buffer, "%-6.3f MHz", freq_B);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 64, 16, str_buffer, LIGHTGRAY);

	sprintf(str_buffer, "%-6.3f MHz", freq_B - freq_A);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 88, 16, str_buffer, LIGHTGRAY);

	float gain_A = (display_values[cursor_XA] - 200) * 0.2f;
	float gain_B = (display_values[cursor_XB] - 200) * 0.2f;

	sprintf(str_buffer, "%-6.3f dB", gain_A);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 120, 16, str_buffer, LIGHTGRAY);

	sprintf(str_buffer, "%-6.3f dB", gain_B);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 144, 16, str_buffer, LIGHTGRAY);

	sprintf(str_buffer, "%-6.3f dB", gain_B - gain_A);
	LCD_ShowString(CURSORBOX_X + 85, CURSORBOX_Y + 168, 16, str_buffer, LIGHTGRAY);
}
