#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include "config/ai_providers.h"
#include "config/app_config.h"
#include "wifi/wifi_manager.h"
#include "wifi/wifi_portal.h"
#include "audio/es8311_driver.h"
#include "audio/audio_recorder.h"
#include "audio/audio_player.h"
#include "audio/edge_tts.h"
#include "audio/sound_effects.h"
#include "display/tft_display.h"
#include "display/face_renderer.h"
#include "display/orientation_manager.h"
#include "display/text_scroller.h"
#include "display/message_history.h"
#include "display/mood_detector.h"
#include "sensors/gyroscope.h"
#include "communication/mqtt_client.h"
#include "communication/ai_client.h"
#include "input/button_handler.h"
#include "system/power_manager.h"
#include "stt/volcengine_stt.h"
#include "conversation_history.h"

// ============================================================
// TTS 音色表
// ============================================================
static const char* ttsVoiceNames[] = {
    TTS_VOICE_0_NAME, TTS_VOICE_1_NAME, TTS_VOICE_2_NAME,
    TTS_VOICE_3_NAME, TTS_VOICE_4_NAME
};
static const char* ttsVoiceIds[] = {
    TTS_VOICE_0_ID, TTS_VOICE_1_ID, TTS_VOICE_2_ID,
    TTS_VOICE_3_ID, TTS_VOICE_4_ID
};
static const char* ttsVoiceLabels[] = {
    "V1:Gentle", "V2:Sweet", "V3:Smart", "V4:Male", "V5:News"
};

// ============================================================
// KEY2 菜单
// ============================================================
enum MenuCategory {
    MENU_COMM_MODE, MENU_AI_MODEL, MENU_ROLE, MENU_TTS_VOICE,
    MENU_ORIENTATION, MENU_TEXT_DISPLAY
};
#define MENU_CATEGORY_COUNT 6
static const char* menuCategoryNames[] = {
    "Mode", "Model", "Role", "Voice", "Orient", "Text"
};

static const char* rolePrompts[ROLE_PRESET_COUNT] = {
    ROLE_0_PROMPT, ROLE_1_PROMPT, ROLE_2_PROMPT
};
static const char* roleNames[ROLE_PRESET_COUNT] = {
    ROLE_0_NAME, ROLE_1_NAME, ROLE_2_NAME
};
static const char* roleLabels[ROLE_PRESET_COUNT] = {
    ROLE_0_LABEL, ROLE_1_LABEL, ROLE_2_LABEL
};

// ============================================================
// 全局对象
// ============================================================
WiFiManager* wifiManager;
WiFiPortal* wifiPortal;
ES8311Driver* audioCodec;
AudioRecorder* recorder;
AudioPlayer* player;
TFTDisplay* display;
FaceRenderer* faceRenderer;
Gyroscope* gyroscope;
OrientationManager* orientationManager;
TextScroller* textScroller;
MessageHistory* messageHistory;
MQTTClient* mqttClient;
AIClient* aiClient;
ButtonHandler* recordButton;
ButtonHandler* modeButton;
VolcengineSTT* stt;
EdgeTTS* tts;
PowerManager* powerManager;
SoundEffects* soundEffects;
ConversationHistory* convHistory;

// ============================================================
// 状态
// ============================================================
enum State { IDLE, RECORDING, PROCESSING, WAITING_REPLY, IN_MENU };
State currentState = IDLE;
CommMode currentCommMode = DEFAULT_COMM_MODE;
int currentProvider = DEFAULT_PROVIDER;
int currentTtsVoice = DEFAULT_TTS_VOICE;
int currentRole = DEFAULT_ROLE;
MenuCategory currentMenu = MENU_COMM_MODE;
bool showText = true;
bool showingHistory = false;
unsigned long menuEnterTime = 0;
unsigned long recordingStartTime = 0;
static const unsigned long MENU_TIMEOUT_MS = 10000;
static const unsigned long RECORDING_TIMEOUT_MS = 10000;
static const unsigned long BOOT_IGNORE_MS = 3000;
// MQTT 模式下等待 OpenClaw 回复的上限。超时必须回 IDLE，
// 否则 Broker 不可达时设备会永远停在「等待回复」状态。
static const unsigned long MQTT_REPLY_TIMEOUT_MS = 30000;
static unsigned long bootTime = 0;
static unsigned long waitingSince = 0;

