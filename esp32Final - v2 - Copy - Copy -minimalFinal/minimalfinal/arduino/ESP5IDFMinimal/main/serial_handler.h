/**
 * @file serial_handler.h
 * @brief Arduino UART communication handler
 */

#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <stdint.h>

/**
 * @brief Initialize UART for Arduino communication
 * @details Configures UART2 on GPIO16 (RX) and GPIO17 (TX)
 *          and starts the serial reading task
 */
void serial_init(void);

/**
 * @brief Parse temperature value from serial data string
 * @param data Serial data string (format: "TEMP:XX.X,HUM:YY.Y")
 * @return Parsed temperature value
 */
float serial_parse_temperature(const char *data);

/**
 * @brief Parse humidity value from serial data string
 * @param data Serial data string (format: "TEMP:XX.X,HUM:YY.Y")
 * @return Parsed humidity value
 */
float serial_parse_humidity(const char *data);

#endif // SERIAL_HANDLER_H