#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <boot_info.h>

void console_init();
void console_set_framebuffer(BOOT_INFO *boot_info);
void console_set_colors(uint32_t fg, uint32_t bg);
void console_get_colors(uint32_t *fg, uint32_t *bg);
void new_line();
int printk(const char *fmt, ...);
void console_scroll_page_up(void);
void console_scroll_page_down(void);
void console_render_text_to_fb(void);
void console_post_font_init(void);

/* シリアル入力関数 */
int serial_received(void);
char serial_getc(void);
char serial_getc_nonblock(void);

#endif /* _CONSOLE_H */