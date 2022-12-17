
#include "platform.h"
#include "lv_conf.h"
#include "lvgl/lvgl.h"
#include "display.h"

#define BUF_DEPTH (DISP_W * DISP_H / 8)

void my_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p) {
	uint32_t len = (area->y2-area->y1 + 1) * (area->x2-area->x1 + 1);
	if(len > DISP_W * DISP_H) {
		len = DISP_W * DISP_H;
	}
	#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 1
	lv_color_t *cp = color_p;
	// uint16_t color_h, color_l;
    for(uint32_t i = 0; i < len; i += 1) {
		cp->full = (cp->full << 8) | (cp->full >> 8);
		cp++;
    }
	#endif
	set_draw_windows(area->x1, area->y1, area->x2, area->y2);
	draw_bitmap(color_p, (area->y2-area->y1 + 1) * (area->x2-area->x1 + 1));
    lv_disp_flush_ready(disp);
}

static lv_obj_t * scr1;

void ui_loading(void) {
	lv_obj_t * spinner = lv_spinner_create(scr1, 1000, 60);
	lv_obj_set_size(spinner, 100, 100);
	lv_obj_center(spinner);
}


static lv_obj_t * label_volt;
static lv_obj_t * label_current;
static lv_obj_t * label_power;
static lv_obj_t * label_resistance;
void ui_data_create(void) {
	lv_obj_t * cont_col = lv_obj_create(lv_scr_act());
	lv_obj_set_size(cont_col, 220, 200);
	lv_obj_set_flex_align(cont_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);

	label_volt = lv_label_create(cont_col);
	label_current = lv_label_create(cont_col);
	label_power = lv_label_create(cont_col);
	label_resistance = lv_label_create(cont_col);
	lv_label_set_text(label_volt, "U= ---- mV");
	lv_label_set_text(label_current, "I= ---- mA");
	lv_label_set_text(label_power, "P= ---- mW");
	lv_label_set_text(label_resistance, "R= ---- mOhm");
}

void ui_data_update(uint32_t* a) {
	lv_label_set_text_fmt(label_volt, "U= %"LV_PRIu32 "mV", a[0]);
	lv_label_set_text_fmt(label_current, "I= %"LV_PRIu32 "mA", a[1]);
	lv_label_set_text_fmt(label_power, "P= %"LV_PRIu32 "mW", a[2]);
	if(a[1] > 600) {
		lv_label_set_text_fmt(label_resistance, "R= %"LV_PRIu32 "mR", a[3]);
	} else {
		lv_label_set_text(label_resistance, "R= ----");
	}
}

void ui_spinner_create(void) {
    lv_obj_t * spinner = lv_spinner_create(lv_scr_act(), 1000, 60);
    lv_obj_set_size(spinner, 20, 20);
}

void ui_init(void) {
	static lv_disp_draw_buf_t draw_buf;
	static lv_color_t buf1[BUF_DEPTH];
	static lv_disp_drv_t disp_drv;        /*Descriptor of a display driver*/
	lv_init();
	lv_disp_draw_buf_init(&draw_buf, buf1, NULL, BUF_DEPTH);
	lv_disp_drv_init(&disp_drv);          /*Basic initialization*/
	disp_drv.flush_cb = my_disp_flush;    /*Set your driver function*/
	disp_drv.draw_buf = &draw_buf;        /*Assign the buffer to the display*/
	disp_drv.hor_res = DISP_W;   /*Set the horizontal resolution of the display*/
	disp_drv.ver_res = DISP_H;   /*Set the vertical resolution of the display*/
	lv_disp_drv_register(&disp_drv);      /*Finally register the driver*/

	scr1 = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(scr1, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_text_color(scr1, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_scr_load(scr1);
	
	ui_data_create();
	ui_spinner_create();
}

void ui_period_hanlder(uint32_t ms) {
	lv_tick_inc(ms);
	lv_timer_handler();
}

