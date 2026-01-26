
// /**
//  * @file mqtt_client_config.c
//  * @brief MQTT client implementation - Production Fixed
//  */

// #include "mqtt_client_config.h"
// #include "config.h"
// #include "wifi_manager.h"
// #include <string.h>
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "mqtt_client.h"

// static const char *TAG = "MQTT";

// static esp_mqtt_client_handle_t mqtt_client = NULL;
// static bool mqtt_connected = false;
// static bool mqtt_client_started = false;  // ✅ Track if client is started
// static int consecutive_failures = 0;       // ✅ Track failures for recovery

// /**
//  * @brief MQTT event handler
//  */
// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
//                                int32_t event_id, void *event_data)
// {
//     esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
//     switch ((esp_mqtt_event_id_t)event_id) {
//         case MQTT_EVENT_CONNECTED:
//             ESP_LOGI(TAG, "✅ MQTT Connected!");
//             mqtt_connected = true;
//             consecutive_failures = 0;  // ✅ Reset failure counter
//             break;
            
//         case MQTT_EVENT_DISCONNECTED:
//             ESP_LOGW(TAG, "🔴 MQTT Disconnected!");
//             mqtt_connected = false;
//             mqtt_client_started = false;  // ✅ Mark as not started
//             consecutive_failures++;
//             break;
            
//         case MQTT_EVENT_PUBLISHED:
//             ESP_LOGD(TAG, "Message published, msg_id=%d", event->msg_id);
//             break;
            
//         case MQTT_EVENT_ERROR:
//             ESP_LOGE(TAG, "❌ MQTT Error");
//             if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//                 ESP_LOGE(TAG, "TCP Transport Error");
//             }
//             consecutive_failures++;
//             break;
            
//         default:
//             break;
//     }
// }

// /**
//  * @brief Clean up and destroy MQTT client
//  */
// static void mqtt_cleanup(void)
// {
//     if (mqtt_client != NULL) {
//         ESP_LOGI(TAG, "🧹 Cleaning up MQTT client...");
        
//         // Stop the client if it's running
//         if (mqtt_client_started) {
//             esp_mqtt_client_stop(mqtt_client);
//             mqtt_client_started = false;
//         }
        
//         // Destroy the client
//         esp_mqtt_client_destroy(mqtt_client);
//         mqtt_client = NULL;
//         mqtt_connected = false;
        
//         ESP_LOGI(TAG, "✅ MQTT client cleaned up");
//     }
// }

// void mqtt_reconnect(void)
// {
//     if (!wifi_is_connected()) {
//         ESP_LOGW(TAG, "⚠️ WiFi not connected, skipping MQTT reconnect");
//         return;
//     }
    
//     // ✅ CRITICAL FIX: If too many failures, recreate client from scratch
//     if (consecutive_failures > 5 && mqtt_client != NULL) {
//         ESP_LOGW(TAG, "⚠️ Too many failures (%d), recreating client...", consecutive_failures);
//         mqtt_cleanup();
//         consecutive_failures = 0;
//     }
    
//     // ✅ Create client if it doesn't exist
//     if (mqtt_client == NULL) {
//         ESP_LOGI(TAG, "🔄 Creating MQTT client...");
        
//         esp_mqtt_client_config_t mqtt_cfg = {
//             .broker.address.uri = MQTT_BROKER_URI,
//             .credentials = {
//                 .username = MQTT_USER,
//                 .authentication.password = MQTT_PASS,
//                 .client_id = MQTT_CLIENT_ID,
//             },
//             .session = {
//                 .keepalive = MQTT_KEEPALIVE,
//             },
//             .network = {
//                 .timeout_ms = MQTT_TIMEOUT_MS,
//                 .disable_auto_reconnect = false,  // ✅ Enable auto-reconnect
//             },
//         };
        
//         mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
//         if (mqtt_client == NULL) {
//             ESP_LOGE(TAG, "❌ Failed to initialize MQTT client");
//             consecutive_failures++;
//             return;
//         }
        
//         esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
//                                        mqtt_event_handler, NULL);
        
//         ESP_LOGI(TAG, "✅ MQTT client created");
//     }
    
