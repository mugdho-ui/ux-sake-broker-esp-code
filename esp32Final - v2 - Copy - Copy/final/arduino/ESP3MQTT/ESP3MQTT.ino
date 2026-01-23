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
#include "esp_pm.h"

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
void disableAllSleepModes();

// ===================== DISABLE SLEEP MODES ======================
void disableAllSleepModes() {
  // 🔴 Disable ALL sleep and power saving features
  
  // 1. Disable light sleep
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  
  // 2. Disable WiFi power save (MOST IMPORTANT!)
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.setSleep(false);
  
  // 3. Disable automatic light sleep
  #ifdef CONFIG_PM_ENABLE
  esp_pm_config_esp32_t pm_config;
  pm_config.max_freq_mhz = 240;  // Max CPU frequency
  pm_config.min_freq_mhz = 240;  // Keep at max (no throttling)
  pm_config.light_sleep_enable = false;  // Disable light sleep
  esp_pm_configure(&pm_config);
  #endif
  
  // 4. Keep CPU at full speed
  setCpuFrequencyMhz(240);
  
  Serial.println("🔴 ALL SLEEP MODES DISABLED - Running at 240MHz");
}

// ===================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("ESP32 Bridge - NO SLEEP MODE");
  Serial.println("========================================");

  // 🔴 CRITICAL: Disable sleep BEFORE WiFi starts
  disableAllSleepModes();

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
  mqtt.setKeepAlive(15);  // Keep connection alive

  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, NULL, 2, &TaskSerial, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 8192, NULL, 3, &TaskMQTT, 0);

  Serial.println("✅ ESP32 Bridge Running - Never Sleep Mode");
  Serial.println("📊 Waiting for sensor data...\n");
}

// ===================== LOOP ======================
void loop() {
  factoryResetCheck();
  server.handleClient();
  
  // Keep WiFi alive
  if (WiFi.status() != WL_CONNECTED && wifiConfigured) {
    Serial.println("⚠️ WiFi disconnected! Reconnecting...");
    setupWiFi();
  }
  
  delay(10);
}

// ===================== WIFI ======================
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // Force no sleep
  esp_wifi_set_ps(WIFI_PS_NONE);  // Double ensure
  
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("🔌 Connecting to WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    Serial.print("📍 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
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
  Serial.print("📍 AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  preferences.putString("ssid", server.arg("ssid"));
  preferences.putString("password", server.arg("password"));
  preferences.putBool("configured", true);
  server.send(200, "text/html", "✅ Saved! Restarting...");
  delay(2000);
  ESP.restart();
}

// ===================== FACTORY RESET ======================
void factoryResetCheck() {
  static unsigned long t = 0;

  if (digitalRead(FACTORY_RESET_PIN) == LOW) {
    if (!t) {
      t = millis();
      Serial.println("🔴 Factory reset button pressed...");
    }
    if (millis() - t > 5000) {
      Serial.println("🔴 FACTORY RESET!");
      preferences.clear();
      delay(500);
      ESP.restart();
    }
  } else {
    t = 0;
  }
}

// ===================== MQTT ======================
void mqttReconnect() {
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print("🔄 MQTT Connecting... ");
    
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("✅ Connected!");
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.println(mqtt.state());
      attempts++;
      delay(2000);
    }
  }
  
  if (!mqtt.connected()) {
    Serial.println("🔴 MQTT Connection Failed! Will retry...");
  }
}

// ===================== SERIAL TASK ======================
void serialTask(void *p) {
  String line = "";
  unsigned long lastDataTime = 0;
  bool firstData = true;

  for (;;) {
    while (Serial2.available()) {
      char c = Serial2.read();
      if (c == '\n') {
        line.trim();
        
        if (line.length() > 0) {
          Serial.print("📩 Arduino → ");
          Serial.println(line);
          
          if (line.indexOf("TEMP:") >= 0 && line.indexOf("HUM:") >= 0) {
            float t = parseSerialData(line, "TEMP:").toFloat();
            float h = parseSerialData(line, "HUM:").toFloat();

            if (t > -50 && t < 100 && h > 0 && h <= 100) {
              xSemaphoreTake(sensorMutex, portMAX_DELAY);
              sensorData.temperature = t;
              sensorData.humidity = h;
              sensorData.dataReceived = true;
              xSemaphoreGive(sensorMutex);
              
              if (firstData) {
                Serial.println("✅ First valid data received!");
                firstData = false;
              }
              
              lastDataTime = millis();
            } else {
              Serial.println("⚠️ Invalid sensor values!");
            }
          } else if (line.indexOf("ERROR") >= 0) {
            Serial.println("⚠️ Arduino sensor error!");
          }
        }
        line = "";
      } else {
        line += c;
      }
    }
    
    // Check if Arduino stopped sending
    if (lastDataTime > 0 && millis() - lastDataTime > 15000) {
      Serial.println("🔴 No data from Arduino for 15 seconds!");
      lastDataTime = millis();
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ===================== MQTT TASK ======================
void mqttTask(void *p) {
  unsigned long failTime = 0;
  unsigned long lastPublish = 0;
  static float lastTemp = 0;
  static float lastHum = 0;
  int publishCount = 0;

  for (;;) {
    // Check MQTT connection
    if (!mqtt.connected()) {
      Serial.println("🔴 MQTT Disconnected!");
      mqttReconnect();
      failTime = millis();
    }

    mqtt.loop();

    // Get sensor data
    xSemaphoreTake(sensorMutex, portMAX_DELAY);
    bool ready = sensorData.dataReceived;
    float t = sensorData.temperature;
    float h = sensorData.humidity;
    if (ready) {
      sensorData.dataReceived = false;
      lastTemp = t;
      lastHum = h;
    }
    xSemaphoreGive(sensorMutex);

    // Publish if new data available
    if (ready) {
      char buf[16];
      
      // Publish Temperature (NO RETAINED FLAG!)
      snprintf(buf, sizeof(buf), "%.1f", t);
      bool pub1 = mqtt.publish(MQTT_TOPIC_TEMP, buf, false);
      
      // Publish Humidity (NO RETAINED FLAG!)
      snprintf(buf, sizeof(buf), "%.1f", h);
      bool pub2 = mqtt.publish(MQTT_TOPIC_HUMIDITY, buf, false);
      
      if (pub1 && pub2) {
        publishCount++;
        Serial.printf("📤 [%d] Published → TEMP: %.1f°C | HUM: %.1f%%\n", 
                      publishCount, t, h);
      } else {
        Serial.println("❌ Publish failed!");
      }
      
      lastPublish = millis();
    }

    // Warning if no new data for too long
    if (lastPublish > 0 && millis() - lastPublish > 10000) {
      Serial.printf("⚠️ No new data for 10s (Last: %.1f°C, %.1f%%)\n", 
                    lastTemp, lastHum);
      lastPublish = millis();
    }

    // HARD FAIL SAFE - Restart if MQTT down for 5 minutes
    if (!mqtt.connected() && millis() - failTime > 300000) {
      Serial.println("🔴 MQTT failed for 5 mins. Restarting ESP32...");
      ESP.restart();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));  // Check every 1 second
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