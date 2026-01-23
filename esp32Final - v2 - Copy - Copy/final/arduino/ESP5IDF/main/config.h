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
#define LED_BUILTIN GPIO_NUM_2

// ===================== UART CONFIG ======================
#define UART_NUM UART_NUM_2
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 2048

// ===================== MQTT CONFIG ======================
#define MQTT_BROKER_URI "mqtt://153.127.60.247:1883"
#define MQTT_BROKER_IP "153.127.60.247"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASS "StrongPassword123"
#define MQTT_CLIENT_ID "ESP32_Bridge"
#define MQTT_KEEPALIVE 15
#define MQTT_TIMEOUT_MS 10000

// MQTT Topics
#define MQTT_TOPIC_TEMP "ESPX"
#define MQTT_TOPIC_HUMIDITY "ESP2X"

// ===================== WIFI PORTAL CONFIG ======================
#define AP_SSID "Room-Bridge"
#define AP_PASSWORD ""
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64
#define WIFI_CONNECT_TIMEOUT_MS 15000

// ===================== TIMING CONFIG ======================
#define FACTORY_RESET_HOLD_MS 5000
#define MQTT_RECONNECT_INTERVAL_MS 10000
#define SERIAL_DATA_TIMEOUT_MS 15000
#define WIFI_RETRY_DELAY_MS 500
#define WIFI_MAX_RETRIES 30

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