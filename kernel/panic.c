#include "panic.h"
#include "util/vga.h"
#include "util/config.h"
#include <stdarg.h>

void kernel_panic(const char *fmt, ...) {
    text_clear();
    
    print("Kernel Crashed. ERROR: ", TEXT_COLOR_RED);
    
    va_list args;
    va_start(args, fmt);
    text_vprintf(TEXT_COLOR_WHITE, fmt, args);
    va_end(args);
    
    __asm__ volatile("cli");
    while(1) {
        __asm__ volatile("hlt");
    }
}

void kernel_print_error(const char *fmt, ...) {
    print("ERROR: ", TEXT_COLOR_RED);
    
    va_list args;
    va_start(args, fmt);
    text_vprintf(TEXT_COLOR_LIGHT_GREY, fmt, args);
    va_end(args);
    
    text_putchar('\n');
}
