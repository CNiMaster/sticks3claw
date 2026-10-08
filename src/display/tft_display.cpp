#include "tft_display.h"
#include "config.h"
#include <driver/ledc.h>
#include <Wire.h>

// M5StickS3 PMIC (M5PM1) I2C 地址和引脚
static const uint8_t PMIC_I2C_ADDR = 0x6e;
static const uint8_t PMIC_SDA = 47;
static const uint8_t PMIC_SCL = 48;

TFTDisplay::TFTDisplay()
    : _tft(nullptr)
    , _width(TFT_WIDTH)
    , _height(TFT_HEIGHT)
    , _brightness(100)
    , _initialized(false)
{
    _tft = new TFT_eSPI();
}

TFTDisplay::~TFTDisplay() {
    delete _tft;
}

void TFTDisplay::initBacklight() {
#ifdef TFT_BL_PIN
    ledc_timer_config_t timerConf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timerConf);

    ledc_channel_config_t channelConf = {
        .gpio_num = TFT_BL_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 255,
        .hpoint = 0
    };
    ledc_channel_config(&channelConf);
    ledc_fade_func_install(0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
#endif
}

// PMIC I2C 辅助函数（使用 Wire）
static uint8_t pmicReadReg(uint8_t reg) {
    Wire.beginTransmission(PMIC_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(PMIC_I2C_ADDR, (uint8_t)1);
    return Wire.read();
}

static void pmicWriteReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(PMIC_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static void pmicSetBit(uint8_t reg, uint8_t bit) {
    uint8_t v = pmicReadReg(reg);
    v |= (1 << bit);
    pmicWriteReg(reg, v);
}

static void pmicClearBit(uint8_t reg, uint8_t bit) {
    uint8_t v = pmicReadReg(reg);
    v &= ~(1 << bit);
    pmicWriteReg(reg, v);
}

bool TFTDisplay::begin() {
    Serial.println("Initializing ST7789 TFT display...");

    // === M5StickS3 PMIC: 通过 I2C 打开 LCD 电源 ===
    Wire.begin(PMIC_SDA, PMIC_SCL);
    delay(10);

    // 扫描 I2C 总线上的设备
    Serial.println("Scanning I2C bus (Wire)...");
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Found device at 0x%02X\n", addr);
        }
    }

    // 读取 PMIC ID 验证通信
    Wire.beginTransmission(PMIC_I2C_ADDR);
    if (Wire.endTransmission(false) == 0) {
        Wire.beginTransmission(PMIC_I2C_ADDR);
        Wire.write(0x00);
        Wire.endTransmission(false);
        Wire.requestFrom(PMIC_I2C_ADDR, (uint8_t)1);
        uint8_t pmicId = Wire.read();
        Serial.printf("PMIC found, ID=0x%02X\n", pmicId);

        // M5PM1 GPIO2 → LCD 电源使能
        // reg 0x16: 设置 GPIO2 为 GPIO 功能（而非其他复用）
        pmicClearBit(0x16, 2);
        // reg 0x10: 设置 GPIO2 方向为输出
        pmicSetBit(0x10, 2);
        // reg 0x13: 设置 GPIO2 为推挽输出
        pmicClearBit(0x13, 2);
        // reg 0x11: 设置 GPIO2 输出高电平 → LCD 电源开
        pmicSetBit(0x11, 2);
        // reg 0x09: 禁用 I2C 空闲休眠模式，确保 PMIC 通信正常
        pmicWriteReg(0x09, 0x00);

        Serial.println("PMIC: LCD power enabled");
        // 验证 reg 0x11 bit2 确实为 1
        uint8_t reg11 = pmicReadReg(0x11);
        Serial.printf("PMIC reg0x11=0x%02X (bit2=%d)\n", reg11, (reg11 >> 2) & 1);
        delay(100);
    } else {
        Serial.println("PMIC not found, trying direct GPIO power...");
        // 如果没有 PMIC，直接拉高可能无效，但尝试一下
        pinMode(TFT_BL_PIN, OUTPUT);
        digitalWrite(TFT_BL_PIN, HIGH);
    }

    // === TFT 初始化 ===
    _tft->begin();
    _initialized = true;
    setRotation(1);  // 标准横屏 MX|MV（rotation 3 的 MY 会翻转Y轴导致眼睛在底部）
    Serial.printf("TFT internal: tft.width=%d tft.height=%d wrapper._w=%d wrapper._h=%d\n",
        _tft->width(), _tft->height(), _width, _height);
    _tft->invertDisplay(true);  // ST7789P3 需要反色
    _tft->fillScreen(TFT_RED);
    delay(100);
    _tft->fillScreen(TFT_WHITE);

    initBacklight();

    _initialized = true;
    Serial.printf("TFT initialized %dx%d\n", _width, _height);

    return true;
}

void TFTDisplay::setRotation(uint8_t r) {
    if (_initialized) {
        _tft->setRotation(r);
        if (r % 2 == 1) {
            // 横屏：宽高交换
            _width = TFT_HEIGHT;
            _height = TFT_WIDTH;
        } else {
            // 竖屏
            _width = TFT_WIDTH;
            _height = TFT_HEIGHT;
        }
    }
}

void TFTDisplay::clear(uint16_t color) {
    if (_initialized) _tft->fillScreen(color);
}

void TFTDisplay::drawText(int x, int y, const char* text, uint16_t color, uint8_t size) {
    if (!_initialized) return;
    _tft->setTextColor(color);
    _tft->setTextSize(size);
    _tft->setCursor(x, y);
    _tft->print(text);
}

void TFTDisplay::drawPixel(int x, int y, uint16_t color) {
    if (_initialized) _tft->drawPixel(x, y, color);
}

void TFTDisplay::fillRect(int x, int y, int width, int height, uint16_t color) {
    if (_initialized) _tft->fillRect(x, y, width, height, color);
}

void TFTDisplay::drawRGB565Image(int x, int y, const uint16_t* data, int width, int height) {
    if (!_initialized || !data) return;
    _tft->pushImage(x, y, width, height, data);
}

void TFTDisplay::setBrightness(uint8_t brightness) {
    _brightness = brightness;
#ifdef TFT_BL_PIN
    uint32_t duty = map(brightness, 0, 100, 0, 255);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
#endif
}

void TFTDisplay::fadeBrightness(uint8_t target, int ms) {
    _brightness = target;
#ifdef TFT_BL_PIN
    uint32_t duty = map(target, 0, 100, 0, 255);
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, ms);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
#endif
}

void TFTDisplay::off(int fadeMs) {
    fadeBrightness(0, fadeMs);
}

void TFTDisplay::on(int fadeMs) {
    fadeBrightness(100, fadeMs);
}
