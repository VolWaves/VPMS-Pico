


#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "platform.h"

#include "display.h"
#include "lvgl/lvgl.h"

#define SPI_USE spi1
#define CMD_END (0xFF)

__inline static void __not_in_flash_func(spi_write_soft)(const uint8_t* src, size_t len) {
	for(size_t i = 0; i < len; i++) {
		uint8_t byte = *src;
		for(uint8_t b = 0; b < 8; b++) {
			if(byte & 0x80) {
				gpio_put(OLED_SDA_PIN, 1);
			} else {
				gpio_put(OLED_SDA_PIN, 0);
			}
			gpio_put(OLED_CLK_PIN, 0);
			asm volatile("nop \n nop \n nop");
			gpio_put(OLED_CLK_PIN, 1);
			byte <<= 1;
		}
		src++;
	}
}
#define spi_write spi_write_soft

// __inline void __not_in_flash_func(spi_write)(const uint8_t* src, size_t len) {
// 	spi_write_blocking(SPI_USE, src, len);
// }

void __not_in_flash_func(spi_write_setting)(const uint8_t* src) {
	gpio_put(OLED_DC_PIN, false);
	while(*src != CMD_END) {
		spi_write(src++, 1);
		gpio_put(OLED_DC_PIN, true);
	}
}


void set_draw_windows(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	uint8_t cmd1[] = {ST7789_CASET, 0, x0, 0, x1, CMD_END};
	uint8_t cmd2[] = {ST7789_RASET, 0, y0, 0, y1, CMD_END};
	uint8_t cmd3[] = {ST7789_RAMWR, CMD_END};
	spi_write_setting((const uint8_t*)cmd1);
	spi_write_setting((const uint8_t*)cmd2);
	spi_write_setting((const uint8_t*)cmd3);
}

void fill_color(uint16_t color) {
	set_draw_windows(0, 0, 239, 239);
	gpio_put(OLED_DC_PIN, true);
	uint8_t _c[2] = {color>>8, color&0xff};
	for (uint8_t i = 0; i < 240; i++) {
		for (uint8_t j = 0; j < 240; j++) {
			spi_write((const uint8_t*)_c, 2);
		}
	}
}

void draw_pixel(int16_t x, int16_t y, uint16_t color) {
	set_draw_windows(x, y, x+1, y+1);

	gpio_put(OLED_DC_PIN, true);
	uint8_t c[2];
	c[0] = color >> 8;
	c[1] = color;
	spi_write((const uint8_t*)c, 2);
}

#define spi_write_cmd(arr...) do{\
		const uint8_t t[]={arr ,CMD_END};\
		spi_write_setting(t);\
	}while(0)

void screen_init(void) {
	gpio_put(OLED_RES_PIN, true);
	sleep_ms(25);
	gpio_put(OLED_RES_PIN, false);
	sleep_ms(125);
	gpio_put(OLED_RES_PIN, true);
	sleep_ms(125);

	spi_write_cmd(ST7789_SLPOUT);
	sleep_ms(150);


	spi_write_cmd(0x36, 0x00);
	// spi_write_cmd(ST7789_COLMOD, 0x05);
	spi_write_cmd(ST7789_COLMOD, 0x55);
	spi_write_cmd(0xB2, 0x0C, 0x0C);
	spi_write_cmd(0xB7, 0x35);
	spi_write_cmd(0xBB, 0x1A);
	spi_write_cmd(0xC0, 0x2C);
	spi_write_cmd(0xC2, 0x01);
	spi_write_cmd(0xC3, 0x0B);
	spi_write_cmd(0xC4, 0x20);
	spi_write_cmd(0xC6, 0x0F);
	spi_write_cmd(0xD0, 0xA4, 0xA1);
	spi_write_cmd(ST7789_INVON);

	spi_write_cmd(0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);
	spi_write_cmd(0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23);
	// spi_write_cmd(0xE0, 0x00, 0x19, 0x1E, 0x0A, 0x09, 0x15, 0x3D, 0x44, 0x51, 0x12, 0x03, 0x00, 0x3F, 0x3F);
	// spi_write_cmd(0xE1, 0x00, 0x18, 0x1E, 0x0A, 0x09, 0x25, 0x3F, 0x43, 0x52, 0x33, 0x03, 0x00, 0x3F, 0x3F);
	sleep_ms(100);

	spi_write_cmd(ST7789_NORON);
	sleep_ms(10);
	spi_write_cmd(ST7789_DISPON);
	sleep_ms(255);
	fill_color(RED);
	sleep_ms(300);
	fill_color(WHITE);
	// draw_pixel(10, 10, MAGENTA);
	// draw_pixel(20, 20, BLUE);
	// draw_pixel(40, 40, MAGENTA);
	// draw_pixel(80, 80, BLUE);
}

void display_init(void) {
	spi_init(SPI_USE, 8000000);
	spi_set_format(SPI_USE, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
	// gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_SPI);
	// gpio_set_function(OLED_CLK_PIN, GPIO_FUNC_SPI);
	gpio_init(OLED_SDA_PIN);
	gpio_set_dir(OLED_SDA_PIN, GPIO_OUT);
	gpio_put(OLED_SDA_PIN, false);
	gpio_init(OLED_CLK_PIN);
	gpio_set_dir(OLED_CLK_PIN, GPIO_OUT);
	gpio_put(OLED_CLK_PIN, true);

	gpio_init(OLED_DC_PIN);
	gpio_set_dir(OLED_DC_PIN, GPIO_OUT);
	gpio_init(OLED_RES_PIN);
	gpio_set_dir(OLED_RES_PIN, GPIO_OUT);
	screen_init();
}
