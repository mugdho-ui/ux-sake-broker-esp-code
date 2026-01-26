/**
 * @file serial_handler.c
 * @brief Arduino UART communication implementation
 */

#include "serial_handler.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "SERIAL";

/**
 * @brief Parse a value from serial data string
 * @param data Source string
 * @param key Key to search for (e.g., "TEMP:")
 * @return Parsed float value
 */
static float parse_serial_value(const char *data, const char *key)
{
    char *start = strstr(data, key);
    if (!start) return 0.0f;
    
    start += strlen(key);
    return atof(start);
}

float serial_parse_temperature(const char *data)
{
    return parse_serial_value(data, "TEMP:");
}

float serial_parse_humidity(const char *data)
{
    return parse_serial_value(data, "HUM:");
}

/**
 * @brief Serial reading task
 * @details Continuously reads UART data from Arduino and parses sensor values
 */
static void serial_task(void *pvParameters)
{
    uint8_t data[128];
    char line[256] = {0};
    int line_pos = 0;
    TickType_t lastDataTime = 0;
    bool firstData = true;
    
    ESP_LOGI(TAG, "📡 Serial task started");
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, sizeof(data) - 1, pdMS_TO_TICKS(10));
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = data[i];
                
                if (c == '\n' || c == '\r') {
                    if (line_pos > 0) {
                        line[line_pos] = '\0';
                        
                        ESP_LOGD(TAG, "📩 Arduino → %s", line);
                        
                        // Parse sensor data
                        if (strstr(line, "TEMP:") && strstr(line, "HUM:")) {
                            float t = serial_parse_temperature(line);
                            float h = serial_parse_humidity(line);
                            
                            // Validate ranges
                            if (t > -50 && t < 100 && h > 0 && h <= 100) {
                                if (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                    sensorData.temperature = t;
                                    sensorData.humidity = h;
                                    sensorData.dataReceived = true;
                                    xSemaphoreGive(sensorMutex);
                                    
                                    if (firstData) {
                                        ESP_LOGI(TAG, "✅ First valid data received!");
                                        firstData = false;
                                    }
                                    
                                    lastDataTime = xTaskGetTickCount();
                                } else {
                                    ESP_LOGW(TAG, "⚠️ Could not acquire sensor mutex");
                                }
                            } else {
                                ESP_LOGW(TAG, "⚠️ Invalid sensor values: T=%.1f, H=%.1f", t, h);
                            }
                        } else if (strstr(line, "ERROR")) {
                            ESP_LOGW(TAG, "⚠️ Arduino sensor error!");
                        }
                        
                        line_pos = 0;
                    }
                } else if (line_pos < sizeof(line) - 1) {
                    line[line_pos++] = c;
                }
            }
        }
        
        // Check if Arduino stopped sending data
        if (lastDataTime > 0 && 
            (xTaskGetTickCount() - lastDataTime) > pdMS_TO_TICKS(SERIAL_DATA_TIMEOUT_MS)) {
            ESP_LOGW(TAG, "🔴 No data from Arduino for %d seconds!", 
                    SERIAL_DATA_TIMEOUT_MS / 1000);
            lastDataTime = xTaskGetTickCount(); // Reset to avoid spam
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void serial_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing UART%d...", UART_NUM);
    
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD2, RXD2, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "✅ UART%d configured: %d baud, RX=%d, TX=%d", 
             UART_NUM, UART_BAUD_RATE, RXD2, TXD2);
    
    // Create serial task
    xTaskCreatePinnedToCore(serial_task, "SerialTask", 
                           STACK_SIZE_SERIAL_TASK, NULL, 
                           PRIORITY_SERIAL_TASK, NULL, 1);
    
    ESP_LOGI(TAG, "✅ Serial task created");
}