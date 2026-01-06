/*
 * ESP32 Multi-Sensor IoT System - Arduino Framework
 * 
 * Sensors:
 * - DHT22 (Temperature & Humidity)
 * - DS18B20 (Waterproof Temperature)
 * - MQ135 (Air Quality/Alcohol)
 * - 2x HC-SR04 (Ultrasonic Distance)
 * 
 * Actuators:
 * - Water Pump (via L298N)
 * - Fan Motor (via L298N with speed control)
 * 
 * Features:
 * - WiFi Manager with Captive Portal
 * - MQTT Communication
 * - Factory Reset via Boot Button
 * - FreeRTOS Multi-tasking
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQUnifiedsensor.h>

// ===================== PIN DEFINITIONS ======================
// Sensors
#define DHT_PIN 5
#define DHT_TYPE DHT22
#define DS18B20_PIN 4
#define MQ135_PIN 33
#define SONAR1_TRIG 15
#define SONAR1_ECHO 16
#define SONAR2_TRIG 17
#define SONAR2_ECHO 18

// Motors (L298N)
#define PUMP_ENABLE 13
#define PUMP_IN3 27
#define PUMP_IN4 32
#define MOTOR_ENABLE 14
#define MOTOR_IN1 12
#define MOTOR_IN2 26

// Factory Reset Button
#define FACTORY_RESET_PIN 0  // Boot button

// PWM Configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define PUMP_PWM_CHANNEL 0
#define MOTOR_PWM_CHANNEL 1

// ===================== THRESHOLDS ======================
#define DS18B20_TEMP_THRESHOLD 30.0
#define SONAR1_DISTANCE_THRESHOLD 12.0

// ===================== MQ135 SETUP ======================
#define MQ135_BOARD "ESP32"
#define MQ135_VOLTAGE 3.3
#define MQ135_ADC_BITS 12
#define MQ135_TYPE "MQ-135"
#define MQ135_RL 1.0
#define MQ135_RATIO_CLEAN_AIR 3.6

// ===================== MQTT CONFIG ======================
#define MQTT_BROKER "bdtmp.ultra-x.jp"
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASS "StrongPassword123"
#define MQTT_CLIENT_ID "ESP32_Client"

// ===================== WIFI PORTAL CONFIG ======================
#define AP_SSID "ESP32-Setup"
#define AP_PASSWORD ""  // Open network

// ===================== GLOBAL OBJECTS ======================
DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
MQUnifiedsensor mq135(MQ135_BOARD, MQ135_VOLTAGE, MQ135_ADC_BITS, MQ135_PIN, MQ135_TYPE);

WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;

// ===================== GLOBAL VARIABLES ======================
// Sensor data structures
struct SensorData {
  float dht_temp = 0;
  float dht_humidity = 0;
  float ds18b20_temp = 0;
  float mq135_alcohol = 0;
  float sonar1_distance = 0;
  float sonar2_distance = 0;
} sensorData;

// Control variables
uint8_t targetFanSpeed = 0;
bool pumpState = false;
bool fanState = false;

// WiFi credentials
String ssid = "";
String password = "";
bool wifiConfigured = false;

// Task handles
TaskHandle_t TaskDHT22;
TaskHandle_t TaskDS18B20;
TaskHandle_t TaskMQ135;
TaskHandle_t TaskSonar1;
TaskHandle_t TaskSonar2;
TaskHandle_t TaskControl;
TaskHandle_t TaskMQTT;

// Mutexes for thread-safe data access
SemaphoreHandle_t sensorMutex;

// ===================== HTML PAGE FOR CAPTIVE PORTAL ======================
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
void setupMotors();
void setupSensors();
void factoryResetCheck();
void startCaptivePortal();
void handleRoot();
void handleSave();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttReconnect();
float readUltrasonic(int trigPin, int echoPin);

// ===================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n🚀 ESP32 Multi-Sensor System Starting...");

  // Initialize NVS
  preferences.begin("wifi", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  wifiConfigured = preferences.getBool("configured", false);

  // Create mutex
  sensorMutex = xSemaphoreCreateMutex();

  // Setup factory reset button
  pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
  
  // Setup motors
  setupMotors();
  
  // Setup sensors
  setupSensors();

  // Setup WiFi
  if (wifiConfigured && ssid.length() > 0) {
    Serial.println("✅ WiFi credentials found");
    setupWiFi();
  } else {
    Serial.println("⚙️ No credentials, starting portal...");
    startCaptivePortal();
    return;  // Don't start tasks in portal mode
  }

  // Setup MQTT
  setupMQTT();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(dht22Task, "DHT22Task", 4096, NULL, 2, &TaskDHT22, 1);
  xTaskCreatePinnedToCore(ds18b20Task, "DS18B20Task", 4096, NULL, 2, &TaskDS18B20, 1);
  xTaskCreatePinnedToCore(mq135Task, "MQ135Task", 4096, NULL, 2, &TaskMQ135, 0);
  xTaskCreatePinnedToCore(sonar1Task, "Sonar1Task", 3072, NULL, 2, &TaskSonar1, 0);
  xTaskCreatePinnedToCore(sonar2Task, "Sonar2Task", 3072, NULL, 2, &TaskSonar2, 1);
  xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, NULL, 3, &TaskControl, 0);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 3, &TaskMQTT, 0);

  Serial.println("✅ All tasks started successfully!");
}

// ===================== MAIN LOOP ======================
void loop() {
  // Check for factory reset
  factoryResetCheck();
  
  // Handle captive portal if active
  if (!wifiConfigured) {
    server.handleClient();
  }
  
  delay(100);
}

// ===================== SETUP FUNCTIONS ======================
void setupMotors() {
  // Configure motor control pins
  pinMode(PUMP_IN3, OUTPUT);
  pinMode(PUMP_IN4, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  digitalWrite(PUMP_IN3, LOW);
  digitalWrite(PUMP_IN4, LOW);
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);

  // Setup PWM channels (compatible with Arduino ESP32 Core 3.x)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // New API (ESP32 Core 3.x)
    ledcAttach(PUMP_ENABLE, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(PUMP_ENABLE, 0);
    
    ledcAttach(MOTOR_ENABLE, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(MOTOR_ENABLE, 0);
  #else
    // Old API (ESP32 Core 2.x)
    ledcSetup(PUMP_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PUMP_ENABLE, PUMP_PWM_CHANNEL);
    ledcWrite(PUMP_PWM_CHANNEL, 0);

    ledcSetup(MOTOR_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_ENABLE, MOTOR_PWM_CHANNEL);
    ledcWrite(MOTOR_PWM_CHANNEL, 0);
  #endif

  Serial.println("✅ Motors initialized");
}

void setupSensors() {
  // DHT22
  dht.begin();
  Serial.println("✅ DHT22 initialized");

  // DS18B20
  ds18b20.begin();
  Serial.println("✅ DS18B20 initialized");

  // MQ135
  mq135.setRegressionMethod(1);
  mq135.init();
  mq135.setRL(MQ135_RL);
  
  Serial.print("Calibrating MQ135");
  float calcR0 = 0;
  for (int i = 0; i < 10; i++) {
    mq135.update();
    calcR0 += mq135.calibrate(MQ135_RATIO_CLEAN_AIR);
    Serial.print(".");
    delay(500);
  }
  mq135.setR0(calcR0 / 10);
  Serial.println(" done!");
  Serial.println("✅ MQ135 initialized");

  // Ultrasonic sensors
  pinMode(SONAR1_TRIG, OUTPUT);
  pinMode(SONAR1_ECHO, INPUT);
  pinMode(SONAR2_TRIG, OUTPUT);
  pinMode(SONAR2_ECHO, INPUT);
  Serial.println("✅ Ultrasonic sensors initialized");
}

void setupWiFi() {
  Serial.print("Connecting to WiFi");
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
    Serial.print("IP: ");
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
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("password");

    // Save to preferences
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);
    preferences.putBool("configured", true);

    Serial.println("Credentials saved: " + newSSID);

    server.send(200, "text/html", 
      "<!DOCTYPE html><html><body><h1>✅ Saved!</h1><p>Restarting...</p></body></html>");
    
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
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
      Serial.println("Button pressed, hold for 5 seconds...");
    }

    if (millis() - pressStart >= 5000) {
      Serial.println("🔄 Factory Reset!");
      preferences.clear();
      delay(1000);
      ESP.restart();
    }
  } else {
    pressing = false;
  }
}

// ===================== MQTT FUNCTIONS ======================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  int value = msg.toInt();
  Serial.printf("📥 MQTT [%s]: %d\n", topic, value);

  // Fan speed control via "text" topic
  if (strcmp(topic, "text") == 0) {
    if (value >= 0 && value <= 255) {
      targetFanSpeed = value;
      Serial.printf("🎛️ Fan speed set to: %d\n", value);
    }
  }
  // Other topic handlers
  else if (strcmp(topic, "CO2") == 0) {
    mqtt.publish("CO2T", value > 30 ? "AF" : "CF");
  }
  else if (strcmp(topic, "bowl") == 0) {
    mqtt.publish("bowlT", value > 30 ? "FO" : "FS");
  }
  else if (strcmp(topic, "sonar") == 0) {
    mqtt.publish("sonarT", value > 30 ? "PO" : "PS");
  }
  else if (strcmp(topic, "sugar") == 0) {
    mqtt.publish("sugarT", value < 30 ? "FFC" : "FFO");
  }
}

void mqttReconnect() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println(" ✅ Connected");
      mqtt.subscribe("text");
      mqtt.subscribe("CO2");
      mqtt.subscribe("bowl");
      mqtt.subscribe("sonar");
      mqtt.subscribe("sugar");
    } else {
      Serial.print(" failed, rc=");
      Serial.println(mqtt.state());
      delay(5000);
    }
  }
}

// ===================== SENSOR READING FUNCTIONS ======================
float readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;

  float distance = duration * 0.034 / 2;
  return distance;
}

// ===================== FREERTOS TASKS ======================
void dht22Task(void *param) {
  for (;;) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      xSemaphoreTake(sensorMutex, portMAX_DELAY);
      sensorData.dht_temp = t;
      sensorData.dht_humidity = h;
      xSemaphoreGive(sensorMutex);
      
      Serial.printf("🌡️ DHT22 - Temp: %.1f°C, Humidity: %.1f%%\n", t, h);
    } else {
      Serial.println("⚠️ DHT22 read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void ds18b20Task(void *param) {
  for (;;) {
    ds18b20.requestTemperatures();
    float temp = ds18b20.getTempCByIndex(0);

    if (temp != DEVICE_DISCONNECTED_C) {
      xSemaphoreTake(sensorMutex, portMAX_DELAY);
      sensorData.ds18b20_temp = temp;
      xSemaphoreGive(sensorMutex);
      
      Serial.printf("🔥 DS18B20 - Temp: %.2f°C\n", temp);
    } else {
      Serial.println("⚠️ DS18B20 read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void mq135Task(void *param) {
  for (;;) {
    mq135.update();
    mq135.setA(77.255);
    mq135.setB(-3.18);
    float alcohol = mq135.readSensor();

    if (alcohol >= 0 && !isinf(alcohol)) {
      xSemaphoreTake(sensorMutex, portMAX_DELAY);
      sensorData.mq135_alcohol = alcohol;
      xSemaphoreGive(sensorMutex);
      
      Serial.printf("🍷 MQ135 - Alcohol: %.2f ppm\n", alcohol);
    } else {
      Serial.println("⚠️ MQ135 read failed");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void sonar1Task(void *param) {
  for (;;) {
    float dist = readUltrasonic(SONAR1_TRIG, SONAR1_ECHO);
    
    if (dist > 0) {
      xSemaphoreTake(sensorMutex, portMAX_DELAY);
      sensorData.sonar1_distance = dist;
      xSemaphoreGive(sensorMutex);
      
      Serial.printf("📏 Sonar1 - Distance: %.2f cm\n", dist);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void sonar2Task(void *param) {
  for (;;) {
    float dist = readUltrasonic(SONAR2_TRIG, SONAR2_ECHO);
    
    if (dist > 0) {
      xSemaphoreTake(sensorMutex, portMAX_DELAY);
      sensorData.sonar2_distance = dist;
      xSemaphoreGive(sensorMutex);
      
      Serial.printf("📏 Sonar2 - Distance: %.2f cm\n", dist);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void controlTask(void *param) {
  uint8_t currentFanSpeed = 0;
  
  for (;;) {
    xSemaphoreTake(sensorMutex, portMAX_DELAY);
    float ds_temp = sensorData.ds18b20_temp;
    float sonar1_dist = sensorData.sonar1_distance;
    xSemaphoreGive(sensorMutex);

    // Fan control based on DS18B20 temperature
    if (ds_temp > DS18B20_TEMP_THRESHOLD) {
      if (targetFanSpeed == 0) targetFanSpeed = 128;
      
      if (!fanState || currentFanSpeed != targetFanSpeed) {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
        
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          ledcWrite(MOTOR_ENABLE, targetFanSpeed);
        #else
          ledcWrite(MOTOR_PWM_CHANNEL, targetFanSpeed);
        #endif
        
        fanState = true;
        currentFanSpeed = targetFanSpeed;
        Serial.printf("🌀 Fan ON at speed %d\n", targetFanSpeed);
      }
    } else {
      if (fanState) {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
        
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          ledcWrite(MOTOR_ENABLE, 0);
        #else
          ledcWrite(MOTOR_PWM_CHANNEL, 0);
        #endif
        
        fanState = false;
        currentFanSpeed = 0;
        Serial.println("🌀 Fan OFF");
      }
    }

    // Pump control based on Sonar1
    if (sonar1_dist < SONAR1_DISTANCE_THRESHOLD && !pumpState) {
      digitalWrite(PUMP_IN3, HIGH);
      digitalWrite(PUMP_IN4, LOW);
      
      #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(PUMP_ENABLE, 255);
      #else
        ledcWrite(PUMP_PWM_CHANNEL, 255);
      #endif
      
      pumpState = true;
      Serial.println("💧 Pump ON");
    } else if (sonar1_dist >= SONAR1_DISTANCE_THRESHOLD && pumpState) {
      digitalWrite(PUMP_IN3, LOW);
      digitalWrite(PUMP_IN4, LOW);
      
      #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(PUMP_ENABLE, 0);
      #else
        ledcWrite(PUMP_PWM_CHANNEL, 0);
      #endif
      
      pumpState = false;
      Serial.println("💧 Pump OFF");
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void mqttTask(void *param) {
  for (;;) {
    if (!mqtt.connected()) {
      mqttReconnect();
    }
    mqtt.loop();

    xSemaphoreTake(sensorMutex, portMAX_DELAY);
    float dht_temp = sensorData.dht_temp;
    float dht_hum = sensorData.dht_humidity;
    float ds_temp = sensorData.ds18b20_temp;
    float mq_alcohol = sensorData.mq135_alcohol;
    float sonar1 = sensorData.sonar1_distance;
    float sonar2 = sensorData.sonar2_distance;
    xSemaphoreGive(sensorMutex);

    char msg[32];
    
    // Publish DHT22
    snprintf(msg, sizeof(msg), "%.1f", dht_temp);
    mqtt.publish("ESP", msg);
    
    snprintf(msg, sizeof(msg), "%.1f", dht_hum);
    mqtt.publish("ESP2", msg);

    // Publish DS18B20
    snprintf(msg, sizeof(msg), "%.2f", ds_temp);
    mqtt.publish("ds18", msg);

    // Publish MQ135
    snprintf(msg, sizeof(msg), "%.2f", mq_alcohol);
    mqtt.publish("CO2", msg);

    // Publish Sonar1
    snprintf(msg, sizeof(msg), "%.2f", sonar1);
    mqtt.publish("level", msg);

    // Publish Sonar2
    snprintf(msg, sizeof(msg), "%.2f", sonar2);
    mqtt.publish("sugar", msg);

    Serial.println("📤 MQTT data published");

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}