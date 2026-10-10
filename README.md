# Sticks3 ESP32-S3 语音助手

基于 ESP32-S3-Pico-1-N8R8 的桌面语音终端：**按住说话下达任务，回复用语音播报 + 屏幕表情**。

它不追求在 1.14 寸屏上做信息交互——屏只负责表情和简短状态，真正的输入输出都是语音。

## 两种用法

**用法 A：接 OpenClaw（推荐）**

Mac 上跑着 OpenClaw，设备通过局域网连它的 Gateway：

- **语音转写由 Mac 本地完成**，设备端不用注册火山引擎、不用填 Token
- 对话走 **WebSocket**，服务端主动推送结果 → **派活这类长任务也能等到**（上限 120 秒）
- 只需一个 Gateway Token

**用法 B：直连 AI 服务**

不装 OpenClaw 也能用，设备直接 HTTPS 调 OpenAI 兼容服务（OpenRouter / Deepseek 等）。
需要自备火山引擎凭证做转写，且是**一问一答**（15 秒超时，适合短任务）。

> 两条路可以同时配好：**填了 Gateway 地址就走 A，连不上自动回退到 B**，不会变砖。

## 最快上手（用法 A）

```
1. 烧录后上电，板子会开一个叫 Sticks3Claw-Setup 的 WiFi 热点
2. 手机或电脑连上它，浏览器打开 192.168.4.1，填你家的 WiFi
3. src/secrets.h 里填 OPENCLAW_GATEWAY_HOST（Mac 的局域网 IP）和
   OPENCLAW_GATEWAY_TOKEN，重新烧录
4. Mac 侧改三处配置（见 [接入 OpenClaw](#4-接入-openclaw)）
```

填完烧录，长按 KEY1 说话，松开就等它回答（语音播报 + 表情变化）。

## 硬件规格

- **主控**: ESP32-S3-Pico-1-N8R8 (8MB Flash + 8MB PSRAM)
- **音频**: ES8311 音频编解码 + MEMS 麦克风 + 扬声器 (I2S 16kHz 16-bit Mono)
- **显示**: 1.14寸 135x240 IPS LCD (ST7789P3, SPI)
- **传感器**: BMI270 六轴 IMU
- **通信**: WiFi (支持多网络自动匹配)
- **其他**: 物理按钮 KEY1/KEY2、红外收发（预留）

## 功能

- **语音对话**: 长按 KEY1 录音 → 火山引擎 STT 识别 → AI 回复 → Edge TTS 播放
- **多 AI 模型**: 通过 KEY2 切换 OpenRouter、Deepseek 等模型，支持自定义 URL/Key/模型/提示词
  （通信统一走 HTTP，把 Provider 的 URL 指向哪里就调用哪里）
- **多音色 TTS**: 5 种中文 Edge TTS 音色可选
- **表情动画**: 7 种情绪（happy/idle/listening/sad/speaking/surprised/thinking），
  由 `face_renderer` 在屏幕上**实时绘制**（参数化五官 + 眨眼动画），不是播放图片
- **情绪自动识别**: 从 AI 回复文本推断表情（`mood_detector`），配合角色提示词里的
  `[emotion:xxx]` 标记切换
- **摇晃交互**: 摇晃设备把三轴加速度作为事件发送到 OpenClaw
- **多 WiFi 自动匹配**: 依次轮询匹配最多 3 个已配置网络
- **浏览器配网**: 未配置 WiFi 时自动开热点 `Sticks3Claw-Setup`，浏览器 `192.168.4.1` 配置
- **对话历史**: 保留多轮上下文（`conversation_history`）
- **文字滚动显示**: 长文本在屏幕上滚动显示（`text_scroller`）
- **消息历史查看**: 屏幕可回看最近的消息（`message_history`）
- **音效**: 实时合成（`sound_effects`）
- **横竖屏自适应**: 按陀螺仪方向切换显示朝向（`orientation_manager`）
- **省电模式**: 三态电源管理 AWAKE / SCREEN_OFF / LIGHT_SLEEP，唤醒后自动恢复

> **关于「离线」**：本项目的语音识别（火山引擎）、语音合成（微软 Edge TTS）、
> AI 对话（OpenRouter / Deepseek）**全部走云端**，需要联网。
> 没有网时设备只会显示表情，不会响应语音。

