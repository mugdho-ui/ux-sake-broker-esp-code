// // /**
// //  * @file app_main.c
// //  * @brief Main application entry point for ESP32 MQTT Bridge - Production Ready
// //  * @version 1.1 - Production Hardened for 24/7 Operation
// //  * 
// //  * Arduino          →  ESP32
// //  * TX (Pin 1)       →  GPIO16 (RX2)
// //  * RX (Pin 0)       →  GPIO17 (TX2)
// //  * GND              →  GND
// //  * 
// //  * Production Features:
// //  * - Watchdog timer protection
// //  * - Heap memory monitoring
// //  * - Tick overflow handling
// //  * - Task health monitoring
// //  * - Automatic recovery from critical errors
// //  */

// // #include <stdio.h>
// // #include <string.h>
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "freertos/semphr.h"
// // #include "esp_system.h"
// // #include "esp_log.h"
// // #include "nvs_flash.h"
// // #include "driver/gpio.h"
// // #include "esp_task_wdt.h"  // ✅ Watchdog timer

// // #include "config.h"
// // #include "wifi_manager.h"
// // #include "mqtt_client_config.h"
// // #include "serial_handler.h"
// // #include "led_config.h"
// // #include "factory_reset.h"

// // static const char *TAG = "APP_MAIN";

// // // Sensor data mutex (shared between modules)
// // SensorData sensorData = {0, 0, false};
// // SemaphoreHandle_t sensorMutex = NULL;

// // // ========== PRODUCTION MONITORING VARIABLES ==========
// // #define WATCHDOG_TIMEOUT_SEC        30      // 30 seconds watchdog
// // #define HEAP_CHECK_INTERVAL_MS      60000   // Check heap every 60s
// // #define MIN_FREE_HEAP_BYTES         10240   // 10KB minimum free heap
// // #define HEALTH_CHECK_INTERVAL_MS    300000  // Health report every 5 minutes

// // static uint32_t system_uptime_seconds = 0;
// // static uint32_t total_restarts = 0;

// // /**
// //  * @brief Safe time difference calculation (handles tick overflow)
// //  * @param current Current tick count
// //  * @param previous Previous tick count
// //  * @return Time difference in ticks
// //  */
// // static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous)
// // {
// //     if (current >= previous) {
// //         return current - previous;
// //     } else {
// //         // Handle tick counter overflow (happens after ~49 days)
// //         return (0xFFFFFFFF - previous) + current + 1;
// //     }
// // }

// // /**
// //  * @brief Initialize NVS flash
// //  */
// // static void init_nvs(void)
// // {
// //     esp_err_t ret = nvs_flash_init();
// //     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// //         ESP_ERROR_CHECK(nvs_flash_erase());
// //         ret = nvs_flash_init();
// //     }
// //     ESP_ERROR_CHECK(ret);
// //     ESP_LOGI(TAG, "✅ NVS initialized");
    
// //     // Load restart counter from NVS
// //     nvs_handle_t nvs_handle;
// //     if (nvs_open("system", NVS_READWRITE, &nvs_handle) == ESP_OK) {
// //         nvs_get_u32(nvs_handle, "restarts", &total_restarts);
// //         total_restarts++;
// //         nvs_set_u32(nvs_handle, "restarts", total_restarts);
// //         nvs_commit(nvs_handle);
// //         nvs_close(nvs_handle);
// //         ESP_LOGI(TAG, "📊 System restart count: %lu", total_restarts);
// //     }
// // }

// // /**
// //  * @brief Initialize watchdog timer for system stability
// //  * Note: In ESP-IDF v5.x, watchdog is often auto-initialized by the system.
// //  * We reconfigure it if needed, or use the existing instance.
// //  */
// // static void init_watchdog(void)
// // {
// //     ESP_LOGI(TAG, "🐕 Configuring watchdog timer (%d seconds)...", WATCHDOG_TIMEOUT_SEC);
    
