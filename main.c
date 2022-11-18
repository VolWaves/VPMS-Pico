
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/sync.h"

#include "scheduler/uevent.h"
#include "scheduler/scheduler.h"

#include "platform.h"

#define LED_PIN PICO_DEFAULT_LED_PIN

volatile uint32_t int_cache;
static __inline void CRITICAL_REGION_ENTER(void) {
	int_cache = save_and_disable_interrupts();
}
static __inline void CRITICAL_REGION_EXIT(void) {
	restore_interrupts(int_cache);
}

bool timer_4hz_callback(struct repeating_timer* t) {
	printf("Repeat at %lld\n", time_us_64());
	uevt_bc_e(UEVT_TIMER_4HZ);
	return true;
}

void led_blink_routine(void) {
	static uint8_t _tick = 0;
	_tick += 1;
	if(_tick & 0x1) {
		gpio_put(LED_PIN, 1);
	} else {
		gpio_put(LED_PIN, 0);
	}
}

void temperature_routine(void) {
	static uint16_t tick = 0;
	static float _t = 0;
	if(tick > 8) {
		tick = 0;
		int32_t t_adc = adc_read();
		_t = 27.0 - ((t_adc - 876) / 2.136f);
		uevt_bc(UEVT_ADC_TEMPERATURE_RESULT, &_t);
	}
	tick += 1;
}

void main_handler(uevt_t* evt) {
	switch(evt->evt_id) {
		case UEVT_TIMER_4HZ:
			led_blink_routine();
			temperature_routine();
			break;
		case UEVT_ADC_TEMPERATURE_RESULT:
			printf("Temperature is %0.2f\n", *((float*)(evt->content)));
			break;
	}
}

int main() {
	app_sched_init();
	user_event_init();
	user_event_handler_regist(main_handler);

	stdio_init_all();

	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	struct repeating_timer timer;
	add_repeating_timer_ms(250, timer_4hz_callback, NULL, &timer);

	while(true) {
		app_sched_execute();
		__wfi();
	}
}
