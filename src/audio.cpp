#include "audio.h"

void cosine() {
    dac_cosine_handle_t chan_handle;
    dac_cosine_config_t cos_cfg = {
        .chan_id = DAC_CHAN_0, // GPIO25 = DAC_CHAN_0, GPIO26 = DAC_CHAN_1
        .freq_hz = 40000,
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten = DAC_COSINE_ATTEN_DEFAULT,
        .phase = DAC_COSINE_PHASE_0,
        .offset = 0,
        .flags={.force_set_freq = false},
    };

    dac_cosine_new_channel(&cos_cfg, &chan_handle);
    dac_cosine_start(chan_handle);
}

bool playUAF(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println("Failed to open UAF file");
    return false;
  }

  UAFHeader hdr;
  if (f.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr) ||
      memcmp(hdr.magic, "UAF1", 4) != 0) {
    Serial.println("Bad or missing UAF header");
    f.close();
    return false;
  }

  Serial.printf("UAF: sampleRate=%u carrierHz=%u sampleCount=%u\n",
                hdr.sampleRate, hdr.carrierHz, hdr.sampleCount);


  dac_continuous_config_t dac_cfg = {
    .chan_mask = DAC_CHANNEL_MASK_CH0,      // GPIO25
    .desc_num  = 16,                          // >5 recommended per Espressif docs
    .buf_size  = 1024,                       // max 4092 per descriptor
    .freq_hz   = hdr.sampleRate,             // from your UAF header
    .offset    = 0,
    .clk_src   = DAC_DIGI_CLK_SRC_DEFAULT,
    .chan_mode = DAC_CHANNEL_MODE_SIMUL,     // only matters with 2 channels, harmless here
  };

  dac_continuous_handle_t dac_handle;
  esp_err_t err = dac_continuous_new_channels(&dac_cfg, &dac_handle);
  if (err != ESP_OK) {
    Serial.printf("dac_continuous_new_channels failed: %d\n", err);
    f.close();
    return false;
  }

  dac_continuous_enable(dac_handle);

  static int16_t rawBuf[CHUNK_SAMPLES];
  static uint8_t dacBuf[CHUNK_SAMPLES];
  uint32_t samplesRemaining = hdr.sampleCount;

  uint32_t lastReadStart = millis();
  while (samplesRemaining > 0) {
    size_t samplesToRead = min((size_t)CHUNK_SAMPLES, (size_t)samplesRemaining);

    uint32_t t0 = millis();
    size_t bytesToRead = samplesToRead * sizeof(int16_t);
    size_t bytesRead = f.read((uint8_t*)rawBuf, samplesToRead * sizeof(int16_t));
    
    uint32_t readTime = millis() - t0;
    if (readTime > 2) {  // 2.67ms budget at 384kHz, flag anything close/over
      Serial.printf("SLOW READ: %lu ms at sample offset %lu\n", readTime, hdr.sampleCount - samplesRemaining);
    }

    if (bytesRead != bytesToRead) {
      Serial.printf("Short read: wanted %u bytes, got %u. Aborting playback.\n", bytesToRead, bytesRead);
      break; // don't continue with a misaligned stream
    }

    // size_t bytesRead = f.read((uint8_t*)rawBuf, samplesToRead * sizeof(int16_t));
    size_t samplesRead = bytesRead / sizeof(int16_t);
    if (samplesRead == 0) break; // ran out of file early, bail cleanly

    // int16 signed [-32768, 32767] -> uint8 unsigned [0, 255], centered at 128
    for (size_t i = 0; i < samplesRead; i++) {
      dacBuf[i] = (uint8_t)(((int32_t)rawBuf[i] + 32768) >> 8);
    }

    size_t bytesLoaded = 0;
    err = dac_continuous_write(dac_handle, dacBuf, samplesRead, &bytesLoaded, -1);
    if (err != ESP_OK) {
      Serial.printf("dac_continuous_write error: %d\n", err);
      break;
    }

    samplesRemaining -= samplesRead;
  }

  f.close();
  dac_continuous_disable(dac_handle);
  dac_continuous_del_channels(dac_handle);
  return true;
}

TaskHandle_t audioTaskHandle = nullptr;
volatile bool audioTaskRunning = false;

void audioTask(void* param) {
  char* path = (char*)param;
  audioTaskRunning = true;

  playUAF(path);

  audioTaskRunning = false;
  free(path);         // clean up the strdup'd copy
  vTaskDelete(nullptr); // task deletes itself when done
}

void startAlarmPlayback(const char* path) {
  if (audioTaskRunning) {
    Serial.println("Playback already running, ignoring");
    return;
  }

  char* pathCopy = strdup(path); // task may outlive the caller's stack frame

  xTaskCreatePinnedToCore(
    audioTask,
    "AudioTask",
    16384,        // stack size in words, bump if you see stack overflow crashes
    pathCopy,
    1,           // priority
    &audioTaskHandle,
    0            // core 0, leaves core 1 free for your loop()/display
  );
}