// //     // ESP-IDF v5.x uses config struct
// //     esp_task_wdt_config_t wdt_config = {
// //         .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,  // Convert seconds to milliseconds
// //         .idle_core_mask = 0,                         // Don't watch idle tasks
// //         .trigger_panic = true                        // Trigger panic on timeout
// //     };
    
// //     esp_err_t ret = esp_task_wdt_init(&wdt_config);
// //     if (ret == ESP_OK) {
// //         ESP_LOGI(TAG, "✅ Watchdog timer initialized");
// //     } else if (ret == ESP_ERR_INVALID_STATE) {
// //         ESP_LOGI(TAG, "✅ Watchdog already active (using existing config)");
// //     } else {
// //         ESP_LOGE(TAG, "❌ Failed to initialize watchdog: %s", esp_err_to_name(ret));
// //     }
    
// //     // Note: Tasks will subscribe to watchdog individually
// // }

// // /**
// //  * @brief Disable all power saving modes
// //  */
// // static void disable_sleep_modes(void)
// // {
// //     ESP_LOGI(TAG, "🔴 Power management will be configured after WiFi init");
// // }

// // /**
// //  * @brief Initialize factory reset button
// //  */
// // static void init_factory_reset_button(void)
// // {
// //     gpio_config_t io_conf = {
// //         .pin_bit_mask = (1ULL << FACTORY_RESET_PIN),
// //         .mode = GPIO_MODE_INPUT,
// //         .pull_up_en = GPIO_PULLUP_ENABLE,
// //         .pull_down_en = GPIO_PULLDOWN_DISABLE,
// //         .intr_type = GPIO_INTR_DISABLE
// //     };
// //     gpio_config(&io_conf);
// // }

// // /**
// //  * @brief Check system health and log diagnostics
// //  */
// // static void check_system_health(void)
// // {
// //     size_t free_heap = esp_get_free_heap_size();
// //     size_t min_heap = esp_get_minimum_free_heap_size();
    
// //     ESP_LOGI(TAG, "========================================");
// //     ESP_LOGI(TAG, "📊 SYSTEM HEALTH REPORT");
// //     ESP_LOGI(TAG, "========================================");
// //     ESP_LOGI(TAG, "⏱️  Uptime: %lu seconds (%.1f hours)", 
// //              system_uptime_seconds, system_uptime_seconds / 3600.0f);
// //     ESP_LOGI(TAG, "💾 Heap: Free=%u bytes, Min=%u bytes", free_heap, min_heap);
// //     ESP_LOGI(TAG, "🔄 Total restarts: %lu", total_restarts);
// //     ESP_LOGI(TAG, "📡 WiFi: %s", wifi_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
// //     ESP_LOGI(TAG, "📤 MQTT: %s", mqtt_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
    
// //     // Check sensor data status
// //     if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
// //         if (sensorData.dataReceived) {
// //             ESP_LOGI(TAG, "🌡️  Sensor: T=%.1f°C, H=%.1f%% ✅", 
// //                      sensorData.temperature, sensorData.humidity);
// //         } else {
// //             ESP_LOGW(TAG, "🌡️  Sensor: No data received ⚠️");
// //         }
// //         xSemaphoreGive(sensorMutex);
// //     }
    
// //     ESP_LOGI(TAG, "========================================\n");
    
// //     // Critical: Check if heap is critically low
// //     if (free_heap < MIN_FREE_HEAP_BYTES) {
// //         ESP_LOGE(TAG, "🔴 CRITICAL: Low memory! Free=%u bytes", free_heap);
// //         ESP_LOGE(TAG, "🔄 Attempting emergency restart in 5 seconds...");
// //         vTaskDelay(pdMS_TO_TICKS(5000));
// //         esp_restart();
// //     }
// // }

