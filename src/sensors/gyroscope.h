#ifndef GYROSCOPE_H
#define GYROSCOPE_H

#include <Arduino.h>
#include <Wire.h>

enum Orientation { ORIENT_LANDSCAPE, ORIENT_PORTRAIT };

class Gyroscope {
public:
    Gyroscope();
    ~Gyroscope();

    bool begin();
    void update();
    bool isInitialized() const { return _initialized; }

    float getAccelX() const { return _ax; }
    float getAccelY() const { return _ay; }
    float getAccelZ() const { return _az; }
    void getAcceleration(float& ax, float& ay, float& az);

    Orientation getOrientation() const { return _orientation; }
    bool orientationChanged();

    bool detectShake();

private:
    bool _initialized;
    float _ax, _ay, _az;
    float _lastAccelMagnitude;
    unsigned long _lastUpdateTime;

    Orientation _orientation;
    Orientation _prevOrientation;
    bool _orientChanged;

    const float SHAKE_THRESHOLD = 15.0;
    const float SHAKE_DECAY = 0.9;
    const float ORIENT_HYSTERESIS = 0.3f;

    bool writeReg(uint8_t reg, uint8_t val);
    bool readRegs(uint8_t reg, uint8_t* buf, uint8_t len);
    void calcOrientation();
};

#endif
