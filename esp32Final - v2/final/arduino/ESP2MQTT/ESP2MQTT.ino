// Arduino          →  ESP32
// TX (Pin 1)       →  GPIO16 (RX2)
// RX (Pin 0)       →  GPIO17 (TX2)
// GND              →  GND

#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include "esp_wifi.h"
#include "esp_sleep.h"

// ===================== PIN DEFINITIONS ======================
#define RXD2 16
#define TXD2 17
#define FACTORY_RESET_PIN 0

// ===================== MQTT CONFIG ======================
#define MQTT_BROKER "bdtmp.ultra-x.jp"
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASS "StrongPassword123"
#define MQTT_CLIENT_ID "ESP32_Bridge"

// MQTT Topics
#define MQTT_TOPIC_TEMP "ESPX"
#define MQTT_TOPIC_HUMIDITY "ESP2X"

// ===================== WIFI PORTAL ======================
#define AP_SSID "ESP32-Bridge"
#define AP_PASSWORD ""

// ===================== OBJECTS ======================
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;

// ===================== DATA STRUCT ======================
struct SensorData {
  float temperature;
  float humidity;
  bool dataReceived;
};

SensorData sensorData = {0, 0, false};
SemaphoreHandle_t sensorMutex;

// ===================== WIFI VARS ======================
String ssid = "";
String password = "";
bool wifiConfigured = false;

// ===================== TASK HANDLES ======================
TaskHandle_t TaskSerial;
TaskHandle_t TaskMQTT;

// ===================== HTML ======================
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>ESP32 WiFi Setup</title>
</head><body style='font-family:Arial;text-align:center;padding:30px'>
<h2>ESP32 WiFi Setup</h2>
<form action='/save' method='POST'>
<input name='ssid' placeholder='SSID' required><br><br>
<input name='password' type='password' placeholder='Password' required><br><br>
<button type='submit'>Save & Restart</button>
</form>
</body></html>
)rawliteral";

// ===================== PROTOTYPES ======================
void setupWiFi();
void startCaptivePortal();
void handleRoot();
void handleSave();
void mqttReconnect();
void serialTask(void *);
void mqttTask(void *);
void factoryResetCheck();
String parseSerialData(String data, String key);

// ===================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000);

  // 🔴 DISABLE ALL SLEEP MODES
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.setSleep(false);



  preferences.begin("wifi", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  wifiConfigured = preferences.getBool("configured", false);

  pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

  sensorMutex = xSemaphoreCreateMutex();

  if (wifiConfigured && ssid.length()) {
    setupWiFi();
  } else {
    startCaptivePortal();
    return;
  }

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, NULL, 2, &TaskSerial, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 3, &TaskMQTT, 0);

  Serial.println("✅ ESP32 Bridge Running");
}

// ===================== LOOP ======================
void loop() {
  factoryResetCheck();
  server.handleClient();
  delay(10);
}

// ===================== WIFI ======================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting to WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    Serial.println(WiFi.localIP());
  } else {
    startCaptivePortal();
  }
}

// ===================== PORTAL ======================
void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  Serial.println("⚙️ WiFi Setup Portal Started");
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("password", server.arg("password"));
  preferences.putBool("configured", true);
  server.send(200, "text/html", "Saved. Restarting...");
  delay(2000);
  ESP.restart();
}

// ===================== FACTORY RESET ======================
void factoryResetCheck() {
  static unsigned long t = 0;

  if (digitalRead(FACTORY_RESET_PIN) == LOW) {
    if (!t) t = millis();
    if (millis() - t > 5000) {
      preferences.clear();
      ESP.restart();
    }
  } else {
    t = 0;
  }
}

// ===================== MQTT ======================
void mqttReconnect() {
  while (!mqtt.connected()) {
    mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
    delay(2000);
  }
}

// ===================== SERIAL TASK ======================
void serialTask(void *p) {
  String line = "";

  for (;;) {
    while (Serial2.available()) {
      char c = Serial2.read();
      if (c == '\n') {
        line.trim();
        if (line.indexOf("TEMP:") >= 0 && line.indexOf("HUM:") >= 0) {
          float t = parseSerialData(line, "TEMP:").toFloat();
          float h = parseSerialData(line, "HUM:").toFloat();

          xSemaphoreTake(sensorMutex, portMAX_DELAY);
          sensorData.temperature = t;
          sensorData.humidity = h;
          sensorData.dataReceived = true;
          xSemaphoreGive(sensorMutex);
        }
        line = "";
      } else {
        line += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ===================== MQTT TASK ======================
void mqttTask(void *p) {
  unsigned long failTime = 0;

  for (;;) {
    if (!mqtt.connected()) {
      mqttReconnect();
      failTime = millis();
    }

    mqtt.loop();

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
    bool ready = sensorData.dataReceived;
    float t = sensorData.temperature;
    float h = sensorData.humidity;
    if (ready) sensorData.dataReceived = false;
    xSemaphoreGive(sensorMutex);

    if (ready) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%.1f", t);
      mqtt.publish(MQTT_TOPIC_TEMP, buf, true);

      snprintf(buf, sizeof(buf), "%.1f", h);
      mqtt.publish(MQTT_TOPIC_HUMIDITY, buf, true);
    }

    // HARD FAIL SAFE
    if (!mqtt.connected() && millis() - failTime > 300000) {
      ESP.restart();
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// ===================== PARSER ======================
String parseSerialData(String d, String k) {
  int s = d.indexOf(k);
  if (s < 0) return "";
  s += k.length();
  int e = d.indexOf(',', s);
  if (e < 0) e = d.length();
  return d.substring(s, e);
}
