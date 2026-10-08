#ifndef AI_PROVIDERS_H
#define AI_PROVIDERS_H

#include "config.h"
#include "secrets.h"
#include "communication/ai_client.h"

// AI 模型配置表
// 所有配置在 secrets.h 中修改（名称、URL、Key、模型、提示词）
static const AIProvider aiProviders[] = {
    { "OpenClaw",  "", "", "", "" },  // MQTT 模式占位
    { AI_PROVIDER_1_NAME, AI_PROVIDER_1_URL, AI_PROVIDER_1_KEY, AI_PROVIDER_1_MODEL, AI_PROVIDER_1_PROMPT },
    { AI_PROVIDER_2_NAME, AI_PROVIDER_2_URL, AI_PROVIDER_2_KEY, AI_PROVIDER_2_MODEL, AI_PROVIDER_2_PROMPT },
};

static const int AI_PROVIDERS_COUNT = sizeof(aiProviders) / sizeof(aiProviders[0]);

#endif
