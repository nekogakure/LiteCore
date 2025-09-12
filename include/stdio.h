#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>

int vsnprintf(char* str, size_t size, const char* format, va_list args);

#endif /* _STDIO_H */