void startRecording();
void stopRecording();
void handleShakeGesture();
void handleMQTTMessage(JsonDocument& msg);
void handleMenuShortPress();
void handleMenuLongPress();
void enterMenu();
void exitMenu();
void displayMenu();
void sendTextToAI(const char* text);
void returnToIdle();
void toggleHistory();
void dismissHistory();
void renderHistory();
const char* getCurrentVoiceId();
const char* getCurrentVoiceName();
const char* getCurrentModelName();

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(3000);
    Serial.println("\n=== Sticks3 ESP32-S3 ===");

    AppConfig::instance().begin();
    bool hasPsram = psramFound();
    if (hasPsram) Serial.printf("PSRAM: %d bytes\n", ESP.getPsramSize());
    else Serial.println("PSRAM: not found");

    display = new TFTDisplay();
    display->begin();
    {
        TFT_eSPI* tft = display->getTft();
        tft->fillScreen(TFT_BLACK);
        tft->fillCircle(5, 5, 5, TFT_RED);
        tft->fillCircle(tft->width()/2, tft->height()/2, 5, TFT_GREEN);
        tft->fillCircle(tft->width()-6, tft->height()-6, 5, TFT_BLUE);
        tft->setTextColor(TFT_WHITE);
        tft->setCursor(10, tft->height()/2 + 15);
        tft->setTextSize(2);
        tft->printf("R=%d %dx%d", 1, tft->width(), tft->height());
        delay(3000);
    }

    gyroscope = new Gyroscope();
    gyroscope->begin();

    faceRenderer = new FaceRenderer(display->getTft());
    if (hasPsram) {
        bool isPortrait = (display->getWidth() < display->getHeight());
        faceRenderer->setLayoutMode(isPortrait);
        faceRenderer->begin(display->getWidth(), display->getHeight());
        faceRenderer->setEmotion("idle");
    }

    orientationManager = new OrientationManager(display, faceRenderer, gyroscope);
    orientationManager->init();

    textScroller = new TextScroller(display->getTft());
    textScroller->setArea(faceRenderer->getEyeAreaHeight(), display->getWidth(),
                          display->getHeight() - faceRenderer->getEyeAreaHeight(),
                          faceRenderer->getCurrentBgColor());
    textScroller->setActive(false);

    wifiManager = new WiFiManager();
    wifiPortal = new WiFiPortal();
    if (AppConfig::instance().isConfigured()) {
        if (!wifiManager->connect()) {
            Serial.println("WiFi failed! Starting config portal...");
            wifiPortal->start();
        } else {
            Serial.printf("IP: %s\n", wifiManager->getIP().c_str());
        }
    } else {
        Serial.println("No WiFi configured! Starting portal...");
        wifiPortal->start();
    }

    audioCodec = new ES8311Driver();
    audioCodec->begin();
    recorder = new AudioRecorder(audioCodec);
    player = new AudioPlayer(audioCodec);
    recorder->begin();
    player->begin();

    messageHistory = new MessageHistory();
    soundEffects = new SoundEffects();
    soundEffects->begin();

    recordButton = new ButtonHandler(RECORD_BUTTON);
    modeButton = new ButtonHandler(MODE_BUTTON);

    {
        auto& cfg = AppConfig::instance();
        mqttClient = new MQTTClient(
            cfg.getMqttHost().c_str(), cfg.getMqttPort(), cfg.getMqttClientId().c_str(),
            cfg.getMqttInboundTopic().c_str(), cfg.getMqttOutboundTopic().c_str());
    }
    mqttClient->begin();
    mqttClient->onMessage(handleMQTTMessage);
    mqttClient->connect();

    aiClient = new AIClient();
    convHistory = new ConversationHistory();
    convHistory->begin();

    {
        auto& cfg = AppConfig::instance();
        stt = new VolcengineSTT(cfg.getSttAppId().c_str(), cfg.getSttToken().c_str(), cfg.getSttCluster().c_str());
    }
    tts = new EdgeTTS(getCurrentVoiceId());

    powerManager = new PowerManager();
    powerManager->begin(display, wifiManager, mqttClient);

    returnToIdle();
    Serial.printf("Init done! Heap:%u PSRAM:%u\n", ESP.getFreeHeap(), ESP.getFreePsram());
    bootTime = millis();
}

