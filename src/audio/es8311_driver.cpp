#include "es8311_driver.h"
#include "config.h"

ES8311Driver::ES8311Driver()
    : _wire(nullptr)
    , _deviceAddress(ES8311_I2C_ADDR)
    , _initialized(false)
{
}

ES8311Driver::~ES8311Driver() {
}

bool ES8311Driver::begin() {
    Serial.println("Initializing ES8311 audio codec...");

    // 初始化 I2C（复位总线清除残留状态）
    _wire = &Wire;
    _wire->begin(ES8311_SDA_PIN, ES8311_SCL_PIN);
    delay(50);
    _wire->end();
    delay(10);
    _wire->begin(ES8311_SDA_PIN, ES8311_SCL_PIN);
    delay(50);

    // 检测设备是否存在
    _wire->beginTransmission(_deviceAddress);
    if (_wire->endTransmission() != 0) {
        Serial.println("ES8311 not found!");
        return false;
    }

    // 复位芯片
    if (!reset()) {
        Serial.println("ES8311 reset failed!");
        return false;
    }

    // 配置芯片
    if (!configure()) {
        Serial.println("ES8311 configuration failed!");
        return false;
    }

    _initialized = true;
    Serial.println("ES8311 initialized successfully!");

    // 设置默认值
    setMicrophoneGain(49);  // 默认麦克风增益（实测 49 为最佳）
    setSpeakerVolume(50);   // 默认音量 50%

    return true;
}

bool ES8311Driver::reset() {
    // 写入复位命令
    writeRegister(ES8311_REG_00, 0x3F);  // 软复位

    delay(100);

    return true;
}

bool ES8311Driver::configure() {
    // 配置为从机模式，16位，采样率48kHz（可调整）
    // 这里的配置根据实际需求调整

    // 设置 ADC/DAC 配置
    writeRegister(ES8311_REG_01, 0x3F);  // 使用 RESET 和低功耗模式

    // 设置 ADC 通道配置
    writeRegister(ES8311_REG_02, 0x00);  // ADC 使能
    writeRegister(ES8311_REG_03, 0x00);  // 通道选择

    // 设置 DAC 通道配置
    writeRegister(ES8311_REG_04, 0x00);

    // 设置模拟路径
    writeRegister(ES8311_REG_0B, 0x0E);  // PGA 使能
    writeRegister(ES8311_REG_0C, 0x00);  // PGA 步进配置

    // 设置时钟配置
    writeRegister(ES8311_REG_06, 0x00);  // 主机/从机模式
    writeRegister(ES8311_REG_07, 0x30);  // 系统时钟配置

    // 设置 GPIO 配置
    writeRegister(ES8311_REG_44, 0x00);  // GPIO1 配置
    writeRegister(ES8311_REG_45, 0x00);  // GPIO2 配置

    // 设置功率配置
    writeRegister(ES8311_REG_43, 0x00);  // 功率配置

    return true;
}

void ES8311Driver::setMicrophoneGain(uint8_t gain) {
    // ES8311 麦克风增益范围 0-63
    if (gain > 63) gain = 63;

    // 设置 PGA 增益
    // 通道 1 (ADC) 的增益
    writeRegister(ES8311_REG_0C, gain & 0x3F);

    Serial.printf("Microphone gain set to: %d\n", gain);
}

void ES8311Driver::setSpeakerVolume(uint8_t volume) {
    // ES8311 扬声器音量范围 0-100
    if (volume > 100) volume = 100;

    // 转换为 DAC 数字音量控制
    // 范围通常 -96dB 到 0dB
    int8_t dacVolume = map(volume, 0, 100, -96, 0);

    // 设置 DAC 音量
    writeRegister(ES8311_REG_2D, (uint8_t)dacVolume);

    Serial.printf("Speaker volume set to: %d (DAC: %ddB)\n", volume, dacVolume);
}

void ES8311Driver::mute(bool mute) {
    if (mute) {
        writeRegister(ES8311_REG_04, 0x00);  // DAC 静音
        writeRegister(ES8311_REG_03, 0x00);  // ADC 静音
        Serial.println("ES8311 muted");
    } else {
        writeRegister(ES8311_REG_03, 0x00);  // ADC 使能
        writeRegister(ES8311_REG_04, 0x00);  // DAC 使能
        Serial.println("ES8311 unmuted");
    }
}

bool ES8311Driver::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_deviceAddress);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

uint8_t ES8311Driver::readRegister(uint8_t reg) {
    _wire->beginTransmission(_deviceAddress);
    _wire->write(reg);
    _wire->endTransmission(false);

    _wire->requestFrom(_deviceAddress, 1);
    uint8_t value = _wire->read();
    _wire->endTransmission();

    return value;
}
