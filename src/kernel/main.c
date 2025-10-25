/**
 * @file main.c
 * @brief LiteCoreカーネルのメインとなるファイル
 */

/**
 * @def VIDEO_MEMORY
 * @brief VGAの開始アドレス
 */
#define VIDEO_MEMORY    (0xB8000)

/**
 * @def COLOR
 * @brief 色属性（黒背景・白文字）
 */
#define COLOR           (0x07)

/**
 * @typedef uint8_t
 * @brief 8ビット符号なし整数型
 */
typedef unsigned char uint8_t;

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

/**
 * @fn print_string()
 * @brief 文字列を表示します
 * @param string 表示する文字列
 */
void print_string(const char* string) {
        uint8_t* video = (uint8_t*)VIDEO_MEMORY;
        uint8_t attr = COLOR;

        while(*string) {
                *video++ = *string++;   // 文字を書き込み
                *video++ = attr;        // 色属性の書き込み
        }
}

/**
 * @fn kmain()
 * @brief カーネルのエントリーポイント
 */
void kmain() {
        clear_screen();
        print_string("Welcome to Litecore kernel!");

        while(1) {
                __asm__ volatile ("cli");
                __asm__ volatile ("hlt");
        }
}