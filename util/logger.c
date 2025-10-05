/**
 * @file logger.c
 * @brief カーネル用ログ出力機能の実装
 * @details シリアルポート（COM1）を使用したデバッグ出力
 */

#include "logger.h"
#include "../include/kernel/cpu.h"

/** @brief COM1シリアルポートのベースアドレス */
#define SERIAL_COM1_PORT 0x3f8

/** @brief シリアルポートの各レジスタオフセット */
#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

/**
 * @brief シリアルポートの送信準備ができているか確認
 * @param com シリアルポートのベースアドレス
 * @return 送信準備ができているかどうか
 */
static int serial_is_transmit_ready(uint16_t com)
{
        return cpu_inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

void logger_init(void)
{
        /* COM1シリアルポートを初期化 */
        cpu_outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), 0x80); /* DLABをセット */
        cpu_outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), 0x03);        /* 38400 baudの低位 */
        cpu_outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT) + 1, 0x00);    /* 38400 baudの高位 */
        cpu_outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_PORT), 0x03); /* 8N1 */
        cpu_outb(SERIAL_FIFO_COMMAND_PORT(SERIAL_COM1_PORT), 0xc7); /* FIFO有効 */
        cpu_outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_PORT), 0x0b); /* RTS/DSRセット */
}

void logger_putchar(char ch)
{
        /* 送信準備ができるまで待機 */
        while (!serial_is_transmit_ready(SERIAL_COM1_PORT)) {
                /* 待機 */
        }
        
        cpu_outb(SERIAL_DATA_PORT(SERIAL_COM1_PORT), ch);
}

void logger_puts(const char* str)
{
        if (str == NULL) {
                return;
        }
        
        while (*str) {
                logger_putchar(*str);
                str++;
        }
}

/**
 * @brief 数値を文字列に変換する簡単な関数
 * @param value 変換する数値
 * @param buffer 出力バッファ
 * @param base 基数（10進数、4進数など）
 */
static void simple_itoa(uint64_t value, char* buffer, int base)
{
        char temp[32];
        int i = 0;
        
        if (value == 0) {
                buffer[0] = '0';
                buffer[1] = '\0';
                return;
        }
        
        while (value > 0) {
                int digit = value % base;
                temp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                value /= base;
        }
        
        /* 逆順でバッファにコピー */
        int j = 0;
        while (i > 0) {
                buffer[j++] = temp[--i];
        }
        buffer[j] = '\0';
}

void logger_print(const char* format, ...)
{
        if (format == NULL) {
                return;
        }
        
        va_list args;
        va_start(args, format);
        
        while (*format) {
                if (*format == '%') {
                        format++;
                        switch (*format) {
                        case 'd': {
                                int value = va_arg(args, int);
                                if (value < 0) {
                                        logger_putchar('-');
                                        value = -value;
                                }
                                char buffer[32];
                                simple_itoa(value, buffer, 10);
                                logger_puts(buffer);
                                break;
                        }
                        case 'u': {
                                unsigned int value = va_arg(args, unsigned int);
                                char buffer[32];
                                simple_itoa(value, buffer, 10);
                                logger_puts(buffer);
                                break;
                        }
                        case 'l': {
                                format++; /* 'l'の次の文字をチェック */
                                if (*format == 'u') {
                                        uint64_t value = va_arg(args, uint64_t);
                                        char buffer[32];
                                        simple_itoa(value, buffer, 10);
                                        logger_puts(buffer);
                                }
                                else if (*format == 'x') {
                                        uint64_t value = va_arg(args, uint64_t);
                                        char buffer[32];
                                        simple_itoa(value, buffer, 16);
                                        logger_puts(buffer);
                                }
                                break;
                        }
                        case 'x': {
                                unsigned int value = va_arg(args, unsigned int);
                                char buffer[32];
                                simple_itoa(value, buffer, 16);
                                logger_puts(buffer);
                                break;
                        }
                        case 's': {
                                const char* str = va_arg(args, const char*);
                                if (str != NULL) {
                                        logger_puts(str);
                                }
                                else {
                                        logger_puts("(null)");
                                }
                                break;
                        }
                        case 'c': {
                                char ch = (char)va_arg(args, int);
                                logger_putchar(ch);
                                break;
                        }
                        case '%': {
                                logger_putchar('%');
                                break;
                        }
                        default:
                                logger_putchar('%');
                                logger_putchar(*format);
                                break;
                        }
                }
                else {
                        logger_putchar(*format);
                }
                format++;
        }
        
        va_end(args);
}