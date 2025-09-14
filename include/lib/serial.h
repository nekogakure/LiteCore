/**
 * @file serial.h
 * @brief Serial port communication interface
 * @details
 * Header file for serial port communication functions
 * @author nekogakure
 */

#ifndef _SERIAL_H
#define _SERIAL_H

/**
 * @brief Initialize the serial port
 * @details Configures the serial port for basic output
 */
void init_serial(void);

/**
 * @brief Write a character to the serial port
 * @param c Character to write
 */
void serial_write_char(char c);

/**
 * @brief Write a string to the serial port
 * @param s String to write
 */
void serial_write_string(const char *s);

/**
 * @brief Write a formatted string to the serial port
 * @param fmt Format string
 * @param ... Variable arguments
 */
void serial_printk(const char *fmt, ...);

#endif /* _SERIAL_H */
