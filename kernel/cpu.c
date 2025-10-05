/**
 * @file cpu.c
 * @brief CPU制御関数の実装
 * @details x86-64 CPU制御機能をインラインアセンブリで実装
 */

#include "../include/kernel/cpu.h"

void cpu_disable_interrupts(void)
{
        __asm__ volatile ("cli" ::: "memory");
}

void cpu_enable_interrupts(void)
{
        __asm__ volatile ("sti" ::: "memory");
}

void cpu_halt(void)
{
        __asm__ volatile ("hlt");
}

uint8_t cpu_inb(uint16_t port)
{
        uint8_t result;
        __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
        return result;
}

void cpu_outb(uint16_t port, uint8_t value)
{
        __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}