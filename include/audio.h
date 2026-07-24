#ifndef AUDIO_H
#define AUDIO_H

// #include <ESP32I2SAudio.h>
// #include "BackgroundAudioWAV.h"
#include <driver/dac_cosine.h>
#include "driver/dac_continuous.h"
#include "SD.h"

void cosine();
// ESP32I2SAudio audio(4,5,6);  
// BackgroundAudioMp3Class<RawDataBuffer<16 * 1024>> BMP(audio);

struct UAFHeader {
  char magic[4];
  uint32_t sampleRate;
  uint32_t carrierHz;
  uint32_t sampleCount;
};

static const size_t CHUNK_SAMPLES = 2048; // tune if you get underruns

bool playUAF(const char* path);
void startAlarmPlayback(const char* path);
#endif
