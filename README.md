# Sticks3 ESP32-S3 语音助手

基于 ESP32-S3-Pico-1-N8R8 的智能语音助手固件，支持语音对话、多 AI 模型切换、Edge TTS 语音合成、MQTT 局域网通信。

## 硬件规格

- **主控**: ESP32-S3-Pico-1-N8R8 (8MB Flash + 8MB PSRAM)
- **音频**: ES8311 音频编解码 + MEMS 麦克风 + 扬声器 (I2S 16kHz 16-bit Mono)
- **显示**: 1.14寸 135x240 IPS LCD (ST7789P3, SPI)
- **传感器**: BMI270 六轴 IMU
- **通信**: WiFi (支持多网络自动匹配)
- **其他**: 物理按钮 KEY1/KEY2、红外收发（预留）

## 功能

- **语音对话**: 长按 KEY1 录音 → 火山引擎 STT 识别 → AI 回复 → Edge TTS 播放
- **双通信模式**: MQTT（局域网对接 OpenClaw）/ HTTP API（直连第三方 AI）
- **多 AI 模型**: 通过 KEY2 切换 OpenRouter、Deepseek 等模型，支持自定义 URL/Key/模型/提示词
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
┌──────────────────────────────────────────────────┐
│               AI / OpenClaw                       │
│  ┌─────────────┐    ┌────────────────────────┐  │
│  │ MQTT Broker  │    │ HTTP API               │  │
│  │ (OpenClaw)   │    │ OpenRouter/Deepseek/.. │  │
│  └──────┬───────┘    └───────────┬────────────┘  │
└─────────┼────────────────────────┼────────────────┘
          │ MQTT                    │ HTTPS
┌─────────┼────────────────────────┼────────────────┐
│         ↓          ESP32-S3      ↓                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ 火山 STT │→ │ AI 处理   │→ │ Edge TTS │→ 播放 │
│  └──────────┘  └──────────┘  └──────────┘        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ ES8311   │  │ ST7789   │  │ BMI270   │        │
│  │ 录音/播放│  │ TFT 显示 │  │ 陀螺仪   │        │
│  └──────────┘  └──────────┘  └──────────┘        │
└──────────────────────────────────────────────────┘
```

## 项目结构

```
sticks3claw/
├── config.h                         # 硬件引脚、MQTT、TTS 音色、系统参数
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
│   │   ├── mqtt_client.h/cpp        # MQTT 客户端 (PubSubClient)
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
- MQTT Broker 地址和端口
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

// ---------- MQTT（可选，仅接入 OpenClaw 时需要）----------
// 填运行 OpenClaw 那台电脑的局域网 IP，不是 127.0.0.1
#define ENV_MQTT_HOST "192.168.x.x"
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

### 4. OpenClaw MQTT 对接（可选）

MQTT 模式下设备不直接调 AI，而是把话发给局域网里的 OpenClaw，由它回复。

**链路上有三方，缺一不可：**

```
ESP32 ──MQTT──► MQTT Broker (Mosquitto) ◄──MQTT── OpenClaw + mqtt 插件 ──► AI
                （必须自己跑，不随插件自带）
```

1. **装并启动一个 MQTT Broker**（这一步最容易漏，插件本身**不带** broker）：

   ```bash
   # macOS
   brew install mosquitto && brew services start mosquitto
   ```
   默认监听 `1883`，默认允许匿名连接（无需用户名密码，与本项目一致）。

2. **安装 OpenClaw 的 MQTT 插件**：

   ```bash
   openclaw plugins install @turquoisebay/mqtt
   ```

   > ⚠️ 包名在 v0.1.12 后已从 `@turquoisebay/openclaw-mqtt` 改名为 **`@turquoisebay/mqtt`**。
   > 旧名仍可装但停在 0.1.12，请用新名。

