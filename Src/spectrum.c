#include "spectrum.h"
#include "lcd.h"
//#include "adc.h"
#include "ads8694.h"
#include "tim.h"

#include <arm_math.h>

extern TIM_HandleTypeDef htim3;

//采样及数据处理
//extern ADC_HandleTypeDef hadc1;
//extern DMA_HandleTypeDef hdma_adc1;
static uint16_t sample_count;
static int32_t adc_sample_buffer[MAX_SAMPLE_COUNT];
float window_func[MAX_SAMPLE_COUNT];
float fft_input[MAX_SAMPLE_COUNT];
float fft_output[MAX_SAMPLE_COUNT];
int16_t fft_mag[MAX_SAMPLE_COUNT / 2];
arm_rfft_fast_instance_f32 rfft;

// 绘图相关
static uint8_t str_buffer[16];
static int16_t display_values[GRID_WIDTH];
static Graph_TypeDef graph;

static float scale_factor = 10.0f;
static uint8_t freq_base;
static uint8_t energy_base;
static int16_t cursor_pos = 49;

void SpectrumDisplay_Init(void)
{
	//TIM3_Init();
	//ADC1_Init();
	freq_base = DIV_1kHz;
	energy_base = 1;

	ADS8694_Init();
	ZLG7290_Init();

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

	LCD_DrawString(GRID_X, GRID_Y + GRID_HEIGHT + 16, 24, "水平档位", WHITE);
	LCD_DrawString(GRID_X + 216, GRID_Y + GRID_HEIGHT + 16, 24, "光标位置", WHITE);

	UpdateFrequencyInfo();
	UpdateCursorInfo();
	UpdateSamplingArgs();
}

void MeasureHarmonics(void)
{

}

void SpectrumDisplay_Start(void)
{
	//HAL_TIM_Base_Start(&htim3);
	//HAL_ADC_Start_DMA(&hadc1, adc_sample_buffer, sample_count);

	for (;;)
	{
		/* Wait for sampling period to complete */
		//HAL_DMA_PollForTransfer(&hdma_adc1, HAL_DMA_FULL_TRANSFER, 0xFFFF);

		/* Stop sampling while conversion */
		//HAL_ADC_Stop_DMA(&hadc1);
		ADS8694_StartSampling();
		arm_offset_q31(adc_sample_buffer, -(1 << 17), adc_sample_buffer, sample_count);
		arm_q31_to_float(adc_sample_buffer, fft_input, sample_count);
		//HAL_ADC_Start_DMA(&hadc1, adc_sample_buffer, sample_count);

		/* Apply window function and FFT */
		arm_mult_f32(fft_input, window_func, fft_input, sample_count);
		arm_rfft_fast_f32(&rfft, fft_input, fft_output, 0);
		/* Compute spectrum magnitude */
		arm_cmplx_mag_f32(fft_output, fft_output, sample_count / 2);

		/* Scale and convert back to int16_t values */
		arm_scale_f32(fft_output, scale_factor, fft_output, sample_count / 2);
		arm_float_to_q15(fft_output, fft_mag, sample_count / 2);

		//float energy_sum = fft_output[66] + fft_output[67] + fft_output[68] + fft_output[69] + fft_output[70];

		Graph_RecoverGrid(&graph, display_values);
		Grpah_RecoverRect(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16);

		/* Linear interpolation for display values */
		for (uint16_t i = 0; i < GRID_WIDTH; i++) {
			uint32_t x = (i << 20) / GRID_WIDTH * sample_count / 2;
			display_values[i] = arm_linear_interp_q15(fft_mag, x, sample_count / 2);
		}

		/* Draw spectrum curve */
		Graph_DrawCurve(&graph, display_values, YELLOW);
		Graph_DrawImg(&graph, cursor_pos - 8, display_values[cursor_pos] + 16, 16, 16, arrow_pattern);

#ifdef GRAPH_USE_BACKBUFFER
		LCD_BackBuffer_Update();
#endif // GRAPH_USE_BACKBUFFER

		switch (ZLG7290_ReadKey())
		{
		case 1:
			freq_base = (freq_base + 1) % 4;
			UpdateSamplingArgs();
			UpdateFrequencyInfo();
			UpdateCursorInfo();
			break;
		case 2:
			energy_base = (energy_base + 1) % 4;
			scale_factor = pow10(energy_base) * 0.1f;
			break;
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
			scale_factor -= pow10(energy_base) * 0.002f;
			break;
		case 13:
			scale_factor += pow10(energy_base) * 0.002f;
			break;
		case 33:
			ADS8694_Init();
			UpdateSamplingArgs();
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
	UpdateCursorInfo();
}

static inline void UpdateCursorInfo(void)
{
	switch (freq_base)
	{
	case DIV_50Hz: sprintf(str_buffer, "%3.1fHz", cursor_pos * 1.0f); break;
	case DIV_100Hz: sprintf(str_buffer, "%3.0fHz", cursor_pos * 2.0f); break;
	case DIV_500Hz: sprintf(str_buffer, "%3.1fkHz", cursor_pos * 0.01f); break;
	case DIV_1kHz: sprintf(str_buffer, "%3.1fkHz", cursor_pos * 0.02f); break;
	default:
		break;
	}

	LCD_FillRect(GRID_X + 320, GRID_Y + GRID_HEIGHT + 16, 160, 24, BLACK);
	LCD_DrawString(GRID_X + 320, GRID_Y + GRID_HEIGHT + 16, 24, str_buffer, GREEN);
}

static void UpdateSamplingArgs(void)
{
	sample_count = sample_count_values[freq_base];

	arm_rfft_fast_init_f32(&rfft, sample_count);
	GenerateWindowFunction(&window_func, sample_count, HAMMING_WINDOW);

	ADS8694_ConfigSampling(&adc_sample_buffer, sample_count, ADS8694_CHANNEL_0, INPUT_RANGE_BIPOLAR_0_625x);

	switch (freq_base)
	{
	case DIV_50Hz:
		ADS8694_SetSamplingRate(1200);
		break;
	case DIV_100Hz:
		ADS8694_SetSamplingRate(2400);
		break;
	case DIV_500Hz:
		ADS8694_SetSamplingRate(12000);
		break;
	case DIV_1kHz:
		ADS8694_SetSamplingRate(24000);
		break;
	default:
		break;
	}
}

static inline void UpdateFrequencyInfo(void)
{
	LCD_FillRect(GRID_X + 100, GRID_Y + GRID_HEIGHT + 16, 108, 24, BLACK);
	LCD_DrawString(GRID_X + 100, GRID_Y + GRID_HEIGHT + 16, 24, freq_base_tag[freq_base], CYAN);
}

static void GenerateWindowFunction(float *window, uint16_t length, uint8_t type)
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

static inline float pow10(uint8_t n)
{
	float val = 1.0f;
	while (n--) {
		val *= 10;
	}
	return val;
}