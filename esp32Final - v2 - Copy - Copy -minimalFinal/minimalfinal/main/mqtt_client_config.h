
// #pragma once

// #include "esp_err.h"
// #include "mqtt_client.h"

// // ===================== BROKER CONFIG ======================
// #define MQTT_BROKER_URI      "mqtt://bdtmp.ultra-x.jp:1883"   
// #define MQTT_BROKER_USER     "admin"
// #define MQTT_BROKER_PASS     "StrongPassword123"
// #define MQTT_TOPIC_SUB       "text"
// #define MQTT_TOPIC_PUB1      "ESP"
// #define MQTT_TOPIC_PUB2      "ESP2"
// #define MQTT_TOPIC_PUB3      "ESP3"
// #define MQTT_CLIENT_ID       "ESP32_Client"
// #define MQTT_KEEPALIVE       60  // seconds


// #define MQTT_TOPIC_CO2       "CO2"
// #define MQTT_TOPIC_BOWL      "bowl"
// #define MQTT_TOPIC_SONAR     "sonar"
// #define MQTT_TOPIC_SUGAR     "sugar"

// #define MQTT_TOPIC_CO2T      "CO2T"
// #define MQTT_TOPIC_BOWLT     "bowlT"
// #define MQTT_TOPIC_SONART    "sonarT"
// #define MQTT_TOPIC_SUGART    "sugarT"

// // ===================== LWT CONFIG ========================
// #define MQTT_LWT_TOPIC       "esp/test/lwt"
// #define MQTT_LWT_MESSAGE     "ESP32 Disconnected Unexpectedly"
// #define MQTT_LWT_QOS         1
// #define MQTT_LWT_RETAIN      0


// esp_err_t mqtt_start(void);
// void mqtt_publish(const char *topic, const char *msg);

#pragma once

#include "esp_err.h"
#include "mqtt_client.h"

// ===================== BROKER CONFIG ======================
#define MQTT_BROKER_URI      "mqtt://bdtmp.ultra-x.jp:1883"   
#define MQTT_BROKER_USER     "admin"
#define MQTT_BROKER_PASS     "StrongPassword123"
#define MQTT_CLIENT_ID       "ESP32_Client"
#define MQTT_KEEPALIVE       60  // seconds

// ===================== MQTT TOPICS ========================
// Subscribe Topics
#define MQTT_TOPIC_SUB       "text"                    // Fan speed control (0-255)
#define MQTT_TOPIC_TEMP_THRESHOLD    "threshold/temp"      // Temperature threshold
#define MQTT_TOPIC_DISTANCE_THRESHOLD "threshold/distance" // Distance threshold

// Publish Topics - Sensor Data
#define MQTT_TOPIC_PUB1      "ESP"
#define MQTT_TOPIC_PUB2      "ESP2"
#define MQTT_TOPIC_PUB3      "ESP3"

#define MQTT_TOPIC_CO2       "CO2"
#define MQTT_TOPIC_BOWL      "bowl"
#define MQTT_TOPIC_SONAR     "sonar"
#define MQTT_TOPIC_SUGAR     "sugar"

// Publish Topics - Status
#define MQTT_TOPIC_CO2T      "CO2T"
#define MQTT_TOPIC_BOWLT     "bowlT"
#define MQTT_TOPIC_SONART    "sonarT"
#define MQTT_TOPIC_SUGART    "sugarT"

// ===================== LWT CONFIG ========================
#define MQTT_LWT_TOPIC       "esp/test/lwt"
#define MQTT_LWT_MESSAGE     "ESP32 Disconnected Unexpectedly"
#define MQTT_LWT_QOS         1
#define MQTT_LWT_RETAIN      0

// ===================== FUNCTION PROTOTYPES ================
esp_err_t mqtt_start(void);
void mqtt_publish(const char *topic, const char *msg);