
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "esp_log.h"
// #include "mqtt_client_config.h"
// #include "driver/ledc.h"
// #include "esp_system.h"
// #include "esp_random.h"  // For random number generation
// #include "wifi_manager.h"
// #include "factory_reset.h"
// #include "dht22_handler.h"
// #include "ds18b20_handler.h"
// #include "mq135_handler.h"
// #include "ultrasonic_handler.h"
// #include "motor_controller.h"
// #include "config.h"
// #include "led_config.h"
// #include "tutorial.h"  
// #define TAG "APP_MAIN"

// // Queue handles (size = 1, will use xQueueOverwrite)
// static QueueHandle_t dht22Queue;
// static QueueHandle_t ds18b20Queue;
// static QueueHandle_t mq135Queue;
// static QueueHandle_t sonar1Queue;
// static QueueHandle_t sonar2Queue;
// static QueueHandle_t randomSugarQueue;  // New queue for random sugar data

// // Task handles
// static TaskHandle_t TaskDHT22Handle;
// static TaskHandle_t TaskDS18B20Handle;
// static TaskHandle_t TaskMQ135Handle;
// static TaskHandle_t TaskSonar1Handle;
// static TaskHandle_t TaskSonar2Handle;
// static TaskHandle_t TaskControlHandle;
// static TaskHandle_t TaskMQTTHandle;
// static TaskHandle_t TaskRandomSugarHandle;  // New task handle

// // Data structures
// typedef dht22_data_t DHT22Data;
// typedef struct { float temperature; } DS18B20Data;
// typedef struct { float alcohol; } MQ135Data;
// typedef struct { float distance; } SonarData;
// typedef struct { float value; } RandomSugarData;  // New data structure

// // Global motor speed control
// static uint8_t target_fan_speed = 0;

// // ============ Random Sugar Task ============
// static void random_sugar_task(void *param) {
//     RandomSugarData data;
    
//     for (;;) {
//         // Generate random number between 20 and 40
//         uint32_t random_val = esp_random();
//         data.value = 20.0f + ((float)(random_val % 2001) / 100.0f); // 20.00 to 40.00
        
//         ESP_LOGI(TAG, "🎲 Random Sugar - Value: %.2f", data.value);
//         xQueueOverwrite(randomSugarQueue, &data);
        
//         vTaskDelay(pdMS_TO_TICKS(5000)); // Send every 5 seconds
//     }
// }

// // ============ DHT22 Task ============
// static void dht22_task(void *param) {
//     DHT22Data data;
//     int consecutive_errors = 0;
    
//     for (;;) {
//         esp_err_t result = dht22_read(&data);
        
//         if (result == ESP_OK) {
//             consecutive_errors = 0;
//             ESP_LOGI(TAG, "🌡️ DHT22 - Temp: %.1f°C, Humidity: %.1f%%", 
//                      data.temperature, data.humidity);
//             xQueueOverwrite(dht22Queue, &data);
//         } else {
//             consecutive_errors++;
//             if (consecutive_errors % 5 == 1) {
//                 ESP_LOGW(TAG, "Failed to read DHT22 (error count: %d)", consecutive_errors);
//             }
//             if (consecutive_errors > 20) {
//                 ESP_LOGE(TAG, "DHT22 failed 20+ times, restarting...");
//                 esp_restart();
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(3000));
//     }
// }

// // ============ DS18B20 Task ============
// static void ds18b20_task(void *param) {
//     DS18B20Data data;
    
//     for (;;) {
//         if (ds18b20_read(&data.temperature) == ESP_OK) {
//             ESP_LOGI(TAG, "🔥 DS18B20 - Temp: %.2f°C", data.temperature);
//             xQueueOverwrite(ds18b20Queue, &data);
//         } else {
//             ESP_LOGW(TAG, "Failed to read DS18B20");
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }

// // ============ MQ135 Task ============
// static void mq135_task(void *param) {
//     MQ135Data data;
    
//     for (;;) {
//         if (mq135_read_alcohol(&data.alcohol) == ESP_OK) {
//             ESP_LOGI(TAG, "🍷 MQ135 - Alcohol: %.2f ppm", data.alcohol);
//             xQueueOverwrite(mq135Queue, &data);
//         } else {
//             ESP_LOGW(TAG, "Failed to read MQ135");
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }

