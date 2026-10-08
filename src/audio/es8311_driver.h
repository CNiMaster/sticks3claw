#ifndef ES8311_DRIVER_H
#define ES8311_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

class ES8311Driver {
public:
    ES8311Driver();
    ~ES8311Driver();

    // 初始化ES8311
    bool begin();

    // 设置麦克风增益 (0-63)
    void setMicrophoneGain(uint8_t gain);

    // 设置扬声器音量 (0-100)
    void setSpeakerVolume(uint8_t volume);

    // 静音/取消静音
    void mute(bool mute);

    // 写寄存器
    bool writeRegister(uint8_t reg, uint8_t value);

    // 读寄存器
    uint8_t readRegister(uint8_t reg);

    // 检查是否初始化
    bool isInitialized() const { return _initialized; }

private:
    TwoWire* _wire;
    uint8_t _deviceAddress;
    bool _initialized;

    // ES8311 寄存器地址
    static constexpr uint8_t ES8311_REG_00 = 0x00;
    static constexpr uint8_t ES8311_REG_01 = 0x01;
    static constexpr uint8_t ES8311_REG_02 = 0x02;
    static constexpr uint8_t ES8311_REG_03 = 0x03;
    static constexpr uint8_t ES8311_REG_04 = 0x04;
    static constexpr uint8_t ES8311_REG_05 = 0x05;
    static constexpr uint8_t ES8311_REG_06 = 0x06;
    static constexpr uint8_t ES8311_REG_07 = 0x07;
    static constexpr uint8_t ES8311_REG_08 = 0x08;
    static constexpr uint8_t ES8311_REG_09 = 0x09;
    static constexpr uint8_t ES8311_REG_0A = 0x0A;
    static constexpr uint8_t ES8311_REG_0B = 0x0B;
    static constexpr uint8_t ES8311_REG_0C = 0x0C;
    static constexpr uint8_t ES8311_REG_0D = 0x0D;
    static constexpr uint8_t ES8311_REG_0E = 0x0E;
    static constexpr uint8_t ES8311_REG_0F = 0x0F;
    static constexpr uint8_t ES8311_REG_10 = 0x10;
    static constexpr uint8_t ES8311_REG_11 = 0x11;
    static constexpr uint8_t ES8311_REG_12 = 0x12;
    static constexpr uint8_t ES8311_REG_13 = 0x13;
    static constexpr uint8_t ES8311_REG_14 = 0x14;
    static constexpr uint8_t ES8311_REG_15 = 0x15;
    static constexpr uint8_t ES8311_REG_16 = 0x16;
    static constexpr uint8_t ES8311_REG_17 = 0x17;
    static constexpr uint8_t ES8311_REG_18 = 0x18;
    static constexpr uint8_t ES8311_REG_19 = 0x19;
    static constexpr uint8_t ES8311_REG_1A = 0x1A;
    static constexpr uint8_t ES8311_REG_1B = 0x1B;
    static constexpr uint8_t ES8311_REG_1C = 0x1C;
    static constexpr uint8_t ES8311_REG_1D = 0x1D;
    static constexpr uint8_t ES8311_REG_20 = 0x20;
    static constexpr uint8_t ES8311_REG_21 = 0x21;
    static constexpr uint8_t ES8311_REG_22 = 0x22;
    static constexpr uint8_t ES8311_REG_23 = 0x23;
    static constexpr uint8_t ES8311_REG_24 = 0x24;
    static constexpr uint8_t ES8311_REG_25 = 0x25;
    static constexpr uint8_t ES8311_REG_26 = 0x26;
    static constexpr uint8_t ES8311_REG_27 = 0x27;
    static constexpr uint8_t ES8311_REG_28 = 0x28;
    static constexpr uint8_t ES8311_REG_29 = 0x29;
    static constexpr uint8_t ES8311_REG_2A = 0x2A;
    static constexpr uint8_t ES8311_REG_2B = 0x2B;
    static constexpr uint8_t ES8311_REG_2C = 0x2C;
    static constexpr uint8_t ES8311_REG_2D = 0x2D;
    static constexpr uint8_t ES8311_REG_43 = 0x43;
    static constexpr uint8_t ES8311_REG_44 = 0x44;
    static constexpr uint8_t ES8311_REG_45 = 0x45;
    static constexpr uint8_t ES8311_REG_46 = 0x46;
    static constexpr uint8_t ES8311_REG_47 = 0x47;
    static constexpr uint8_t ES8311_REG_48 = 0x48;
    static constexpr uint8_t ES8311_REG_49 = 0x49;
    static constexpr uint8_t ES8311_REG_4A = 0x4A;
    static constexpr uint8_t ES8311_REG_4B = 0x4B;

    // 初始化配置
    bool reset();
    bool configure();
};

#endif