//     // ✅ CRITICAL FIX: Only start once, let auto-reconnect handle the rest
//     if (!mqtt_client_started) {
//         ESP_LOGI(TAG, "🔄 Starting MQTT client...");
//         ESP_LOGI(TAG, "   Broker: %s", MQTT_BROKER_URI);
//         ESP_LOGI(TAG, "   Keepalive: %d seconds", MQTT_KEEPALIVE);
        
//         esp_err_t err = esp_mqtt_client_start(mqtt_client);
//         if (err == ESP_OK) {
//             mqtt_client_started = true;  // ✅ Mark as started
//             ESP_LOGI(TAG, "✅ MQTT client started - auto-reconnect enabled");
//         } else if (err == ESP_FAIL) {
//             // Client already running - this is OK
//             mqtt_client_started = true;
//             ESP_LOGI(TAG, "✅ MQTT client already running");
//         } else {
//             ESP_LOGE(TAG, "❌ MQTT client start failed: %s", esp_err_to_name(err));
//             consecutive_failures++;
//         }
//     } else if (!mqtt_connected) {
//         // Client is running but not connected - ESP-IDF is handling reconnection
//         ESP_LOGD(TAG, "⏳ Waiting for auto-reconnect...");
//     }
// }

// bool mqtt_is_connected(void)
// {
//     return mqtt_connected;
// }

// bool mqtt_publish_sensor_data(float temperature, float humidity)
// {
//     if (!mqtt_connected) {
//         ESP_LOGW(TAG, "⚠️ MQTT not connected, cannot publish");
//         return false;
//     }
    
//     char temp_buf[16];
//     char hum_buf[16];
    
//     snprintf(temp_buf, sizeof(temp_buf), "%.1f", temperature);
//     snprintf(hum_buf, sizeof(hum_buf), "%.1f", humidity);
    
//     int temp_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, 
//                                               temp_buf, 0, 0, 0);
//     int hum_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HUMIDITY, 
//                                              hum_buf, 0, 0, 0);
    
//     if (temp_msg_id >= 0 && hum_msg_id >= 0) {
//         ESP_LOGI(TAG, "📤 Published → TEMP: %.1f°C | HUM: %.1f%%", 
//                  temperature, humidity);
//         return true;
//     } else {
//         ESP_LOGE(TAG, "❌ Publish failed!");
//         consecutive_failures++;
//         return false;
//     }
// }

// /**
//  * @brief MQTT task - manages connection and publishes data
//  */
// static void mqtt_task(void *pvParameters)
// {
//     TickType_t lastReconnect = 0;
//     TickType_t lastPublish = 0;
//     float lastTemp = 0;
//     float lastHum = 0;
//     int publishCount = 0;
    
//     ESP_LOGI(TAG, "📡 MQTT task started");
    
//     while (1) {
//         TickType_t current_time = xTaskGetTickCount();
        
//         // ✅ Only recreate client if it failed too many times
//         // Otherwise, let ESP-IDF's auto-reconnect handle it
//         if (!mqtt_connected && consecutive_failures > 5) {
//             // Use safe time difference to handle tick overflow
//             TickType_t time_since_reconnect;
//             if (current_time >= lastReconnect) {
//                 time_since_reconnect = current_time - lastReconnect;
//             } else {
//                 time_since_reconnect = (0xFFFFFFFF - lastReconnect) + current_time + 1;
//             }
            
//             if (time_since_reconnect > pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS)) {
//                 ESP_LOGW(TAG, "⚠️ Too many failures, recreating client...");
//                 mqtt_reconnect();
//                 lastReconnect = current_time;
//             }
//         } else if (!mqtt_client_started) {
//             // Client not started yet - start it once
//             mqtt_reconnect();
//         }
        
//         // Check for new sensor data
//         if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//             bool ready = sensorData.dataReceived;
//             float t = sensorData.temperature;
//             float h = sensorData.humidity;
            
//             if (ready) {
//                 sensorData.dataReceived = false;
//                 lastTemp = t;
//                 lastHum = h;
                
//                 xSemaphoreGive(sensorMutex);
                
//                 // Publish if MQTT is connected
//                 if (mqtt_connected) {
//                     if (mqtt_publish_sensor_data(t, h)) {
//                         publishCount++;
//                         lastPublish = current_time;
//                     }
//                 }
//             } else {
//                 xSemaphoreGive(sensorMutex);
//             }
//         }
        
