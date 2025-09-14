/**
 * @file serial.c
 * @brief Serial port communication implementation
 * @details
 * Provides functions for communication through the serial port
 * @author nekogakure
 */

#include "lib/serial.h"
#include <stdint.h>

/** @brief COM1 serial port address */
#define SERIAL_PORT_COM1 0x3F8

/**
 * @brief Write a byte to a port
 * @param port Port address
 * @param value Value to write
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read a byte from a port
 * @param port Port address
 * @return The value read
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Initialize the serial port
 * @details Configures the serial port for basic output
 */
void init_serial(void) {
    // Disable all interrupts
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    
    // Enable DLAB to set baud rate
    outb(SERIAL_PORT_COM1 + 3, 0x80);
    
    // Set divisor to 3 for ~38400 baud
    outb(SERIAL_PORT_COM1 + 0, 0x03);
    outb(SERIAL_PORT_COM1 + 1, 0x00);
    
    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT_COM1 + 3, 0x03);
    
    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT_COM1 + 2, 0xC7);
    
    // IRQs enabled, RTS/DSR set
    outb(SERIAL_PORT_COM1 + 4, 0x0B);
}

/**
 * @brief Check if the serial port transmit buffer is empty
 * @return 0 if buffer is full, non-zero if empty
 */
static int is_transmit_empty(void) {
    return inb(SERIAL_PORT_COM1 + 5) & 0x20;
}

/**
 * @brief Write a character to the serial port
 * @param c Character to write
 */
void serial_write_char(char c) {
    // Wait for the transmit buffer to be empty
    while (is_transmit_empty() == 0);
    
    // Send the character
    outb(SERIAL_PORT_COM1, c);
}

/**
 * @brief Write a string to the serial port
 * @param s String to write
 */
void serial_write_string(const char *s) {
    while (*s) {
        if (*s == '\n') {
            serial_write_char('\r'); // Add carriage return for proper newline
        }
        serial_write_char(*s++);
    }
}

/**
 * @brief Write a formatted string to the serial port
 * @param fmt Format string
 * @param ... Variable arguments
 */
void serial_printk(const char *fmt, ...) {
    // Simple implementation that just prints the string for now
    serial_write_string(fmt);
}
