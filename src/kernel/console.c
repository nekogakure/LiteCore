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
 * @fn new_line()
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
 * @fn console_putc()
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
 * @fn console_write()
 * @brief 文字列を書き込む
 */
static void console_write(const char* s) {
        while (*s) {
                console_putc(*s++);
        }
}

/**
 * @fn printk()
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
                        if (fmt[i] == 'd') {
                                int val = va_arg(args, int);
                                char numbuf[32];
                                int n = 0, k = 0;
                                if (val < 0) {
                                        buffer[j++] = '-';
                                        val = -val;
                                }
                                do {
                                        numbuf[n++] = '0' + (val % 10);
                                        val /= 10;
                                } while (val && n < (int)sizeof(numbuf));
                                for (k = n - 1; k >= 0 && j < (int)sizeof(buffer) - 1; k--)
                                        buffer[j++] = numbuf[k];
                        } else if (fmt[i] == 's') {
                                const char* s = va_arg(args, const char*);
                                while (*s && j < (int)sizeof(buffer) - 1)
                                        buffer[j++] = *s++;
                        } else if (fmt[i] == 'c') {
                                char c = (char)va_arg(args, int);
                                buffer[j++] = c;
                        } else {
                                buffer[j++] = '%';
                                buffer[j++] = fmt[i];
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