//         // Warning if no new data for too long
//         if (lastPublish > 0) {
//             TickType_t time_since_publish;
//             if (current_time >= lastPublish) {
//                 time_since_publish = current_time - lastPublish;
//             } else {
//                 time_since_publish = (0xFFFFFFFF - lastPublish) + current_time + 1;
//             }
            
//             if (time_since_publish > pdMS_TO_TICKS(60000)) {  // 60 seconds
//                 ESP_LOGW(TAG, "⚠️ No new data for 60s (Last: %.1f°C, %.1f%%)", 
//                         lastTemp, lastHum);
//                 lastPublish = current_time; // Reset to avoid spam
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// void mqtt_client_init(void)
// {
//     ESP_LOGI(TAG, "🔧 Initializing MQTT subsystem...");
    
//     // Initial connection attempt
//     mqtt_reconnect();
    
//     // Create MQTT task
//     BaseType_t ret = xTaskCreatePinnedToCore(
//         mqtt_task, 
//         "MQTTTask", 
//         STACK_SIZE_MQTT_TASK, 
//         NULL, 
//         PRIORITY_MQTT_TASK, 
//         NULL, 
//         0
//     );
    
//     if (ret == pdPASS) {
//         ESP_LOGI(TAG, "✅ MQTT task created");
//     } else {
//         ESP_LOGE(TAG, "❌ Failed to create MQTT task!");
//     }
// }
// /**
//  * @file mqtt_client_config.c
//  * @brief MQTT client implementation - FIXED
//  * @version 1.1 - Prevents multiple start() calls
//  */

// #include "mqtt_client_config.h"
// #include "config.h"
// #include "wifi_manager.h"
// #include <string.h>
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "mqtt_client.h"

// static const char *TAG = "MQTT";

// static esp_mqtt_client_handle_t mqtt_client = NULL;
// static bool mqtt_connected = false;
// static bool mqtt_client_created = false;   // ✅ Track if client exists
// static bool mqtt_client_started = false;   // ✅ Track if client is started
// static int consecutive_failures = 0;

// // ✅ Forward declaration - function টা আগে declare করতে হবে
// static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous);

// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
//                                int32_t event_id, void *event_data)
// {
//     esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
//     switch ((esp_mqtt_event_id_t)event_id) {
//         case MQTT_EVENT_CONNECTED:
//             ESP_LOGI(TAG, "✅ MQTT Connected!");
//             mqtt_connected = true;
//             consecutive_failures = 0;
//             break;
            
//         case MQTT_EVENT_DISCONNECTED:
//             ESP_LOGW(TAG, "🔴 MQTT Disconnected");
//             mqtt_connected = false;
//             consecutive_failures++;
//             break;
            
//         case MQTT_EVENT_PUBLISHED:
//             ESP_LOGD(TAG, "📤 Message published, msg_id=%d", event->msg_id);
//             break;
            
//         case MQTT_EVENT_ERROR:
//             ESP_LOGE(TAG, "❌ MQTT Error");
//             if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//                 ESP_LOGE(TAG, "   TCP Transport Error");
//             }
//             consecutive_failures++;
//             break;
            
//         default:
//             break;
//     }
// }

// static void mqtt_cleanup(void)
// {
//     if (mqtt_client != NULL) {
//         ESP_LOGI(TAG, "🧹 Cleaning up MQTT client...");
        
//         if (mqtt_client_started) {
//             esp_mqtt_client_stop(mqtt_client);
//             mqtt_client_started = false;
//         }
        
//         esp_mqtt_client_destroy(mqtt_client);
//         mqtt_client = NULL;
//         mqtt_client_created = false;
//         mqtt_connected = false;
        
//         ESP_LOGI(TAG, "✅ MQTT client cleaned up");
//     }
// }

// void mqtt_reconnect(void)
// {
//     if (!wifi_is_connected()) {
//         ESP_LOGW(TAG, "⚠️ WiFi not connected, skipping MQTT reconnect");
//         return;
//     }
    
//     // ✅ If too many failures, recreate from scratch
//     if (consecutive_failures > 5 && mqtt_client_created) {
//         ESP_LOGW(TAG, "⚠️ Too many failures (%d), recreating client...", consecutive_failures);
//         mqtt_cleanup();
//         consecutive_failures = 0;
//     }
    
//     // ✅ Create client if it doesn't exist
//     if (!mqtt_client_created) {
//         ESP_LOGI(TAG, "🔄 Creating MQTT client...");
        
