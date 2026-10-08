#include "emoji_renderer.h"
#include <ArduinoJson.h>

EmojiRenderer::EmojiRenderer(TFTDisplay* display)
    : _display(display)
    , _frames(nullptr)
    , _frameCount(0)
    , _currentFrame(0)
    , _lastFrameTime(0)
    , _playing(false)
    , _loop(false)
{
}

EmojiRenderer::~EmojiRenderer() {
    stop();

    if (_frames) {
        delete[] _frames;
    }
}

bool EmojiRenderer::loadEmotions(const char* directory) {
    Serial.printf("Loading emotions from: %s\n", directory);

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
        return false;
    }

    // 检查表情目录是否存在
    if (!LittleFS.exists(directory)) {
        Serial.printf("Emotions directory not found: %s\n", directory);
        return false;
    }

    // TODO: 扫描目录加载所有表情
    // 这里暂时只打印成功信息
    Serial.println("Emotions loaded successfully!");

    return true;
}

bool EmojiRenderer::play(const char* emotionName) {
    // 停止当前动画
    stop();

    // 加载新动画
    if (!loadAnimation(emotionName)) {
        Serial.printf("Failed to load emotion: %s\n", emotionName);
        return false;
    }

    _playing = true;
    _currentFrame = 0;
    _lastFrameTime = millis();

    Serial.printf("Playing emotion: %s (%d frames)\n", emotionName, _frameCount);

    return true;
}

void EmojiRenderer::update() {
    if (!_playing || _frameCount == 0) {
        return;
    }

    unsigned long currentTime = millis();

    // 检查是否需要切换到下一帧
    if (currentTime - _lastFrameTime >= _frames[_currentFrame].duration) {
        _currentFrame++;

        // 检查动画是否结束
        if (_currentFrame >= _frameCount) {
            if (_loop) {
                _currentFrame = 0;  // 循环播放
            } else {
                stop();  // 单次播放完成
                return;
            }
        }

        // 绘制下一帧
        drawFrame(_frames[_currentFrame].filename);
        _lastFrameTime = currentTime;
    }
}

void EmojiRenderer::stop() {
    _playing = false;

    if (_frames) {
        delete[] _frames;
        _frames = nullptr;
    }

    _frameCount = 0;
    _currentFrame = 0;
}

bool EmojiRenderer::loadAnimation(const char* emotionName) {
    char path[128];
    getEmotionPath(emotionName, path, sizeof(path));

    // 读取配置文件
    char configPath[144];
    snprintf(configPath, sizeof(configPath), "%s/config.json", path);

    if (!LittleFS.exists(configPath)) {
        Serial.printf("Config file not found: %s\n", configPath);
        return false;
    }

    fs::File configFile = LittleFS.open(configPath, "r");
    if (!configFile) {
        Serial.printf("Failed to open config file: %s\n", configPath);
        return false;
    }

    // 解析JSON配置
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error != DeserializationError::Ok) {
        Serial.printf("JSON parse error: %s\n", error.c_str());
        return false;
    }

    // 获取帧数量
    JsonArray frames = doc["frames"];
    _frameCount = frames.size();

    if (_frameCount == 0) {
        Serial.println("No frames in emotion config!");
        return false;
    }

    // 分配帧数组
    _frames = new EmotionFrame[_frameCount];

    // 解析每一帧
    for (int i = 0; i < _frameCount; i++) {
        JsonObject frame = frames[i];

        strlcpy(_frames[i].filename, frame["file"] | "", sizeof(_frames[i].filename));
        _frames[i].duration = frame["duration"] | 100;  // 默认100ms
    }

    // 获取循环设置
    _loop = doc["loop"] | false;

    Serial.printf("Loaded emotion: %s (%d frames, loop: %s)\n",
                 emotionName, _frameCount, _loop ? "yes" : "no");

    return true;
}

void EmojiRenderer::drawFrame(const char* filename) {
    char fullPath[192];
    snprintf(fullPath, sizeof(fullPath), "/emotions/%s", filename);

    if (!LittleFS.exists(fullPath)) {
        Serial.printf("Frame file not found: %s\n", fullPath);
        return;
    }

    fs::File frameFile = LittleFS.open(fullPath, "r");
    if (!frameFile) {
        Serial.printf("Failed to open frame file: %s\n", fullPath);
        return;
    }

    // 获取文件大小
    size_t fileSize = frameFile.size();

    // 读取RGB565数据
    uint16_t* imageData = (uint16_t*)ps_malloc(fileSize);

    if (!imageData) {
        Serial.println("Failed to allocate memory for frame!");
        frameFile.close();
        return;
    }

    // 读取数据
    size_t readCount = frameFile.read((uint8_t*)imageData, fileSize);
    frameFile.close();

    if (readCount != fileSize) {
        Serial.printf("Read error: expected %u, got %u\n", fileSize, readCount);
        free(imageData);
        return;
    }

    // 显示图片
    _display->drawRGB565Image(0, 0, imageData, _display->getWidth(), _display->getHeight());

    free(imageData);
}

void EmojiRenderer::getEmotionPath(const char* emotionName, char* path, size_t maxLength) {
    snprintf(path, maxLength, "/emotions/%s", emotionName);
}
