#include "wifi_portal.h"
#include "config/app_config.h"
#include <ArduinoJson.h>

// 配网页面 HTML（内嵌 PROGMEM）
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sticks3Claw Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px}
h1{color:#00d4ff;font-size:1.5em;margin-bottom:16px;text-align:center}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px}
.card h2{color:#00d4ff;font-size:1.1em;margin-bottom:8px}
input,select{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#0f3460;color:#fff;font-size:16px;margin-bottom:8px}
button{width:100%;padding:12px;background:#00d4ff;color:#000;border:none;border-radius:8px;font-size:16px;font-weight:bold;cursor:pointer;margin-top:4px}
button:disabled{background:#555;cursor:default}
.wifi-item{padding:10px;border:1px solid #333;border-radius:8px;margin-bottom:4px;cursor:pointer;display:flex;justify-content:space-between}
.wifi-item:hover{background:#0f3460}
.rssi{color:#888;font-size:0.8em}
#status{text-align:center;padding:12px;border-radius:8px;margin-top:8px;display:none}
.ok{background:#0a4d2e;color:#0f0}
.err{background:#4d0a0a;color:#f00}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid #fff;border-top-color:transparent;border-radius:50%;animation:spin .8s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
</style></head><body>
<h1>Sticks3Claw</h1>
<div class="card"><h2>WiFi</h2>
<div id="scanResult"><p>Scanning...</p></div>
<div id="wifiForm" style="display:none">
<input type="text" id="ssid" placeholder="WiFi name">
<input type="password" id="pass" placeholder="Password">
<button onclick="connect()">Connect</button>
</div></div>
<div id="status"></div>
<script>
let networks=[];
fetch('/scan').then(r=>r.json()).then(d=>{networks=d;render()});
function render(){
  let h='';networks.sort((a,b)=>b.rssi-a.rssi);
  networks.forEach(n=>{
    let lock=n.secure?'&#128274;':'';
    let q=n.rssi>-50?'Excellent':n.rssi>-60?'Good':n.rssi>-70?'Fair':'Weak';
    h+=`<div class="wifi-item" onclick="pick('${n.ssid}')">${lock} ${n.ssid}<span class="rssi">${q}</span></div>`;
  });
  document.getElementById('scanResult').innerHTML=h||'<p>No networks found</p>';
  document.getElementById('wifiForm').style.display='block';
}
function pick(s){document.getElementById('ssid').value=s;document.getElementById('pass').focus()}
function connect(){
  let s=document.getElementById('ssid').value,p=document.getElementById('pass').value;
  if(!s)return;
  show('Connecting...','');document.querySelector('button').disabled=true;
  fetch('/connect?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p))
  .then(r=>r.json()).then(d=>{
    if(d.ok){show('Connected! IP: '+d.ip,'ok');setTimeout(()=>{window.location='http://'+d.ip+'/settings'},2000)}
    else{show('Failed: '+d.error,'err');document.querySelector('button').disabled=false}
  }).catch(()=>{show('Connection error','err');document.querySelector('button').disabled=false});
}
function show(msg,cls){
  let s=document.getElementById('status');s.style.display='block';
  s.className=cls||'';s.innerHTML=cls==='ok'||cls==='err'?msg:'<span class="spinner"></span> '+msg;
}
</script></body></html>
)rawliteral";

// 设置页面 HTML
static const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sticks3Claw Settings</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px}
h1{color:#00d4ff;font-size:1.5em;margin-bottom:16px;text-align:center}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px}
.card h2{color:#00d4ff;font-size:1.1em;margin-bottom:8px}
input,select,textarea{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#0f3460;color:#fff;font-size:14px;margin-bottom:8px}
textarea{resize:vertical;min-height:60px}
button{width:100%;padding:12px;background:#00d4ff;color:#000;border:none;border-radius:8px;font-size:16px;font-weight:bold;cursor:pointer}
label{display:block;color:#aaa;font-size:0.85em;margin-bottom:2px}
#msg{text-align:center;padding:8px;border-radius:8px;margin-top:8px;display:none}
</style></head><body>
<h1>Settings</h1>
<div class="card"><h2>AI Provider</h2>
<label>Mode</label><select id="mode"><option value="0">MQTT (OpenClaw)</option><option value="1">HTTP API</option></select>
<label>Active Provider</label><select id="active"><option value="1">Provider 1</option><option value="2">Provider 2</option></select>
<label>Name</label><input id="p_name">
<label>API URL</label><input id="p_url">
<label>API Key</label><input id="p_key" type="password">
<label>Model</label><input id="p_model">
<label>System Prompt</label><textarea id="p_prompt"></textarea>
</div>
<div class="card"><h2>MQTT</h2>
<label>Broker Host</label><input id="mqtt_host">
<label>Port</label><input id="mqtt_port" type="number">
<label>Inbound Topic</label><input id="mqtt_in">
<label>Outbound Topic</label><input id="mqtt_out">
</div>
<div class="card"><h2>STT (Volcengine)</h2>
<label>App ID</label><input id="stt_appid">
<label>Token</label><input id="stt_token" type="password">
<label>Cluster</label><input id="stt_cluster">
</div>
<div class="card"><h2>System</h2>
<label>Brightness (0-100)</label><input id="brightness" type="number">
<label>Volume (0-100)</label><input id="volume" type="number">
<label>Screen Off Timeout (ms)</label><input id="screen_off" type="number">
<label>Sleep Timeout (ms)</label><input id="sleep" type="number">
</div>
<button onclick="save()">Save All</button>
<div id="msg"></div>
<script>
fetch('/api/settings').then(r=>r.json()).then(d=>{
  document.getElementById('mode').value=d.mode||'0';
  document.getElementById('active').value=d.active||'1';
  if(d.provider){
    document.getElementById('p_name').value=d.provider.name||'';
    document.getElementById('p_url').value=d.provider.url||'';
    document.getElementById('p_key').value=d.provider.key||'';
    document.getElementById('p_model').value=d.provider.model||'';
    document.getElementById('p_prompt').value=d.provider.prompt||'';
  }
  document.getElementById('mqtt_host').value=d.mqtt_host||'';
  document.getElementById('mqtt_port').value=d.mqtt_port||1883;
  document.getElementById('mqtt_in').value=d.mqtt_in||'';
  document.getElementById('mqtt_out').value=d.mqtt_out||'';
  document.getElementById('stt_appid').value=d.stt_appid||'';
  document.getElementById('stt_token').value=d.stt_token||'';
  document.getElementById('stt_cluster').value=d.stt_cluster||'';
  document.getElementById('brightness').value=d.brightness||100;
  document.getElementById('volume').value=d.volume||80;
  document.getElementById('screen_off').value=d.screen_off||30000;
  document.getElementById('sleep').value=d.sleep||300000;
});
function save(){
  let p={
    mode:document.getElementById('mode').value,
    active:document.getElementById('active').value,
    p_name:document.getElementById('p_name').value,
    p_url:document.getElementById('p_url').value,
    p_key:document.getElementById('p_key').value,
    p_model:document.getElementById('p_model').value,
    p_prompt:document.getElementById('p_prompt').value,
    mqtt_host:document.getElementById('mqtt_host').value,
    mqtt_port:document.getElementById('mqtt_port').value,
    mqtt_in:document.getElementById('mqtt_in').value,
    mqtt_out:document.getElementById('mqtt_out').value,
    stt_appid:document.getElementById('stt_appid').value,
    stt_token:document.getElementById('stt_token').value,
    stt_cluster:document.getElementById('stt_cluster').value,
    brightness:document.getElementById('brightness').value,
    volume:document.getElementById('volume').value,
    screen_off:document.getElementById('screen_off').value,
    sleep:document.getElementById('sleep').value
  };
  let f=new FormData();f.append('data',JSON.stringify(p));
  fetch('/api/settings',{method:'POST',body:f}).then(r=>r.json()).then(d=>{
    let m=document.getElementById('msg');m.style.display='block';
    m.style.background=d.ok?'#0a4d2e':'#4d0a0a';m.textContent=d.ok?'Saved!':'Error';
    setTimeout(()=>{m.style.display='none'},3000);
  });
}
</script></body></html>
)rawliteral";

WiFiPortal::WiFiPortal() : _running(false), _server(nullptr), _dnsServer(nullptr) {}

WiFiPortal::~WiFiPortal() { stop(); }

bool WiFiPortal::start() {
    if (_running) return true;

    Serial.println("WiFiPortal: Starting AP mode...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Sticks3Claw-Setup");

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("WiFiPortal: AP started at %s\n", ip.toString().c_str());

    _dnsServer = new DNSServer();
    _dnsServer->start(53, "*", ip);

    _server = new AsyncWebServer(80);
    setupRoutes();
    _server->begin();

    _running = true;
    return true;
}

void WiFiPortal::stop() {
    if (!_running) return;

    if (_server) { _server->end(); delete _server; _server = nullptr; }
    if (_dnsServer) { _dnsServer->stop(); delete _dnsServer; _dnsServer = nullptr; }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _running = false;
    Serial.println("WiFiPortal: Stopped");
}

bool WiFiPortal::isRunning() { return _running; }

void WiFiPortal::setOnConnected(std::function<void()> cb) { _connectedCallback = cb; }

void WiFiPortal::setupRoutes() {
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { handleRoot(req); });
    _server->on("/scan", HTTP_GET, [this](AsyncWebServerRequest* req) { handleScan(req); });
    _server->on("/connect", HTTP_GET, [this](AsyncWebServerRequest* req) { handleConnect(req); });
    _server->on("/status", HTTP_GET, [this](AsyncWebServerRequest* req) { handleStatus(req); });
    _server->on("/settings", HTTP_GET, [this](AsyncWebServerRequest* req) { handleSettingsPage(req); });
    _server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
        handleStatus(req);
    });
    _server->on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest* req) {
        handleSaveSettings(req);
    });
    _server->onNotFound([](AsyncWebServerRequest* req) {
        req->redirect("/");
    });
}

void WiFiPortal::handleRoot(AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", PORTAL_HTML);
}

void WiFiPortal::handleScan(AsyncWebServerRequest* request) {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                ",\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
}

void WiFiPortal::handleConnect(AsyncWebServerRequest* request) {
    if (!request->hasParam("ssid")) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"No SSID\"}");
        return;
    }

    String ssid = request->getParam("ssid")->value();
    String pass = request->hasParam("pass") ? request->getParam("pass")->value() : "";

    Serial.printf("WiFiPortal: Connecting to %s...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        AppConfig::instance().addWiFi(ssid, pass);
        String ip = WiFi.localIP().toString();
        Serial.printf("WiFiPortal: Connected! IP=%s\n", ip.c_str());
        request->send(200, "application/json", "{\"ok\":true,\"ip\":\"" + ip + "\"}");
        // 发送响应后再关闭 portal
        delay(500);
        stop();
        if (_connectedCallback) _connectedCallback();
    } else {
        request->send(200, "application/json", "{\"ok\":false,\"error\":\"Timeout\"}");
        WiFi.mode(WIFI_AP_STA);
    }
}

void WiFiPortal::handleStatus(AsyncWebServerRequest* request) {
    auto& cfg = AppConfig::instance();
    int active = cfg.getActiveProvider();
    auto prov = cfg.getProvider(active);
    String json = "{";
    json += "\"mode\":" + String(cfg.getCommMode());
    json += ",\"active\":" + String(active);
    json += ",\"provider\":{\"name\":\"" + prov.name + "\",\"url\":\"" + prov.url + "\",\"key\":\"" + prov.key + "\",\"model\":\"" + prov.model + "\",\"prompt\":\"" + prov.prompt + "\"}";
    json += ",\"mqtt_host\":\"" + cfg.getMqttHost() + "\"";
    json += ",\"mqtt_port\":" + String(cfg.getMqttPort());
    json += ",\"mqtt_in\":\"" + cfg.getMqttInboundTopic() + "\"";
    json += ",\"mqtt_out\":\"" + cfg.getMqttOutboundTopic() + "\"";
    json += ",\"stt_appid\":\"" + cfg.getSttAppId() + "\"";
    json += ",\"stt_token\":\"" + cfg.getSttToken() + "\"";
    json += ",\"stt_cluster\":\"" + cfg.getSttCluster() + "\"";
    json += ",\"brightness\":" + String(cfg.getBrightness());
    json += ",\"volume\":" + String(cfg.getVolume());
    json += ",\"screen_off\":" + String(cfg.getScreenOffTimeout());
    json += ",\"sleep\":" + String(cfg.getSleepTimeout());
    json += "}";
    request->send(200, "application/json", json);
}

void WiFiPortal::handleSettingsPage(AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", SETTINGS_HTML);
}

void WiFiPortal::handleSaveSettings(AsyncWebServerRequest* request) {
    if (!request->hasParam("data", true)) {
        request->send(400, "application/json", "{\"ok\":false}");
        return;
    }

    String data = request->getParam("data", true)->value();
    JsonDocument doc;
    if (deserializeJson(doc, data)) {
        request->send(400, "application/json", "{\"ok\":false}");
        return;
    }

    auto& cfg = AppConfig::instance();
    cfg.setCommMode(doc["mode"].as<int>());
    cfg.setActiveProvider(doc["active"].as<int>());

    AIProviderConfig prov;
    prov.name = doc["p_name"].as<String>();
    prov.url = doc["p_url"].as<String>();
    prov.key = doc["p_key"].as<String>();
    prov.model = doc["p_model"].as<String>();
    prov.prompt = doc["p_prompt"].as<String>();
    cfg.setProvider(cfg.getActiveProvider(), prov);

    cfg.setMqttHost(doc["mqtt_host"].as<String>());
    cfg.setMqttPort(doc["mqtt_port"].as<int>());

    cfg.setSttCredentials(doc["stt_appid"].as<String>(), doc["stt_token"].as<String>(), doc["stt_cluster"].as<String>());

    cfg.setBrightness(doc["brightness"].as<int>());
    cfg.setVolume(doc["volume"].as<int>());
    cfg.setScreenOffTimeout(doc["screen_off"].as<int>());
    cfg.setSleepTimeout(doc["sleep"].as<int>());

    Serial.println("WiFiPortal: Settings saved");
    request->send(200, "application/json", "{\"ok\":true}");
}