//         esp_mqtt_client_config_t mqtt_cfg = {
//             .broker.address.uri = MQTT_BROKER_URI,
//             .credentials = {
//                 .username = MQTT_USER,
//                 .authentication.password = MQTT_PASS,
//                 .client_id = MQTT_CLIENT_ID,
//             },
//             .session = {
//                 .keepalive = MQTT_KEEPALIVE,
//             },
//             .network = {
//                 .timeout_ms = MQTT_TIMEOUT_MS,
//                 .disable_auto_reconnect = false,  // ✅ ESP-IDF handles reconnection
//             },
//         };
        
//         mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
//         if (mqtt_client == NULL) {
//             ESP_LOGE(TAG, "❌ Failed to initialize MQTT client");
//             consecutive_failures++;
//             return;
//         }
        
//         esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
//                                        mqtt_event_handler, NULL);
        
//         mqtt_client_created = true;
//         ESP_LOGI(TAG, "✅ MQTT client created");
//     }
    
//     // ✅ CRITICAL FIX: Only call start() ONCE
//     if (mqtt_client_created && !mqtt_client_started) {
//         ESP_LOGI(TAG, "🔄 Starting MQTT client...");
//         ESP_LOGI(TAG, "   Broker: %s", MQTT_BROKER_URI);
//         ESP_LOGI(TAG, "   Keepalive: %d seconds", MQTT_KEEPALIVE);
        
//         esp_err_t err = esp_mqtt_client_start(mqtt_client);
//         if (err == ESP_OK) {
//             mqtt_client_started = true;
//             ESP_LOGI(TAG, "✅ MQTT client started - auto-reconnect enabled");
//         } else if (err == ESP_FAIL) {
//             // Already running - this is OK
//             mqtt_client_started = true;
//             ESP_LOGI(TAG, "✅ MQTT client already running");
//         } else {
//             ESP_LOGE(TAG, "❌ MQTT start failed: %s", esp_err_to_name(err));
//             consecutive_failures++;
//             mqtt_cleanup();  // ✅ Clean up on failure
//         }
//     }
// }

// bool mqtt_is_connected(void)
// {
//     return mqtt_connected;
// }

// bool mqtt_publish_sensor_data(float temperature, float humidity)
// {
//     if (!mqtt_connected) {
//         ESP_LOGD(TAG, "⚠️ MQTT not connected, cannot publish");
//         return false;
//     }
    
//     char temp_buf[16];
//     char hum_buf[16];
    
//     snprintf(temp_buf, sizeof(temp_buf), "%.1f", temperature);
//     snprintf(hum_buf, sizeof(hum_buf), "%.1f", humidity);
    
//     int temp_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, 
//                                               temp_buf, 0, 0, 0);
//     int hum_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HUMIDITY, 
//                                              hum_buf, 0, 0, 0);
    
//     if (temp_msg_id >= 0 && hum_msg_id >= 0) {
//         ESP_LOGI(TAG, "📤 Published → TEMP: %.1f°C | HUM: %.1f%%", 
//                  temperature, humidity);
//         return true;
//     } else {
//         ESP_LOGE(TAG, "❌ Publish failed!");
//         consecutive_failures++;
//         return false;
//     }
// }

// static void mqtt_task(void *pvParameters)
// {
//     TickType_t lastReconnect = 0;
//     TickType_t lastPublish = 0;
//     float lastTemp = 0;
//     float lastHum = 0;
    
//     ESP_LOGI(TAG, "📡 MQTT task started");
    
//     // ✅ Initial connection attempt
//     mqtt_reconnect();
    
//     while (1) {
//         TickType_t current_time = xTaskGetTickCount();
        
//         // ✅ Only recreate if critical failure
//         if (!mqtt_connected && consecutive_failures > 5) {
//             TickType_t time_since_reconnect = safe_time_diff(current_time, lastReconnect);
            
//             if (time_since_reconnect > pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS)) {
//                 ESP_LOGW(TAG, "⚠️ Attempting recovery from failures...");
//                 mqtt_reconnect();
//                 lastReconnect = current_time;
//             }
//         } else if (!mqtt_client_started) {
//             // Not started yet - try to start
//             mqtt_reconnect();
//         }
        
