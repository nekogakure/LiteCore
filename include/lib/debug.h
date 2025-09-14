/**
 * @file debug.h
 * @brief Debug port output interface
 * @details
 * Header file for debug port output functions
 * @author nekogakure
 */

#ifndef _DEBUG_H
#define _DEBUG_H

/**
 * @brief Output a character to the debug port
 * @param c Character to output
 */
void debug_putchar(char c);

/**
 * @brief Output a string to the debug port
 * @param s String to output
 */
void debug_print(const char *s);

#endif /* _DEBUG_H */
