/**
 * @file led_config.h
 * @brief LED status indicator configuration
 */

#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <stdbool.h>

/**
 * @brief Initialize LED and start status indication task
 * @details LED behavior:
 *          - Solid ON: WiFi connected
 *          - Blinking: WiFi disconnected
 */
void led_init(void);

/**
 * @brief Set LED state
 * @param state true = ON, false = OFF
 */
void led_set_state(bool state);

#endif // LED_CONFIG_H