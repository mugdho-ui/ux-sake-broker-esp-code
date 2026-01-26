// /**
//  * @file app_main.c
//  * @brief Main application entry point for ESP32 MQTT Bridge
//  * 
//  * Arduino          →  ESP32
//  * TX (Pin 1)       →  GPIO16 (RX2)
//  * RX (Pin 0)       →  GPIO17 (TX2)
//  * GND              →  GND
//  */

// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "driver/gpio.h"

// #include "config.h"
// #include "wifi_manager.h"
// #include "mqtt_client_config.h"
// #include "serial_handler.h"
// #include "led_config.h"
// #include "factory_reset.h"

// static const char *TAG = "APP_MAIN";

// // Sensor data mutex (shared between modules)
// SensorData sensorData = {0, 0, false};  // ✅ DEFINE the global variable
// SemaphoreHandle_t sensorMutex = NULL;

// /**
//  * @brief Initialize NVS flash
//  */
// static void init_nvs(void)
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
//     ESP_LOGI(TAG, "✅ NVS initialized");
// }

// /**
//  * @brief Disable all power saving modes
//  */
// static void disable_sleep_modes(void)
// {
//     // Disable WiFi power save mode (most important for stability)
//     // Note: WiFi must be initialized before this can be called
//     // This will be done after WiFi init in wifi_manager
    
//     ESP_LOGI(TAG, "🔴 Power management will be configured after WiFi init");
// }

// /**
//  * @brief Initialize factory reset button
//  */
// static void init_factory_reset_button(void)
// {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << FACTORY_RESET_PIN),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&io_conf);
// }

// /**
//  * @brief Main monitoring task
//  */
// static void monitor_task(void *pvParameters)
// {
//     while (1) {
//         factory_reset_check();
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// /**
//  * @brief Application entry point
//  */
// void app_main(void)
// {
//     ESP_LOGI(TAG, "\n========================================");
//     ESP_LOGI(TAG, "ESP32 MQTT Bridge - v1.0");
//     ESP_LOGI(TAG, "========================================\n");

//     // Initialize NVS
//     init_nvs();

//     // Disable sleep modes for stable operation
//     disable_sleep_modes();

//     // Initialize factory reset GPIO
//     init_factory_reset_button();

//     // Create mutex for sensor data protection
//     sensorMutex = xSemaphoreCreateMutex();
//     if (sensorMutex == NULL) {
//         ESP_LOGE(TAG, "Failed to create sensor mutex!");
//         return;
//     }

//     // Initialize LED
//     led_init();

//     // Initialize UART for Arduino communication
//     serial_init();

//     // Initialize WiFi and connect
//     wifi_manager_init();

//     // Wait for WiFi connection before starting MQTT
//     vTaskDelay(pdMS_TO_TICKS(2000));

//     // Initialize MQTT client
//     mqtt_client_init();

//     // Create monitoring task
//     xTaskCreate(monitor_task, "MonitorTask", 2048, NULL, 1, NULL);

//     ESP_LOGI(TAG, "✅ System initialization complete");
//     ESP_LOGI(TAG, "📊 Waiting for sensor data...\n");
// }
/**
 * @file app_main.c
 * @brief Main application entry point for ESP32 MQTT Bridge
 * 
 * Arduino          →  ESP32
 * TX (Pin 1)       →  GPIO16 (RX2)
 * RX (Pin 0)       →  GPIO17 (TX2)
 * GND              →  GND
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "config.h"
#include "wifi_manager.h"
#include "mqtt_client_config.h"
#include "serial_handler.h"
#include "led_config.h"
#include "factory_reset.h"

static const char *TAG = "APP_MAIN";

// Sensor data mutex (shared between modules)
SensorData sensorData = {0, 0, false};  // ✅ DEFINE the global variable
SemaphoreHandle_t sensorMutex = NULL;

/**
 * @brief Initialize NVS flash
 */
static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✅ NVS initialized");
}

/**
 * @brief Disable all power saving modes
 */
static void disable_sleep_modes(void)
{
    // Disable WiFi power save mode (most important for stability)
    // Note: WiFi must be initialized before this can be called
    // This will be done after WiFi init in wifi_manager
    
    ESP_LOGI(TAG, "🔴 Power management will be configured after WiFi init");
}

/**
 * @brief Initialize factory reset button
 */
static void init_factory_reset_button(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FACTORY_RESET_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

/**
 * @brief Main monitoring task
 */
static void monitor_task(void *pvParameters)
{
    while (1) {
        factory_reset_check();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "ESP32 MQTT Bridge - v1.0");
    ESP_LOGI(TAG, "========================================\n");

    // Initialize NVS
    init_nvs();

    // Disable sleep modes for stable operation
    disable_sleep_modes();

    // Initialize factory reset GPIO
    init_factory_reset_button();

    // Create mutex for sensor data protection
    sensorMutex = xSemaphoreCreateMutex();
    if (sensorMutex == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor mutex!");
        return;
    }

    // Initialize UART for Arduino communication
    serial_init();

    // Initialize WiFi and connect (this creates the event group)
    wifi_manager_init();

    // Initialize LED AFTER WiFi (LED task needs wifi_event_group to exist)
    led_init();

    // Wait for WiFi connection before starting MQTT
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Initialize MQTT client
    mqtt_client_init();

    // Create monitoring task
    xTaskCreate(monitor_task, "MonitorTask", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "✅ System initialization complete");
    ESP_LOGI(TAG, "📊 Waiting for sensor data...\n");
}