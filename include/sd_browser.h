#ifndef SD_BROWSER_H
#define SD_BROWSER_H

#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>

#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>

#include <SPI.h>

static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;

void initSD();
void printDirectory(File, int);
#endif