//         // ✅ Check for new sensor data
//         if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
//             if (sensorData.dataReceived) {
//                 float t = sensorData.temperature;
//                 float h = sensorData.humidity;
//                 sensorData.dataReceived = false;
                
//                 lastTemp = t;
//                 lastHum = h;
                
//                 xSemaphoreGive(sensorMutex);
                
//                 if (mqtt_connected) {
//                     mqtt_publish_sensor_data(t, h);
//                     lastPublish = current_time;
//                 }
//             } else {
//                 xSemaphoreGive(sensorMutex);
//             }
//         }
        
//         // ✅ Warning if no data for too long
//         if (lastPublish > 0) {
//             TickType_t time_since_publish = safe_time_diff(current_time, lastPublish);
            
//             if (time_since_publish > pdMS_TO_TICKS(60000)) {
//                 ESP_LOGW(TAG, "⚠️ No new data for 60s (Last: %.1f°C, %.1f%%)", 
//                         lastTemp, lastHum);
//                 lastPublish = current_time;
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// // ✅ Helper function definition (এখন সব function call এর AFTER আছে)
// static inline TickType_t safe_time_diff(TickType_t current, TickType_t previous)
// {
//     if (current >= previous) {
//         return current - previous;
//     } else {
//         return (0xFFFFFFFF - previous) + current + 1;
//     }
// }

// void mqtt_client_init(void)
// {
//     ESP_LOGI(TAG, "🔧 Initializing MQTT subsystem...");
    
//     BaseType_t ret = xTaskCreatePinnedToCore(
//         mqtt_task, 
//         "MQTTTask", 
//         STACK_SIZE_MQTT_TASK, 
//         NULL, 
//         PRIORITY_MQTT_TASK, 
//         NULL, 
//         0
//     );
    
//     if (ret == pdPASS) {
//         ESP_LOGI(TAG, "✅ MQTT task created");
//     } else {
//         ESP_LOGE(TAG, "❌ Failed to create MQTT task!");
//     }
// }

