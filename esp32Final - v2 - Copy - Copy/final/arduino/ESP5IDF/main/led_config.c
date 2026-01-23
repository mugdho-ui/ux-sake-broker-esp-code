/**
 * @file led_config.c
 * @brief LED status indicator implementation
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
 * @brief LED status task
 * @details Controls LED based on WiFi connection status
 */
static void led_task(void *pvParameters)
{
    while (1) {
        if (wifi_is_connected()) {
            // WiFi connected - LED stays ON
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // WiFi not connected - LED blinks
            led_set_state(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_set_state(false);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void led_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing LED on GPIO%d...", LED_BUILTIN);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BUILTIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Turn off LED initially
    led_set_state(false);
    
    // Create LED task
    xTaskCreatePinnedToCore(led_task, "LEDTask", 
                           STACK_SIZE_LED_TASK, NULL, 
                           PRIORITY_LED_TASK, NULL, 0);
    
    ESP_LOGI(TAG, "✅ LED task created");
}