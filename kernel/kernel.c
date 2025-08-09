#include "kernel.h"

void kernel_init() {
    text_init();
    memory_init();
    paging_init();
}

void kernel_main_loop() {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void kernel_halt() {
    __asm__ volatile ("cli; hlt");
}
