
// Arduino          →  ESP32
// TX (Pin 1)       →  GPIO16 (RX2)
// RX (Pin 0)       →  GPIO17 (TX2)
// GND              →  GND

/*
 * ESP32 Serial Bridge to MQTT
 * Receives data from Arduino via Serial and publishes to MQTT
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>

// ===================== PIN DEFINITIONS ======================
// RX2 = GPIO16, TX2 = GPIO17 (Serial2)
#define RXD2 16
#define TXD2 17

// Factory Reset Button
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

// ===================== WIFI PORTAL CONFIG ======================
#define AP_SSID "ESP32-Bridge"
#define AP_PASSWORD ""

// ===================== GLOBAL OBJECTS ======================
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;

// ===================== GLOBAL VARIABLES ======================
struct SensorData {
  float temperature = 0;
  float humidity = 0;
  bool dataReceived = false;
} sensorData;

String ssid = "";
String password = "";
bool wifiConfigured = false;

TaskHandle_t TaskSerial;
TaskHandle_t TaskMQTT;

SemaphoreHandle_t sensorMutex;

// ===================== HTML PAGE ======================
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<meta charset='UTF-8'>
<title>ESP32 Wi-Fi Setup</title>
<style>
body{font-family:Arial;text-align:center;background:#f0f0f0;padding:20px}
.container{max-width:400px;margin:auto;background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
h1{color:#333;margin-bottom:10px}
.subtitle{color:#666;margin-bottom:30px}
input,button{width:100%;padding:12px;margin:10px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box;font-size:16px}
button{background:#4CAF50;color:white;border:none;cursor:pointer;font-weight:bold}
button:hover{background:#45a049}
.info{color:#666;font-size:14px;margin-top:20px}
</style></head><body>
<div class='container'>
<h1>🌐 ESP32 Setup</h1>
<p class='subtitle'>Configure WiFi</p>
<form action='/save' method='POST'>
<input type='text' name='ssid' placeholder='WiFi SSID' required>
<input type='password' name='password' placeholder='Password' required>
<button type='submit'>Save & Restart</button>
</form>
<div class='info'>Device will restart after saving</div>
</div></body></html>
)rawliteral";

// ===================== FUNCTION PROTOTYPES ======================
void setupWiFi();
void setupMQTT();
void factoryResetCheck();
void startCaptivePortal();
void handleRoot();
void handleSave();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
void serialTask(void *param);
void mqttTask(void *param);
String parseSerialData(String data, String key);

// ===================== SETUP ======================
void setup() {
  Serial.begin(115200);  // USB Serial for debugging
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // Serial from Arduino
  delay(1000);
  Serial.println("\n🚀 ESP32 Serial-to-MQTT Bridge Starting...");

  // Initialize NVS
  preferences.begin("wifi", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  wifiConfigured = preferences.getBool("configured", false);

  // Create mutex
  sensorMutex = xSemaphoreCreateMutex();

  // Setup factory reset button
  pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

  // Setup WiFi
  if (wifiConfigured && ssid.length() > 0) {
    Serial.println("✅ WiFi credentials found");
    setupWiFi();
  } else {
    Serial.println("⚙️ No credentials, starting portal...");
    startCaptivePortal();
    return;
  }

  // Setup MQTT
  setupMQTT();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, NULL, 2, &TaskSerial, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 3, &TaskMQTT, 0);

  Serial.println("✅ All tasks started successfully!");
  Serial.println("📡 Waiting for data from Arduino...");
}

// ===================== MAIN LOOP ======================
void loop() {
  factoryResetCheck();
  
  if (!wifiConfigured) {
    server.handleClient();
  }
  
  delay(100);
}

// ===================== SETUP FUNCTIONS ======================
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n⚠️ Failed to connect, starting portal...");
    startCaptivePortal();
  }
}

void setupMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqttReconnect();
}

// ===================== WIFI PORTAL FUNCTIONS ======================
void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  Serial.println("📱 AP Started: " + String(AP_SSID));
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("✅ Web server started");
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("password");

    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);
    preferences.putBool("configured", true);

    Serial.println("✅ Credentials saved: " + newSSID);

    server.send(200, "text/html",
      "<!DOCTYPE html><html><body style='font-family:Arial;text-align:center;padding:50px'>"
      "<h1 style='color:#4CAF50'>✅ Saved!</h1>"
      "<p>Device is restarting...</p>"
      "<p>Please reconnect to your WiFi network</p>"
      "</body></html>");
    
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing SSID or Password");
  }
}

// ===================== FACTORY RESET ======================
void factoryResetCheck() {
  static unsigned long pressStart = 0;
  static bool pressing = false;

  if (digitalRead(FACTORY_RESET_PIN) == LOW) {
    if (!pressing) {
      pressing = true;
      pressStart = millis();
      Serial.println("⚠️ Button pressed, hold for 5 seconds to factory reset...");
    }

    if (millis() - pressStart >= 5000) {
      Serial.println("🔄 Factory Reset Initiated!");
      preferences.clear();
      delay(1000);
      ESP.restart();
    }
  } else {
    if (pressing && (millis() - pressStart < 5000)) {
      Serial.println("Button released (no reset)");
    }
    pressing = false;
  }
}

// ===================== MQTT FUNCTIONS ======================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.printf("📥 MQTT Message [%s]: %s\n", topic, msg.c_str());
}

void mqttReconnect() {
  int retryCount = 0;
  while (!mqtt.connected() && retryCount < 3) {
    Serial.print("Connecting to MQTT broker...");
    
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println(" ✅ Connected");
    } else {
      Serial.print(" ❌ Failed, rc=");
      Serial.println(mqtt.state());
      retryCount++;
      if (retryCount < 3) {
        delay(5000);
      }
    }
  }
}

// ===================== HELPER FUNCTIONS ======================
String parseSerialData(String data, String key) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) return "";
  
  startIndex += key.length();
  int endIndex = data.indexOf(',', startIndex);
  if (endIndex == -1) endIndex = data.length();
  
  return data.substring(startIndex, endIndex);
}

// ===================== FREERTOS TASKS ======================
void serialTask(void *param) {
  String receivedData = "";
  
  for (;;) {
    // Read data from Arduino via Serial2
    while (Serial2.available()) {
      char c = Serial2.read();
      
      if (c == '\n') {
        // Complete line received
        receivedData.trim();
        Serial.println("📡 Received from Arduino: " + receivedData);
        
        // Parse data format: TEMP:25.5,HUM:60.2
        if (receivedData.indexOf("TEMP:") != -1 && receivedData.indexOf("HUM:") != -1) {
          String tempStr = parseSerialData(receivedData, "TEMP:");
          String humStr = parseSerialData(receivedData, "HUM:");
          
          float temp = tempStr.toFloat();
          float hum = humStr.toFloat();
          
          // Update shared data
          xSemaphoreTake(sensorMutex, portMAX_DELAY);
          sensorData.temperature = temp;
          sensorData.humidity = hum;
          sensorData.dataReceived = true;
          xSemaphoreGive(sensorMutex);
          
          Serial.printf("✅ Parsed - Temp: %.1f°C, Humidity: %.1f%%\n", temp, hum);
        } else if (receivedData == "ERROR") {
          Serial.println("⚠️ Arduino reported sensor error");
        }
        
        receivedData = "";
      } else {
        receivedData += c;
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void mqttTask(void *param) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(5000);
  
  for (;;) {
    if (!mqtt.connected()) {
      Serial.println("⚠️ MQTT disconnected, reconnecting...");
      mqttReconnect();
    }
    
    mqtt.loop();

    // Read sensor data
    xSemaphoreTake(sensorMutex, portMAX_DELAY);
    float temp = sensorData.temperature;
    float hum = sensorData.humidity;
    bool hasData = sensorData.dataReceived;
    xSemaphoreGive(sensorMutex);

    if (hasData && mqtt.connected()) {
      char msg[32];
      
      // Publish Temperature
      snprintf(msg, sizeof(msg), "%.1f", temp);
      if (mqtt.publish(MQTT_TOPIC_TEMP, msg)) {
        Serial.printf("📤 Published Temp: %.1f°C → %s\n", temp, MQTT_TOPIC_TEMP);
      }
      
      // Publish Humidity
      snprintf(msg, sizeof(msg), "%.1f", hum);
      if (mqtt.publish(MQTT_TOPIC_HUMIDITY, msg)) {
        Serial.printf("📤 Published Humidity: %.1f%% → %s\n", hum, MQTT_TOPIC_HUMIDITY);
      }
      
      Serial.println("✅ MQTT publish complete\n");
    } else if (!hasData) {
      Serial.println("⚠️ No data from Arduino yet\n");
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}