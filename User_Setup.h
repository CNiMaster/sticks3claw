// TFT_eSPI User Setup for Sticks3 (ST7789P3, 135x240)
#define USER_SETUP_INFO "Sticks3"

#define ST7789_DRIVER

#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define TFT_MOSI 39
#define TFT_SCLK 40
#define TFT_CS   41
#define TFT_DC   45
#define TFT_RST  21
#define TFT_BL   38

#define TFT_RGB_ORDER TFT_RGB

#define SPI_FREQUENCY  27000000

#define CGRAM_OFFSET

#define LOAD_GLCD
#define LOAD_FONT2

// Fix ESP-IDF 5.x REG_SPI_BASE conflict with TFT_eSPI on ESP32-S3
// M5StickS3 uses SPI3_HOST (HSPI=2) for TFT, not SPI2_HOST
#ifndef CONFIG_IDF_TARGET_ESP32S3
#define CONFIG_IDF_TARGET_ESP32S3
#endif
#include "soc/soc.h"
#undef REG_SPI_BASE
#define REG_SPI_BASE(i) (((i)>1) ? (DR_REG_SPI3_BASE) : (DR_REG_SPI2_BASE))
#define SPI_PORT 2  // HSPI=2 on ESP32-S3, maps to SPI3_HOST (same as M5GFX)
