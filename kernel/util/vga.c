#include "vga.h"

static unsigned char* text_buffer = (unsigned char*)TEXT_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char current_color = 0x07;

static inline unsigned char text_entry_color(enum text_color fg, enum text_color bg) {
    return fg | bg << 4;
}

void vga_init() {
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

static void int_to_string(int num, char* str, int base) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_negative = 0;
    
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }
    
    while (num != 0) {
        int remainder = num % base;
        str[i++] = (remainder < 10) ? remainder + '0' : (remainder - 10) + 'a';
        num = num / base;
    }
    
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

static void hex_to_string(unsigned long long num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    int i = 0;
    while (num != 0) {
        int digit = num % 16;
        str[i++] = digit < 10 ? digit + '0' : (digit - 10) + 'a';
        num /= 16;
    }
    
    str[i] = '\0';
    
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void printk_int(const char* format, int value) {
    char buffer[32];
    
    while (*format) {
        if (*format == '%' && *(format + 1) == 'd') {
            int_to_string(value, buffer, 10);
            printk(buffer);
            format += 2;
        } else {
            char temp[2] = {*format, '\0'};
            printk(temp);
            format++;
        }
    }
}

void printk_hex(const char* format, unsigned long long value) {
    char buffer[32];
    
    while (*format) {
        if (*format == '%' && *(format + 1) == 'x') {
            hex_to_string(value, buffer);
            printk(buffer);
            format += 2;
        } else {
            char temp[2] = {*format, '\0'};
            printk(temp);
            format++;
        }
    }
}

static int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

void text_putchar_color(char c, unsigned char color) {
    unsigned char old_color = current_color;
    current_color = color;
    text_putchar(c);
    current_color = old_color;
}

void print(const char* str, unsigned char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        text_putchar_color(str[i], color);
    }
}

void print_hex(unsigned long num, unsigned char color) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[17];
    buffer[16] = '\0';
    
    int i = 15;
    while (i >= 0) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;
        i--;
        if (num == 0 && i != 14) break;
    }
    
    print("0x", color);
    print(&buffer[i+1], color);
}

void print_dec(long num, unsigned char color) {
    char buffer[21];
    buffer[20] = '\0';
    
    int i = 19;
    int negative = 0;
    
    if (num < 0) {
        negative = 1;
        num = -num;
    }
    
    do {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
    
    if (negative) {
        buffer[i--] = '-';
    }
    
    print(&buffer[i+1], color);
}

void text_vprintf(unsigned char color, const char* fmt, va_list args) {
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i+1] != '\0') {
            i++;
            switch (fmt[i]) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    print(str ? str : "(null)", color);
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    print_dec(num, color);
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    print_hex(num, color);
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    print_hex((unsigned long)ptr, color);
                    break;
                }
                case '%':
                    text_putchar_color('%', color);
                    break;
                default:
                    text_putchar_color('%', color);
                    text_putchar_color(fmt[i], color);
            }
        } else {
            text_putchar_color(fmt[i], color);
        }
    }
}

void printf(unsigned char color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    text_vprintf(color, fmt, args);
    va_end(args);
}
