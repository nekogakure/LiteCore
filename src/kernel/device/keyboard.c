#include <config.h>
#include <util/io.h>
#include <stdint.h>
#include <stdbool.h>

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
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b', // Backspace
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', // Enter
        0, // ctrl 
        'a','s','d','f','g','h','j','k','l',';','\'', '`', 0, // left shift
        '\\','z','x','c','v','b','n','m',',','.','/', 0, // right shift 
        '*', 0, // alt
        ' ', // space
};

/**
 * @var scancode_map
 * @brief キーマップ（SHIFT押下時）
 */
static const char scancode_map_shift[128] = {
        0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
        0, // ctrl 
        'A','S','D','F','G','H','J','K','L',':','"','~', 0,
        '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0,
        ' ',
};

/**
 * @fn keyboard_init
 * @brief キーボードの初期化（現状は何もしない）
 */
void keyboard_init(void) {
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

        // 修飾キー状態
        static bool shift_down = false;
        static bool ctrl_down = false;
        static bool alt_down = false;

        // ブレーク（リリース）コードは上位ビットが1？
        if (sc & 0x80) {
                uint8_t base = sc & 0x7F;
                // リリースされた修飾キーのフラグをクリア
                if (base == 0x2A || base == 0x36) { // Shift
                        shift_down = false;
                        return;
                }
                if (base == 0x1D) { // Ctrl
                        ctrl_down = false;
                        return;
                }
                if (base == 0x38) { // Alt
                        alt_down = false;
                        return;
                }

                // 他のリリースは無視
                return;
        }

        // 押下コードの処理
        if (sc == 0x2A || sc == 0x36) { // Shift
                shift_down = true;
                return;
        }
        if (sc == 0x1D) { // Ctrl
                ctrl_down = true;
                return;
        }
        if (sc == 0x38) { // Alt
                alt_down = true;
                return;
        }

        // 通常キー
        char out = 0;
        if (sc < sizeof(scancode_map)) {
                if (shift_down) {
                        out = scancode_map_shift[sc];
                } else {
                        out = scancode_map[sc];
                }
        }

        if (out) {
                // Ctrlが押されている場合は^をつける
                if (ctrl_down) {
                        char up = out;
                        if (up >= 'a' && up <= 'z') {
                                up = up - 'a' + 'A';
                        }
                        if (up >= 'A' && up <= 'Z') {
                                char s[3] = {'^', up, '\0'};
                                printk("%s", s);
                                return;
                        }
                        // それ以外
                        printk("[CTRL+0x%x]", (unsigned)out);
                        return;
                }

                // Altが押されている場合は角括弧でラップして表示
                if (alt_down) {
                        char s[4] = {'[', out, ']', '\0'};
                        printk("%s", s);
                        return;
                }

                char s[2] = {out, '\0'};
                printk("%s", s);
        } else {
                // 表示できないキーは6進数で表示
                printk("[0x%x]", sc);
        }
}
