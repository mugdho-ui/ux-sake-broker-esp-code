/**
 * @file mqtt_client_config.h
 * @brief MQTT client configuration and management
 */

#ifndef MQTT_CLIENT_CONFIG_H
#define MQTT_CLIENT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize MQTT client and start connection task
 * @details Creates a FreeRTOS task that manages MQTT connection
 *          and publishes sensor data
 */
void mqtt_client_init(void);

/**
 * @brief Check if MQTT client is connected
 * @return true if connected to broker
 */
bool mqtt_is_connected(void);

/**
 * @brief Publish temperature and humidity to MQTT broker
 * @param temperature Temperature value in Celsius
 * @param humidity Humidity value in percentage
 * @return true if published successfully
 */
bool mqtt_publish_sensor_data(float temperature, float humidity);

/**
 * @brief Force MQTT reconnection attempt
 */
void mqtt_reconnect(void);

#endif // MQTT_CLIENT_CONFIG_H