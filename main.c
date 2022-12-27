
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/sync.h"

#include "scheduler/uevent.h"
#include "scheduler/scheduler.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812/ws2812.pio.h"

#include "platform.h"

#include "display.h"
#include "ui.h"

#define IS_RGBW false
#define NUM_PIXELS 1

static inline void put_pixel(uint32_t pixel_grb) {
	pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}
#include <math.h>
typedef struct RGBColor {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGBColor;

RGBColor hsv2rgb(float h, float s, float v) {
	float r, g, b;

	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	switch(i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}

	RGBColor color;
	color.r = r * 255;
	color.g = g * 255;
	color.b = b * 255;

	return color;
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

#include "pico/sync.h"
critical_section_t scheduler_lock;
static __inline void CRITICAL_REGION_INIT(void) {
	critical_section_init(&scheduler_lock);
}
static __inline void CRITICAL_REGION_ENTER(void) {
	critical_section_enter_blocking(&scheduler_lock);
}
static __inline void CRITICAL_REGION_EXIT(void) {
	critical_section_exit(&scheduler_lock);
}

bool timer_1000hz_callback(struct repeating_timer* t) {
	static uint8_t f = 0;
	static uint16_t c = 0;
	uevt_bc_e(UEVT_TIMER_1000HZ);
	f += 1;
	c += 1;
	if((f & 0x7) == 0) {
		uevt_bc_e(UEVT_TIMER_125HZ);
	}
	if((f & 0xF) == 0) {
		uevt_bc_e(UEVT_TIMER_62HZ);
	}
	if(c >= 1000) {
		c = 0;
		uevt_bc_e(UEVT_TIMER_1HZ);
	}
	return true;
}

void led_blink_routine(void) {
	static uint8_t _tick = 0;
	_tick += 1;
	if(_tick == 64) {
		RGBColor color = hsv2rgb(0.7, 0.99, 0.2);
		put_pixel(urgb_u32(color.r, color.g, color.b));
	}
	if(_tick == 74) {
		put_pixel(0);
	}
	if(_tick == 100) {
		_tick = 0;
	}
}

void temperature_routine(void) {
	static uint16_t tick = 0;
	static float _t = 0;
	if(tick > 200) {
		tick = 0;
		adc_select_input(4);
		int32_t t_adc = adc_read();
		_t = 27.0 - ((t_adc - 876) / 2.136f);
		uevt_bc(UEVT_ADC_TEMPERATURE_RESULT, &_t);
	}
	tick += 1;
}

void main_handler(uevt_t* evt) {
	static uint32_t tick = 0;
	switch(evt->evt_id) {
		case UEVT_TIMER_1000HZ:
			break;
		case UEVT_TIMER_125HZ:
			led_blink_routine();
			// temperature_routine();
			ui_period_hanlder(8);
			break;
		case UEVT_TIMER_62HZ: {
			static uint8_t slower = 0;
			if((slower++ & 0x3) == 0) {
				adc_capture_test();
			}
		} break;
		case UEVT_TIMER_1HZ:
			printf("[%08d]:\n", tick++);
			break;
		case UEVT_ADC_TEMPERATURE_RESULT:
			printf("Temperature is %0.2f\n", *((float*)(evt->content)));
			break;
	}
}

#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/xosc.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
void clock_init(void) {
	stdio_init_all();
}

#define CAPTURE_DEPTH 256
uint16_t capture_buf[CAPTURE_DEPTH * 4];

void adc_setup(void) {
	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);
	adc_gpio_init(29);
	// sample rate 8e3
	adc_set_clkdiv(15625);
	adc_set_temp_sensor_enabled(true);
	adc_set_round_robin(0xF);
	adc_fifo_setup(
		true, // Write each completed conversion to the sample FIFO
		true, // Enable DMA data request (DREQ)
		1,    // DREQ (and IRQ) asserted when at least 1 sample present
		false,
		false);
}

typedef int_fast16_t elem_type;
elem_type median_buf[7];

#ifndef ELEM_SWAP(a, b)
	#define ELEM_SWAP(a, b) \
		{ \
			register elem_type t = (a); \
			(a) = (b); \
			(b) = t; \
		}
#endif

elem_type quick_select_median(elem_type arr[], uint16_t n) {
	uint16_t low, high;
	uint16_t median;
	uint16_t middle, ll, hh;
	low = 0;
	high = n - 1;
	median = (low + high) / 2;
	for(;;) {
		if(high <= low) /* One element only */
			return arr[median];
		if(high == low + 1) { /* Two elements only */
			if(arr[low] > arr[high])
				ELEM_SWAP(arr[low], arr[high]);
			return arr[median];
		}
		/* Find median of low, middle and high items; swap into position low */
		middle = (low + high) / 2;
		if(arr[middle] > arr[high])
			ELEM_SWAP(arr[middle], arr[high]);
		if(arr[low] > arr[high])
			ELEM_SWAP(arr[low], arr[high]);
		if(arr[middle] > arr[low])
			ELEM_SWAP(arr[middle], arr[low]);
		/* Swap low item (now in position middle) into position (low+1) */
		ELEM_SWAP(arr[middle], arr[low + 1]);
		/* Nibble from each end towards middle, swapping items when stuck */
		ll = low + 1;
		hh = high;
		for(;;) {
			do
				ll++;
			while(arr[low] > arr[ll]);
			do
				hh--;
			while(arr[hh] > arr[low]);
			if(hh < ll)
				break;
			ELEM_SWAP(arr[ll], arr[hh]);
		}
		/* Swap middle item (in position low) back into correct position */
		ELEM_SWAP(arr[low], arr[hh]);
		/* Re-set active partition */
		if(hh <= median)
			low = ll;
		if(hh >= median)
			high = hh - 1;
	}
	return arr[median];
}

void adc_capture_test(void) {
	static uint32_t old_datas[4] = { 0, 0, 0, 0 };
	static elem_type m1_min = 4096, m2_min = 4096, m3_min = 4096;
	uint dma_chan = dma_claim_unused_channel(true);
	dma_channel_config dma_cfg = dma_channel_get_default_config(dma_chan);

	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_cfg, false);
	channel_config_set_write_increment(&dma_cfg, true);

	channel_config_set_dreq(&dma_cfg, DREQ_ADC);

	dma_channel_configure(dma_chan, &dma_cfg,
						  capture_buf,   // dst
						  &adc_hw->fifo, // src
						  CAPTURE_DEPTH, // transfer count
						  true           // start immediately
	);
	adc_run(true);
	dma_channel_wait_for_finish_blocking(dma_chan);
	adc_run(false);
	adc_fifo_drain();
	dma_channel_unclaim(dma_chan);
	printf("Capture finished\n");
	for(int i = 0; i < 7; i++) {
		median_buf[i] = capture_buf[i * 4];
	}
	elem_type m = quick_select_median(median_buf, 7) - 82;
	if(m < 0) {
		m = 0;
	}
	uint32_t volt = (uint32_t)m * 107 / 50;
	printf("U = %d mv\n", volt);

	for(int i = 0; i < 7; i++) {
		median_buf[i] = capture_buf[i * 4 + 1];
	}
	elem_type m1 = quick_select_median(median_buf, 7);
	if(m1 < m1_min) {
		m1_min = m1;
	}
	if(m1 < m1_min) {
		m1 = m1_min;
	}
	// printf("ADC1 = %d\n", m1);

	for(int i = 0; i < 7; i++) {
		median_buf[i] = capture_buf[i * 4 + 2];
	}
	elem_type m2 = quick_select_median(median_buf, 7);
	if(m2 < m2_min) {
		m2_min = m2;
	}
	if(m2 < m2_min) {
		m2 = m2_min;
	}

	// printf("ADC2 = %d\n", m2);

	for(int i = 0; i < 7; i++) {
		median_buf[i] = capture_buf[i * 4 + 3];
	}
	elem_type m3 = quick_select_median(median_buf, 7);
	if(m3 < m3_min) {
		m3_min = m3;
	}
	if(m3 < m3_min) {
		m3 = m3_min;
	}
	// printf("ADC3 = %d\n", m3);

	uint32_t current;
	if(m3 < 3770) {
		current = (((uint32_t)m3 - m3_min) * 37 + 5) / 10;
	} else if(m2 < 3770) {
		current = (((uint32_t)m2 - m2_min) * 93 + 5) / 10;
	} else {
		current = (((uint32_t)m1 - m1_min) * 300 + 5) / 10;
	}
	printf("I = %d mA\n", current);

	uint32_t power = current * volt / 1000;
	printf("P = %d mW\n", power);
	uint32_t resistance = 0;
	uint32_t datas[4] = { volt, current, power, resistance };
	if(current > 100) {
		resistance = volt * 1000 / current;
		datas[3] = resistance;
		printf("R = %d mOhm\n", resistance);
	} else {
		datas[0] = 0;
		datas[1] = 0;
		datas[2] = 0;
		datas[3] = 0xFFFFFFFF;
	}
	if((datas[0] != old_datas[0]) || (datas[1] != old_datas[1]) || (datas[2] != old_datas[2]) || (datas[3] != old_datas[3])) {
		old_datas[0] = datas[0];
		old_datas[1] = datas[1];
		old_datas[2] = datas[2];
		old_datas[3] = datas[3];
		ui_data_update(old_datas);
	}
}

int main() {
	xosc_init();
	clock_init();

	CRITICAL_REGION_INIT();
	app_sched_init();
	user_event_init();
	user_event_handler_regist(main_handler);

	adc_setup();

	sleep_ms(10);
	display_init();
	ui_init();

	PIO pio = pio0;
	uint offset = pio_add_program(pio, &ws2812_program);
	ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);

	struct repeating_timer timer;
	add_repeating_timer_ms(1, timer_1000hz_callback, NULL, &timer);

	while(true) {
		app_sched_execute();
		__wfi();
	}
}
