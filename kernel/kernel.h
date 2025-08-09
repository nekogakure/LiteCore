#ifndef KERNEL_H
#define KERNEL_H

#include "util/vga.h"
#include "../mem/memory.h"
#include "../mem/paging.h"

/**
 * @brief Init a kernel
 */
void kernel_init();

/**
 * @brief Main kernel loop
 * @details Runs the main loop of the kernel
 */
void kernel_main_loop();

/**
 * @brief stop the kernel
 * @details Halts the kernel by disabling interrupts and entering a halt state
 * This function is used to stop the kernel gracefully.
 */
void kernel_halt();

#endif
