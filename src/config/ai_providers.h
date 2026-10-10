#ifndef AI_PROVIDERS_H
#define AI_PROVIDERS_H

#include "config.h"
#include "secrets.h"
#include "communication/ai_client.h"

// AI 模型配置表
// 所有配置在 secrets.h 中修改（名称、URL、Key、模型、提示词）
//
// 想接 OpenClaw 就把其中一项的 URL 指向它的 Gateway：
//   "http://<电脑IP>:18789/v1/chat/completions"、Key 填 Gateway Token、
//   Model 填 "openclaw/default"。
// 注意：表里不要留全空项——setup() 会挑第一个非空项作为默认值，
// 但空项仍会占一个菜单位，切过去必然请求失败。
static const AIProvider aiProviders[] = {
    { AI_PROVIDER_1_NAME, AI_PROVIDER_1_URL, AI_PROVIDER_1_KEY, AI_PROVIDER_1_MODEL, AI_PROVIDER_1_PROMPT },
    { AI_PROVIDER_2_NAME, AI_PROVIDER_2_URL, AI_PROVIDER_2_KEY, AI_PROVIDER_2_MODEL, AI_PROVIDER_2_PROMPT },
};

static const int AI_PROVIDERS_COUNT = sizeof(aiProviders) / sizeof(aiProviders[0]);

#endif
