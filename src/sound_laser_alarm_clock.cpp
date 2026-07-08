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

// void file_menu(String);
String getCurrentTime(); 
String convertTo12HTime(String time24);
void handleRoot();

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -7 * 3600;
const int daylightOffset_sec = 0;

unsigned long lastTimeUpdate = 0;
static int lastMinute = -1;
struct tm timeInfo;

WebServer server(80);

static constexpr const size_t buf_num = 3;
static constexpr const size_t buf_size = 1024;
static uint8_t wav_data[buf_num][buf_size];

struct __attribute__((packed)) wav_header_t
{
  char RIFF[4];
  uint32_t chunk_size;
  char WAVEfmt[8];
  uint32_t fmt_chunk_size;
  uint16_t audiofmt;
  uint16_t channel;
  uint32_t sample_rate;
  uint32_t byte_per_sec;
  uint16_t block_size;
  uint16_t bit_per_sample;
};

struct __attribute__((packed)) sub_chunk_t
{
  char identifier[4];
  uint32_t chunk_size;
  uint8_t data[1];
};

static bool playSdWav(const char* filename)
{
  auto file = SD.open(filename);

  if (!file) { return false; }

  wav_header_t wav_header;
  file.read((uint8_t*)&wav_header, sizeof(wav_header_t));

  ESP_LOGD("wav", "RIFF           : %.4s" , wav_header.RIFF          );
  ESP_LOGD("wav", "chunk_size     : %d"   , wav_header.chunk_size    );
  ESP_LOGD("wav", "WAVEfmt        : %.8s" , wav_header.WAVEfmt       );
  ESP_LOGD("wav", "fmt_chunk_size : %d"   , wav_header.fmt_chunk_size);
  ESP_LOGD("wav", "audiofmt       : %d"   , wav_header.audiofmt      );
  ESP_LOGD("wav", "channel        : %d"   , wav_header.channel       );
  ESP_LOGD("wav", "sample_rate    : %d"   , wav_header.sample_rate   );
  ESP_LOGD("wav", "byte_per_sec   : %d"   , wav_header.byte_per_sec  );
  ESP_LOGD("wav", "block_size     : %d"   , wav_header.block_size    );
  ESP_LOGD("wav", "bit_per_sample : %d"   , wav_header.bit_per_sample);

  if ( memcmp(wav_header.RIFF,    "RIFF",     4)
    || memcmp(wav_header.WAVEfmt, "WAVEfmt ", 8)
    || wav_header.audiofmt != 1
    || wav_header.bit_per_sample < 8
    || wav_header.bit_per_sample > 16
    || wav_header.channel == 0
    || wav_header.channel > 2
    )
  {
    file.close();
    return false;
  }

  file.seek(offsetof(wav_header_t, audiofmt) + wav_header.fmt_chunk_size);
  sub_chunk_t sub_chunk;

  file.read((uint8_t*)&sub_chunk, 8);

  ESP_LOGD("wav", "sub id         : %.4s" , sub_chunk.identifier);
  ESP_LOGD("wav", "sub chunk_size : %d"   , sub_chunk.chunk_size);

  while(memcmp(sub_chunk.identifier, "data", 4))
  {
    if (!file.seek(sub_chunk.chunk_size, SeekMode::SeekCur)) { break; }
    file.read((uint8_t*)&sub_chunk, 8);

    ESP_LOGD("wav", "sub id         : %.4s" , sub_chunk.identifier);
    ESP_LOGD("wav", "sub chunk_size : %d"   , sub_chunk.chunk_size);
  }

  if (memcmp(sub_chunk.identifier, "data", 4))
  {
    file.close();
    return false;
  }

  int32_t data_len = sub_chunk.chunk_size;
  bool flg_16bit = (wav_header.bit_per_sample >> 4);

  size_t idx = 0;
  while (data_len > 0) {
    size_t len = data_len < buf_size ? data_len : buf_size;
    len = file.read(wav_data[idx], len);
    data_len -= len;

    if (flg_16bit) {
      M5.Speaker.playRaw((const int16_t*)wav_data[idx], len >> 1, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    } else {
      M5.Speaker.playRaw((const uint8_t*)wav_data[idx], len, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    }
    idx = idx < (buf_num - 1) ? idx + 1 : 0;
  }
  file.close();

  return true;
}

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
  Serial.print("Connecting to WiFi...");
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

  server.on("/", HTTP_GET, handleRoot);
  server.begin();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  Serial.println("\n\nInitialization Complete!");
  
  M5.Lcd.drawRect(5, ICON_HEIGHT - 10, SCREEN_W - 5, -(SCREEN_H - ICON_HEIGHT) + 10, RED);
  // file_menu("/");

  File blue = SD.open("/Mr.Blue_Sky.wav");
  size_t size = blue.size();
  uint8_t* buf = (uint8_t*)malloc(size);
  blue.read(buf, size);

  

  ROOT = SD.open("/");

  printDirectory(ROOT, 0);
}

void loop() {
  // lv_task_handler();
  // lv_timer_handler();
  M5.update();
  server.handleClient();

    // Update time on screen (1/min)
  if (getLocalTime(&timeInfo)) {
    if (timeInfo.tm_sec != lastMinute) {
      // M5.Lcd.clearDisplay();
      M5.Lcd.fillRect(0, 0, M5.Lcd.width(), ICON_HEIGHT - 10, BLACK);

      lastMinute = timeInfo.tm_sec;
      String time = convertTo12HTime(getCurrentTime());

      // Center
      int x = (M5.Lcd.width() / 2) - (M5.Lcd.textWidth(time) / 2);
      int y = (M5.Lcd.height() / 2) - 10;

      // M5.Lcd.setCursor(x - 10, y);

      // M5.Lcd.fillRect(x - 10, y, M5.Lcd.textWidth("00:00"), 16, GREEN);

      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.setTextSize(4);
      M5.Lcd.drawString(time, x, y);

      Serial.println(time);
      if (time == "2:32:00 PM") {
      M5.Speaker.setVolume(100);
        playSdWav("/Mr.Blue_Sky.wav");
        String vol = String(M5.Speaker.getVolume());
        Serial.println("Vol: " + vol + " isPlaying: " + M5.Speaker.isPlaying());
      }
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
  String ss = timeinfo.tm_sec < 10 ? "0" + String(timeinfo.tm_sec) : String(timeinfo.tm_sec);

  return hh + ":" + mm + ":" + ss;
}

String convertTo12HTime(String time24) {
  int colonIndex = time24.indexOf(':');
  if (colonIndex == -1) {
    return time24; // Invalid format, return as is
  }

  int hour = time24.substring(0, colonIndex).toInt();
  String minutes = time24.substring(colonIndex + 1);
  // int secColonIdx = minutes.indexOf(":");
  // String seconds = minutes.substring(secColonIdx);
  String period = "AM";

  // Serial.println("Mins: " + minutes + "Secs: \"" + seconds + "\"");

  if (hour >= 12) {
    period = "PM";

    if (hour > 12) {
      hour -= 12;
    }
  } else if (hour == 0) {
    hour = 12; // Midnight case
  }

  return String(hour) + ":" + minutes + " " + /*seconds + " " +*/ period;
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