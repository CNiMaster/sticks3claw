#include "audio_recorder.h"
#include "config.h"
#include <driver/i2s.h>

static bool s_i2sInstalled = false;

bool i2s_driver_ensure_installed() {
    if (s_i2sInstalled) return true;

    Serial.println("Installing I2S driver...");

    i2s_config_t i2sCfg = {};
    i2sCfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
    i2sCfg.sample_rate = I2S_SAMPLE_RATE;
    i2sCfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2sCfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;  // STEREO — ES8311 强制要求
    i2sCfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2sCfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2sCfg.dma_buf_count = 8;
    i2sCfg.dma_buf_len = 1024;  // ESP32-S3 I2S limit: max 1024
    i2sCfg.use_apll = false;
    i2sCfg.tx_desc_auto_clear = true;
    i2sCfg.fixed_mclk = I2S_SAMPLE_RATE * 256;  // 256x MCLK — ES8311 锁相环要求

    i2s_pin_config_t pinCfg = {};
    pinCfg.mck_io_num = I2S_MCLK_PIN;
    pinCfg.bck_io_num = I2S_BCLK_PIN;
    pinCfg.ws_io_num = I2S_WS_PIN;
    pinCfg.data_out_num = I2S_DIN_PIN;   // To speaker (ES8311 DIN)
    pinCfg.data_in_num = I2S_DOUT_PIN;    // From mic (ES8311 DOUT)

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sCfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("i2s_driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    err = i2s_set_pin(I2S_NUM_0, &pinCfg);
    if (err != ESP_OK) {
        Serial.printf("i2s_set_pin failed: %s\n", esp_err_to_name(err));
        return false;
    }

    s_i2sInstalled = true;
    Serial.println("I2S driver installed (duplex)!");
    return true;
}

// ============================================================
// AudioRecorder
// ============================================================

AudioRecorder::AudioRecorder(ES8311Driver* codec)
    : _codec(codec)
    , _recording(false)
    , _recordBuffer(nullptr)
    , _bufferSize(0)
    , _recordedBytes(0)
    , _readIndex(0)
{
}

AudioRecorder::~AudioRecorder() {
    stop();
    if (_recordBuffer) {
        free(_recordBuffer);
        _recordBuffer = nullptr;
    }
}

bool AudioRecorder::begin() {
    Serial.println("Initializing audio recorder...");

    if (!i2s_driver_ensure_installed()) return false;

    _bufferSize = 512 * 1024;  // 512KB = ~16s at 16kHz 16bit mono
    _recordBuffer = (uint8_t*)ps_malloc(_bufferSize);

    if (!_recordBuffer) {
        Serial.println("Failed to allocate record buffer!");
        return false;
    }

    Serial.printf("Record buffer: %u bytes in PSRAM\n", _bufferSize);
    Serial.println("Audio recorder initialized!");
    return true;
}

bool AudioRecorder::start() {
    if (_recording) return false;

    _recording = true;
    _recordedBytes = 0;
    _readIndex = 0;

    // Clear DMA buffers
    i2s_zero_dma_buffer(I2S_NUM_0);

    Serial.println("Recording started");
    return true;
}

void AudioRecorder::stop() {
    if (!_recording) return;
    _recording = false;
    Serial.printf("Recording stopped: %u bytes\n", _recordedBytes);
}

size_t AudioRecorder::read(uint8_t* buffer, size_t length) {
    if (!_recording) return 0;

    // STEREO 模式：I2S 返回双声道交织数据，用栈缓冲区避免 malloc
    size_t stereoLen = length * 2;
    uint8_t stereoBuf[2048];
    if (stereoLen > sizeof(stereoBuf)) stereoLen = sizeof(stereoBuf);

    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_NUM_0, stereoBuf, stereoLen, &bytesRead, pdMS_TO_TICKS(10));
    if (err != ESP_OK || bytesRead == 0) return 0;

    // 从双声道提取左声道（偶数为左声道）
    size_t monoFrames = bytesRead / 4;
    size_t monoOut = monoFrames < length / 2 ? monoFrames : length / 2;
    for (size_t i = 0; i < monoOut; i++) {
        ((int16_t*)buffer)[i] = ((int16_t*)stereoBuf)[i * 2];
    }

    size_t monoBytes = monoOut * 2;

    if (_recordBuffer && _recordedBytes + monoBytes <= _bufferSize) {
        memcpy(_recordBuffer + _recordedBytes, buffer, monoBytes);
        _recordedBytes += monoBytes;
    }

    return monoBytes;
}

size_t AudioRecorder::available() {
    return _recordedBytes - _readIndex;
}

size_t AudioRecorder::getCapturedData(uint8_t* buffer, size_t length) {
    size_t toCopy = min(length, (size_t)(_recordedBytes - _readIndex));
    if (toCopy == 0) return 0;
    memcpy(buffer, _recordBuffer + _readIndex, toCopy);
    _readIndex += toCopy;
    return toCopy;
}
