#include "spectrum_display.h"
#include "lcd.h"
#include "adc.h"
#include "tim.h"

#include <arm_math.h>

extern TIM_HandleTypeDef htim3;

//采样及数据处理
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
static int16_t adc_samples_buffer[SAMPLE_COUNT];

// 绘图相关
static uint8_t str_buffer[8];
static int16_t display_values[SAMPLE_COUNT];
static Graph_TypeDef graph;

static float scale_factor = 0.005f;
static uint8_t freq_base;
static int16_t cursor_pos = 49;

void SpectrumDisplay_Init(void)
{
	TIM3_Init();
	ADC1_Init();
	ZLG7290_Init();

	freq_base = DIV_10kHz;

	//GUI 初始化
	graph.X = GRID_X;
	graph.Y = GRID_Y;

	graph.Width = GRID_WIDTH;
	graph.Height = GRID_HEIGHT;
	graph.RoughGridWidth = 50;
	graph.RoughGridHeight = 50;
	graph.FineGridWidth = 10;
	graph.FineGridHeight = 10;

	graph.BorderColor = WHITE;
	graph.BackgroudColor = BLACK;
	graph.RoughGridColor = GRAY;
	graph.FineGridColor = DARKGRAY;
	Graph_Init(&graph);


	LCD_DrawString(GRID_X, GRID_Y + GRID_HEIGHT + 16, 24, "水平档位:", WHITE);
	LCD_DrawString(GRID_X + 256, GRID_Y + GRID_HEIGHT + 16, 24, "光标位置:", WHITE);

	UpdateFrequencyInfo();
}

void SpectrumDisplay_Start(void)
{
	float window_func[SAMPLE_COUNT];
	float fft_input[SAMPLE_COUNT];
	float fft_output[SAMPLE_COUNT];
	int16_t fft_mag[SAMPLE_COUNT / 2];
	arm_rfft_fast_instance_f32 rfft;

	float sample_factor = 0.48438;

	arm_rfft_fast_init_f32(&rfft, SAMPLE_COUNT);
	GenerateWindowFunction(window_func, SAMPLE_COUNT, HAMMING_WINDOW);
	
	HAL_TIM_Base_Start(&htim3);
	HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, SAMPLE_COUNT);

	for (;;)
	{
		/* Wait for sampling period to complete */
		HAL_DMA_PollForTransfer(&hdma_adc1, HAL_DMA_FULL_TRANSFER, 0xFFFF);

		/* Stop sampling while conversion */
		HAL_ADC_Stop_DMA(&hadc1);
		arm_q15_to_float(adc_samples_buffer, fft_input, SAMPLE_COUNT);
		HAL_ADC_Start_DMA(&hadc1, adc_samples_buffer, SAMPLE_COUNT);

		/* Apply window function and FFT */
		arm_mult_f32(fft_input, window_func, fft_input, SAMPLE_COUNT);
		arm_rfft_fast_f32(&rfft, fft_input, fft_output, 0);
		/* Compute spectrum magnitude */
		arm_cmplx_mag_f32(fft_output, fft_output, SAMPLE_COUNT / 2);
		/* Scale and convert back to int16_t values */
		arm_scale_f32(fft_output, scale_factor, fft_output, SAMPLE_COUNT / 2);
		arm_float_to_q15(fft_output, fft_mag, SAMPLE_COUNT / 2);

		float sum = fft_output[66] + fft_output[67] + fft_output[68] + fft_output[69] + fft_output[70];

		Graph_RecoverGrid(&graph, display_values);
		Grpah_RecoverRect(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16);

		/* Linear interpolation for display values */
		for (size_t i = 0; i < GRID_WIDTH; i++) {
			uint32_t x = (i << 20) / GRID_WIDTH * SAMPLE_COUNT / 2;
			display_values[i] = arm_linear_interp_q15(fft_mag, x, SAMPLE_COUNT / 2) >> 3;
		}

		/* Draw spectrum curve */
		Graph_DrawCurve(&graph, display_values, YELLOW);
		Graph_DrawImg(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16, arrow_pattern);

#ifdef GRAPH_USE_BACKBUFFER
		LCD_BackBuffer_Update();
#endif // GRAPH_USE_BACKBUFFER
		
		switch (ZLG7290_ReadKey())
		{
		case 3:
			Grpah_RecoverRect(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16);
			cursor_pos = GRID_WIDTH / 2;
			break;
		case 4:
			MoveCursor(0);
			break;
		case 5:
			MoveCursor(1);
			break;
		case 12:
			scale_factor -= 0.0005f;
			if (scale_factor < 0.0005f) scale_factor = 0.0005f;
			break;
		case 13:
			scale_factor += 0.0005f;
			break;
		default:
			break;
		}
		Delay_ms(16);
	}
}