// // ============ Sonar1 Task ============
// static void sonar1_task(void *param) {
//     SonarData data;
    
//     for (;;) {
//         if (ultrasonic_read_sonar1(&data.distance) == ESP_OK) {
//             ESP_LOGI(TAG, "📏 Sonar1 - Distance: %.2f cm", data.distance);
//             xQueueOverwrite(sonar1Queue, &data);
//         } else {
//             ESP_LOGW(TAG, "Failed to read Sonar1");
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// // ============ Sonar2 Task ============
// static void sonar2_task(void *param) {
//     SonarData data;
    
//     for (;;) {
//         if (ultrasonic_read_sonar2(&data.distance) == ESP_OK) {
//             ESP_LOGI(TAG, "📏 Sonar2 - Distance: %.2f cm", data.distance);
//             xQueueOverwrite(sonar2Queue, &data);
//         } else {
//             ESP_LOGW(TAG, "Failed to read Sonar2");
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// // ============ Control Task (Motor Logic) ============
// static void control_task(void *param) {
//     DS18B20Data ds_data;
//     SonarData sonar_data;
//     bool pump_state = false;
//     bool fan_state = false;
//     uint8_t current_fan_speed = 0;
    
//     for (;;) {
//         // Check DS18B20 temperature for fan control
//         if (xQueuePeek(ds18b20Queue, &ds_data, 0) == pdTRUE) {
//             if (ds_data.temperature > DS18B20_TEMP_THRESHOLD) {
//                 if (target_fan_speed == 0) {
//                     target_fan_speed = 128;
//                 }
                
//                 if (!fan_state || current_fan_speed != target_fan_speed) {
//                     motor_set_fan_speed(target_fan_speed);
//                     fan_state = true;
//                     current_fan_speed = target_fan_speed;
//                     ESP_LOGI(TAG, "🌡️ Temp > %.1f°C, Fan ON at speed %d", 
//                              DS18B20_TEMP_THRESHOLD, target_fan_speed);
//                 }
//             } else {
//                 if (fan_state) {
//                     motor_set_fan_speed(0);
//                     fan_state = false;
//                     current_fan_speed = 0;
//                     ESP_LOGI(TAG, "🌡️ Temp < %.1f°C, Fan OFF", DS18B20_TEMP_THRESHOLD);
//                 }
//             }
//         }
        
//         // Check Sonar1 for water pump control
//         if (xQueuePeek(sonar1Queue, &sonar_data, 0) == pdTRUE) {
//             if (sonar_data.distance > SONAR1_DISTANCE_THRESHOLD && !pump_state) {
//                 motor_set_pump(true);
//                 pump_state = true;
//                 ESP_LOGI(TAG, "💧 Distance > %.1fcm, Pump ON", SONAR1_DISTANCE_THRESHOLD);
//             } else if (sonar_data.distance <= SONAR1_DISTANCE_THRESHOLD && pump_state) {
//                 motor_set_pump(false);
//                 pump_state = false;
//                 ESP_LOGI(TAG, "💧 Distance <= %.1fcm, Pump OFF", SONAR1_DISTANCE_THRESHOLD);
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(200));
//     }
// }

// // ============ MQTT Task ============
// static void mqtt_task(void *param) {
//     char msg[32];
//     DHT22Data dht_data;
//     DS18B20Data ds_data;
//     MQ135Data mq_data;
//     SonarData sonar2_data;
//     SonarData sonar1_data;
//     RandomSugarData random_data;  // New data
    
//     for (;;) {
//         if (xQueuePeek(dht22Queue, &dht_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.1f", dht_data.temperature);
//             mqtt_publish(MQTT_TOPIC_PUB1, msg);
            
//             snprintf(msg, sizeof(msg), "%.1f", dht_data.humidity);
//             mqtt_publish(MQTT_TOPIC_PUB2, msg);
            
//             ESP_LOGI(TAG, "📤 MQTT: DHT22 - Temp=%.1f°C, Humidity=%.1f%%", 
//                      dht_data.temperature, dht_data.humidity);
//         }
        