> **关于 `data/emotions/`**：这是 `generate_emotions.py` 生成的 `.rgb565` 帧素材
> （7 种情绪共 25 帧，尺寸与屏幕一致为 135×240）。**当前固件并未读取它们**——
> 表情由 `face_renderer` 实时绘制。该目录是预留的离线素材，若后续启用需要自行
> 集成 LittleFS 读取逻辑。
>
> **烧录时不必管文件系统**：`platformio.ini` 里配了 `board_build.filesystem = littlefs`，
> 但因为表情素材未被使用，`pio run --target upload` 不会触发文件系统打包。
> 若哪天启用了该目录，Apple Silicon Mac 上可能遇到
> `mklittlefs: Bad CPU type in executable`（PlatformIO 装的是 x86_64 版工具），
> 用 Rosetta 跑 PlatformIO 或手动编译 LittleFS 镜像即可绕过。

## 系统架构

```
                    ┌──────────────────────────┐
                    │  OpenClaw Gateway        │
                    │  或 OpenRouter/Deepseek  │
                    │  （OpenAI 兼容 HTTP）     │
                    └────────────┬─────────────┘
                                 │ HTTPS / HTTP
┌────────────────────────────────┼────────────────┐
│                    ESP32-S3    ↓                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │ 火山 STT │→ │ AI 请求   │→ │ Edge TTS │→播放│
│  └──────────┘  └──────────┘  └──────────┘      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │ ES8311   │  │ ST7789   │  │ BMI270   │      │
│  │ 录音/播放│  │ 表情显示 │  │ 陀螺仪   │      │
│  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────────────────────────────┘

按住 KEY1 说话 → STT 转文字 → 发给 AI → 回复转语音播报 + 屏幕表情
```

## 项目结构

```
sticks3claw/
├── config.h                         # 硬件引脚、TTS 音色、系统参数
├── env/
│   ├── secrets.example.h            # 🔑 凭证模板（复制为 secrets.h 后填写）
│   └── secrets.h                    # 真实凭证（gitignore，永不提交）
├── src/
│   ├── secrets.h                    # 凭证加载 + AI 模型定义（安全占位，无真实值）
│   ├── sticks3claw.cpp              # 主程序（状态机、菜单、初始化）
│   ├── config/
│   │   ├── ai_providers.h           # AI 模型配置表
│   │   ├── app_config.h/cpp         # NVS 配置管理（默认值从 secrets 加载）
│   │   └── settings.h               # NVS 封装
│   ├── wifi/
│   │   ├── wifi_manager.h/cpp       # WiFi 连接管理（多网络轮询匹配）
│   │   └── wifi_portal.h/cpp        # 浏览器配网门户（AP + HTTP 服务）
│   ├── audio/
│   │   ├── es8311_driver.h/cpp      # ES8311 音频编解码器驱动
│   │   ├── audio_recorder.h/cpp     # 麦克风录音 (I2S)
│   │   ├── audio_player.h/cpp       # 扬声器播放 (I2S)
│   │   ├── edge_tts.h/cpp           # Edge TTS 语音合成 (WebSocket)
│   │   └── sound_effects.h/cpp      # 音效合成
│   ├── communication/
│   │   ├── ai_client.h/cpp          # AI HTTP 客户端 (OpenAI 兼容格式)
│   │   └── conversation_history.h   # 多轮对话上下文
│   ├── display/
│   │   ├── tft_display.h/cpp        # ST7789 TFT 显示驱动
│   │   ├── face_renderer.h/cpp      # 表情渲染
│   │   ├── emoji_renderer.h/cpp     # 表情动画渲染
│   │   ├── eye_config.h             # 眼睛样式配置
│   │   ├── mood_detector.h/cpp      # 从回复文本推断情绪
│   │   ├── message_history.h/cpp    # 消息历史显示
│   │   ├── text_scroller.h/cpp      # 长文本滚动
│   │   └── orientation_manager.h/cpp# 横竖屏自适应
│   ├── sensors/
│   │   └── gyroscope.h/cpp          # BMI270 六轴 IMU
│   ├── input/
│   │   ├── button_handler.h/cpp     # 按钮处理 (KEY1/KEY2)
│   │   └── touch_handler.h/cpp      # ⚠️ 未接线：电容触摸代码存在但主程序未调用
│   ├── system/
│   │   └── power_manager.cpp        # 三态电源管理（ext1 唤醒 + GPIO 中断）
│   ├── stt/
│   │   └── volcengine_stt.h/cpp     # 火山引擎语音识别 (WebSocket)
│   └── utils/
│       ├── base64.h/cpp             # Base64 编解码
│       └── logger.h                 # ⚠️ 未接线：日志工具存在但主程序未调用
├── platformio.ini                   # PlatformIO 配置和依赖
├── generate_emotions.py             # 生成 .rgb565 帧素材（当前固件未使用，见功能章说明）
├── User_Setup.h                     # TFT_eSPI 库配置（引脚/字体）
└── data/
    └── emotions/                    # 预留的 .rgb565 帧素材（当前固件未使用）
```

