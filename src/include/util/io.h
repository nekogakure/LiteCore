#ifndef _IO_H
#define _IO_H

#include <stdint.h>

void _write(const char *string);
int printk(const char *string, ...);
void clear_screen();
void new_line();
void cpu_halt();
void timer_handler(uint32_t payload, void *ctx);

/* ポートI/O関数 */
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint32_t inl(uint16_t port);
void outl(uint16_t port, uint32_t value);

#endif /* _IO_H */