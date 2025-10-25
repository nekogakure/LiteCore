#include <config.h>
#include <util/io.h>
#include <stdarg.h>


/** 
 * @var cursor_row
 * @brief コンソール内のカーソルの現在の行位置
 */
static int cursor_row = 0;


/**
 * @var cursor_col
 * @brief コンソール内のカーソルの現在の列位置
 */
static int cursor_col = 0;

/**
 * @var CONSOLE_COLS
 * @brief コンソール列文字数
 */
static const int CONSOLE_COLS = 80;

/**
 * @var CONSOLE_ROWS
 * @brief コンソール行数
 */
static const int CONSOLE_ROWS = 25;

void console_init() {
        clear_screen();
        cursor_row = 0;
        cursor_col = 0;
}

/**
 * @fn new_line
 * @brief 改行を行い、スクロールが必要ならスクロールする
 */
void new_line() {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= CONSOLE_ROWS) {
                // スクロール処理
                uint8_t* video = (uint8_t*)VIDEO_MEMORY;
                uint8_t attr = COLOR;
                for (int r = 0; r < CONSOLE_ROWS - 1; r++) {
                        for (int c = 0; c < CONSOLE_COLS; c++) {
                                int dst = (r * CONSOLE_COLS + c) * 2;
                                int src = ((r + 1) * CONSOLE_COLS + c) * 2;
                                video[dst] = video[src];
                                video[dst + 1] = video[src + 1];
                        }
                }
                
                // 一番下の行をスペースでクリア
                int last = (CONSOLE_ROWS - 1) * CONSOLE_COLS;
                for (int c = 0; c < CONSOLE_COLS; c++) {
                        video[(last + c) * 2] = ' ';
                        video[(last + c) * 2 + 1] = attr;
                }
                cursor_row = CONSOLE_ROWS - 1;
        }
}

/**
 * @fn console_putc
 * @brief 位置文字表示
 */
static void console_putc(char ch) {
        uint8_t* video = (uint8_t*)VIDEO_MEMORY;
        uint8_t attr = COLOR;

        if (ch == '\n') {
                new_line();
                return;
        }

        int pos = (cursor_row * CONSOLE_COLS + cursor_col) * 2;
        video[pos] = (uint8_t)ch;
        video[pos + 1] = attr;
        cursor_col++;
        if (cursor_col >= CONSOLE_COLS) {
                new_line();
        }
}

/**
 * @fn console_write
 * @brief 文字列を書き込む
 */
static void console_write(const char* s) {
        while (*s) {
                console_putc(*s++);
        }
}

/**
 * @fn printk
 * @brief printfのようにVGAに出力（返り値: 出力した文字数）
 */
int printk(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        int i = 0, j = 0;

        while (fmt[i] && j < (int)sizeof(buffer) - 1) {
                if (fmt[i] == '%' && fmt[i+1]) {
                        i++;
                        /* width とパディングの簡易サポート（例: 02） */
                        int width = 0;
                        int pad_zero = 0;
                        if (fmt[i] == '0') {
                                pad_zero = 1;
                                i++;
                                while (fmt[i] >= '0' && fmt[i] <= '9') {
                                        width = width * 10 + (fmt[i] - '0');
                                        i++;
                                }
                        } else {
                                while (fmt[i] >= '0' && fmt[i] <= '9') {
                                        width = width * 10 + (fmt[i] - '0');
                                        i++;
                                }
                        }

                        char spec = fmt[i];
                        if (spec == 'd') {
                                int val = va_arg(args, int);
                                char numbuf[32];
                                int n = 0, k = 0;
                                unsigned int uval;
                                if (val < 0) {
                                        buffer[j++] = '-';
                                        uval = (unsigned int)(-val);
                                } else {
                                        uval = (unsigned int)val;
                                }
                                do {
                                        numbuf[n++] = '0' + (uval % 10);
                                        uval /= 10;
                                } while (uval && n < (int)sizeof(numbuf));
                                if (width > n) {
                                        int pad = width - n;
                                        while (pad-- > 0 && j < (int)sizeof(buffer) - 1)
                                                buffer[j++] = pad_zero ? '0' : ' ';
                                }
                                for (k = n - 1; k >= 0 && j < (int)sizeof(buffer) - 1; k--)
                                        buffer[j++] = numbuf[k];
                        } else if (spec == 'u') {
                                unsigned int val = va_arg(args, unsigned int);
                                char numbuf[32];
                                int n = 0, k = 0;
                                do {
                                        numbuf[n++] = '0' + (val % 10);
                                        val /= 10;
                                } while (val && n < (int)sizeof(numbuf));
                                if (width > n) {
                                        int pad = width - n;
                                        while (pad-- > 0 && j < (int)sizeof(buffer) - 1)
                                                buffer[j++] = pad_zero ? '0' : ' ';
                                }
                                for (k = n - 1; k >= 0 && j < (int)sizeof(buffer) - 1; k--)
                                        buffer[j++] = numbuf[k];
                        } else if (spec == 'x' || spec == 'X') {
                                unsigned int val = va_arg(args, unsigned int);
                                char numbuf[32];
                                int n = 0, k = 0;
                                const char* hex = (spec == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
                                do {
                                        numbuf[n++] = hex[val & 0xF];
                                        val >>= 4;
                                } while (val && n < (int)sizeof(numbuf));
                                if (width > n) {
                                        int pad = width - n;
                                        while (pad-- > 0 && j < (int)sizeof(buffer) - 1)
                                                buffer[j++] = pad_zero ? '0' : ' ';
                                }
                                for (k = n - 1; k >= 0 && j < (int)sizeof(buffer) - 1; k--)
                                        buffer[j++] = numbuf[k];
                        } else if (spec == 's') {
                                const char* s = va_arg(args, const char*);
                                while (*s && j < (int)sizeof(buffer) - 1)
                                        buffer[j++] = *s++;
                        } else if (spec == 'c') {
                                char c = (char)va_arg(args, int);
                                buffer[j++] = c;
                        } else {
                                buffer[j++] = '%';
                                buffer[j++] = spec;
                        }
                } else if (fmt[i] == '\\' && fmt[i+1] == 'n') {
                        buffer[j++] = '\n';
                        i++;
                } else if (fmt[i] == '\n') {
                        buffer[j++] = '\n';
                } else {
                        buffer[j++] = fmt[i];
                }
                i++;
        }
        buffer[j] = '\0';
        va_end(args);

        console_write(buffer);
        return j;
}