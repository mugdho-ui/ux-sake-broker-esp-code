

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client_config.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "wifi_manager.h"
#include "factory_reset.h"
#include "dht22_handler.h"
#include "ds18b20_handler.h"
#include "mq135_handler.h"
#include "ultrasonic_handler.h"
#include "motor_controller.h"
#include "config.h"

#define TAG "APP_MAIN"

// Queue handles (size = 1, will use xQueueOverwrite)
static QueueHandle_t dht22Queue;
static QueueHandle_t ds18b20Queue;
static QueueHandle_t mq135Queue;
static QueueHandle_t sonar1Queue;
static QueueHandle_t sonar2Queue;

// Task handles
static TaskHandle_t TaskDHT22Handle;
static TaskHandle_t TaskDS18B20Handle;
static TaskHandle_t TaskMQ135Handle;
static TaskHandle_t TaskSonar1Handle;
static TaskHandle_t TaskSonar2Handle;
static TaskHandle_t TaskControlHandle;
static TaskHandle_t TaskMQTTHandle;

// Data structures
typedef dht22_data_t DHT22Data;
typedef struct { float temperature; } DS18B20Data;
typedef struct { float alcohol; } MQ135Data;
typedef struct { float distance; } SonarData;

// Global motor speed control
static uint8_t target_fan_speed = 0;

