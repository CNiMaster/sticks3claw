#ifndef ENV_SECRETS_H
#define ENV_SECRETS_H

// ============================================================
// 🔐 隐私配置模板 — 复制为 secrets.h 后填写真实值
//
//   cp env/secrets.example.h env/secrets.h
//
// env/secrets.h 已被 .gitignore 排除，永远不会被提交。
// 这是本项目唯一需要你填写的文件，其他配置都有可用默认值。
// ============================================================

// ---------- WiFi（必填）----------
// 设备最多轮询匹配 3 个已配置网络，留空项会被跳过
#define ENV_WIFI_SSID       "你的WiFi名称"
#define ENV_WIFI_PASSWORD   "你的WiFi密码"
#define ENV_WIFI_SSID_2     ""
#define ENV_WIFI_PASSWORD_2 ""

// ---------- 火山引擎 STT（语音识别，必填）----------
// 控制台 → 语音技术 → 资源应用 → 创建应用
// 拿到 App ID / Token 后，集群名固定为 volc.seedasr.sauc.duration
// https://console.volcengine.com/speech/app
#define ENV_VOLCENGINE_APP_ID   ""
#define ENV_VOLCENGINE_TOKEN    ""
#define ENV_VOLCENGINE_CLUSTER  "volc.seedasr.sauc.duration"

// ---------- AI Key（对话，必填其一）----------
// Provider 1 默认走 OpenRouter，模型 openrouter/free 可免费用
#define ENV_AI1_KEY ""

// Provider 2 在 src/secrets.h 里配置（默认留空，未启用）

// ---------- 接 OpenClaw（可选）----------
// 不走这个文件：OpenClaw Gateway 的地址/Key/模型配在 src/secrets.h 的
// AI_PROVIDER_1_* 或 AI_PROVIDER_2_* 里，见该文件内的说明。
#endif