//         if (xQueuePeek(ds18b20Queue, &ds_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.2f", ds_data.temperature);
//             mqtt_publish("ds18", msg);
//             ESP_LOGI(TAG, "📤 MQTT: DS18B20 - Temp=%.2f°C", ds_data.temperature);
//         }
        
//         if (xQueuePeek(mq135Queue, &mq_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.2f", mq_data.alcohol);
//             mqtt_publish("CO2", msg);
//             ESP_LOGI(TAG, "📤 MQTT: MQ135 - Alcohol=%.2f ppm", mq_data.alcohol);
//         }
        
//         if (xQueuePeek(sonar1Queue, &sonar1_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.2f", sonar1_data.distance);
//             mqtt_publish("level", msg);
//             ESP_LOGI(TAG, "📤 MQTT: Sonar1 - Distance=%.2f cm (level)", sonar1_data.distance);
//         }
        
//         if (xQueuePeek(sonar2Queue, &sonar2_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.2f", sonar2_data.distance);
//             mqtt_publish("sugar", msg);
//             ESP_LOGI(TAG, "📤 MQTT: Sonar2 - Distance=%.2f cm", sonar2_data.distance);
//         }
        
//         // Publish random sugar data to "auto" topic
//         if (xQueuePeek(randomSugarQueue, &random_data, 0) == pdTRUE) {
//             snprintf(msg, sizeof(msg), "%.2f", random_data.value);
//             mqtt_publish("auto", msg);
//             ESP_LOGI(TAG, "📤 MQTT: Random Sugar - Value=%.2f (topic: auto)", random_data.value);
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

// // ============ Watchdog Task ============
// static void watchdog_task(void *param) {
//     for (;;) {
//         vTaskDelay(pdMS_TO_TICKS(30000));
        
//         // Check if critical tasks are still running
//         if (eTaskGetState(TaskDHT22Handle) == eDeleted ||
//             eTaskGetState(TaskMQTTHandle) == eDeleted) {
//             ESP_LOGW(TAG, "⚠️ Critical task died, restarting...");
//             esp_restart();
//         }
//     }
// }

// // ============ Main ============
// void app_main(void) {
//     ESP_LOGI(TAG, "🚀 ESP32 Multi-Sensor System Starting...");
    
//     // LED initialize করা (সবার প্রথমে)
//     led_init();
//     led_off();
    
//     factory_reset_init();
//     ESP_ERROR_CHECK(wifi_manager_init());
    
//     if (wifi_manager_is_configured()) {
//         ESP_LOGI(TAG, "✅ Wi-Fi credentials found");
        
//         // WiFi connecting শুরু - LED blink করানো
//         ESP_LOGI(TAG, "🔄 Starting LED blink for connecting...");
//         led_blink_start();
//         vTaskDelay(pdMS_TO_TICKS(500));
//         ESP_LOGI(TAG, "🔄 Connecting to Wi-Fi...");
        
//         esp_err_t wifi_result = wifi_manager_connect();
//         ESP_LOGI(TAG, "📡 WiFi connect result: %s", esp_err_to_name(wifi_result));
        
//         if (wifi_result == ESP_OK) {

//             //DNS
//  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
//         if (netif != NULL) {
//             esp_netif_dns_info_t dns_info;
            
//             // Primary DNS: 8.8.8.8
//             dns_info.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 8, 8);
//             dns_info.ip.type = ESP_IPADDR_TYPE_V4;
//             esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
            
//             // Secondary DNS: 8.8.4.4
//             dns_info.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 4, 4);
//             esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
            
//             ESP_LOGI(TAG, "✅ DNS Set: Primary=8.8.8.8, Backup=8.8.4.4");
//         }

// //DNS


