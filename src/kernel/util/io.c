#include <config.h>

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