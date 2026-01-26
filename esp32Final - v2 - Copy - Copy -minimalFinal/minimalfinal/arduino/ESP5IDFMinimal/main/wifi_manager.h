/**
 * @file wifi_manager.h
 * @brief WiFi connection and captive portal management
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/**
 * @brief Initialize WiFi manager
 * @details Loads credentials from NVS and connects if available,
 *          otherwise starts captive portal
 */
void wifi_manager_init(void);

/**
 * @brief Start WiFi in station mode with saved credentials
 */
void wifi_start_station(void);

/**
 * @brief Start captive portal for WiFi configuration
 */
void wifi_start_portal(void);

/**
 * @brief Save WiFi credentials to NVS
 * @param ssid WiFi SSID
 * @param password WiFi password
 */
void wifi_save_credentials(const char *ssid, const char *password);

/**
 * @brief Load WiFi credentials from NVS
 * @param ssid Buffer to store SSID (min 32 bytes)
 * @param password Buffer to store password (min 64 bytes)
 * @return true if credentials loaded successfully
 */
bool wifi_load_credentials(char *ssid, char *password);

/**
 * @brief Check if WiFi is connected
 * @return true if connected
 */
bool wifi_is_connected(void);

/**
 * @brief Get WiFi event group handle
 * @return Event group handle
 */
EventGroupHandle_t wifi_get_event_group(void);

#endif // WIFI_MANAGER_H