//             ESP_LOGI(TAG, "✅ WiFi Connected! Stopping blink...");
//             led_blink_stop();
//             vTaskDelay(pdMS_TO_TICKS(100));
//             ESP_LOGI(TAG, "🔵 Turning LED ON...");
//             led_on();
//             vTaskDelay(pdMS_TO_TICKS(100));
//             ESP_LOGI(TAG, "✅ Connected to Wi-Fi - LED should be ON now!");
//         } else {
//             ESP_LOGW(TAG, "⚠️ Connection failed, starting portal...");
//             wifi_manager_start_portal();
//             ESP_LOGI(TAG, "📱 Connect to 'Bowl-Setup' at 192.168.4.1");
//             return;
//         }
//     } else {
//         ESP_LOGI(TAG, "⚙️ No credentials, starting portal...");
//         led_blink_start();
//         wifi_manager_start_portal();
//         ESP_LOGI(TAG, "📱 Connect to 'Bowl-Setup' at 192.168.4.1");
//         return;
//     }
    
//     ESP_ERROR_CHECK(mqtt_start());
//     ESP_LOGI(TAG, "✅ MQTT Started");
    
//     // Initialize hardware
//     ESP_ERROR_CHECK(dht22_init());
    
//     esp_err_t err = ds18b20_init();
//     if (err != ESP_OK) {
//         ESP_LOGW("APP_MAIN", "DS18B20 not detected, continuing without it");
//     }

//     ESP_ERROR_CHECK(mq135_init());
//     ESP_ERROR_CHECK(ultrasonic_init());
//     ESP_ERROR_CHECK(motor_controller_init());
    
//     // Create queues with size 1 (latest value only)
//     dht22Queue = xQueueCreate(1, sizeof(DHT22Data));
//     ds18b20Queue = xQueueCreate(1, sizeof(DS18B20Data));
//     mq135Queue = xQueueCreate(1, sizeof(MQ135Data));
//     sonar1Queue = xQueueCreate(1, sizeof(SonarData));
//     sonar2Queue = xQueueCreate(1, sizeof(SonarData));
//     randomSugarQueue = xQueueCreate(1, sizeof(RandomSugarData));  // New queue
    
//     // Create tasks
//     xTaskCreatePinnedToCore(dht22_task, "DHT22Task", 4096, NULL, 2, &TaskDHT22Handle, 1);
//     xTaskCreatePinnedToCore(ds18b20_task, "DS18B20Task", 4096, NULL, 2, &TaskDS18B20Handle, 1);
//     xTaskCreatePinnedToCore(mq135_task, "MQ135Task", 4096, NULL, 2, &TaskMQ135Handle, 0);
//     xTaskCreatePinnedToCore(sonar1_task, "Sonar1Task", 3072, NULL, 2, &TaskSonar1Handle, 0);
//     xTaskCreatePinnedToCore(sonar2_task, "Sonar2Task", 3072, NULL, 2, &TaskSonar2Handle, 1);
//     xTaskCreatePinnedToCore(control_task, "ControlTask", 4096, NULL, 3, &TaskControlHandle, 0);
//     xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 3, &TaskMQTTHandle, 0);
//     xTaskCreatePinnedToCore(watchdog_task, "WatchdogTask", 2048, NULL, 1, NULL, 1);
    
//     // Create Random Sugar Task
//     xTaskCreatePinnedToCore(random_sugar_task, "RandomSugarTask", 3072, NULL, 2, &TaskRandomSugarHandle, 0);
    
//     ESP_LOGI(TAG, "✅ All tasks started successfully!");
//     ESP_LOGI(TAG, "🎲 Random Sugar Task: Publishing 20-40 range to 'auto' topic every 5s");
// }

// // MQTT callback to control fan speed
// void mqtt_set_fan_speed_from_text(uint8_t speed) {
//     target_fan_speed = speed;
//     ESP_LOGI(TAG, "🎛️ Fan speed updated from MQTT: %d", speed);
// }
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client_config.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "esp_random.h"
#include "wifi_manager.h"
#include "factory_reset.h"
#include "dht22_handler.h"
#include "ds18b20_handler.h"
#include "mq135_handler.h"
#include "ultrasonic_handler.h"
#include "motor_controller.h"
#include "config.h"
#include "led_config.h"
#include "tutorial.h"  
#define TAG "APP_MAIN"