static void MoveCursor(_Bool right_left_select)
{
	static int16_t hit_count;

	Grpah_RecoverRect(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16);

	if ((hit_count < 0 && right_left_select) || (hit_count > 0 && !right_left_select)) {
		hit_count = 0;
	}

	hit_count += (right_left_select) ? 1 : -1;
	cursor_pos += ((right_left_select) ? 1 : -1) + hit_count / 10;

	cursor_pos = (cursor_pos < 0) ? GRID_WIDTH - 1 : cursor_pos;
	cursor_pos = (cursor_pos >= GRID_WIDTH) ? 0 : cursor_pos;

	LCD_FillRect(GRID_X + 368, GRID_Y + GRID_HEIGHT + 16, 160, 24, BLACK);
	switch (freq_base)
	{
	case DIV_10kHz: sprintf(str_buffer, "%3.1f kHz", cursor_pos * 0.2f); break;
	default:
		break;
	}
	LCD_DrawString(GRID_X + 368, GRID_Y + GRID_HEIGHT + 16, 24, str_buffer, GREEN);
}

static void UpdateFrequencyInfo(void)
{
	LCD_FillRect(GRID_X + 108, GRID_Y + GRID_HEIGHT + 16, 108, 24, BLACK);
	LCD_DrawString(GRID_X + 108, GRID_Y + GRID_HEIGHT + 16, 24, freq_base_tag[freq_base], CYAN);
}

static void GenerateWindowFunction(float *window, uint16_t length, WindowFunc_Type type)
{
	switch (type)
	{
	case RECTANGLE_WINDOW:
		for (uint16_t i = 0; i < length; i++) {
			window[i] = 1.0f;
		}
		break;

	case BARTLETT_WINDOW:
		for (uint16_t i = 0; i < length / 2; i++) {
			window[i] = 2.0f * i / length;
			window[i + length / 2] = 1.0f - 2.0f * i / length;
		}
		break;

	case HAMMING_WINDOW:
		for (uint16_t i = 0; i < length; i++) {
			window[i] = 0.54f - 0.46f * arm_cos_f32(2.0f * PI * i / length);
		}
		break;

	case BLACKMAN_WINDOW:
		for (uint16_t i = 0; i < length; i++) {
			window[i] = 0.42f
				- 0.5f * arm_cos_f32(2.0f * PI * i / (length - 1))
				+ 0.08f * arm_cos_f32(4.0f * PI * i / (length - 1));
		}
		break;

	case FLATTOP_WINDOW:
		for (uint16_t i = 0; i < length; i++) {
			window[i] = 0.21557895f
				- 0.41663158f * arm_cos_f32(2.0f * PI * i / (length - 1))
				+ 0.27726316f * arm_cos_f32(4.0f * PI * i / (length - 1))
				- 0.08357895f * arm_cos_f32(6.0f * PI * i / (length - 1))
				+ 0.00694737f * arm_cos_f32(8.0f * PI * i / (length - 1));
		}
		break;

	default: 
		break;
	}
}