// // /**
// //  * @brief Main monitoring task - Production Enhanced
// //  * @details Monitors:
// //  *  - Factory reset button
// //  *  - System heap memory
// //  *  - Overall system health
// //  *  - Watchdog timer feeding
// //  */
// // static void monitor_task(void *pvParameters)
// // {
// //     TickType_t last_heap_check = xTaskGetTickCount();
// //     TickType_t last_health_report = xTaskGetTickCount();
// //     TickType_t last_uptime_update = xTaskGetTickCount();
    
// //     ESP_LOGI(TAG, "🔧 Monitor task started");
    
// //     // Subscribe THIS task to the watchdog
// //     esp_err_t ret = esp_task_wdt_add(NULL);
// //     if (ret == ESP_OK) {
// //         ESP_LOGI(TAG, "✅ Monitor task subscribed to watchdog");
// //     } else {
// //         ESP_LOGW(TAG, "⚠️ Failed to subscribe to watchdog: %s", esp_err_to_name(ret));
// //     }
    
// //     while (1) {
// //         TickType_t current_time = xTaskGetTickCount();
        
// //         // ========== WATCHDOG FEEDING ==========
// //         // Only reset if we're subscribed
// //         if (ret == ESP_OK) {
// //             esp_task_wdt_reset();
// //         }
        
// //         // ========== FACTORY RESET CHECK ==========
// //         factory_reset_check();
        
// //         // ========== UPTIME COUNTER ==========
// //         if (safe_time_diff(current_time, last_uptime_update) >= pdMS_TO_TICKS(1000)) {
// //             system_uptime_seconds++;
// //             last_uptime_update = current_time;
// //         }
        
// //         // ========== HEAP MONITORING ==========
// //         if (safe_time_diff(current_time, last_heap_check) >= pdMS_TO_TICKS(HEAP_CHECK_INTERVAL_MS)) {
// //             size_t free_heap = esp_get_free_heap_size();
// //             size_t min_heap = esp_get_minimum_free_heap_size();
            
// //             ESP_LOGI(TAG, "💾 Heap check: Free=%u, Min=%u", free_heap, min_heap);
            
// //             // Warning if heap is getting low
// //             if (free_heap < MIN_FREE_HEAP_BYTES * 2) {
// //                 ESP_LOGW(TAG, "⚠️ Heap running low: %u bytes free", free_heap);
// //             }
            
// //             // Critical: Restart if heap critically low
// //             if (free_heap < MIN_FREE_HEAP_BYTES) {
// //                 ESP_LOGE(TAG, "🔴 CRITICAL: Out of memory!");
// //                 ESP_LOGE(TAG, "🔄 Emergency restart in 3 seconds...");
// //                 vTaskDelay(pdMS_TO_TICKS(3000));
// //                 esp_restart();
// //             }
            
// //             last_heap_check = current_time;
// //         }
        
// //         // ========== SYSTEM HEALTH REPORT ==========
// //         if (safe_time_diff(current_time, last_health_report) >= pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL_MS)) {
// //             check_system_health();
// //             last_health_report = current_time;
// //         }
        
// //         vTaskDelay(pdMS_TO_TICKS(100));
// //     }
// // }

// // /**
// //  * @brief Application entry point - Production Ready
// //  */
// // void app_main(void)
// // {
// //     ESP_LOGI(TAG, "\n========================================");
// //     ESP_LOGI(TAG, "ESP32 MQTT Bridge - v1.1 Production");
// //     ESP_LOGI(TAG, "========================================");
// //     ESP_LOGI(TAG, "Features: Watchdog | Heap Monitor | Auto-Recovery");
// //     ESP_LOGI(TAG, "========================================\n");

// //     // ========== PHASE 1: CORE INITIALIZATION ==========
    
// //     // Initialize NVS (loads restart counter)
// //     init_nvs();

// //     // Initialize watchdog timer FIRST (critical for production)
// //     init_watchdog();

// //     // Disable sleep modes for stable operation
// //     disable_sleep_modes();

