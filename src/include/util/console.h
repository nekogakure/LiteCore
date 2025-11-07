#ifndef _CONSOLE_H
#define _CONSOLE_H

void console_init();
void new_line();
int printk(const char *fmt, ...);
void console_scroll_page_up(void);
void console_scroll_page_down(void);

/* シリアル入力関数 */
int serial_received(void);
char serial_getc(void);
char serial_getc_nonblock(void);

#endif /* _CONSOLE_H */