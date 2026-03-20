/*
 * Name: Levi Terry 
 * UAT
 * Directional Alarm Clock SIP Project
*/

#pragma region Imports
// #include <M5Core2.h>
#include <M5Unified.h>
#include "display.h"
#include "sd_browser.h"
#pragma endregion

File ROOT;

void file_menu(String);

void setup() {
  Serial.begin(115200);
   
  while (!Serial);
  
  Serial.print("Initializing M5...");
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Speaker.begin();
  // M5.Touch.begin();
  Serial.println("done.");
  
  Serial.print("Initializing SD card...");
  initSD();
  Serial.println("done.");

  Serial.print("Initializing LVGL...");
  initDisplay();
  Serial.println("done.");

  Serial.println("\n\nInitialization Complete!");
  
  M5.Lcd.drawRect(5, iconHeight - 10, SCREEN_W - 5, -(SCREEN_H - iconHeight) + 10, RED);
  file_menu("/");

  File blue = SD.open("/Mr.Blue_Sky.mp3");
  size_t size = blue.size();
  uint8_t* buf = (uint8_t*)malloc(size);
  blue.read(buf, size);

  M5.Speaker.playWav(buf, size);

  ROOT = SD.open("/");

  printDirectory(ROOT, 0);
}

void loop() {
  lv_task_handler();
  lv_timer_handler();
  M5.update();
}
