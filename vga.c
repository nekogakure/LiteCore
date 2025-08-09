#include "vga.h"

static unsigned char* text_buffer = (unsigned char*)TEXT_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char current_color = 0x07;

static inline unsigned char text_entry_color(enum text_color fg, enum text_color bg) {
    return fg | bg << 4;
}

void text_init() {
    cursor_x = 0;
    cursor_y = 0;
    current_color = text_entry_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    
    text_clear();
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
}

void text_clear() {
    for (int i = 0; i < TEXT_WIDTH * TEXT_HEIGHT * 2; i += 2) {
        text_buffer[i] = ' ';
        text_buffer[i + 1] = current_color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

static void update_cursor() {
    unsigned short pos = cursor_y * TEXT_WIDTH + cursor_x;
    
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)0x0F), "d"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)(pos & 0xFF)), "d"((unsigned short)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)0x0E), "d"((unsigned short)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)((pos >> 8) & 0xFF)), "d"((unsigned short)0x3D5));
}

static void scroll() {
    for (int i = 0; i < (TEXT_HEIGHT - 1) * TEXT_WIDTH * 2; i++) {
        text_buffer[i] = text_buffer[i + TEXT_WIDTH * 2];
    }
    
    for (int i = (TEXT_HEIGHT - 1) * TEXT_WIDTH * 2; i < TEXT_HEIGHT * TEXT_WIDTH * 2; i += 2) {
        text_buffer[i] = ' ';
        text_buffer[i + 1] = current_color;
    }
    cursor_y = TEXT_HEIGHT - 1;
}

void text_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else {
        int index = (cursor_y * TEXT_WIDTH + cursor_x) * 2;
        
        unsigned char* vga_mem = (unsigned char*)TEXT_MEMORY;
        vga_mem[index] = c;
        vga_mem[index + 1] = current_color;
        
        cursor_x++;
    }
    
    if (cursor_x >= TEXT_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= TEXT_HEIGHT) {
        scroll();
    }
    
    update_cursor();
}

void printk(const char* str) {
    if (!str) return;
    
    while (*str) {
        text_putchar(*str);
        str++;
    }
}

void text_set_color(enum text_color fg, enum text_color bg) {
    current_color = text_entry_color(fg, bg);
}
