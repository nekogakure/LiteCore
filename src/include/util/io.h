#ifndef _IO_H
#define _IO_H

void _write(const char *string);
int printk(const char *string, ...);
void clear_screen();
void new_line();
void cpu_halt();
void timer_handler(uint32_t payload, void *ctx);

#endif /* _IO_H */