// Queue handles
static QueueHandle_t dht22Queue;
static QueueHandle_t ds18b20Queue;
static QueueHandle_t mq135Queue;
static QueueHandle_t sonar1Queue;
static QueueHandle_t sonar2Queue;
static QueueHandle_t randomSugarQueue;

// Task handles
static TaskHandle_t TaskDHT22Handle;
static TaskHandle_t TaskDS18B20Handle;
static TaskHandle_t TaskMQ135Handle;
static TaskHandle_t TaskSonar1Handle;
static TaskHandle_t TaskSonar2Handle;
static TaskHandle_t TaskControlHandle;
static TaskHandle_t TaskMQTTHandle;
static TaskHandle_t TaskRandomSugarHandle;

// Data structures
typedef dht22_data_t DHT22Data;
typedef struct { float temperature; } DS18B20Data;
typedef struct { float alcohol; } MQ135Data;
typedef struct { float distance; } SonarData;
typedef struct { float value; } RandomSugarData;

// Global motor speed control
static uint8_t target_fan_speed = 0;

// ============ DYNAMIC THRESHOLDS (can be changed via MQTT) ============
static float ds18b20_temp_threshold = DEFAULT_DS18B20_TEMP_THRESHOLD;
static float sonar1_distance_threshold = DEFAULT_SONAR1_DISTANCE_THRESHOLD;

