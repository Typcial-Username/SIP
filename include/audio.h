#ifndef AUDIO_H
#define AUDIO_H

#include <ESP32I2SAudio.h>
#include "BackgroundAudioWAV.h"
#include "driver/dac_cosine.h"

void cosine();
// ESP32I2SAudio audio(4,5,6);
// BackgroundAudioMp3Class<RawDataBuffer<16 * 1024>> BMP(audio);
#endif