// ============================================================
// Loop
// ============================================================
void loop() {
    if (wifiPortal->isRunning()) { faceRenderer->update(); delay(10); return; }

    powerManager->update();
    wifiManager->update();
    mqttClient->update();

    recordButton->update();
    modeButton->update();

    if (millis() - bootTime < BOOT_IGNORE_MS) {
        recordButton->reset();
        modeButton->reset();
        faceRenderer->update();
        delay(10);
        return;
    }

    if (!powerManager->isAwake()) {
        if (recordButton->wasClicked() || recordButton->wasLongPressed(1000)
            || modeButton->wasClicked() || modeButton->wasLongPressed(800)) {
            powerManager->activity();
            soundEffects->play("ding", player);
            faceRenderer->setEmotion("surprised");
            returnToIdle();
        }
        static unsigned long lastGyroSleep = 0;
        if (millis() - lastGyroSleep >= GYRO_UPDATE_INTERVAL) {
            gyroscope->update();
            lastGyroSleep = millis();
            if (gyroscope->detectShake()) {
                powerManager->wake();
                soundEffects->play("ding", player);
                faceRenderer->setEmotion("surprised");
                returnToIdle();
            }
        }
        faceRenderer->update();
        delay(10);
        return;
    }

    orientationManager->update();
    bool portrait = orientationManager->isPortrait();

    if (currentState != IN_MENU) {
        int textH = min(50, display->getHeight() / 3);
        int textY = display->getHeight() - textH;
        if (portrait) {
            textY = faceRenderer->getEyeAreaHeight();
            textH = display->getHeight() - faceRenderer->getEyeAreaHeight();
        }
        textScroller->setArea(textY, display->getWidth(), textH, faceRenderer->getCurrentBgColor());
    }

    bool voiceActive = (currentState == RECORDING || currentState == PROCESSING
                        || currentState == WAITING_REPLY);
    textScroller->setActive(voiceActive && !showingHistory && showText && currentState != IN_MENU);

    if (currentState == IN_MENU) {
        if (recordButton->wasClicked()) {
            powerManager->activity();
            menuEnterTime = millis();
            handleMenuShortPress();
        }
    } else {
        if (recordButton->wasLongPressed(600) && currentState == IDLE) {
            startRecording();
        } else if (currentState == RECORDING) {
            uint8_t tmpBuf[1024];
            recorder->read(tmpBuf, sizeof(tmpBuf));
            if (!recordButton->isPressed() || millis() - recordingStartTime >= RECORDING_TIMEOUT_MS) {
                stopRecording();
            }
        }
    }

    if (currentState == IN_MENU) {
        if (modeButton->wasClicked() || modeButton->wasDoublePressed()) {
            powerManager->activity();
            menuEnterTime = millis();
            currentMenu = (MenuCategory)((currentMenu + 1) % MENU_CATEGORY_COUNT);
            displayMenu();
        } else if (modeButton->wasLongPressed(800)) {
            powerManager->activity();
            exitMenu();
        }
    } else {
        if (modeButton->wasDoublePressed()) {
            powerManager->activity();
            if (currentState == IDLE) enterMenu();
        } else if (modeButton->wasLongPressed(800)) {
            powerManager->activity();
            if (currentState == IDLE) toggleHistory();
        } else if (modeButton->wasClicked()) {
            powerManager->activity();
            if (showingHistory) dismissHistory();
        }
    }

    if (currentState == IN_MENU && millis() - menuEnterTime >= MENU_TIMEOUT_MS) {
        exitMenu();
    }

    static unsigned long lastGyro = 0;
    if (millis() - lastGyro >= GYRO_UPDATE_INTERVAL) {
        gyroscope->update();
        lastGyro = millis();
        if (gyroscope->detectShake()) handleShakeGesture();
    }

    // MQTT 模式下可能永远等不到回复（Broker 没开 / OpenClaw 未运行）。
    // 没有超时的话设备会一直卡在 WAITING_REPLY，既不说话也不休眠。
    if (currentState == WAITING_REPLY && currentCommMode == COMM_MQTT) {
        if (millis() - waitingSince >= MQTT_REPLY_TIMEOUT_MS) {
            Serial.println("MQTT reply timeout");
            textScroller->addLine("Timeout", TFT_RED);
            returnToIdle();
        }
    }

    if (showingHistory) renderHistory();
    else if (currentState == IN_MENU) faceRenderer->update();
    else { faceRenderer->update(); textScroller->update(); }

    delay(10);
}

