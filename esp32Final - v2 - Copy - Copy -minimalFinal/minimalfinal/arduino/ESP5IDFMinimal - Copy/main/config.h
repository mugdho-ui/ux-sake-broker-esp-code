/**
 * @file config.h
 * @brief Global configuration and definitions
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ===================== PIN DEFINITIONS ======================
#define RXD2 16
#define TXD2 17
#define FACTORY_RESET_PIN GPIO_NUM_0
#define LED_BUILTIN GPIO_NUM_2  // ✅ Built-in LED (Blue LED on most ESP32 boards)

// ===================== UART CONFIG ======================
#define UART_NUM UART_NUM_2
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 2048

// ===================== MQTT CONFIG ======================
#define MQTT_BROKER_URI "mqtt://bdtmp.ultra-x.jp:1883"
#define MQTT_BROKER_IP "153.127.60.247"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASS "StrongPassword123"
#define MQTT_CLIENT_ID "ESP32_Bridge"
#define MQTT_KEEPALIVE 60
#define MQTT_TIMEOUT_MS 15000
#define MQTT_RECONNECT_INTERVAL_MS 15000

// MQTT Topics
#define MQTT_TOPIC_TEMP "ESPX"
#define MQTT_TOPIC_HUMIDITY "ESP2X"

// ✅ MQTT QoS and Retain settings
#define MQTT_QOS 0          // QoS 0 = At most once (faster, no duplicates)
#define MQTT_RETAIN 0       // Don't retain messages

// ===================== WIFI PORTAL CONFIG ======================
#define AP_SSID "Room-Bridge"
#define AP_PASSWORD ""
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64
#define WIFI_CONNECT_TIMEOUT_MS 10000  // ✅ Reduced from 15000 to 10000 (fail faster)

// ===================== TIMING CONFIG ======================
#define FACTORY_RESET_HOLD_MS 5000
#define SERIAL_DATA_TIMEOUT_MS 15000
#define WIFI_RETRY_DELAY_MS 500
#define WIFI_MAX_RETRIES 30

// ✅ Minimum publish interval to prevent flooding (1 second)
#define MIN_PUBLISH_INTERVAL_MS 1000

// ===================== SENSOR DATA STRUCTURE ======================
typedef struct {
    float temperature;
    float humidity;
    bool dataReceived;
} SensorData;

// ===================== GLOBAL VARIABLES ======================
extern SensorData sensorData;
extern SemaphoreHandle_t sensorMutex;

// ===================== TASK PRIORITIES ======================
#define PRIORITY_SERIAL_TASK 2
#define PRIORITY_MQTT_TASK 3
#define PRIORITY_LED_TASK 1
#define PRIORITY_MONITOR_TASK 1

// ===================== TASK STACK SIZES ======================
#define STACK_SIZE_SERIAL_TASK 4096
#define STACK_SIZE_MQTT_TASK 8192
#define STACK_SIZE_LED_TASK 2048
#define STACK_SIZE_MONITOR_TASK 2048

#endif // CONFIG_H