
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/sync.h"

#include "scheduler/uevent.h"
#include "scheduler/scheduler.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812/ws2812.pio.h"

#include "platform.h"

#include "display.h"

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

bool timer_100hz_callback(struct repeating_timer* t) {
	uevt_bc_e(UEVT_TIMER_100HZ);
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
		int32_t t_adc = adc_read();
		_t = 27.0 - ((t_adc - 876) / 2.136f);
		uevt_bc(UEVT_ADC_TEMPERATURE_RESULT, &_t);
	}
	tick += 1;
}

void main_handler(uevt_t* evt) {
	switch(evt->evt_id) {
		case UEVT_TIMER_100HZ:
			led_blink_routine();
			temperature_routine();
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

int main() {
	xosc_init();
	stdio_init_all();
	clock_init();

	CRITICAL_REGION_INIT();
	app_sched_init();
	user_event_init();
	user_event_handler_regist(main_handler);

	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	adc_gpio_init(28);
	adc_gpio_init(29);
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

	sleep_ms(10);
	display_init();

	PIO pio = pio0;
	uint offset = pio_add_program(pio, &ws2812_program);
	ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);


	struct repeating_timer timer;
	add_repeating_timer_ms(10, timer_100hz_callback, NULL, &timer);

	while(true) {
		app_sched_execute();
		__wfi();
	}
}
