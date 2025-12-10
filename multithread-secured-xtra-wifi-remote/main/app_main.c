#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client_config.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "led_config.h"
#include "wifi_manager.h"  // ✅ নতুন Wi-Fi Manager
#include "factory_reset.h"  // ✅ Factory Reset বাটন

#define TAG "APP_MAIN"

// Queue and tasks
static QueueHandle_t sensorQueue;
static TaskHandle_t TaskSensorHandle;
static TaskHandle_t TaskMQTTHandle;

typedef struct {
    int rawValue;
    float voltage;
} SensorData;

// --- Watchdog Task ---
static void watchdog_task(void *param) {
    TickType_t lastSensorTick = xTaskGetTickCount();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        if (xTaskGetTickCount() - lastSensorTick > pdMS_TO_TICKS(30000)) {
            ESP_LOGW(TAG, "Watchdog: No sensor update, restarting...");
            esp_restart();
        }
    }
}

// --- Sensor Task ---
static void sensor_task(void *param) {
    for (;;) {
        int val = adc1_get_raw(ADC1_CHANNEL_5);  // GPIO33
        float voltage = val * (3.3f / 4095.0f);

        SensorData data = {val, voltage};
        if (xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "[Sensor] Sent: raw=%d, voltage=%.2f", val, voltage);
        } else {
            ESP_LOGW(TAG, "[Sensor] Queue full, skipping");
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// --- MQTT Task ---
static void mqtt_task(void *param) {
    char msg1[16], msg2[16];
    for (;;) {
        SensorData data;
        if (xQueueReceive(sensorQueue, &data, pdMS_TO_TICKS(500))) {
            snprintf(msg1, sizeof(msg1), "%d", data.rawValue);
            snprintf(msg2, sizeof(msg2), "%.2f", data.voltage);

            mqtt_publish(MQTT_TOPIC_PUB1, msg1);
            mqtt_publish(MQTT_TOPIC_PUB2, msg2);

            ESP_LOGI(TAG, "[MQTT] Published: %s, %s", msg1, msg2);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "🚀 ESP32 Starting with Wi-Fi Manager...");
    
    // ============ Factory Reset Button Setup ============
    factory_reset_init();  // ✅ BOOT বাটন দিয়ে reset
    
    // ============ Wi-Fi Manager Setup ============
    ESP_ERROR_CHECK(wifi_manager_init());
    
    if (wifi_manager_is_configured()) {
        ESP_LOGI(TAG, "✅ Wi-Fi credentials found, connecting...");
        
        if (wifi_manager_connect() == ESP_OK) {
            ESP_LOGI(TAG, "✅ Connected to Wi-Fi");
        } else {
            ESP_LOGW(TAG, "⚠️ Failed to connect, starting portal...");
            wifi_manager_start_portal();
            ESP_LOGI(TAG, "📱 Connect to 'ESP32-Setup' and go to 192.168.4.1");
            // Portal mode এ থাকবে, main tasks start হবে না
            return;
        }
    } else {
        ESP_LOGI(TAG, "⚙️ No credentials, starting configuration portal...");
        wifi_manager_start_portal();
        ESP_LOGI(TAG, "📱 Connect to 'ESP32-Setup' and go to 192.168.4.1");
        return;
    }
    
    // ============ MQTT Setup ============
    ESP_ERROR_CHECK(mqtt_start());
    ESP_LOGI(TAG, "✅ MQTT Client Started");

    // ============ ADC Setup ============
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);

    // ============ LED Setup ============
    led_init();

    // ============ Queue Creation ============
    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    if (sensorQueue == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create queue!");
        return;
    }

    // ============ Task Creation ============
    xTaskCreatePinnedToCore(sensor_task, "SensorTask", 4096, NULL, 2, &TaskSensorHandle, 1);
    xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 3, &TaskMQTTHandle, 0);
    xTaskCreatePinnedToCore(watchdog_task, "WatchdogTask", 2048, NULL, 1, NULL, 1);
    
    ESP_LOGI(TAG, "✅ All tasks started successfully!");
}