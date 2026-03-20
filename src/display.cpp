#include "display.h"
#include "sd_browser.h"
#include <SD.h>
#include <esp_timer.h>
#include <M5Unified.h>
#include <lvgl.h>

void initDisplay() {
  lv_init();
  
  display = lv_display_create(SCREEN_W, SCREEN_H);lv_display_set_flush_cb(display, disp_flush);
  
  draw_buf = (uint32_t*)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_SPIRAM);
  
  if (!draw_buf) {
    Serial.println("Draw buffer alloc failed.");
    while (1);
  }

  lv_display_set_buffers(display, draw_buf, nullptr, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchpad_read);

  lv_tick_set_cb(timer_func);

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif
}

void file_menu(String path)
{
  file_list = lv_list_create(lv_screen_active());
  lv_obj_set_size(file_list, SCREEN_W, SCREEN_H);
  lv_obj_set_scrollbar_mode(file_list, LV_SCROLLBAR_MODE_AUTO);

  File dir = SD.open(path);

  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (!entry.isDirectory())
    {
      String name = entry.name();

      lv_obj_t *btn = lv_list_add_btn(file_list, LV_SYMBOL_AUDIO, name.c_str());

      lv_obj_add_event_cb(
        btn,
        file_btn_event,
        LV_EVENT_CLICKED,
        (void*)(name.c_str())
      );
    }

    entry.close();
  }

  dir.close();
  // Clear screen by creating a black background
//   lv_obj_t * screen = lv_screen_active();
//   lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  
//   // Create label for directory
//   lv_obj_t * label = lv_label_create(screen);
//   lv_label_set_text(label, ("Dir: " + path).c_str());
//   lv_obj_set_style_text_color(label, lv_color_white(), 0);
//   lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
//   lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

//   // Create up arrow button
//   lv_obj_t * up_btn = lv_btn_create(screen);
//   lv_obj_set_size(up_btn, 40, 30);
//   lv_obj_align(up_btn, LV_ALIGN_BOTTOM_LEFT, 40, -10);
  
//   // Create down arrow button
//   lv_obj_t * down_btn = lv_btn_create(screen);
//   lv_obj_set_size(down_btn, 40, 30);
//   lv_obj_align(down_btn, LV_ALIGN_BOTTOM_LEFT, 40 + BTN_GAP, -10);

//   enter_icon(40 + BTN_GAP * 2, bottom - 30, 30, 15, 0xFFFFFF);

  // // Create enter button
  // lv_obj_t * enter_btn = lv_btn_create(screen);
  // lv_obj_set_size(enter_btn, 40, 30);
  // lv_obj_align(enter_btn, LV_ALIGN_BOTTOM_LEFT, 150 + BTN_GAP, -10);
}

lv_obj_t *file_list;

static void file_btn_event(lv_event_t *e)
{
  lv_obj_t *btn = lv_event_get_target(e);
  const char *filename = (const char*)lv_event_get_user_data(e);

  Serial.print("Selected file: ");
  Serial.println(filename);

  // later this will trigger audio playback
}


uint32_t timer_func() {
  return (esp_timer_get_time() / 1000LL);
}

void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t width = lv_area_get_width(area);
  uint32_t height = lv_area_get_height(area);

  // Swap color order to match M5Core2's RGB565 expectations
  lv_draw_sw_rgb565_swap(px_map, width * height);

  M5.Lcd.pushImage(area->x1, area->y1, width, height, (uint16_t *)px_map);
  lv_display_flush_ready(disp);
}
 
/* Read M5Core2 touch input */
void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  touch_detail_t tp = M5.Touch.getDetail();

  if (tp.x >= 0 && tp.y >= 0) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = tp.x;
    data->point.y = tp.y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
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
