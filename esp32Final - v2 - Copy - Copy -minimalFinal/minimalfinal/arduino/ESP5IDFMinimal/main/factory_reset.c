/**
 * @file factory_reset.c
 * @brief Factory reset button implementation
 */

#include "factory_reset.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "FACTORY_RESET";

void factory_reset_check(void)
{
    static TickType_t press_time = 0;
    static bool button_pressed = false;
    
    int button_state = gpio_get_level(FACTORY_RESET_PIN);
    
    if (button_state == 0) {  // Button pressed (active low)
        if (!button_pressed) {
            press_time = xTaskGetTickCount();
            button_pressed = true;
            ESP_LOGI(TAG, "🔴 Factory reset button pressed...");
        }
        
        TickType_t elapsed = xTaskGetTickCount() - press_time;
        
        if (elapsed > pdMS_TO_TICKS(FACTORY_RESET_HOLD_MS)) {
            ESP_LOGW(TAG, "🔴 FACTORY RESET TRIGGERED!");
            ESP_LOGW(TAG, "🗑️  Erasing all saved data...");
            
            // Erase all NVS data
            esp_err_t err = nvs_flash_erase();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "✅ NVS erased successfully");
            } else {
                ESP_LOGE(TAG, "❌ NVS erase failed: %s", esp_err_to_name(err));
            }
            
            vTaskDelay(pdMS_TO_TICKS(500));
            
            ESP_LOGW(TAG, "🔄 Restarting device...");
            esp_restart();
        }
    } else {
        if (button_pressed) {
            button_pressed = false;
            ESP_LOGI(TAG, "Factory reset cancelled (button released)");
        }
        press_time = 0;
    }
}