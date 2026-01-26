// /**
//  * @file led_config.c
//  * @brief LED status indicator implementation
//  */

// #include "led_config.h"
// #include "config.h"
// #include "wifi_manager.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_log.h"

// static const char *TAG = "LED";

// void led_set_state(bool state)
// {
//     gpio_set_level(LED_BUILTIN, state ? 1 : 0);
// }

// /**
//  * @brief LED status task
//  * @details Controls LED based on WiFi connection status
//  */
// static void led_task(void *pvParameters)
// {
//     while (1) {
//         if (wifi_is_connected()) {
//             // WiFi connected - LED stays ON
//             led_set_state(true);
//             vTaskDelay(pdMS_TO_TICKS(100));
//         } else {
//             // WiFi not connected - LED blinks
//             led_set_state(true);
//             vTaskDelay(pdMS_TO_TICKS(500));
//             led_set_state(false);
//             vTaskDelay(pdMS_TO_TICKS(500));
//         }
//     }
// }

// void led_init(void)
// {
//     ESP_LOGI(TAG, "🔧 Initializing LED on GPIO%d...", LED_BUILTIN);
    
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL << LED_BUILTIN),
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&io_conf);
    
//     // Turn off LED initially
//     led_set_state(false);
    
//     // Create LED task
//     xTaskCreatePinnedToCore(led_task, "LEDTask", 
//                            STACK_SIZE_LED_TASK, NULL, 
//                            PRIORITY_LED_TASK, NULL, 0);
    
//     ESP_LOGI(TAG, "✅ LED task created");
// }
/**
 * @file led_config.c
 * @brief LED status indicator implementation - FIXED
 * @version 1.1 - Added safety checks and better logging
 */

#include "led_config.h"
#include "config.h"
#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";

void led_set_state(bool state)
{
    gpio_set_level(LED_BUILTIN, state ? 1 : 0);
}

/**
 * @brief LED status task with improved safety
 */
static void led_task(void *pvParameters)
{
    bool last_wifi_state = false;
    bool first_check = true;
    
    ESP_LOGI(TAG, "📡 LED task started");
    
    // ✅ Wait for WiFi event group to be ready
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    while (1) {
        bool wifi_connected = wifi_is_connected();
        
        // ✅ Log state changes for debugging
        if (first_check || (wifi_connected != last_wifi_state)) {
            if (wifi_connected) {
                ESP_LOGI(TAG, "💡 LED: WiFi Connected → Solid ON");
            } else {
                ESP_LOGI(TAG, "💡 LED: WiFi Disconnected → Blinking");
            }
            last_wifi_state = wifi_connected;
            first_check = false;
        }
        
        if (wifi_connected) {
            // ✅ WiFi connected - LED stays ON (solid)
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(500));  // Check every 500ms
        } else {
            // ✅ WiFi not connected - LED blinks
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(200));  // ON for 200ms
            led_set_state(false);
            vTaskDelay(pdMS_TO_TICKS(200));  // OFF for 200ms
        }
    }
}

void led_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing LED on GPIO%d...", LED_BUILTIN);
    
    // ✅ Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BUILTIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ GPIO config failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // ✅ Turn off LED initially (will start blinking if WiFi not connected)
    led_set_state(false);
    ESP_LOGI(TAG, "✅ LED GPIO configured");
    
    // ✅ Create LED task
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        led_task, 
        "LEDTask", 
        STACK_SIZE_LED_TASK, 
        NULL, 
        PRIORITY_LED_TASK, 
        NULL, 
        0
    );
    
    if (task_ret == pdPASS) {
        ESP_LOGI(TAG, "✅ LED task created successfully");
    } else {
        ESP_LOGE(TAG, "❌ Failed to create LED task!");
    }
}