// ============================================================
void returnToIdle() {
    currentState = IDLE;
    showingHistory = false;
    // 录音/识别/等待 AI/播放全流程结束，恢复省电
    if (powerManager) powerManager->setBusy(false);
    faceRenderer->setEmotion("idle");
    faceRenderer->resume();
}

// ============================================================
// 录音
// ============================================================
void startRecording() {
    powerManager->activity();
    powerManager->setBusy(true);   // 录音链路期间禁止熄屏/休眠
    currentState = RECORDING;
    recordingStartTime = millis();
    faceRenderer->setEmotion("listening");
    textScroller->addLine("Recording...", TFT_RED);
    messageHistory->add("Recording...", Message::STATUS);
    recorder->start();
}

void stopRecording() {
    recorder->stop();
    currentState = PROCESSING;
    faceRenderer->setEmotion("thinking");
    textScroller->addLine("STT...", TFT_YELLOW);
    textScroller->setActive(true);
    textScroller->update();
    faceRenderer->update();

    size_t audioLength = recorder->available();
    if (audioLength == 0) { textScroller->addLine("No audio", TFT_RED); returnToIdle(); return; }

    uint8_t* audioBuffer = (uint8_t*)ps_malloc(audioLength);
    if (!audioBuffer) { returnToIdle(); return; }
    recorder->getCapturedData(audioBuffer, audioLength);

    String text = stt->recognize(audioBuffer, audioLength);
    free(audioBuffer);

    if (text.length() == 0) { textScroller->addLine("No speech", TFT_RED); returnToIdle(); return; }

    textScroller->addLine(text.c_str(), TFT_WHITE);
    messageHistory->add(text.c_str(), Message::STT_RESULT);
    textScroller->update();
    faceRenderer->update();
    sendTextToAI(text.c_str());
}

void sendTextToAI(const char* text) {
    bool sent = false;
    if (currentCommMode == COMM_MQTT) {
        sent = mqttClient->isConnected() && mqttClient->publishText(text);
        if (sent) {
            currentState = WAITING_REPLY;
            waitingSince = millis();
            powerManager->setBusy(true);
            textScroller->addLine("Waiting...", TFT_GREEN);
            textScroller->setActive(true);
        }
    } else {
        faceRenderer->setEmotion("thinking");
        textScroller->addLine("Thinking...", TFT_YELLOW);
        textScroller->setActive(true);
        currentState = WAITING_REPLY;
        powerManager->setBusy(true);   // AI 请求可能数十秒，期间不得休眠断网
        convHistory->add("user", text);
        static HistoryEntry historyBuf[50];
        int historyCount = convHistory->getRecent(historyBuf, 50);
        AIProvider roleProvider = aiProviders[currentProvider];
        roleProvider.systemPrompt = rolePrompts[currentRole];
        String reply = aiClient->chat(roleProvider, text, historyBuf, historyCount);

        if (reply.length() > 0) {
            String cleanReply;
            const char* emotion = MoodDetector::parseEmotionTag(reply.c_str(), cleanReply);
            if (strlen(emotion) == 0) emotion = MoodDetector::detectMood(cleanReply.c_str());
            convHistory->add("assistant", cleanReply.c_str());
            textScroller->addLine(cleanReply.c_str(), TFT_CYAN);
            messageHistory->add(cleanReply.c_str(), Message::AI_REPLY);
            for (int i = 0; i < 30; i++) { textScroller->update(); faceRenderer->update(); delay(16); }
            faceRenderer->setEmotion(emotion);
            faceRenderer->update();
            faceRenderer->setEmotion("speaking");

            uint8_t* pcmData = nullptr;
            size_t pcmLength = 0;
            if (tts->synthesize(cleanReply.c_str(), &pcmData, &pcmLength) && pcmData) {
                player->play(pcmData, pcmLength);
                while (player->isPlaying()) { faceRenderer->update(); textScroller->update(); delay(10); }
                player->releasePlaybackBuffer();
                free(pcmData);
            }
            returnToIdle();
        } else {
            textScroller->addLine("AI error", TFT_RED);
            returnToIdle();
        }
    }
    if (!sent && currentCommMode == COMM_MQTT) { textScroller->addLine("Send fail", TFT_RED); returnToIdle(); }
}