## 快速开始

### 1. 环境准备

使用 PlatformIO（推荐）：

1. 安装 [VS Code](https://code.visualstudio.com/) + PlatformIO 扩展
2. 打开项目文件夹，自动安装依赖

### 2. 配置

**config.h** — 硬件引脚和系统配置（一般不需要改）：

- WiFi SSID/密码（支持 3 个网络自动匹配）
- TTS 音色列表
- 按钮和传感器引脚

### 2. 配置凭证

**这是唯一需要你填写的文件。** 复制模板：

```bash
cp env/secrets.example.h env/secrets.h
```

`env/secrets.h` 已被 `.gitignore` 排除，永远不会被提交。编辑它填入四类凭证：

```cpp
// ---------- WiFi（必填）----------
#define ENV_WIFI_SSID       "你的WiFi名称"
#define ENV_WIFI_PASSWORD   "你的WiFi密码"
// 可选：第二个网络，留空则跳过
#define ENV_WIFI_SSID_2     ""
#define ENV_WIFI_PASSWORD_2 ""

// ---------- 火山引擎 STT（语音识别，必填）----------
// 控制台 → 语音技术 → 资源应用 → 创建应用
#define ENV_VOLCENGINE_APP_ID   ""
#define ENV_VOLCENGINE_TOKEN    ""
#define ENV_VOLCENGINE_CLUSTER  "volc.seedasr.sauc.duration"

// ---------- AI Key（对话，必填其一）----------
// 默认走 OpenRouter，模型 openrouter/free 可免费使用
#define ENV_AI1_KEY ""

// ---------- 接 OpenClaw ----------
// 不在这个文件里配：OpenClaw Gateway 的地址 / Token / 模型填在
// src/secrets.h 的 AI_PROVIDER_1_* 或 AI_PROVIDER_2_*，见那里说明。
```

**不想改代码？用浏览器配网。** 如果 `ENV_WIFI_SSID` 留空，设备上电后会自己开一个
名为 `Sticks3Claw-Setup` 的热点；用手机或电脑连上它，浏览器打开 `192.168.4.1`
即可在网页里填 WiFi、音色等设置并存进设备。设置完成后会写进设备的 NVS，之后每次
上电自动连接，不再需要热点。

> 两种方式任选其一。改 `env/secrets.h` 适合想用版本管理配置的场景；
> 浏览器配网适合只想快速跑起来、或后续不想动代码的场景。

**AI 模型切换与提示词**：模型名/URL/提示词在 `src/secrets.h` 中改（`AI_PROVIDER_1_*` /
`AI_PROVIDER_2_*`），Key 仍从 `env/secrets.h` 读取。

### 3. 编译上传

```bash
pio run --target upload
pio device monitor -b 115200
```

串口输出里出现 `IP: x.x.x.x` 表示联网成功；出现 `Init done!` 表示初始化完成。

### 4. 接入 OpenClaw

设备通过 **WebSocket** 连 Gateway（Gateway 原生就是 WS，双向，不需要 broker 或插件）。

```
按住 KEY1 录音
   │
   ├─ PCM 分片 → talk.session.appendAudio ──► Mac: 本地 STT → 文字
   │
   └─ 文字 → chat.send ──► Mac: Agent（长任务也行）
                              │
ESP32 ◄── 回复文本 ────────────┘
   │
   └─ Edge TTS 播报 + 屏幕表情
```

> **版本要求**：Talk mode 从 OpenClaw **2026.4.10** 起可用，2026.6 起成熟。
> 不确定就跑 `openclaw --version`。

#### 第 1 步：Mac 侧改三处配置

编辑 `~/.openclaw/openclaw.json`（先备份）：

```json5
{
  gateway: {
    port: 18789,
    bind: "lan",                              // ① 原为 "loopback"
    auth: { mode: "token", token: "一个长随机串" },   // ② 非 loopback 强制鉴权
  },
}
```

① **是关键**——`bind: "loopback"` 只听 127.0.0.1，设备从局域网 IP 根本连不进来。
② 非 loopback 绑定下 Gateway 会拒绝无鉴权启动。

> ⚠️ `bind: "lan"` 后局域网内任何人都能碰到你的 Gateway：**必须设 token**，
> 不要把 18789 端口转发到公网（已有大量暴露实例被扫到）。

#### 第 2 步：设备侧填两个值

`src/secrets.h`：

```cpp
#define OPENCLAW_GATEWAY_HOST   "192.168.1.20"   // Mac 的局域网 IP
#define OPENCLAW_GATEWAY_TOKEN  "与上面 auth.token 一致"
```

留空 `OPENCLAW_GATEWAY_HOST` 即完全不启用，设备会走 HTTP 直连路径。

#### 第 3 步：烧录，看串口

正常会依次出现：

```
OpenClaw: connecting ws://192.168.1.20:18789
OpenClaw: ws connected, waiting challenge
OpenClaw: hello-ok (protocol 4)
OpenClaw: ready
```

**首连接可能需要配对**：设备不是 loopback 客户端，Mac 上会出现待批准请求，
跑一次 `openclaw pairing approve` 即可（之后凭 device token 自动连）。

### 关于 TTS：目前还在设备端

转写已经交给 Mac 了，但**播报仍用设备上的 Edge TTS**（微软在线服务）。
想让播报也走 Mac（用 macOS 自带语音，音色更好且不依赖外网），需要在 Mac 上：

```json5
{ talk: { provider: "system", speechLocale: "zh-CN" } }
```

并让固件改调 `talk.speak` RPC 拿音频。**这一步尚未实现**——`talk.speak` 的
返回格式（采样率、是否 base64）还没在实机上确认过，确认后再接。

### 关于「派活」这类耗时任务

| 连的是 | 任务多久能等 | 说明 |
|---|---|---|
| OpenClaw（WS） | **最长 120 秒** | 服务端推送结果，不受单次 HTTP 超时限制 |
| HTTP 直连 | 15 秒 | 超时报 `AI error` 回待机 |

超过 120 秒的任务仍然会超时。若要「派完就不管、干完再通知」，需要
`chat.send` 之外的异步任务通道——那是后续工作。

## 操作说明

### 按钮

| 操作 | 功能 |
|------|------|
| 长按 KEY1 | 开始录音 |
| 松开 KEY1 | 停止录音，发送到 AI |
| KEY2 短按 | 在当前分类内切换选项 |
| KEY2 长按 (0.8s) | 切换菜单分类 |

### 菜单分类（KEY2 长按切换）

| 分类 | 短按切换 | 说明 |
|------|---------|------|
| Model | AI 模型名 | 当前可用的 AI 模型（指向 OpenClaw 就选它那一项） |
| Role | 角色预设 | 说话风格 |
| Voice | 音色名 | Edge TTS 中文音色 |
| Orient | 横屏 / 竖屏 | 显示方向 |
| Text | 开 / 关 | 是否显示文字 |

### 状态流转

```
IDLE → RECORDING → PROCESSING → WAITING_REPLY → PLAYING → IDLE
```

- **RECORDING**: 录音中，显示 "listening" 表情
- **PROCESSING**: 火山引擎 STT 语音识别中
- **WAITING_REPLY**: 等待 AI 回复（HTTP 请求进行中）
- **PLAYING**: Edge TTS 语音播放中，显示 "speaking" 表情

### 请求格式

设备发出的是标准 OpenAI Chat Completions 请求，多轮历史一并带上：

```json
{
  "model": "openclaw/default",
  "messages": [
    {"role": "system", "content": "<角色提示词>"},
    {"role": "user", "content": "你好"}
  ],
  "max_tokens": 500
}
```

鉴权头为 `Authorization: Bearer <provider 的 Key>`。
回复取 `choices[0].message.content`，随后交给 TTS 播报，同时按内容推断表情。

## 硬件引脚映射

| 功能 | GPIO | 器件 |
|------|------|------|
| TFT_MOSI | 39 | ST7789P3 |
| TFT_SCLK | 40 | ST7789P3 |
| TFT_RS | 45 | ST7789P3 |
| TFT_CS | 41 | ST7789P3 |
| TFT_RST | 21 | ST7789P3 |
| TFT_BL | 38 | ST7789P3 |
| I2S_MCLK | 18 | ES8311 |
| I2S_DOUT | 14 | ES8311 (麦克风) |
| I2S_BCLK | 17 | ES8311 |
| I2S_WS | 15 | ES8311 |
| I2S_DIN | 16 | ES8311 (扬声器) |
| I2C_SDA | 47 | ES8311 / BMI270 |
| I2C_SCL | 48 | ES8311 / BMI270 |
| KEY1 | 11 | 录音按钮 |
| KEY2 | 12 | 菜单按钮 |
| IR_TX | 46 | 红外发射（预留） |
| IR_RX | 42 | 红外接收（预留） |

## 依赖库

| 库 | 用途 |
|----|------|
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | JSON 处理 |
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) | ST7789 显示驱动 |
| [SparkFun BMI270](https://github.com/sparkfun/SparkFun_BMI270_Arduino_Library) | IMU 传感器 |
| [WebSockets](https://github.com/Links2004/arduinoWebSockets) | 火山 STT + Edge TTS |
| [ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) | 配网门户 |

## 常见问题

### 怎么配 WiFi？两种方式

**方式一（推荐，最省事）**：不填 `ENV_WIFI_SSID`，直接烧录。设备上电后会自动开一个
名为 `Sticks3Claw-Setup` 的 WiFi 热点，用手机或电脑连上，浏览器打开 `192.168.4.1`，
在网页里填 WiFi 名称密码。填完保存到设备，之后自动连接。

**方式二**：复制 `env/secrets.example.h` 为 `env/secrets.h`，填好 `ENV_WIFI_SSID` /
`ENV_WIFI_PASSWORD` 再编译烧录。适合想用 git 管理配置、或需要填多个备选网络的场景。

### 串口没有输出 / 找不到串口
- 确认数据线是**数据线**而非纯充电线（这类线很常见）
- 若芯片识别为 ESP32-C3（`Connecting...` 后一直 `Wrong boot mode`），说明买错了型号，
  本项目需要 **ESP32-S3**
- 找不到端口时按住 BOOT 键再插 USB，或试另一个 USB 口/数据线

### WiFi 连不上
- 用浏览器配网方式可绕过多数问题（见上）
- 确认 2.4GHz 频段——本项目**不支持 5GHz WiFi**
- 配了多个网络时会依次轮询匹配（最多 3 个），间隔约几秒

### 表情不显示 / 屏幕空白
- 表情资源在 LittleFS 里，需要一起烧录。`pio run --target upload` 默认会打包上传；
  若只烧了固件，用 `pio run --target uploadfs` 单独补传文件系统
- 检查屏幕接线（`TFT_RST` / `TFT_BL` 是否接好）

### AI 回复报错
- 检查 `env/secrets.h` 里的 `ENV_AI1_KEY` 是否有效
- 默认模型是 `openrouter/free`，可免费使用；若换成了付费模型需确认账户余额
- 串口会打印 HTTP 状态码和错误信息

### TTS 没有声音
- Edge TTS 需要能访问 `speech.platform.bing.com`，确认网络可达
- 检查音量：菜单里（KEY2 长按切换分类）确认当前音色不是静音类

### 一直重启
- 多半是内存不足。本项目需要 8MB PSRAM 的 ESP32-S3-Pico-**N8R8**；
  若买的是 N4R2 或无 PSRAM 版本，会在初始化时崩溃

### 能不能离线用？
**不能。** 语音识别、语音合成、AI 对话全部走云端，需要联网。
离线状态下设备只会显示表情，不响应语音。

## 许可证

MIT License，见 [LICENSE](LICENSE)。
