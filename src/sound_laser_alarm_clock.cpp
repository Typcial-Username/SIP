/*
 * Name: Levi Terry 
 * UAT
 * Directional Alarm Clock SIP Project
*/

#pragma region Imports
#include <M5Core2.h>
// #include <M5Unified.h>

#include <lvgl.h>

#include "draw/sw/lv_draw_sw.h"

#include <esp_timer.h>

#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>

#include <SPI.h>

#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>

#include <SD.h>
#pragma endregion

#pragma region Globals
#define SCREEN_W     320
#define SCREEN_H     240

#define BTN_GAP      110

#define iconHeight   SCREEN_H - 25
#define bottom       SCREEN_H - 10

uint32_t DRAW_BUF_SIZE = SCREEN_W * 40 * (LV_COLOR_DEPTH / 8);
uint32_t *draw_buf;

lv_display_t *display;
lv_indev_t *indev;

static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;
#pragma endregion

File root;

void printDirectory(File, int);
uint32_t my_timer_function();
void my_disp_flush(lv_display_t*, const lv_area_t*, uint8_t*);
void my_touchpad_read(lv_indev_t*, lv_indev_data_t*);
void file_menu(String);

void setup() {
  Serial.begin(115200);
   
  while (!Serial);
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_CSPIN, SPI, 25000000)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
  }
  Serial.println("done.");

  Serial.print("Initializing M5...");
  M5.begin();
  Serial.println("done.");
  Serial.print("Initializing LVGL...");
  lv_init();
  Serial.println("done.");

  Serial.println("Initialization Complete!");

  lv_tick_set_cb(my_timer_function);
 
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif
 
  display = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_flush_cb(display, my_disp_flush);

  draw_buf = (uint32_t*)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_SPIRAM);
  if (!draw_buf) {
      Serial.println("Draw buffer alloc failed.");
      while (1);
  }
  lv_display_set_buffers(display, draw_buf, nullptr, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  M5.Lcd.drawRect(5, iconHeight - 10, SCREEN_W - 5, -(SCREEN_H - iconHeight) + 10, RED);
  file_menu("/");

  root = SD.open("/");

  printDirectory(root, 0);
}

void printDirectory(File dir, int numTabs = 0) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void enter_icon(int x, int y, int w, int h, int color)
{
  static lv_point_precise_t line_points[] = { { (x + w), y }, { (x + w), (y + h)}, { x, (y + h)} };
  static lv_style_t line_style;
  lv_style_init(&line_style);
  lv_color_t Color = lv_color_make((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
  lv_style_set_line_color(&line_style, Color);
  lv_style_set_line_width(&line_style, 2);

  lv_obj_t * line1;
  line1 = lv_line_create(lv_screen_active());
  lv_line_set_points(line1, line_points, 3);
  lv_obj_add_style(line1, &line_style, 0);
  lv_obj_center(line1);

  // Triangle
  static lv_point_precise_t tri_points[] = { 
    {x + 5, (y + h) - 5}, 
    {x + 5, (y + h) + 5}, 
    {x, y + h},
    {x + 5, (y + h) - 5}  // Close the triangle by returning to start
  };
  
  lv_obj_t * triangle = lv_line_create(lv_screen_active());
  lv_line_set_points(triangle, tri_points, 4);
  lv_obj_add_style(triangle, &line_style, 0);
  lv_obj_center(triangle);
}

void file_menu(String dir)
{
  // Clear screen by creating a black background
  lv_obj_t * screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  
  // Create label for directory
  lv_obj_t * label = lv_label_create(screen);
  lv_label_set_text(label, ("Dir: " + dir).c_str());
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

  // Create up arrow button
  lv_obj_t * up_btn = lv_btn_create(screen);
  lv_obj_set_size(up_btn, 40, 30);
  lv_obj_align(up_btn, LV_ALIGN_BOTTOM_LEFT, 40, -10);
  
  // Create down arrow button
  lv_obj_t * down_btn = lv_btn_create(screen);
  lv_obj_set_size(down_btn, 40, 30);
  lv_obj_align(down_btn, LV_ALIGN_BOTTOM_LEFT, 40 + BTN_GAP, -10);

  enter_icon(40 + BTN_GAP * 2, bottom - 30, 30, 15, 0xFFFFFF);

  // // Create enter button
  // lv_obj_t * enter_btn = lv_btn_create(screen);
  // lv_obj_set_size(enter_btn, 40, 30);
  // lv_obj_align(enter_btn, LV_ALIGN_BOTTOM_LEFT, 150 + BTN_GAP, -10);
}

uint32_t my_timer_function() {
  return (esp_timer_get_time() / 1000LL);
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t width = lv_area_get_width(area);
    uint32_t height = lv_area_get_height(area);
 
    // Swap color order to match M5Core2's RGB565 expectations
    lv_draw_sw_rgb565_swap(px_map, width * height);
 
    M5.Lcd.pushImage(area->x1, area->y1, width, height, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}
 
/* Read M5Core2 touch input */
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    TouchPoint_t tp = M5.Touch.getPressPoint();
    if (tp.x >= 0 && tp.y >= 0) {
      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = tp.x;
      data->point.y = tp.y;
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
    }
}

void loop() {
  lv_task_handler();
  lv_timer_handler();
  M5.update();
}
