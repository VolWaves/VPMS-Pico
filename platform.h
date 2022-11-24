
#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include <stdint.h>
#include <stdbool.h>

#if G_LOG_ENABLED == 1
	#define LOG_RAW(...) printf(__VA_ARGS__)
#endif

#define WS2812_PIN (16)

#define OLED_DC_PIN (12)
#define OLED_RES_PIN (13)
#define OLED_SDA_PIN (14)
#define OLED_CLK_PIN (15)

// REGEXP: #define UEVT_\S+_BASE
#define UEVT_APP_BASE (0xAE00)
#define UEVT_APP_POWERUP (UEVT_APP_BASE | 0x01)
#define UEVT_APP_BOOT (UEVT_APP_BASE | 0x0B)
#define UEVT_APP_SETUP (UEVT_APP_BASE | 0x02)
#define UEVT_APP_START (UEVT_APP_BASE | 0x03)
#define UEVT_APP_BEFORE_SLEEP (UEVT_APP_BASE | 0x0E)
#define UEVT_APP_SLEEP (UEVT_APP_BASE | 0x0C)
#define UEVT_APP_WAKE (UEVT_APP_BASE | 0x0D)

#define UEVT_RTC_BASE (0x0000)
#define UEVT_TIMER_4HZ (UEVT_RTC_BASE | 0x10)
#define UEVT_TIMER_100HZ (UEVT_RTC_BASE | 0x14)

#define UEVT_BTN_BASE (0x0100)
#define UEVT_BTN_LEFT_DOWN (UEVT_BTN_BASE | 0x01)
#define UEVT_BTN_LEFT_RELEASE (UEVT_BTN_BASE | 0x02)
#define UEVT_BTN_LEFT_LONG (UEVT_BTN_BASE | 0x03)
#define UEVT_BTN_LEFT_REPEAT (UEVT_BTN_BASE | 0x04)

#define UEVT_BTN_RIGHT_DOWN (UEVT_BTN_BASE | 0x11)
#define UEVT_BTN_RIGHT_RELEASE (UEVT_BTN_BASE | 0x12)
#define UEVT_BTN_RIGHT_LONG (UEVT_BTN_BASE | 0x13)
#define UEVT_BTN_RIGHT_REPEAT (UEVT_BTN_BASE | 0x14)

#define UEVT_BTN_ENTER_DOWN (UEVT_BTN_BASE | 0x21)
#define UEVT_BTN_ENTER_RELEASE (UEVT_BTN_BASE | 0x22)
#define UEVT_BTN_ENTER_LONG (UEVT_BTN_BASE | 0x23)
#define UEVT_BTN_ENTER_REPEAT (UEVT_BTN_BASE | 0x24)

#define UEVT_ADC_BASE (0x0200)
#define UEVT_ADC_TEMPERATURE_RESULT (UEVT_ADC_BASE | 0x01)

#endif