// ============ DHT22 Task ============
static void dht22_task(void *param) {
    DHT22Data data;
    int consecutive_errors = 0;
    
    for (;;) {
        esp_err_t result = dht22_read(&data);
        
        if (result == ESP_OK) {
            consecutive_errors = 0;
            ESP_LOGI(TAG, "🌡️ DHT22 - Temp: %.1f°C, Humidity: %.1f%%", 
                     data.temperature, data.humidity);
            xQueueOverwrite(dht22Queue, &data);
        } else {
            consecutive_errors++;
            if (consecutive_errors % 5 == 1) { // Log every 5th error
                ESP_LOGW(TAG, "Failed to read DHT22 (error count: %d)", consecutive_errors);
            }
            if (consecutive_errors > 20) {
                ESP_LOGE(TAG, "DHT22 failed 20+ times, restarting...");
                esp_restart();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// ============ DS18B20 Task ============
static void ds18b20_task(void *param) {
    DS18B20Data data;
    
    for (;;) {
        if (ds18b20_read(&data.temperature) == ESP_OK) {
            ESP_LOGI(TAG, "🔥 DS18B20 - Temp: %.2f°C", data.temperature);
            xQueueOverwrite(ds18b20Queue, &data);
        } else {
            ESP_LOGW(TAG, "Failed to read DS18B20");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ============ MQ135 Task ============
static void mq135_task(void *param) {
    MQ135Data data;
    
    for (;;) {
        if (mq135_read_alcohol(&data.alcohol) == ESP_OK) {
            ESP_LOGI(TAG, "🍷 MQ135 - Alcohol: %.2f ppm", data.alcohol);
            xQueueOverwrite(mq135Queue, &data);
        } else {
            ESP_LOGW(TAG, "Failed to read MQ135");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ============ Sonar1 Task ============
static void sonar1_task(void *param) {
    SonarData data;
    
    for (;;) {
        if (ultrasonic_read_sonar1(&data.distance) == ESP_OK) {
            ESP_LOGI(TAG, "📏 Sonar1 - Distance: %.2f cm", data.distance);
            xQueueOverwrite(sonar1Queue, &data);
        } else {
            ESP_LOGW(TAG, "Failed to read Sonar1");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============ Sonar2 Task ============
static void sonar2_task(void *param) {
    SonarData data;
    
    for (;;) {
        if (ultrasonic_read_sonar2(&data.distance) == ESP_OK) {
            ESP_LOGI(TAG, "📏 Sonar2 - Distance: %.2f cm", data.distance);
            xQueueOverwrite(sonar2Queue, &data);
        } else {
            ESP_LOGW(TAG, "Failed to read Sonar2");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============ Control Task (Motor Logic) ============
static void control_task(void *param) {
    DS18B20Data ds_data;
    SonarData sonar_data;
    bool pump_state = false;
    bool fan_state = false;
    uint8_t current_fan_speed = 0;
    
    for (;;) {
        // Check DS18B20 temperature for fan control
        if (xQueuePeek(ds18b20Queue, &ds_data, 0) == pdTRUE) {
            if (ds_data.temperature > DS18B20_TEMP_THRESHOLD) {
                if (target_fan_speed == 0) {
                    target_fan_speed = 128;
                }
                
                if (!fan_state || current_fan_speed != target_fan_speed) {
                    motor_set_fan_speed(target_fan_speed);
                    fan_state = true;
                    current_fan_speed = target_fan_speed;
                    ESP_LOGI(TAG, "🌡️ Temp > %.1f°C, Fan ON at speed %d", 
                             DS18B20_TEMP_THRESHOLD, target_fan_speed);
                }
            } else {
                if (fan_state) {
                    motor_set_fan_speed(0);
                    fan_state = false;
                    current_fan_speed = 0;
                    ESP_LOGI(TAG, "🌡️ Temp < %.1f°C, Fan OFF", DS18B20_TEMP_THRESHOLD);
                }
            }
        }
        
        // Check Sonar1 for water pump control
        if (xQueuePeek(sonar1Queue, &sonar_data, 0) == pdTRUE) {
            if (sonar_data.distance > SONAR1_DISTANCE_THRESHOLD && !pump_state) {
                motor_set_pump(true);
                pump_state = true;
                ESP_LOGI(TAG, "💧 Distance > %.1fcm, Pump ON", SONAR1_DISTANCE_THRESHOLD);
            } else if (sonar_data.distance <= SONAR1_DISTANCE_THRESHOLD && pump_state) {
                motor_set_pump(false);
                pump_state = false;
                ESP_LOGI(TAG, "💧 Distance <= %.1fcm, Pump OFF", SONAR1_DISTANCE_THRESHOLD);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ============ MQTT Task ============
static void mqtt_task(void *param) {
    char msg[32];
    DHT22Data dht_data;
    DS18B20Data ds_data;
    MQ135Data mq_data;
    SonarData sonar2_data;
    SonarData sonar1_data;
    
    for (;;) {
        if (xQueuePeek(dht22Queue, &dht_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.1f", dht_data.temperature);
            mqtt_publish(MQTT_TOPIC_PUB1, msg);
            
            snprintf(msg, sizeof(msg), "%.1f", dht_data.humidity);
            mqtt_publish(MQTT_TOPIC_PUB2, msg);
            
            ESP_LOGI(TAG, "📤 MQTT: DHT22 - Temp=%.1f°C, Humidity=%.1f%%", 
                     dht_data.temperature, dht_data.humidity);
        }
        
        if (xQueuePeek(ds18b20Queue, &ds_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.2f", ds_data.temperature);
            mqtt_publish("ds18", msg);
            ESP_LOGI(TAG, "📤 MQTT: DS18B20 - Temp=%.2f°C", ds_data.temperature);
        }
        
        if (xQueuePeek(mq135Queue, &mq_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.2f", mq_data.alcohol);
            mqtt_publish("CO2", msg);
            ESP_LOGI(TAG, "📤 MQTT: MQ135 - Alcohol=%.2f ppm", mq_data.alcohol);
        }
        
        if (xQueuePeek(sonar1Queue, &sonar1_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.2f", sonar1_data.distance);
            mqtt_publish("level", msg);
            ESP_LOGI(TAG, "📤 MQTT: Sonar1 - Distance=%.2f cm (level)", sonar1_data.distance);
        }
        
        if (xQueuePeek(sonar2Queue, &sonar2_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.2f", sonar2_data.distance);
            mqtt_publish("sugar", msg);
            ESP_LOGI(TAG, "📤 MQTT: Sonar2 - Distance=%.2f cm", sonar2_data.distance);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ============ Watchdog Task ============
static void watchdog_task(void *param) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        
        // Check if critical tasks are still running
        if (eTaskGetState(TaskDHT22Handle) == eDeleted ||
            eTaskGetState(TaskMQTTHandle) == eDeleted) {
            ESP_LOGW(TAG, "⚠️ Critical task died, restarting...");
            esp_restart();
        }
    }
}

// ============ Main ============
void app_main(void) {
    ESP_LOGI(TAG, "🚀 ESP32 Multi-Sensor System Starting...");
    
    factory_reset_init();
    ESP_ERROR_CHECK(wifi_manager_init());
    
    if (wifi_manager_is_configured()) {
        ESP_LOGI(TAG, "✅ Wi-Fi credentials found");
        if (wifi_manager_connect() == ESP_OK) {
            ESP_LOGI(TAG, "✅ Connected to Wi-Fi");
        } else {
            ESP_LOGW(TAG, "⚠️ Connection failed, starting portal...");
            wifi_manager_start_portal();
            ESP_LOGI(TAG, "📱 Connect to 'ESP32-Setup' at 192.168.4.1");
            return;
        }
    } else {
        ESP_LOGI(TAG, "⚙️ No credentials, starting portal...");
        wifi_manager_start_portal();
        ESP_LOGI(TAG, "📱 Connect to 'ESP32-Setup' at 192.168.4.1");
        return;
    }
    
    ESP_ERROR_CHECK(mqtt_start());
    ESP_LOGI(TAG, "✅ MQTT Started");
    
    // Initialize hardware
    ESP_ERROR_CHECK(dht22_init());
    // ESP_ERROR_CHECK(ds18b20_init());
    esp_err_t err = ds18b20_init();
if (err != ESP_OK) {
    ESP_LOGW("APP_MAIN", "DS18B20 not detected, continuing without it");
}

    ESP_ERROR_CHECK(mq135_init());
    ESP_ERROR_CHECK(ultrasonic_init());
    ESP_ERROR_CHECK(motor_controller_init());
    
    // Create queues with size 1 (latest value only)
    dht22Queue = xQueueCreate(1, sizeof(DHT22Data));
    ds18b20Queue = xQueueCreate(1, sizeof(DS18B20Data));
    mq135Queue = xQueueCreate(1, sizeof(MQ135Data));
    sonar1Queue = xQueueCreate(1, sizeof(SonarData));
    sonar2Queue = xQueueCreate(1, sizeof(SonarData));
    
    // Create tasks
    xTaskCreatePinnedToCore(dht22_task, "DHT22Task", 4096, NULL, 2, &TaskDHT22Handle, 1);
    xTaskCreatePinnedToCore(ds18b20_task, "DS18B20Task", 4096, NULL, 2, &TaskDS18B20Handle, 1);
    xTaskCreatePinnedToCore(mq135_task, "MQ135Task", 4096, NULL, 2, &TaskMQ135Handle, 0);
    xTaskCreatePinnedToCore(sonar1_task, "Sonar1Task", 3072, NULL, 2, &TaskSonar1Handle, 0);
    xTaskCreatePinnedToCore(sonar2_task, "Sonar2Task", 3072, NULL, 2, &TaskSonar2Handle, 1);
    xTaskCreatePinnedToCore(control_task, "ControlTask", 4096, NULL, 3, &TaskControlHandle, 0);
    xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 3, &TaskMQTTHandle, 0);
    xTaskCreatePinnedToCore(watchdog_task, "WatchdogTask", 2048, NULL, 1, NULL, 1);
    
    ESP_LOGI(TAG, "✅ All tasks started successfully!");
}

// MQTT callback to control fan speed
void mqtt_set_fan_speed_from_text(uint8_t speed) {
    target_fan_speed = speed;
    ESP_LOGI(TAG, "🎛️ Fan speed updated from MQTT: %d", speed);
}