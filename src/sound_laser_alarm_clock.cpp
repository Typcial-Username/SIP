/*
 * Name: Levi Terry 
 * UAT
 * Directional Alarm Clock SIP Project
*/

#pragma region Imports
// #include <M5Core2.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "display.h"
#include "sd_browser.h"
#include "secrets.h"
#include "time.h"
#pragma endregion

File ROOT;

void file_menu(String);
String getCurrentTime(); 
String convertTo12HTime(String time24);

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -7 * 3600;
const int daylightOffset_sec = 0;

unsigned long lastTimeUpdate = 0;
static int lastMinute = -1;
struct tm startTimeInfo;

WebServer server(80);

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

  // Serial.print("Initializing LVGL...");
  // initDisplay();
  // Serial.println("done.");

   // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  IPAddress localIP = WiFi.localIP();
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(localIP);
  Serial.println();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  Serial.println("\n\nInitialization Complete!");
  
  M5.Lcd.drawRect(5, ICON_HEIGHT - 10, SCREEN_W - 5, -(SCREEN_H - ICON_HEIGHT) + 10, RED);
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
  // lv_task_handler();
  // lv_timer_handler();
  M5.update();

    // Update time on screen (1/min)
  if (getLocalTime(&startTimeInfo)) {
    if (startTimeInfo.tm_min != lastMinute) {
      lastMinute = startTimeInfo.tm_min;
      String time = convertTo12HTime(getCurrentTime());

      Serial.println("Current time: " + time);

      // Center
      int x = (M5.Lcd.width() - M5.Lcd.textWidth(time)) / 2;
      int y = 5;

      M5.Lcd.setCursor(x - 10, y);

      M5.Lcd.fillRect(x - 10, y, M5.Lcd.textWidth("00:00"), 16, GREEN);

      M5.Lcd.setTextColor(DARKGREY, GREEN);
      M5.Lcd.drawString(time, x - 10, y);
    }
  }
}

String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return String("");
  }

  // Pad hour, min, sec with leading zeros
  String hh = String(timeinfo.tm_hour);   // keep hour as-is for 24H
  String mm = timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min);
  // String ss = timeinfo.tm_sec < 10 ? "0" + String(timeinfo.tm_sec) : String(timeinfo.tm_sec);

  return hh + ":" + mm/* + ":" + ss*/;
}

String convertTo12HTime(String time24) {
  Serial.println("Converting 24H time: " + time24);
  int colonIndex = time24.indexOf(':');
  if (colonIndex == -1) {
    return time24; // Invalid format, return as is
  }

  int hour = time24.substring(0, colonIndex).toInt();
  String minutes = time24.substring(colonIndex + 1);
  String period = "AM";

  if (hour >= 12) {
    period = "PM";

    if (hour > 12) {
      hour -= 12;
    }
  } else if (hour == 0) {
    hour = 12; // Midnight case
  }

  return String(hour) + ":" + minutes + " " + period;
}

void handleRoot() {
  
  File file = SPIFFS.open("/index.html", "r");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  }
  else {
    server.send(404, "text/plain", "File not found");
  }
}