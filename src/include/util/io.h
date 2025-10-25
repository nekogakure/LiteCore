#ifndef _IO_H
#define _IO_H

void _write(const char* string);
int printk(const char* string, ...);
void clear_screen();
void kstop();

#endif /* _IO_H */