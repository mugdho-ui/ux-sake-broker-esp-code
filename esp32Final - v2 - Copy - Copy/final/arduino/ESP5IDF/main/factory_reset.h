/**
 * @file factory_reset.h
 * @brief Factory reset button handler
 */

#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Check factory reset button state
 * @details If button (GPIO0) is held for 5 seconds, performs factory reset:
 *          - Erases all NVS data (WiFi credentials)
 *          - Restarts the device
 * 
 * This function should be called periodically from main loop
 */
void factory_reset_check(void);

#endif // FACTORY_RESET_H