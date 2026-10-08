#ifndef STICKS3_CONFIG_H
#define STICKS3_CONFIG_H

// ============================================================
// WiFi 配置（支持多个，自动匹配）
// 密码和敏感信息在 env/secrets.h 中定义（不提交 Git）
// ============================================================
#include "secrets.h"
#define WIFI_SSID     SECRET_WIFI_SSID
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD
#define WIFI_SSID_2   ENV_WIFI_SSID_2
#define WIFI_PASSWORD_2 ENV_WIFI_PASSWORD_2
#define WIFI_SSID_3   ""
#define WIFI_PASSWORD_3 ""
#define WIFI_TIMEOUT 20000

// ============================================================
// 通信模式
// ============================================================
enum CommMode {
    COMM_MQTT,     // MQTT 局域网 → OpenClaw
    COMM_HTTP_API  // HTTP API → 第三方 AI
};
#define DEFAULT_COMM_MODE  COMM_MQTT
#define DEFAULT_PROVIDER   0

// ============================================================
// MQTT 配置
// ============================================================
#define MQTT_BROKER_HOST  ENV_MQTT_HOST   // 在 env/secrets.h 中设置
#define MQTT_BROKER_PORT  1883
#define MQTT_CLIENT_ID    "sticks3"          // 自动追加 MAC 后缀
#define MQTT_INBOUND_TOPIC  "openclaw/inbound"
#define MQTT_OUTBOUND_TOPIC "openclaw/outbound"
#define MQTT_QOS          1
#define MQTT_KEEPALIVE    60
#define MQTT_RECONNECT_INTERVAL 5000
#define MQTT_BUFFER_SIZE  1024

// ============================================================
// Edge TTS 配置
// ============================================================
// 中文音色列表（KEY2 长按切换）
#define TTS_VOICES_COUNT 5
#define TTS_VOICE_0_NAME "晓晓"     // 默认，温柔女声
#define TTS_VOICE_0_ID   "zh-CN-XiaoxiaoNeural"
#define TTS_VOICE_1_NAME "晓依"     // 甜美女声
#define TTS_VOICE_1_ID   "zh-CN-XiaoyiNeural"
#define TTS_VOICE_2_NAME "晓涵"     // 知性女声
#define TTS_VOICE_2_ID   "zh-CN-XiaohanNeural"
#define TTS_VOICE_3_NAME "云扬"     // 男声
#define TTS_VOICE_3_ID   "zh-CN-YunyangNeural"
#define TTS_VOICE_4_NAME "晓辰"     // 新闻女声
#define TTS_VOICE_4_ID   "zh-CN-XiaochenNeural"

#define DEFAULT_TTS_VOICE 0
#define EDGE_TTS_RATE   "+0%"
#define EDGE_TTS_VOLUME "+0%"

// ============================================================
// I2S 音频配置
// ESP32-S3: G18(MCLK) G14(DOUT) G17(BCLK) G15(LRCK) G16(DIN)
// ============================================================
#define I2S_MCLK_PIN  18
#define I2S_BCLK_PIN  17
#define I2S_WS_PIN    15
#define I2S_DIN_PIN   16
#define I2S_DOUT_PIN  14
#define I2S_SAMPLE_RATE 16000
#define AUDIO_BUFFER_SIZE 2048

// ============================================================
// ES8311 音频编解码器
// ESP32-S3: G48(SCL) G47(SDA)
// ============================================================
#define ES8311_I2C_ADDR    0x18
#define ES8311_SDA_PIN     47
#define ES8311_SCL_PIN     48

// ============================================================
// TFT LCD 显示 (ST7789P3 - 135x240)
// ESP32-S3: G39(MOSI) G40(SCK) G45(RS) G41(CS) G21(RST) G38(BL)
// ============================================================
#define TFT_WIDTH   135
#define TFT_HEIGHT  240
#define TFT_MOSI_PIN 39
#define TFT_SCLK_PIN 40
#define TFT_RS_PIN   45
#define TFT_CS_PIN   41
#define TFT_RST_PIN  21
#define TFT_BL_PIN   38

// ============================================================
// BMI270 陀螺仪
// ESP32-S3: G48(SCL) G47(SDA)
// ============================================================
#define BMI270_SDA_PIN 47
#define BMI270_SCL_PIN 48
#define BMI270_ADDR    0x68
#define GYRO_UPDATE_INTERVAL 50
#define ORIENTATION_DEBOUNCE_MS  1000
#define DOUBLE_PRESS_MS          400
#define MOOD_DETECT_MAX_TEXT_LEN 200

// ============================================================
// 按钮
// ESP32-S3: G11(KEY1) G12(KEY2)
// ============================================================
#define KEY1_PIN 11
#define KEY2_PIN 12
#define RECORD_BUTTON KEY1_PIN
#define MODE_BUTTON   KEY2_PIN

// ============================================================
// 红外
// ESP32-S3: G46(IR_TX) G42(IR_RX)
// ============================================================
#define IR_TX_PIN 46
#define IR_RX_PIN 42

// ============================================================
// 角色预设（用户在 secrets.h 中填写 prompt 内容）
// ============================================================
#define ROLE_PRESET_COUNT 3
#define DEFAULT_ROLE 0
#define ROLE_0_NAME    "助手"
#define ROLE_0_LABEL   "Assistant"
#define ROLE_1_NAME    "陪伴"
#define ROLE_1_LABEL   "Companion"
#define ROLE_2_NAME    "导师"
#define ROLE_2_LABEL   "Tutor"

// ============================================================
// 对话历史
// ============================================================
#define CONV_HISTORY_MAX_AGE_DAYS 3
#define CONV_HISTORY_FILE "/conv_history.json"

// ============================================================
// 系统配置
// ============================================================
#define PSRAM_SIZE 8 * 1024 * 1024
#define USE_PSRAM true
#define ENABLE_SERIAL_DEBUG true
#define SERIAL_BAUD 115200

#define AUDIO_BUFFER_POOL_SIZE 10
#define DISPLAY_BUFFER_POOL_SIZE 5

// ============================================================
// 电源管理
// ============================================================
#define SCREEN_OFF_TIMEOUT_MS   30000   // 30 秒无操作熄屏
#define SLEEP_TIMEOUT_MS        300000  // 5 分钟无操作休眠

#endif