// ============ Random Sugar Task ============
static void random_sugar_task(void *param) {
    RandomSugarData data;
    
    for (;;) {
        uint32_t random_val = esp_random();
        data.value = 20.0f + ((float)(random_val % 2001) / 100.0f);
        
        ESP_LOGI(TAG, "🎲 Random Sugar - Value: %.2f", data.value);
        xQueueOverwrite(randomSugarQueue, &data);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ============ DHT22 Task ============
// static void dht22_task(void *param) {
//     DHT22Data data;
//     int consecutive_errors = 0;
    
//     for (;;) {
//         esp_err_t result = dht22_read(&data);
        
//         if (result == ESP_OK) {
//             consecutive_errors = 0;
//             ESP_LOGI(TAG, "🌡️ DHT22 - Temp: %.1f°C, Humidity: %.1f%%", 
//                      data.temperature, data.humidity);
//             xQueueOverwrite(dht22Queue, &data);
//         } else {
//             consecutive_errors++;
//             if (consecutive_errors % 5 == 1) {
//                 ESP_LOGW(TAG, "Failed to read DHT22 (error count: %d)", consecutive_errors);
//             }
//             if (consecutive_errors > 20) {
//                 ESP_LOGE(TAG, "DHT22 failed 20+ times, restarting...");
//                 esp_restart();
//             }
//         }
        
//         vTaskDelay(pdMS_TO_TICKS(3000));
//     }
// }
static void dht22_task(void *param) {
    DHT22Data data;
    int consecutive_errors = 0;
    bool sensor_marked_dead = false;  // NEW
    
    for (;;) {
        if (sensor_marked_dead) {
            // Sensor is dead, sleep for a long time
            vTaskDelay(pdMS_TO_TICKS(60000)); // Check once per minute
            consecutive_errors = 0; // Try again
            sensor_marked_dead = false;
            ESP_LOGI(TAG, "🔄 Retrying DHT22 after 1 minute...");
            continue;
        }
        
        esp_err_t result = dht22_read(&data);
        
        if (result == ESP_OK) {
            consecutive_errors = 0;
            ESP_LOGI(TAG, "🌡️ DHT22 - Temp: %.1f°C, Humidity: %.1f%%", 
                     data.temperature, data.humidity);
            xQueueOverwrite(dht22Queue, &data);
        } else {
            consecutive_errors++;
            if (consecutive_errors % 5 == 1) {
                ESP_LOGW(TAG, "Failed to read DHT22 (error count: %d)", consecutive_errors);
            }
            
            if (consecutive_errors > 20) {
                ESP_LOGE(TAG, "❌ DHT22 failed 20+ times - marking as DEAD");
                sensor_marked_dead = true;  // Mark it dead
                consecutive_errors = 0;
                // DON'T RESTART - just disable this sensor temporarily
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

// ============ Control Task (Motor Logic with Dynamic Thresholds) ============
static void control_task(void *param) {
    DS18B20Data ds_data;
    SonarData sonar_data;
    bool pump_state = false;
    bool fan_state = false;
    uint8_t current_fan_speed = 0;
    
    for (;;) {
        // Check DS18B20 temperature for fan control (using dynamic threshold)
        if (xQueuePeek(ds18b20Queue, &ds_data, 0) == pdTRUE) {
            if (ds_data.temperature > ds18b20_temp_threshold) {
                if (target_fan_speed == 0) {
                    target_fan_speed = 128;
                }
                
                if (!fan_state || current_fan_speed != target_fan_speed) {
                    motor_set_fan_speed(target_fan_speed);
                    fan_state = true;
                    current_fan_speed = target_fan_speed;
                    ESP_LOGI(TAG, "🌡️ Temp > %.1f°C (threshold), Fan ON at speed %d", 
                             ds18b20_temp_threshold, target_fan_speed);
                }
            } else {
                if (fan_state) {
                    motor_set_fan_speed(0);
                    fan_state = false;
                    current_fan_speed = 0;
                    ESP_LOGI(TAG, "🌡️ Temp < %.1f°C (threshold), Fan OFF", 
                             ds18b20_temp_threshold);
                }
            }
        }
        
        // Check Sonar1 for water pump control (using dynamic threshold)
        if (xQueuePeek(sonar1Queue, &sonar_data, 0) == pdTRUE) {
            if (sonar_data.distance > sonar1_distance_threshold && !pump_state) {
                motor_set_pump(true);
                pump_state = true;
                ESP_LOGI(TAG, "💧 Distance > %.1fcm (threshold), Pump ON", 
                         sonar1_distance_threshold);
            } else if (sonar_data.distance <= sonar1_distance_threshold && pump_state) {
                motor_set_pump(false);
                pump_state = false;
                ESP_LOGI(TAG, "💧 Distance <= %.1fcm (threshold), Pump OFF", 
                         sonar1_distance_threshold);
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
    RandomSugarData random_data;
    
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
        
        if (xQueuePeek(randomSugarQueue, &random_data, 0) == pdTRUE) {
            snprintf(msg, sizeof(msg), "%.2f", random_data.value);
            mqtt_publish("auto", msg);
            ESP_LOGI(TAG, "📤 MQTT: Random Sugar - Value=%.2f (topic: auto)", random_data.value);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ============ Watchdog Task ============
static void watchdog_task(void *param) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        
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
    
    led_init();
    led_off();
    
    factory_reset_init();
    ESP_ERROR_CHECK(wifi_manager_init());
    
    if (wifi_manager_is_configured()) {
        ESP_LOGI(TAG, "✅ Wi-Fi credentials found");
        
        ESP_LOGI(TAG, "🔄 Starting LED blink for connecting...");
        led_blink_start();
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG, "🔄 Connecting to Wi-Fi...");
        
        esp_err_t wifi_result = wifi_manager_connect();
        ESP_LOGI(TAG, "📡 WiFi connect result: %s", esp_err_to_name(wifi_result));
        
        if (wifi_result == ESP_OK) {
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif != NULL) {
                esp_netif_dns_info_t dns_info;
                
                dns_info.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 8, 8);
                dns_info.ip.type = ESP_IPADDR_TYPE_V4;
                esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
                
                dns_info.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 4, 4);
                esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
                
                ESP_LOGI(TAG, "✅ DNS Set: Primary=8.8.8.8, Backup=8.8.4.4");
            }

            ESP_LOGI(TAG, "✅ WiFi Connected! Stopping blink...");
            led_blink_stop();
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP_LOGI(TAG, "🔵 Turning LED ON...");
            led_on();
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP_LOGI(TAG, "✅ Connected to Wi-Fi - LED should be ON now!");
        } else {
            ESP_LOGW(TAG, "⚠️ Connection failed, starting portal...");
            wifi_manager_start_portal();
            ESP_LOGI(TAG, "📱 Connect to 'Bowl-Setup' at 192.168.4.1");
            return;
        }
    } else {
        ESP_LOGI(TAG, "⚙️ No credentials, starting portal...");
        led_blink_start();
        wifi_manager_start_portal();
        ESP_LOGI(TAG, "📱 Connect to 'Bowl-Setup' at 192.168.4.1");
        return;
    }
    
    ESP_ERROR_CHECK(mqtt_start());
    ESP_LOGI(TAG, "✅ MQTT Started");
    
    // Initialize hardware
    ESP_ERROR_CHECK(dht22_init());
    
    esp_err_t err = ds18b20_init();
    if (err != ESP_OK) {
        ESP_LOGW("APP_MAIN", "DS18B20 not detected, continuing without it");
    }

    ESP_ERROR_CHECK(mq135_init());
    ESP_ERROR_CHECK(ultrasonic_init());
    ESP_ERROR_CHECK(motor_controller_init());
    
    // Create queues with size 1
    dht22Queue = xQueueCreate(1, sizeof(DHT22Data));
    ds18b20Queue = xQueueCreate(1, sizeof(DS18B20Data));
    mq135Queue = xQueueCreate(1, sizeof(MQ135Data));
    sonar1Queue = xQueueCreate(1, sizeof(SonarData));
    sonar2Queue = xQueueCreate(1, sizeof(SonarData));
    randomSugarQueue = xQueueCreate(1, sizeof(RandomSugarData));
    
    // Create tasks
    xTaskCreatePinnedToCore(dht22_task, "DHT22Task", 4096, NULL, 2, &TaskDHT22Handle, 1);
    xTaskCreatePinnedToCore(ds18b20_task, "DS18B20Task", 4096, NULL, 2, &TaskDS18B20Handle, 1);
    xTaskCreatePinnedToCore(mq135_task, "MQ135Task", 4096, NULL, 2, &TaskMQ135Handle, 0);
    xTaskCreatePinnedToCore(sonar1_task, "Sonar1Task", 3072, NULL, 2, &TaskSonar1Handle, 0);
    xTaskCreatePinnedToCore(sonar2_task, "Sonar2Task", 3072, NULL, 2, &TaskSonar2Handle, 1);
    xTaskCreatePinnedToCore(control_task, "ControlTask", 4096, NULL, 3, &TaskControlHandle, 0);
    xTaskCreatePinnedToCore(mqtt_task, "MQTTTask", 4096, NULL, 3, &TaskMQTTHandle, 0);
    xTaskCreatePinnedToCore(watchdog_task, "WatchdogTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(random_sugar_task, "RandomSugarTask", 3072, NULL, 2, &TaskRandomSugarHandle, 0);
    
    ESP_LOGI(TAG, "✅ All tasks started successfully!");
    ESP_LOGI(TAG, "🎲 Random Sugar Task: Publishing 20-40 range to 'auto' topic every 5s");
    ESP_LOGI(TAG, "🎯 Default Thresholds - Temp: %.1f°C, Distance: %.1fcm", 
             ds18b20_temp_threshold, sonar1_distance_threshold);
}

// ============ MQTT Callbacks ============
// Callback to control fan speed
void mqtt_set_fan_speed_from_text(uint8_t speed) {
    target_fan_speed = speed;
    ESP_LOGI(TAG, "🎛️ Fan speed updated from MQTT: %d", speed);
}

// Callback to set DS18B20 temperature threshold
void mqtt_set_temp_threshold(float threshold) {
    if (threshold > 0 && threshold < 100) {
        ds18b20_temp_threshold = threshold;
        ESP_LOGI(TAG, "🌡️ Temperature threshold updated from MQTT: %.1f°C", threshold);
    } else {
        ESP_LOGW(TAG, "⚠️ Invalid temperature threshold: %.1f (must be 0-100)", threshold);
    }
}

// Callback to set Sonar1 distance threshold
void mqtt_set_distance_threshold(float threshold) {
    if (threshold > 0 && threshold < 400) {
        sonar1_distance_threshold = threshold;
        ESP_LOGI(TAG, "💧 Distance threshold updated from MQTT: %.1fcm", threshold);
    } else {
        ESP_LOGW(TAG, "⚠️ Invalid distance threshold: %.1f (must be 0-400)", threshold);
    }
}