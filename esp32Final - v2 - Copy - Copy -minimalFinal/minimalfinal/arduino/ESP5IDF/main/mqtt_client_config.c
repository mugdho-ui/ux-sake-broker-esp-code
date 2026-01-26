/**
 * @file mqtt_client_config.c
 * @brief MQTT client implementation
 */

#include "mqtt_client_config.h"
#include "config.h"
#include "wifi_manager.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

/**
 * @brief MQTT event handler
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ MQTT Connected!");
            mqtt_connected = true;
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "🔴 MQTT Disconnected!");
            mqtt_connected = false;
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "❌ MQTT Error");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP Transport Error");
            }
            break;
            
        default:
            break;
    }
}

void mqtt_reconnect(void)
{
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "⚠️ WiFi not connected, skipping MQTT reconnect");
        return;
    }
    
    if (mqtt_client == NULL) {
        ESP_LOGI(TAG, "🔄 Initializing MQTT client...");
        
        esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = MQTT_BROKER_URI,
            .credentials = {
                .username = MQTT_USER,
                .authentication.password = MQTT_PASS,
                .client_id = MQTT_CLIENT_ID,
            },
            .session = {
                .keepalive = MQTT_KEEPALIVE,
            },
            .network = {
                .timeout_ms = MQTT_TIMEOUT_MS,
            },
        };
        
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        if (mqtt_client == NULL) {
            ESP_LOGE(TAG, "❌ Failed to initialize MQTT client");
            return;
        }
        
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
                                       mqtt_event_handler, NULL);
    }
    
    if (!mqtt_connected) {
        ESP_LOGI(TAG, "🔄 Connecting to MQTT broker...");
        ESP_LOGI(TAG, "   Broker: %s", MQTT_BROKER_URI);
        esp_err_t err = esp_mqtt_client_start(mqtt_client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "❌ MQTT client start failed: %s", esp_err_to_name(err));
        }
    }
}

bool mqtt_is_connected(void)
{
    return mqtt_connected;
}

bool mqtt_publish_sensor_data(float temperature, float humidity)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "⚠️ MQTT not connected, cannot publish");
        return false;
    }
    
    char temp_buf[16];
    char hum_buf[16];
    
    snprintf(temp_buf, sizeof(temp_buf), "%.1f", temperature);
    snprintf(hum_buf, sizeof(hum_buf), "%.1f", humidity);
    
    int temp_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, 
                                              temp_buf, 0, 0, 0);
    int hum_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HUMIDITY, 
                                             hum_buf, 0, 0, 0);
    
    if (temp_msg_id >= 0 && hum_msg_id >= 0) {
        ESP_LOGI(TAG, "📤 Published → TEMP: %.1f°C | HUM: %.1f%%", 
                 temperature, humidity);
        return true;
    } else {
        ESP_LOGE(TAG, "❌ Publish failed!");
        return false;
    }
}

/**
 * @brief MQTT task - manages connection and publishes data
 */
static void mqtt_task(void *pvParameters)
{
    TickType_t lastReconnect = 0;
    TickType_t lastPublish = 0;
    float lastTemp = 0;
    float lastHum = 0;
    int publishCount = 0;
    
    while (1) {
        // Reconnect if needed (with rate limiting)
        if (!mqtt_connected) {
            if ((xTaskGetTickCount() - lastReconnect) > pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS)) {
                mqtt_reconnect();
                lastReconnect = xTaskGetTickCount();
            }
        }
        
        // Check for new sensor data
        if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool ready = sensorData.dataReceived;
            float t = sensorData.temperature;
            float h = sensorData.humidity;
            
            if (ready) {
                sensorData.dataReceived = false;
                lastTemp = t;
                lastHum = h;
                
                xSemaphoreGive(sensorMutex);
                
                // Publish if MQTT is connected
                if (mqtt_connected) {
                    if (mqtt_publish_sensor_data(t, h)) {
                        publishCount++;
                        lastPublish = xTaskGetTickCount();
                    }
                }
            } else {
                xSemaphoreGive(sensorMutex);
            }
        }
        
        // Warning if no new data for too long
        if (lastPublish > 0 && 
            (xTaskGetTickCount() - lastPublish) > pdMS_TO_TICKS(10000)) {
            ESP_LOGW(TAG, "⚠️ No new data for 10s (Last: %.1f°C, %.1f%%)", 
                    lastTemp, lastHum);
            lastPublish = xTaskGetTickCount(); // Reset to avoid spam
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void mqtt_client_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing MQTT client...");
    
    // Initial connection attempt
    mqtt_reconnect();
    
    // Create MQTT task
    xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 
                           STACK_SIZE_MQTT_TASK, NULL, 
                           PRIORITY_MQTT_TASK, NULL, 0);
    
    ESP_LOGI(TAG, "✅ MQTT task created");
}