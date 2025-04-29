#ifndef RTK_SERIAL_H
#define RTK_SERIAL_H

#include <stdint.h>

/**
 * @brief Sets up the UART serial port with the specified configuration.
 */
void setup_serial_port(void);

/**
 * @brief Reads a data packet from the UART serial interface.
 * 
 * @param total_length Pointer to a variable where the total length of the received data will be stored.
 *                     This includes the header, payload, and CRC.
 * @return A pointer to the dynamically allocated buffer containing the received data, or NULL if an error occurs.
 *         The caller is responsible for freeing the allocated memory using `free`.
 */
uint8_t *read_serial_data(uint16_t *total_length);

#endif // RTK_SERIAL_H