3. **配置插件**（`~/.openclaw/openclaw.json`）：

   ```json
   {
     "channels": {
       "mqtt": {
         "brokerUrl": "mqtt://localhost:1883",
         "topics": { "inbound": "openclaw/inbound", "outbound": "openclaw/outbound" },
         "qos": 1
       }
     }
   }
   ```
   然后 `openclaw gateway restart`。

4. **设备上填 broker 地址**：`env/secrets.h` 的 `ENV_MQTT_HOST` 填**运行 OpenClaw 那台电脑的局域网 IP**
   （如 `192.168.1.20`），**不是** `127.0.0.1`，也不能填设备自己的地址。

5. 设备上用 KEY2 菜单切到 `Mode = MQTT`。启动后串口应出现 `MQTT connected!` 与
   `Subscribed to openclaw/outbound: OK`。

**Topic 与消息格式**（与插件默认一致，通常无需改）：

| 方向 | Topic | 载荷 |
|---|---|---|
| 设备 → OpenClaw | `openclaw/inbound` | `{"senderId":"sticks3","text":"你好","correlationId":"msg_1"}` |
| OpenClaw → 设备 | `openclaw/outbound` | `{"senderId":"openclaw","text":"...","kind":"final","ts":...}` |

**已知限制**：

- **所有设备共用同一个会话**。插件按 `senderId` 分组会话（`mqtt:{senderId}`），
  而本项目 `senderId` 是固定值 `sticks3`（不带 MAC）。若同时跑多台设备，
  它们的对话历史会在 OpenClaw 侧混在一起。想要独立记忆，需自行把 `senderId`
  改成带 MAC 后缀。
- **回复里的 `emotion` 字段对不上**。插件返回 `kind`/`ts`，没有 `emotion`；
  本项目读 `msg["emotion"]` 拿不到时会退回用 `MoodDetector` 从文本推断情绪。
  功能仍然可用，只是少了一层由 OpenClaw 精确指定表情的能力。

**安全提醒**：任何能往 `openclaw/inbound` 发消息的设备都能驱动你的 Agent。
MQTT 只在受信任的内网使用——不要把 broker 端口暴露到公网。

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
| Mode | MQTT / API | 通信模式 |
| Model | AI 模型名 | 当前可用的 AI 模型 |
| Voice | 音色名 | Edge TTS 中文音色 |

### 状态流转

```
IDLE → RECORDING → PROCESSING → WAITING_REPLY → PLAYING → IDLE
```

- **RECORDING**: 录音中，显示 "listening" 表情
- **PROCESSING**: 火山引擎 STT 语音识别中
- **WAITING_REPLY**: 等待 AI 回复（MQTT 模式等待 Broker 推送 / API 模式等待 HTTP 响应）
- **PLAYING**: Edge TTS 语音播放中，显示 "speaking" 表情

### MQTT 消息格式

> 以下格式以 `src/communication/mqtt_client.cpp` 的实际实现为准。

**ESP32 → OpenClaw** (`openclaw/inbound`):
```json
{
  "senderId": "sticks3",
  "text": "你好",
  "correlationId": "msg_123"
}
```

`correlationId` 由设备自动生成，用于请求-响应配对。多台设备同时接入时，靠 topic 区分或自行扩展 `senderId`（当前实现为固定值，非 MAC 派生）。

**OpenClaw → ESP32** (`openclaw/outbound`):
```json
{
  "text": "你好呀，有什么可以帮你的？",
  "emotion": "happy"
}
```

**事件** (ESP32 → OpenClaw):

摇晃事件实际嵌套在 `metadata` 下，与文本消息结构不同：
```json
{
  "senderId": "sticks3",
  "correlationId": "msg_124",
  "metadata": {
    "event": "shake",
    "timestamp": 12345,
    "accel": { "x": 0.1, "y": 0.2, "z": 9.8 }
  }
}
```

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
| [PubSubClient](https://github.com/knolleary/pubsubclient) | MQTT 通信 |

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
