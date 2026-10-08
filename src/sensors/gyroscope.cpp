#include "gyroscope.h"
#include "config.h"
#include <math.h>

Gyroscope::Gyroscope()
    : _initialized(false)
    , _ax(0), _ay(0), _az(0)
    , _lastAccelMagnitude(0)
    , _lastUpdateTime(0)
    , _orientation(ORIENT_LANDSCAPE)
    , _prevOrientation(ORIENT_LANDSCAPE)
    , _orientChanged(false)
{
}

Gyroscope::~Gyroscope() {
}

bool Gyroscope::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(BMI270_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool Gyroscope::readRegs(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(BMI270_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(BMI270_ADDR, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

bool Gyroscope::begin() {
    Serial.println("Initializing BMI270 (minimal I2C driver)...");

    // Wire 已由 TFTDisplay 初始化（同一物理总线）
    delay(10);

    // 验证芯片 ID
    uint8_t chipId = 0;
    if (!readRegs(0x00, &chipId, 1)) {
        Serial.println("BMI270: I2C read failed");
        return false;
    }
    Serial.printf("BMI270 chip ID: 0x%02X\n", chipId);
    if (chipId != 0x24 && chipId != 0x26) {
        Serial.println("BMI270: unexpected chip ID");
        return false;
    }

    // 软复位
    if (!writeReg(0x7E, 0xB6)) {
        Serial.println("BMI270: soft reset failed");
        return false;
    }
    delay(50);

    // 禁用高级省电 (PWR_CONF 0x7C = 0x00)
    writeReg(0x7C, 0x00);
    delay(5);

    // 加速度配置 (ACC_CONF 0x40): normal mode, ODR=100Hz, 过采样=4x
    // 0xA8 = bits[7]=1(normal), bits[6:4]=100(ODR_100Hz), bits[3:0]=1000(osr4)
    writeReg(0x40, 0xA8);
    delay(5);

    // 使能加速度计 (PWR_CTRL 0x7D, bit2 = accel enable)
    if (!writeReg(0x7D, 0x04)) {
        Serial.println("BMI270: enable accel failed");
        return false;
    }
    delay(10);

    // 读加速度验证
    uint8_t buf[6] = {0};
    readRegs(0x0C, buf, 6);
    int16_t rawX = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t rawY = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t rawZ = (int16_t)((buf[5] << 8) | buf[4]);
    Serial.printf("BMI270 init accel: rawX=%d rawY=%d rawZ=%d\n", rawX, rawY, rawZ);
    float fax = rawY / 4096.0f, fay = rawX / 4096.0f;
    Serial.printf("  mapped: ax=%.2f ay=%.2f → likely %s\n",
        fax, fay,
        fabsf(fax) > fabsf(fay) ? "LANDSCAPE" : "PORTRAIT");

    _initialized = true;
    Serial.println("BMI270 accel ready");
    return true;
}

void Gyroscope::update() {
    if (!_initialized) return;

    unsigned long now = millis();
    if (now - _lastUpdateTime < GYRO_UPDATE_INTERVAL) return;
    _lastUpdateTime = now;

    uint8_t buf[6] = {0};
    if (!readRegs(0x0C, buf, 6)) return;

    int16_t rawX = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t rawY = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t rawZ = (int16_t)((buf[5] << 8) | buf[4]);

    // M5StickS3: BMI270 安装方向导致 X/Y 寄存器与物理轴相反
    _ax = rawY / 4096.0f;
    _ay = rawX / 4096.0f;
    _az = rawZ / 4096.0f;

    calcOrientation();
}

void Gyroscope::getAcceleration(float& ax, float& ay, float& az) {
    ax = _ax; ay = _ay; az = _az;
}

void Gyroscope::calcOrientation() {
    float absX = fabsf(_ax);
    float absY = fabsf(_ay);

    if (absY > absX + ORIENT_HYSTERESIS) {
        _orientation = ORIENT_PORTRAIT;
    } else if (absX > absY + ORIENT_HYSTERESIS) {
        _orientation = ORIENT_LANDSCAPE;
    }
    // 在滞后范围内保持当前朝向

    _orientChanged = (_orientation != _prevOrientation);

    static unsigned long lastLog = 0;
    if (millis() - lastLog >= 3000) {
        lastLog = millis();
        Serial.printf("IMU: ax=%.2f ay=%.2f az=%.2f |aX|=%.2f |aY|=%.2f → %s\n",
            _ax, _ay, _az, absX, absY,
            _orientation == ORIENT_PORTRAIT ? "PORTRAIT" : "LANDSCAPE");
    }
}

bool Gyroscope::orientationChanged() {
    if (_orientChanged) {
        _orientChanged = false;
        _prevOrientation = _orientation;
        return true;
    }
    return false;
}

bool Gyroscope::detectShake() {
    float mag = sqrtf(_ax * _ax + _ay * _ay + _az * _az);
    float delta = fabsf(mag - _lastAccelMagnitude);
    _lastAccelMagnitude = _lastAccelMagnitude * SHAKE_DECAY + mag * (1.0f - SHAKE_DECAY);

    if (delta > SHAKE_THRESHOLD) return true;
    return false;
}
