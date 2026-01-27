/**
 * @file mqtt_client_config.c
 * @brief MQTT client implementation - FIXED
 * @version 1.1 - Prevents multiple start() calls
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
static bool mqtt_client_created = false;   // ✅ Track if client exists
static bool mqtt_client_started = false;   // ✅ Track if client is started
static int consecutive_failures = 0;

// ✅ Forward declaration - function টা আগে declare করতে হবে
static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ MQTT Connected!");
            mqtt_connected = true;
            consecutive_failures = 0;
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "🔴 MQTT Disconnected");
            mqtt_connected = false;
            consecutive_failures++;
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "📤 Message published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "❌ MQTT Error");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "   TCP Transport Error");
            }
            consecutive_failures++;
            break;
            
        default:
            break;
    }
}

static void mqtt_cleanup(void)
{
    if (mqtt_client != NULL) {
        ESP_LOGI(TAG, "🧹 Cleaning up MQTT client...");
        
        if (mqtt_client_started) {
            esp_mqtt_client_stop(mqtt_client);
            mqtt_client_started = false;
        }
        
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        mqtt_client_created = false;
        mqtt_connected = false;
        
        ESP_LOGI(TAG, "✅ MQTT client cleaned up");
    }
}

void mqtt_reconnect(void)
{
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "⚠️ WiFi not connected, skipping MQTT reconnect");
        return;
    }
    
    // ✅ If too many failures, recreate from scratch
    if (consecutive_failures > 5 && mqtt_client_created) {
        ESP_LOGW(TAG, "⚠️ Too many failures (%d), recreating client...", consecutive_failures);
        mqtt_cleanup();
        consecutive_failures = 0;
    }
    
    // ✅ Create client if it doesn't exist
    if (!mqtt_client_created) {
        ESP_LOGI(TAG, "🔄 Creating MQTT client...");
        
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
                .disable_auto_reconnect = false,  // ✅ ESP-IDF handles reconnection
            },
        };
        
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        if (mqtt_client == NULL) {
            ESP_LOGE(TAG, "❌ Failed to initialize MQTT client");
            consecutive_failures++;
            return;
        }
        
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
                                       mqtt_event_handler, NULL);
        
        mqtt_client_created = true;
        ESP_LOGI(TAG, "✅ MQTT client created");
    }
    
    // ✅ CRITICAL FIX: Only call start() ONCE
    if (mqtt_client_created && !mqtt_client_started) {
        ESP_LOGI(TAG, "🔄 Starting MQTT client...");
        ESP_LOGI(TAG, "   Broker: %s", MQTT_BROKER_URI);
        ESP_LOGI(TAG, "   Keepalive: %d seconds", MQTT_KEEPALIVE);
        
        esp_err_t err = esp_mqtt_client_start(mqtt_client);
        if (err == ESP_OK) {
            mqtt_client_started = true;
            ESP_LOGI(TAG, "✅ MQTT client started - auto-reconnect enabled");
        } else if (err == ESP_FAIL) {
            // Already running - this is OK
            mqtt_client_started = true;
            ESP_LOGI(TAG, "✅ MQTT client already running");
        } else {
            ESP_LOGE(TAG, "❌ MQTT start failed: %s", esp_err_to_name(err));
            consecutive_failures++;
            mqtt_cleanup();  // ✅ Clean up on failure
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
        ESP_LOGD(TAG, "⚠️ MQTT not connected, cannot publish");
        return false;
    }
    
    char temp_buf[16];
    char hum_buf[16];
    
    snprintf(temp_buf, sizeof(temp_buf), "%.1f", temperature);
    snprintf(hum_buf, sizeof(hum_buf), "%.1f", humidity);
    
    // ✅ Use proper QoS and retain settings
    int temp_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, 
                                              temp_buf, 0, MQTT_QOS, MQTT_RETAIN);
    
    // ✅ Small delay between publishes to prevent flooding
    vTaskDelay(pdMS_TO_TICKS(50));
    
    int hum_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HUMIDITY, 
                                             hum_buf, 0, MQTT_QOS, MQTT_RETAIN);
    
    if (temp_msg_id >= 0 && hum_msg_id >= 0) {
        ESP_LOGI(TAG, "📤 Published → TEMP: %.1f°C | HUM: %.1f%%", 
                 temperature, humidity);
        return true;
    } else {
        ESP_LOGE(TAG, "❌ Publish failed!");
        consecutive_failures++;
        return false;
    }
}

static void mqtt_task(void *pvParameters)
{
    TickType_t lastReconnect = 0;
    TickType_t lastPublish = 0;
    float lastTemp = 0;
    float lastHum = 0;
    
    ESP_LOGI(TAG, "📡 MQTT task started");
    
    // ✅ Initial connection attempt
    mqtt_reconnect();
    
    while (1) {
        TickType_t current_time = xTaskGetTickCount();
        
        // ✅ Only recreate if critical failure
        if (!mqtt_connected && consecutive_failures > 5) {
            TickType_t time_since_reconnect = safe_time_diff(current_time, lastReconnect);
            
            if (time_since_reconnect > pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS)) {
                ESP_LOGW(TAG, "⚠️ Attempting recovery from failures...");
                mqtt_reconnect();
                lastReconnect = current_time;
            }
        } else if (!mqtt_client_started) {
            // Not started yet - try to start
            mqtt_reconnect();
        }
        
        // ✅ Check for new sensor data
        if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (sensorData.dataReceived) {
                float t = sensorData.temperature;
                float h = sensorData.humidity;
                sensorData.dataReceived = false;
                
                lastTemp = t;
                lastHum = h;
                
                xSemaphoreGive(sensorMutex);
                
                if (mqtt_connected) {
                    mqtt_publish_sensor_data(t, h);
                    lastPublish = current_time;
                }
            } else {
                xSemaphoreGive(sensorMutex);
            }
        }
        
        // ✅ Warning if no data for too long
        if (lastPublish > 0) {
            TickType_t time_since_publish = safe_time_diff(current_time, lastPublish);
            
            if (time_since_publish > pdMS_TO_TICKS(60000)) {
                ESP_LOGW(TAG, "⚠️ No new data for 60s (Last: %.1f°C, %.1f%%)", 
                        lastTemp, lastHum);
                lastPublish = current_time;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ✅ Helper function definition (এখন সব function call এর AFTER আছে)
static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous)
{
    if (current >= previous) {
        return current - previous;
    } else {
        return (0xFFFFFFFF - previous) + current + 1;
    }
}

void mqtt_client_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing MQTT subsystem...");
    
    BaseType_t ret = xTaskCreatePinnedToCore(
        mqtt_task, 
        "MQTTTask", 
        STACK_SIZE_MQTT_TASK, 
        NULL, 
        PRIORITY_MQTT_TASK, 
        NULL, 
        0
    );
    
    if (ret == pdPASS) {
        ESP_LOGI(TAG, "✅ MQTT task created");
    } else {
        ESP_LOGE(TAG, "❌ Failed to create MQTT task!");
    }
}