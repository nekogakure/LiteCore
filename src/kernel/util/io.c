#include <config.h>
#include <stdarg.h>

/**
 * @fn _write()
 * @brief 文字列を表示します
 * @param string 表示する文字列
 */
void _write(const char* string) {
        uint8_t* video = (uint8_t*)VIDEO_MEMORY;
        uint8_t attr = COLOR;

        while(*string) {
                *video++ = *string++;   // 文字を書き込み
                *video++ = attr;        // 色属性の書き込み
        }
}

/**
 * @fn printk()
 * @brief printfのようにVGAに出力
 */
void printk(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        int i = 0, j = 0;

        // クソ適当クソぐちゃぐちゃなコード許してください（（
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
                } else {
                        buffer[j++] = fmt[i];
                }
                i++;
        }
        buffer[j] = '\0';
        va_end(args);
        _write(buffer);
}


/**
 * @fn clear_screen()
 * @brief VGAのスクリーンをすべてスペースで埋め尽くします
 */
void clear_screen() {
        uint8_t* video = (uint8_t*)VIDEO_MEMORY;
        uint8_t attr = COLOR;

        for (int i = 0; i < 80 * 25; i++) {
                *video++ = ' ';         // スペース
                *video++ = attr;        // 色属性の書き込み
        }
}