// //     // Initialize factory reset GPIO
// //     init_factory_reset_button();

// //     // Create mutex for sensor data protection
// //     sensorMutex = xSemaphoreCreateMutex();
// //     if (sensorMutex == NULL) {
// //         ESP_LOGE(TAG, "❌ Failed to create sensor mutex!");
// //         ESP_LOGE(TAG, "🔄 Restarting in 3 seconds...");
// //         vTaskDelay(pdMS_TO_TICKS(3000));
// //         esp_restart();
// //         return;
// //     }
// //     ESP_LOGI(TAG, "✅ Sensor mutex created");

// //     // ========== PHASE 2: COMMUNICATION INITIALIZATION ==========
    
// //     // Initialize UART for Arduino communication
// //     serial_init();

// //     // Initialize WiFi and connect (this creates the event group)
// //     wifi_manager_init();

// //     // Initialize LED AFTER WiFi (LED task needs wifi_event_group to exist)
// //     led_init();

// //     // Wait for WiFi connection before starting MQTT
// //     ESP_LOGI(TAG, "⏳ Waiting for WiFi connection...");
// //     vTaskDelay(pdMS_TO_TICKS(2000));

// //     // Initialize MQTT client
// //     mqtt_client_init();

// //     // ========== PHASE 3: START MONITORING ==========
    
// //     // Create monitoring task with larger stack for production
// //     BaseType_t ret = xTaskCreatePinnedToCore(
// //         monitor_task,
// //         "MonitorTask",
// //         4096,  // ✅ Increased from 2048 to 4096 for safety
// //         NULL,
// //         5,     // Higher priority for monitoring
// //         NULL,
// //         0      // Pin to core 0
// //     );
    
// //     if (ret != pdPASS) {
// //         ESP_LOGE(TAG, "❌ Failed to create monitor task!");
// //         ESP_LOGE(TAG, "🔄 Restarting in 3 seconds...");
// //         vTaskDelay(pdMS_TO_TICKS(3000));
// //         esp_restart();
// //         return;
// //     }

// //     ESP_LOGI(TAG, "\n========================================");
// //     ESP_LOGI(TAG, "✅ System initialization complete");
// //     ESP_LOGI(TAG, "🐕 Watchdog: Active (%ds timeout)", WATCHDOG_TIMEOUT_SEC);
// //     ESP_LOGI(TAG, "💾 Heap monitoring: Every %ds", HEAP_CHECK_INTERVAL_MS / 1000);
// //     ESP_LOGI(TAG, "📊 Health reports: Every %d minutes", HEALTH_CHECK_INTERVAL_MS / 60000);
// //     ESP_LOGI(TAG, "📡 Waiting for sensor data...");
// //     ESP_LOGI(TAG, "========================================\n");
    
// //     // Initial health check
// //     vTaskDelay(pdMS_TO_TICKS(5000));
// //     check_system_health();
// // }

// /**
//  * @file app_main.c
//  * @brief Main application entry point for ESP32 MQTT Bridge - Production Ready
//  * @version 1.2 - FIXED: Initialization order & LED race condition
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
// #include "esp_task_wdt.h"

// #include "config.h"
// #include "wifi_manager.h"
// #include "mqtt_client_config.h"
// #include "serial_handler.h"
// #include "led_config.h"
// #include "factory_reset.h"

// static const char *TAG = "APP_MAIN";

// // Sensor data mutex (shared between modules)
// SensorData sensorData = {0, 0, false};
// SemaphoreHandle_t sensorMutex = NULL;

// // ========== PRODUCTION MONITORING VARIABLES ==========
// #define WATCHDOG_TIMEOUT_SEC        30
// #define HEAP_CHECK_INTERVAL_MS      60000
// #define MIN_FREE_HEAP_BYTES         10240
// #define HEALTH_CHECK_INTERVAL_MS    300000

// static uint32_t system_uptime_seconds = 0;
// static uint32_t total_restarts = 0;

