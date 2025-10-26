#include <stdint.h>

void load_idt(void* ptr, unsigned size) {
        (void)size;
        __asm__ volatile ("lidtl (%0)" : : "r" (ptr));
}
