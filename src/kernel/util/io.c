#include <config.h>
#include <util/io.h>

/**
 * @fn _write
 * @brief 文字列を表示します
 * @param string 表示する文字列
 */
void _write(const char *string) {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	uint8_t attr = COLOR;

	while (*string) {
		*video++ = *string++; // 文字を書き込み
		*video++ = attr; // 色属性の書き込み
	}
}

/**
 * @fn clear_screen
 * @brief VGAのスクリーンをすべてスペースで埋め尽くします
 */
void clear_screen() {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	uint8_t attr = COLOR;

	for (int i = 0; i < 80 * 25; i++) {
		*video++ = ' '; // スペース
		*video++ = attr; // 色属性の書き込み
	}
}


/**
 * @brief CPUを休止状態にする（次の割り込みまで）
 */
void cpu_halt(void) {
	// STI命令で割り込みを有効にしてからHLT
	__asm__ volatile("sti; hlt");
}

/**
 * @brief タイマー割り込みハンドラ（IRQ 0）
 * システムタイマーの割り込みを静かに処理（何もしないサボり関数)
 */
void timer_handler(uint32_t payload, void *ctx) {
	(void)payload;
	(void)ctx;
}

/**
 * @brief ポートから1バイト読み取る
 * @param port ポート番号
 * @return 読み取った値
 */
uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/**
 * @brief ポートに1バイト書き込む
 * @param port ポート番号
 * @param value 書き込む値
 */
void outb(uint16_t port, uint8_t value) {
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief ポートから2バイト（ワード）読み取る
 * @param port ポート番号
 * @return 読み取った値
 */
uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/**
 * @brief ポートに2バイト（ワード）書き込む
 * @param port ポート番号
 * @param value 書き込む値
 */
void outw(uint16_t port, uint16_t value) {
	__asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}