// ============================================================
// MQTT
// ============================================================
void handleMQTTMessage(JsonDocument& msg) {
    powerManager->activity();
    const char* text = msg["text"] | "";
    if (strlen(text) == 0) return;
    String cleanText;
    const char* emotion = msg["emotion"] | "";
    if (strlen(emotion) > 0) { MoodDetector::parseEmotionTag(text, cleanText); if (cleanText.length() == 0) cleanText = text; }
    else { emotion = MoodDetector::parseEmotionTag(text, cleanText); if (strlen(emotion) == 0) { emotion = MoodDetector::detectMood(text); cleanText = text; } }

    textScroller->clear();
    textScroller->addLine(cleanText.c_str(), TFT_CYAN);
    messageHistory->add(cleanText.c_str(), Message::AI_REPLY);
    faceRenderer->setEmotion(emotion);

    if (currentState == WAITING_REPLY) {
        textScroller->setActive(true);
        for (int i = 0; i < 30; i++) { textScroller->update(); faceRenderer->update(); delay(16); }
        faceRenderer->setEmotion("speaking");
        uint8_t* pcmData = nullptr; size_t pcmLength = 0;
        if (tts->synthesize(cleanText.c_str(), &pcmData, &pcmLength) && pcmData) {
            player->play(pcmData, pcmLength);
            while (player->isPlaying()) { faceRenderer->update(); textScroller->update(); delay(10); }
            player->releasePlaybackBuffer(); free(pcmData);
        }
        returnToIdle();
    }
}