// static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous)
// {
//     if (current >= previous) {
//         return current - previous;
//     } else {
//         return (0xFFFFFFFF - previous) + current + 1;
//     }
// }

// static void init_nvs(void)
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
//     ESP_LOGI(TAG, "✅ NVS initialized");
    
//     nvs_handle_t nvs_handle;
//     if (nvs_open("system", NVS_READWRITE, &nvs_handle) == ESP_OK) {
//         nvs_get_u32(nvs_handle, "restarts", &total_restarts);
//         total_restarts++;
//         nvs_set_u32(nvs_handle, "restarts", total_restarts);
//         nvs_commit(nvs_handle);
//         nvs_close(nvs_handle);
//         ESP_LOGI(TAG, "📊 System restart count: %lu", total_restarts);
//     }
// }

// static void init_watchdog(void)
// {
//     ESP_LOGI(TAG, "🐕 Configuring watchdog timer (%d seconds)...", WATCHDOG_TIMEOUT_SEC);
    
//     esp_task_wdt_config_t wdt_config = {
//         .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
//         .idle_core_mask = 0,
//         .trigger_panic = true
//     };
    
//     esp_err_t ret = esp_task_wdt_init(&wdt_config);
//     if (ret == ESP_OK) {
//         ESP_LOGI(TAG, "✅ Watchdog timer initialized");
//     } else if (ret == ESP_ERR_INVALID_STATE) {
//         ESP_LOGI(TAG, "✅ Watchdog already active");
//     } else {
//         ESP_LOGE(TAG, "❌ Failed to initialize watchdog: %s", esp_err_to_name(ret));
//     }
// }

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

// static void check_system_health(void)
// {
//     size_t free_heap = esp_get_free_heap_size();
//     size_t min_heap = esp_get_minimum_free_heap_size();
    
//     ESP_LOGI(TAG, "========================================");
//     ESP_LOGI(TAG, "📊 SYSTEM HEALTH REPORT");
//     ESP_LOGI(TAG, "========================================");
//     ESP_LOGI(TAG, "⏱️  Uptime: %lu seconds (%.1f hours)", 
//              system_uptime_seconds, system_uptime_seconds / 3600.0f);
//     ESP_LOGI(TAG, "💾 Heap: Free=%u bytes, Min=%u bytes", free_heap, min_heap);
//     ESP_LOGI(TAG, "🔄 Total restarts: %lu", total_restarts);
//     ESP_LOGI(TAG, "📡 WiFi: %s", wifi_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
//     ESP_LOGI(TAG, "📤 MQTT: %s", mqtt_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
    
//     if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//         if (sensorData.dataReceived) {
//             ESP_LOGI(TAG, "🌡️  Sensor: T=%.1f°C, H=%.1f%% ✅", 
//                      sensorData.temperature, sensorData.humidity);
//         } else {
//             ESP_LOGW(TAG, "🌡️  Sensor: No data received ⚠️");
//         }
//         xSemaphoreGive(sensorMutex);
//     }
    
//     ESP_LOGI(TAG, "========================================\n");
    
//     if (free_heap < MIN_FREE_HEAP_BYTES) {
//         ESP_LOGE(TAG, "🔴 CRITICAL: Low memory! Free=%u bytes", free_heap);
//         ESP_LOGE(TAG, "🔄 Emergency restart in 5 seconds...");
//         vTaskDelay(pdMS_TO_TICKS(5000));
//         esp_restart();
//     }
// }

// static void monitor_task(void *pvParameters)
// {
//     TickType_t last_heap_check = xTaskGetTickCount();
//     TickType_t last_health_report = xTaskGetTickCount();
//     TickType_t last_uptime_update = xTaskGetTickCount();
    
//     ESP_LOGI(TAG, "🔧 Monitor task started");
    
