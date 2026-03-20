#ifndef DISPLAY_H
#include <lvgl.h>

#include "draw/sw/lv_draw_sw.h"

#define DISPLAY_H

#define SCREEN_W     320
#define SCREEN_H     240

#define iconHeight   SCREEN_H - 25
#define bottom       SCREEN_H - 10
#define BTN_GAP      110

uint32_t DRAW_BUF_SIZE = SCREEN_W * 40 * (LV_COLOR_DEPTH / 8);
uint32_t *draw_buf;

lv_display_t *display;
lv_indev_t *indev;

void initDisplay();
void disp_flush(lv_display_t*, const lv_area_t*, uint8_t*);
void touchpad_read(lv_indev_t*, lv_indev_data_t*);
void enter_icon(int, int, int, int, int);
uint32_t timer_func();
static void file_btn_event(lv_event_t *e);
#endif