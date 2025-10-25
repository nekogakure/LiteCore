#include <config.h>
#include <util/io.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
        __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
}

/**
 * @def KBD_DATA_PORT
 * @brief キーボードのデータポートアドレス
 */
#define KBD_DATA_PORT 0x60

static const char scancode_map[128] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b', /* Backspace */
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', /* Enter */
        0, /* ctrl */ 'a','s','d','f','g','h','j','k','l',';','\'', '`', 0, /* left shift */
        '\\','z','x','c','v','b','n','m',',','.','/', 0, /* right shift */ '*', 0, /* alt */
        ' ', /* space */
};

/**
 * @fn keyboard_init
 * @brief キーボードの初期化（現状は何もしない）
 */
void keyboard_init(void) {
        /* 必要であれば、ここでPICのIRQ1を有効化できます。現状では何もしてません */
        printk("Keyboard initialize success\n");
}

/**
 * @fn keyboard_poll
 * @brief キーボードからの入力をポーリングし、押されたキーを処理
 */
void keyboard_poll(void) {
        // ステータスポート(0x64)のビット0を確認し、データがある時だけ0x60から読み取る
        uint8_t status = inb(0x64);
        if ((status & 0x01) == 0) {
                return; // データ無し 
        }

        uint8_t sc = inb(KBD_DATA_PORT);
        if (sc == 0) return;

        // ブレーク（リリース）コードは上位ビットが1になる (set1)
        if (sc & 0x80) {
                return; // リリースは無視
        }

        if (sc < sizeof(scancode_map) && scancode_map[sc]) {
                char ch = scancode_map[sc];
                char s[2] = {ch, '\0'};
                printk("%s", s);
        } else {
                // 表示できないキー: 16進数で表示
                printk("0x%x", sc);
        }
}