/**
 * @file mqtt_client_config.c
 * @brief MQTT client - Rate Limited & Burst Prevention
 * @version 1.3 - FIXED: Data burst issue with proper rate limiting
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

// ✅ Rate limiting: Minimum 3 seconds between publishes
#define MIN_PUBLISH_INTERVAL_MS     3000
#define BURST_DETECTION_COUNT       5      // If 5+ messages in quick succession
#define BURST_COOLDOWN_MS           10000  // Wait 10 seconds after burst

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static bool mqtt_client_created = false;
static bool mqtt_client_started = false;
static int consecutive_failures = 0;

// ✅ Burst detection
static uint32_t messages_in_last_second = 0;
static TickType_t burst_cooldown_until = 0;

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
    
    if (consecutive_failures > 5 && mqtt_client_created) {
        ESP_LOGW(TAG, "⚠️ Too many failures (%d), recreating client...", consecutive_failures);
        mqtt_cleanup();
        consecutive_failures = 0;
    }
    
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
                .disable_auto_reconnect = false,
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
    
    if (mqtt_client_created && !mqtt_client_started) {
        ESP_LOGI(TAG, "🔄 Starting MQTT client...");
        ESP_LOGI(TAG, "   Broker: %s", MQTT_BROKER_URI);
        ESP_LOGI(TAG, "   Rate limit: 1 msg per %d seconds", MIN_PUBLISH_INTERVAL_MS/1000);
        
        esp_err_t err = esp_mqtt_client_start(mqtt_client);
        if (err == ESP_OK) {
            mqtt_client_started = true;
            ESP_LOGI(TAG, "✅ MQTT client started");
        } else if (err == ESP_FAIL) {
            mqtt_client_started = true;
            ESP_LOGI(TAG, "✅ MQTT client already running");
        } else {
            ESP_LOGE(TAG, "❌ MQTT start failed: %s", esp_err_to_name(err));
            consecutive_failures++;
            mqtt_cleanup();
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
    
    int temp_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, 
                                              temp_buf, 0, 0, 0);
    int hum_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HUMIDITY, 
                                             hum_buf, 0, 0, 0);
    
    if (temp_msg_id >= 0 && hum_msg_id >= 0) {
        ESP_LOGI(TAG, "📤 Published → T=%.1f°C H=%.1f%%", temperature, humidity);
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
    TickType_t lastBurstCheck = 0;
    float lastTemp = 0;
    float lastHum = 0;
    uint32_t discarded_count = 0;
    
    ESP_LOGI(TAG, "📡 MQTT task started (rate limited mode)");
    
    mqtt_reconnect();
    
    while (1) {
        TickType_t current_time = xTaskGetTickCount();
        
        // ========== CONNECTION MANAGEMENT ==========
        if (!mqtt_connected && consecutive_failures > 5) {
            TickType_t time_since_reconnect = safe_time_diff(current_time, lastReconnect);
            
            if (time_since_reconnect > pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS)) {
                ESP_LOGW(TAG, "⚠️ Attempting recovery...");
                mqtt_reconnect();
                lastReconnect = current_time;
            }
        } else if (!mqtt_client_started) {
            mqtt_reconnect();
        }
        
        // ========== BURST DETECTION RESET ==========
        if (safe_time_diff(current_time, lastBurstCheck) >= pdMS_TO_TICKS(1000)) {
            if (messages_in_last_second >= BURST_DETECTION_COUNT) {
                ESP_LOGW(TAG, "⚠️ BURST DETECTED! %lu msgs in 1s - cooldown activated", 
                         messages_in_last_second);
                burst_cooldown_until = current_time + pdMS_TO_TICKS(BURST_COOLDOWN_MS);
            }
            messages_in_last_second = 0;
            lastBurstCheck = current_time;
        }
        
        // ========== SENSOR DATA PROCESSING ==========
        if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (sensorData.dataReceived) {
                float t = sensorData.temperature;
                float h = sensorData.humidity;
                
                // ✅ ALWAYS clear flag immediately (prevents buffer overflow)
                sensorData.dataReceived = false;
                
                xSemaphoreGive(sensorMutex);
                
                // ✅ Check if we're in burst cooldown
                if (current_time < burst_cooldown_until) {
                    TickType_t cooldown_remaining = safe_time_diff(burst_cooldown_until, current_time);
                    ESP_LOGD(TAG, "🚫 In cooldown, discarding data (%.1fs left)", 
                             cooldown_remaining * portTICK_PERIOD_MS / 1000.0f);
                    discarded_count++;
                    vTaskDelay(pdMS_TO_TICKS(500));
                    continue;
                }
                
                // ✅ Check rate limit
                TickType_t time_since_publish = safe_time_diff(current_time, lastPublish);
                
                if (!mqtt_connected) {
                    ESP_LOGD(TAG, "⏳ Waiting for MQTT connection...");
                    discarded_count++;
                } 
                else if (time_since_publish < pdMS_TO_TICKS(MIN_PUBLISH_INTERVAL_MS)) {
                    // Too soon - discard silently
                    discarded_count++;
                    ESP_LOGD(TAG, "⏱️ Rate limit: wait %.1fs (T=%.1f H=%.1f)", 
                             (MIN_PUBLISH_INTERVAL_MS - time_since_publish * portTICK_PERIOD_MS) / 1000.0f,
                             t, h);
                } 
                else {
                    // ✅ OK to publish
                    if (mqtt_publish_sensor_data(t, h)) {
                        lastPublish = current_time;
                        lastTemp = t;
                        lastHum = h;
                        messages_in_last_second++;
                        
                        if (discarded_count > 0) {
                            ESP_LOGI(TAG, "📊 Discarded %lu msgs since last publish", discarded_count);
                            discarded_count = 0;
                        }
                    }
                }
            } else {
                xSemaphoreGive(sensorMutex);
            }
        }
        
        // ========== NO DATA WARNING ==========
        if (lastPublish > 0) {
            TickType_t time_since_publish = safe_time_diff(current_time, lastPublish);
            
            if (time_since_publish > pdMS_TO_TICKS(60000)) {
                ESP_LOGW(TAG, "⚠️ No publish for 60s (Last: T=%.1f H=%.1f)", 
                         lastTemp, lastHum);
                lastPublish = current_time;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(300));  // ✅ Check every 300ms
    }
}

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
    ESP_LOGI(TAG, "🔧 Initializing MQTT with rate limiting...");
    
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
        ESP_LOGI(TAG, "✅ MQTT task created (rate: 1 msg/%ds)", MIN_PUBLISH_INTERVAL_MS/1000);
    } else {
        ESP_LOGE(TAG, "❌ Failed to create MQTT task!");
    }
}