// ============================================================
// 历史
// ============================================================
void toggleHistory() {
    if (showingHistory) dismissHistory();
    else { showingHistory = true; faceRenderer->pause(); display->getTft()->fillScreen(0x0000); }
}
void dismissHistory() { showingHistory = false; faceRenderer->resume(); }
void renderHistory() {
    TFT_eSPI* tft = display->getTft();
    int w = display->getWidth();
    int h = display->getHeight();
    tft->setTextSize(1); tft->setTextWrap(true);
    tft->setTextColor(TFT_YELLOW); tft->setCursor(2, 2); tft->print("History (K2 dismiss)");
    int y = 16;
    int count = messageHistory->count();
    int startIdx = count > 10 ? count - 10 : 0;
    for (int i = startIdx; i < count && y < h - 10; i++) {
        const Message& msg = messageHistory->get(count - 1 - i);
        uint16_t color;
        switch (msg.type) {
            case Message::AI_REPLY: color = TFT_CYAN; break;
            case Message::STT_RESULT: color = TFT_WHITE; break;
            default: color = 0x7BEF; break;
        }
        tft->setTextColor(color); tft->setCursor(2, y);
        char buf[60]; strncpy(buf, msg.text, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
        tft->print(buf);
        y += 14;
    }
}

// ============================================================
// 菜单
// ============================================================
void enterMenu() { currentState = IN_MENU; showingHistory = false; menuEnterTime = millis(); displayMenu(); }
void exitMenu() { currentState = IDLE; faceRenderer->resume(); }

void handleMenuShortPress() {
    menuEnterTime = millis();
    switch (currentMenu) {
        case MENU_COMM_MODE:
            currentCommMode = (currentCommMode == COMM_MQTT) ? COMM_HTTP_API : COMM_MQTT;
            if (currentCommMode == COMM_HTTP_API && strlen(aiProviders[currentProvider].apiUrl) == 0)
                for (int i = 1; i < AI_PROVIDERS_COUNT; i++)
                    if (strlen(aiProviders[i].apiUrl) > 0) { currentProvider = i; break; }
            break;
        case MENU_AI_MODEL:
            if (AI_PROVIDERS_COUNT <= 1) break;
            do { currentProvider = (currentProvider + 1) % AI_PROVIDERS_COUNT; }
            while (strlen(aiProviders[currentProvider].apiUrl) == 0 && AI_PROVIDERS_COUNT > 1);
            break;
        case MENU_ROLE:
            currentRole = (currentRole + 1) % ROLE_PRESET_COUNT;
            convHistory->clear();
            break;
        case MENU_TTS_VOICE:
            currentTtsVoice = (currentTtsVoice + 1) % TTS_VOICES_COUNT;
            delete tts; tts = new EdgeTTS(getCurrentVoiceId());
            break;
        case MENU_ORIENTATION:
            orientationManager->forceOrientation(orientationManager->isPortrait() ? ORIENT_LANDSCAPE : ORIENT_PORTRAIT);
            break;
        case MENU_TEXT_DISPLAY:
            showText = !showText;
            break;
    }
    displayMenu();
}

void handleMenuLongPress() {
    menuEnterTime = millis();
    currentMenu = (MenuCategory)((currentMenu + 1) % MENU_CATEGORY_COUNT);
    displayMenu();
}

void displayMenu() {
    TFT_eSPI* tft = display->getTft();
    int w = display->getWidth();
    int h = display->getHeight();
    faceRenderer->pause();
    tft->fillScreen(0x0000);
    tft->setTextSize(3);
    tft->setTextColor(TFT_YELLOW);
    int catW = strlen(menuCategoryNames[currentMenu]) * 18;
    tft->setCursor((w - catW) / 2, 8);
    tft->print(menuCategoryNames[currentMenu]);
    tft->setTextSize(4);
    uint16_t valColor = TFT_WHITE;

    switch (currentMenu) {
        case MENU_COMM_MODE: {
            const char* mode = (currentCommMode == COMM_MQTT) ? "MQTT" : "API";
            valColor = (currentCommMode == COMM_MQTT) ? TFT_GREEN : TFT_CYAN;
            tft->setTextColor(valColor);
            int modeW = strlen(mode) * 24;
            tft->setCursor((w - modeW) / 2, 45);
            tft->print(mode);
            tft->setTextSize(1);
            tft->setTextColor(0x7BEF);
            if (currentCommMode == COMM_MQTT && mqttClient) {
                tft->setCursor(4, h - 16);
                tft->print(mqttClient->isConnected() ? "Broker: OK" : "Broker: ...");
            }
            break;
        }
        case MENU_AI_MODEL:  tft->setTextColor(TFT_CYAN); tft->setCursor(4, 50); tft->print(getCurrentModelName()); break;
        case MENU_ROLE:      tft->setTextColor(TFT_MAGENTA); tft->setCursor((w - strlen(roleNames[currentRole])*24)/2, 50); tft->print(roleNames[currentRole]); break;
        case MENU_TTS_VOICE: tft->setTextColor(TFT_WHITE); tft->setCursor(4, 50); tft->print(ttsVoiceLabels[currentTtsVoice]); break;
        case MENU_ORIENTATION:
            valColor = orientationManager->isPortrait() ? TFT_CYAN : TFT_GREEN;
            tft->setTextColor(valColor);
            tft->setCursor(4, 50);
            tft->print(orientationManager->isPortrait() ? "PORTRAIT" : "LAND");
            break;
        case MENU_TEXT_DISPLAY:
            tft->setTextColor(showText ? TFT_GREEN : TFT_RED);
            tft->setCursor(4, 50);
            tft->print(showText ? "ON" : "OFF");
            break;
    }

    tft->setTextSize(1);
    int bottomY = h - 8;
    int tabW = w / MENU_CATEGORY_COUNT;
    for (int i = 0; i < MENU_CATEGORY_COUNT; i++) {
        int tx = i * tabW + tabW / 2 - strlen(menuCategoryNames[i]) * 3;
        if (i == currentMenu) { tft->setTextColor(TFT_YELLOW); tft->drawLine(i*tabW+2, bottomY+8, (i+1)*tabW-2, bottomY+8, TFT_YELLOW); }
        else { tft->setTextColor(0x5AEB); }
        tft->setCursor(tx, bottomY);
        tft->print(menuCategoryNames[i]);
    }
}

const char* getCurrentVoiceId() { return ttsVoiceIds[currentTtsVoice]; }
const char* getCurrentVoiceName() { return ttsVoiceNames[currentTtsVoice]; }
const char* getCurrentModelName() { return aiProviders[currentProvider].name; }

// ============================================================
// 手势
// ============================================================
void handleShakeGesture() {
    powerManager->activity();
    faceRenderer->setEmotion("surprised");
    soundEffects->play("boing", player);
    JsonDocument eventDoc;
    eventDoc["type"] = "shake";
    eventDoc["timestamp"] = millis();
    float ax, ay, az;
    gyroscope->getAcceleration(ax, ay, az);
    eventDoc["accel"]["x"] = ax; eventDoc["accel"]["y"] = ay; eventDoc["accel"]["z"] = az;
    String eventJson; serializeJson(eventDoc, eventJson);
    if (mqttClient->isConnected()) mqttClient->publishEvent("shake", eventJson.c_str());
}