//     esp_err_t ret = esp_task_wdt_add(NULL);
//     if (ret == ESP_OK) {
//         ESP_LOGI(TAG, "✅ Monitor task subscribed to watchdog");
//     }
    
//     while (1) {
//         TickType_t current_time = xTaskGetTickCount();
        
//         if (ret == ESP_OK) {
//             esp_task_wdt_reset();
//         }
        
//         factory_reset_check();
        
//         if (safe_time_diff(current_time, last_uptime_update) >= pdMS_TO_TICKS(1000)) {
//             system_uptime_seconds++;
//             last_uptime_update = current_time;
//         }
        
//         if (safe_time_diff(current_time, last_heap_check) >= pdMS_TO_TICKS(HEAP_CHECK_INTERVAL_MS)) {
//             size_t free_heap = esp_get_free_heap_size();
            
//             if (free_heap < MIN_FREE_HEAP_BYTES * 2) {
//                 ESP_LOGW(TAG, "⚠️ Heap running low: %u bytes free", free_heap);
//             }
            
//             if (free_heap < MIN_FREE_HEAP_BYTES) {
//                 ESP_LOGE(TAG, "🔴 CRITICAL: Out of memory!");
//                 vTaskDelay(pdMS_TO_TICKS(3000));
//                 esp_restart();
//             }
            
//             last_heap_check = current_time;
//         }
        
//         if (safe_time_diff(current_time, last_health_report) >= pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL_MS)) {
//             check_system_health();
//             last_health_report = current_time;
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// void app_main(void)
// {
//     ESP_LOGI(TAG, "\n========================================");
//     ESP_LOGI(TAG, "ESP32 MQTT Bridge - v1.2 Production");
//     ESP_LOGI(TAG, "========================================");
//     ESP_LOGI(TAG, "🔧 FIXED: WiFi→LED init order");
//     ESP_LOGI(TAG, "========================================\n");

//     // ========== PHASE 1: CORE INITIALIZATION ==========
//     init_nvs();
//     init_watchdog();
//     init_factory_reset_button();

//     sensorMutex = xSemaphoreCreateMutex();
//     if (sensorMutex == NULL) {
//         ESP_LOGE(TAG, "❌ Failed to create sensor mutex!");
//         vTaskDelay(pdMS_TO_TICKS(3000));
//         esp_restart();
//         return;
//     }
//     ESP_LOGI(TAG, "✅ Sensor mutex created");

//     // ========== PHASE 2: COMMUNICATION INITIALIZATION ==========
//     serial_init();

//     // ✅ CRITICAL FIX: Initialize WiFi FIRST
//     ESP_LOGI(TAG, "🔧 Initializing WiFi (creates event group)...");
//     wifi_manager_init();
    
//     // ✅ Wait to ensure event group is fully created
//     vTaskDelay(pdMS_TO_TICKS(500));
    
//     // ✅ NOW safe to initialize LED (it depends on wifi_event_group)
//     ESP_LOGI(TAG, "🔧 Initializing LED (depends on WiFi)...");
//     led_init();

//     // Wait for WiFi connection
//     ESP_LOGI(TAG, "⏳ Waiting for WiFi connection...");
//     vTaskDelay(pdMS_TO_TICKS(2000));

//     mqtt_client_init();

//     // ========== PHASE 3: START MONITORING ==========
//     BaseType_t ret = xTaskCreatePinnedToCore(
//         monitor_task,
//         "MonitorTask",
//         4096,
//         NULL,
//         5,
//         NULL,
//         0
//     );
    
//     if (ret != pdPASS) {
//         ESP_LOGE(TAG, "❌ Failed to create monitor task!");
//         vTaskDelay(pdMS_TO_TICKS(3000));
//         esp_restart();
//         return;
//     }

