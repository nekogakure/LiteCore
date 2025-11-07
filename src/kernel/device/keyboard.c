#include <config.h>
#include <util/io.h>
#include <util/console.h>
#include <stdint.h>
#include <stdbool.h>
#include <interrupt/irq.h>

#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int buffer_read_pos = 0;
static volatile int buffer_write_pos = 0;

static void buffer_put(char c) {
	int next_pos = (buffer_write_pos + 1) % KEY_BUFFER_SIZE;
	if (next_pos != buffer_read_pos) {
		key_buffer[buffer_write_pos] = c;
		buffer_write_pos = next_pos;
	}
}

// バッファから文字を取得
static char buffer_get(void) {
	if (buffer_read_pos == buffer_write_pos) {
		return 0; // バッファが空
	}
	char c = key_buffer[buffer_read_pos];
	buffer_read_pos = (buffer_read_pos + 1) % KEY_BUFFER_SIZE;
	return c;
}

#define KBD_DATA_PORT 0x60

/* key maps */
static const char scancode_map[128] = {
	0,   27,  '1',	'2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
	'o', 'p', '[',	']',  '\n', 0,	 'a', 's',  'd', 'f', 'g', 'h',
	'j', 'k', 'l',	';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm',	',',  '.',  '/', 0,   '*',  0,	 ' ',
};

static const char scancode_map_shift[128] = {
	0,   27,  '!',	'@',  '#',  '$', '%', '^', '&', '*', '(', ')',
	'_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{',	'}',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	'<',  '>',  '?', 0,   '*', 0,	' ',
};

/* 上位16bit: IRQ番号, 下位16bit: scancodewoエンコードしてraiseする */
#define KBD_EVENT(irq, sc) (((irq) << 16) | ((sc) & 0xFFFF))

/* IRQ1のISR: scancodeを読み取ってイベントをenqueueするだけ */
static void kbd_isr(uint32_t unused, void *ctx) {
	(void)ctx;
	(void)unused;
	uint8_t status = inb(0x64);
	if ((status & 0x01) == 0)
		return; /* no data */
	uint8_t sc = inb(KBD_DATA_PORT);
	if (sc == 0)
		return;
	/* IRQ 1 */
	interrupt_raise(KBD_EVENT(1, sc));
}

/* 実際にscancodeを処理して表示するハンドラ */
static void kbd_process(uint32_t sc_payload, void *ctx) {
	(void)ctx;
	uint8_t sc = sc_payload & 0xFF;

	static bool shift_down = false;
	static bool ctrl_down = false;
	static bool alt_down = false;

	if (sc & 0x80) {
		uint8_t base = sc & 0x7F;
		if (base == 0x2A || base == 0x36) {
			shift_down = false;
			return;
		}
		if (base == 0x1D) {
			ctrl_down = false;
			return;
		}
		if (base == 0x38) {
			alt_down = false;
			return;
		}
		return;
	}

	if (sc == 0x2A || sc == 0x36) {
		shift_down = true;
		return;
	}
	if (sc == 0x1D) {
		ctrl_down = true;
		return;
	}
	if (sc == 0x38) {
		alt_down = true;
		return;
	}

	char out = 0;
	if (sc < sizeof(scancode_map)) {
		out = shift_down ? scancode_map_shift[sc] : scancode_map[sc];
	}

	if (out) {
		if (ctrl_down) {
			char up = out;
			if (up >= 'a' && up <= 'z')
				up = up - 'a' + 'A';
			if (up >= 'A' && up <= 'Z') {
				// バッファに追加
				buffer_put('^');
				buffer_put(up);
				// 画面にも表示
				char s[3] = { '^', up, '\0' };
				printk("%s", s);
				return;
			}
			printk("[CTRL+0x%x]", (unsigned)out);
			return;
		}
		if (alt_down) {
			// Alt+キーはバッファに追加しない（現状）
			char s[4] = { '[', out, ']', '\0' };
			printk("%s", s);
			return;
		}
		// すべての文字（制御文字含む）をバッファに追加
		// シェルが適切に処理する
		buffer_put(out);
		// 画面には表示しない（シェルが表示する）
	} else {
		// 未知のスキャンコードは無視（ログ出力しない）
		// 特定のキー（PgUp、PgDnなど）が必要な場合はここで処理
		/*
                if (sc == 0x49) { // PGUP
                        console_scroll_page_up();
                        return;
                }
                if (sc == 0x51) { // PGDN
                        console_scroll_page_down();
                        return;
                }
                */
	}
}

/**
 * @fn keyboard_init
 * @brief キーボードドライバを初期化
 */
void keyboard_init(void) {
	// 同期側の処理ハンドラを登録 (FIFOイベントの処理)
	interrupt_register(1, kbd_process, NULL);
#ifdef INIT_MSG
	printk("Keyboard initialize success\n");
#endif
}

void keyboard_poll(void) {
	kbd_isr(0, NULL);
}

/**
 * @brief キーボードから1文字取得（ブロッキング）
 * @return 取得した文字
 */
char keyboard_getchar(void) {
	char c;
	while ((c = buffer_get()) == 0) {
		// バッファが空の場合、ポーリングして待つ
		keyboard_poll();
	}
	return c;
}

/**
 * @brief キーボードから1文字取得（ポーリング）
 * @return 取得した文字、バッファが空の場合0
 */
char keyboard_getchar_poll(void) {
	keyboard_poll();
	return buffer_get();
}