//     ESP_LOGI(TAG, "\n========================================");
//     ESP_LOGI(TAG, "✅ System initialization complete");
//     ESP_LOGI(TAG, "🐕 Watchdog: Active (%ds timeout)", WATCHDOG_TIMEOUT_SEC);
//     ESP_LOGI(TAG, "💡 LED: Blinking=Disconnected, Solid=Connected");
//     ESP_LOGI(TAG, "========================================\n");
    
//     vTaskDelay(pdMS_TO_TICKS(5000));
//     check_system_health();
// }
/**
 * @file app_main.c
 * @brief ESP32 MQTT Bridge - FIXED: Portal failure issue
 * @version 1.3 - LED & Monitor start BEFORE WiFi to avoid hang
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
#include "esp_task_wdt.h"

#include "config.h"
#include "wifi_manager.h"
#include "mqtt_client_config.h"
#include "serial_handler.h"
#include "led_config.h"
#include "factory_reset.h"

static const char *TAG = "APP_MAIN";

SensorData sensorData = {0, 0, false};
SemaphoreHandle_t sensorMutex = NULL;

#define WATCHDOG_TIMEOUT_SEC        30
#define HEAP_CHECK_INTERVAL_MS      60000
#define MIN_FREE_HEAP_BYTES         10240
#define HEALTH_CHECK_INTERVAL_MS    300000

static uint32_t system_uptime_seconds = 0;
static uint32_t total_restarts = 0;

// ✅ Temporary event group for LED (before WiFi init)
static EventGroupHandle_t temp_wifi_event_group = NULL;

static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous)
{
    if (current >= previous) {
        return current - previous;
    } else {
        return (0xFFFFFFFF - previous) + current + 1;
    }
}

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✅ NVS initialized");
    
    nvs_handle_t nvs_handle;
    if (nvs_open("system", NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_get_u32(nvs_handle, "restarts", &total_restarts);
        total_restarts++;
        nvs_set_u32(nvs_handle, "restarts", total_restarts);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "📊 System restart count: %lu", total_restarts);
    }
}

static void init_watchdog(void)
{
    ESP_LOGI(TAG, "🐕 Configuring watchdog timer (%d seconds)...", WATCHDOG_TIMEOUT_SEC);
    
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    
    esp_err_t ret = esp_task_wdt_init(&wdt_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ Watchdog timer initialized");
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "✅ Watchdog already active");
    }
}

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

static void check_system_health(void)
{
    size_t free_heap = esp_get_free_heap_size();
    size_t min_heap = esp_get_minimum_free_heap_size();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "📊 SYSTEM HEALTH REPORT");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "⏱️  Uptime: %lu seconds (%.1f hours)", 
             system_uptime_seconds, system_uptime_seconds / 3600.0f);
    ESP_LOGI(TAG, "💾 Heap: Free=%u bytes, Min=%u bytes", free_heap, min_heap);
    ESP_LOGI(TAG, "🔄 Total restarts: %lu", total_restarts);
    ESP_LOGI(TAG, "📡 WiFi: %s", wifi_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
    ESP_LOGI(TAG, "📤 MQTT: %s", mqtt_is_connected() ? "Connected ✅" : "Disconnected ⚠️");
    
    if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (sensorData.dataReceived) {
            ESP_LOGI(TAG, "🌡️  Sensor: T=%.1f°C, H=%.1f%% ✅", 
                     sensorData.temperature, sensorData.humidity);
        } else {
            ESP_LOGW(TAG, "🌡️  Sensor: No data received ⚠️");
        }
        xSemaphoreGive(sensorMutex);
    }
    
    ESP_LOGI(TAG, "========================================\n");
    
    if (free_heap < MIN_FREE_HEAP_BYTES) {
        ESP_LOGE(TAG, "🔴 CRITICAL: Low memory! Restarting...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }
}

static void monitor_task(void *pvParameters)
{
    TickType_t last_heap_check = xTaskGetTickCount();
    TickType_t last_health_report = xTaskGetTickCount();
    TickType_t last_uptime_update = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "🔧 Monitor task started (factory reset enabled)");
    
    esp_err_t ret = esp_task_wdt_add(NULL);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ Monitor subscribed to watchdog");
    }
    
    while (1) {
        TickType_t current_time = xTaskGetTickCount();
        
        if (ret == ESP_OK) {
            esp_task_wdt_reset();
        }
        
        // ✅ CRITICAL: Factory reset works even if WiFi fails
        factory_reset_check();
        
        if (safe_time_diff(current_time, last_uptime_update) >= pdMS_TO_TICKS(1000)) {
            system_uptime_seconds++;
            last_uptime_update = current_time;
        }
        
        if (safe_time_diff(current_time, last_heap_check) >= pdMS_TO_TICKS(HEAP_CHECK_INTERVAL_MS)) {
            size_t free_heap = esp_get_free_heap_size();
            
            if (free_heap < MIN_FREE_HEAP_BYTES * 2) {
                ESP_LOGW(TAG, "⚠️ Heap low: %u bytes", free_heap);
            }
            
            if (free_heap < MIN_FREE_HEAP_BYTES) {
                ESP_LOGE(TAG, "🔴 Out of memory!");
                vTaskDelay(pdMS_TO_TICKS(3000));
                esp_restart();
            }
            
            last_heap_check = current_time;
        }
        
        if (safe_time_diff(current_time, last_health_report) >= pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL_MS)) {
            check_system_health();
            last_health_report = current_time;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "ESP32 MQTT Bridge - v1.3");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "✅ FIXED: Portal starts if router offline");
    ESP_LOGI(TAG, "✅ FIXED: Factory reset works anytime");
    ESP_LOGI(TAG, "========================================\n");

    // ========== PHASE 1: CORE INIT ==========
    init_nvs();
    init_watchdog();
    init_factory_reset_button();

    sensorMutex = xSemaphoreCreateMutex();
    if (sensorMutex == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create mutex!");
        esp_restart();
        return;
    }
    
    // ✅ Create temp event group for LED (before WiFi)
    temp_wifi_event_group = xEventGroupCreate();

    // ========== PHASE 2: START CRITICAL TASKS FIRST ==========
    
    // ✅ Start monitor task FIRST (so factory reset works immediately)
    ESP_LOGI(TAG, "🔧 Starting monitor task...");
    BaseType_t ret = xTaskCreatePinnedToCore(
        monitor_task, "MonitorTask", 4096, NULL, 5, NULL, 0
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "❌ Monitor task failed!");
        esp_restart();
        return;
    }
    
    // ✅ Start LED task (will use temp event group until WiFi ready)
    ESP_LOGI(TAG, "🔧 Starting LED task...");
    led_init();
    
    // ========== PHASE 3: SERIAL & WiFi ==========
    serial_init();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "🔌 Initializing WiFi...");
    ESP_LOGI(TAG, "   (This may take up to 10 seconds)");
    ESP_LOGI(TAG, "========================================");
    
    wifi_manager_init();  // ✅ This will timeout and start portal if router offline
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // ========== PHASE 4: MQTT (only if WiFi connected) ==========
    if (wifi_is_connected()) {
        ESP_LOGI(TAG, "✅ WiFi connected - starting MQTT...");
        mqtt_client_init();
    } else {
        ESP_LOGW(TAG, "⚠️ WiFi portal mode - MQTT disabled");
        ESP_LOGI(TAG, "📍 Connect to '%s' and configure WiFi", AP_SSID);
    }

    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "✅ System Ready");
    ESP_LOGI(TAG, "🐕 Watchdog: %ds timeout", WATCHDOG_TIMEOUT_SEC);
    ESP_LOGI(TAG, "💡 LED: Blinking = No WiFi | Solid = Connected");
    ESP_LOGI(TAG, "🔴 Factory Reset: Hold BOOT for 5 seconds");
    ESP_LOGI(TAG, "========================================\n");
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    check_system_health();
}