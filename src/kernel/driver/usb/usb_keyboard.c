#include <driver/usb/usb_keyboard.h>
#include <driver/usb/xhci.h>
#include <driver/usb/hid.h>
#include <util/io.h>
#include <util/console.h>
#include <string.h>

// USB HID スキャンコードからASCII文字へのマッピング
static const char usb_scancode_map[128] = {
	0,    0,    0,	  0,
	'a',  'b',  'c',  'd', // 0-7
	'e',  'f',  'g',  'h',
	'i',  'j',  'k',  'l', // 8-15
	'm',  'n',  'o',  'p',
	'q',  'r',  's',  't', // 16-23
	'u',  'v',  'w',  'x',
	'y',  'z',  '1',  '2', // 24-31
	'3',  '4',  '5',  '6',
	'7',  '8',  '9',  '0', // 32-39
	'\n', 27,   '\b', '\t',
	' ',  '-',  '=',  '[', // 40-47 (Enter, Esc, Backspace, Tab, Space)
	']',  '\\', '#',  ';',
	'\'', '`',  ',',  '.', // 48-55
	'/',  0,    0,	  0,
	0,    0,    0,	  0, // 56-63 (CapsLock, F1-F6)
};

static const char usb_scancode_map_shift[128] = {
	0,    0,   0,	 0,    'A', 'B', 'C', 'D', // 0-7
	'E',  'F', 'G',	 'H',  'I', 'J', 'K', 'L', // 8-15
	'M',  'N', 'O',	 'P',  'Q', 'R', 'S', 'T', // 16-23
	'U',  'V', 'W',	 'X',  'Y', 'Z', '!', '@', // 24-31
	'#',  '$', '%',	 '^',  '&', '*', '(', ')', // 32-39
	'\n', 27,  '\b', '\t', ' ', '_', '+', '{', // 40-47
	'}',  '|', '~',	 ':',  '"', '~', '<', '>', // 48-55
	'?',  0,   0,	 0,    0,   0,	 0,   0, // 56-63
};

// キーボード状態
static bool usb_kb_initialized = false;
static bool usb_kb_available = false;
static uint8_t previous_keys[6] = { 0 };
static struct xhci_hc *usb_hc = NULL;
static uint8_t usb_kb_slot_id = 0;

// キーバッファ
#define USB_KEY_BUFFER_SIZE 256
static char usb_key_buffer[USB_KEY_BUFFER_SIZE];
static volatile int usb_buffer_read_pos = 0;
static volatile int usb_buffer_write_pos = 0;

static void usb_buffer_put(char c) {
	int next_pos = (usb_buffer_write_pos + 1) % USB_KEY_BUFFER_SIZE;
	if (next_pos != usb_buffer_read_pos) {
		usb_key_buffer[usb_buffer_write_pos] = c;
		usb_buffer_write_pos = next_pos;
	}
}

static char usb_buffer_get(void) {
	if (usb_buffer_read_pos == usb_buffer_write_pos) {
		return 0; // バッファが空
	}
	char c = usb_key_buffer[usb_buffer_read_pos];
	usb_buffer_read_pos = (usb_buffer_read_pos + 1) % USB_KEY_BUFFER_SIZE;
	return c;
}

/**
 * @brief USBスキャンコードからASCII文字への変換
 */
char usb_scancode_to_ascii(uint8_t scancode, bool shift) {
	if (scancode >= 128) {
		return 0;
	}

	if (shift) {
		return usb_scancode_map_shift[scancode];
	} else {
		return usb_scancode_map[scancode];
	}
}

/**
 * @brief キーボードレポートを処理
 */
static void process_keyboard_report(struct hid_keyboard_report *report) {
	if (!report) {
		return;
	}

	/* Shiftキーが押されているか確認 */
	bool shift = (report->modifier &
		      (HID_MOD_LEFT_SHIFT | HID_MOD_RIGHT_SHIFT)) != 0;
	bool ctrl = (report->modifier &
		     (HID_MOD_LEFT_CTRL | HID_MOD_RIGHT_CTRL)) != 0;

	/* 新しく押されたキーを検出 */
	for (int i = 0; i < 6; i++) {
		uint8_t keycode = report->keycode[i];

		if (keycode == 0) {
			continue; /* 空のスロット */
		}

		/* 前回押されていなかったキーか確認 */
		bool was_pressed = false;
		for (int j = 0; j < 6; j++) {
			if (previous_keys[j] == keycode) {
				was_pressed = true;
				break;
			}
		}

		if (!was_pressed) {
			/* 新しいキー押下 */
			char c = usb_scancode_to_ascii(keycode, shift);

			if (c != 0) {
				if (ctrl) {
					/* Ctrl+キーの処理 */
					if (c >= 'a' && c <= 'z') {
						c = c - 'a' + 'A';
					}
					if (c >= 'A' && c <= 'Z') {
						/* Ctrl+文字をバッファに追加 */
						usb_buffer_put('^');
						usb_buffer_put(c);
					}
				} else {
					/* 通常の文字をバッファに追加 */
					usb_buffer_put(c);
				}
			}
		}
	}

	/* 前回のキー状態を更新 */
	for (int i = 0; i < 6; i++) {
		previous_keys[i] = report->keycode[i];
	}
}

/**
 * @brief USB キーボード初期化
 */
int usb_keyboard_init(void) {
	// xHCIコントローラーを検索
	usb_hc = xhci_find_controller();
	if (usb_hc == NULL) {
		printk("USB Keyboard: No xHCI controller found\n");
		usb_kb_available = false;
		usb_kb_initialized = true;
		return -1;
	}

	printk("USB Keyboard: xHCI controller found\n");

	// キーボードデバイスのスロットIDを取得
	usb_kb_slot_id = xhci_get_keyboard_slot(usb_hc);
	if (usb_kb_slot_id == 0) {
		printk("USB Keyboard: No keyboard device found\n");
		usb_kb_available = false;
		usb_kb_initialized = true;
		return -2;
	}

	printk("USB Keyboard: Keyboard detected on slot %u\n", usb_kb_slot_id);

	// キーボードポーリングをセットアップ
	if (xhci_setup_keyboard_polling(usb_hc, usb_kb_slot_id) != 0) {
		printk("USB Keyboard: Failed to setup polling\n");
		usb_kb_available = false;
		usb_kb_initialized = true;
		return -3;
	}

	usb_kb_available = true;
	usb_kb_initialized = true;

	printk("USB Keyboard initialized (primary)\n");
	return 0;
}

/**
 * @brief USB キーボードポーリング
 * 定期的に呼び出してキーボード入力を処理
 */
void usb_keyboard_poll(void) {
	if (!usb_kb_initialized || !usb_kb_available || !usb_hc ||
	    usb_kb_slot_id == 0) {
		return;
	}

	/* キーボードレポートを取得 */
	uint8_t report_buffer[8];
	if (xhci_poll_keyboard(usb_hc, usb_kb_slot_id, report_buffer) == 0) {
		/* レポートを処理 */
		struct hid_keyboard_report *report =
			(struct hid_keyboard_report *)report_buffer;
		process_keyboard_report(report);
	}
}

/**
 * @brief USB キーボードが利用可能か
 */
bool usb_keyboard_available(void) {
	return usb_kb_initialized && usb_kb_available;
}

/**
 * @brief USB キーボードから文字を取得
 */
char usb_keyboard_getc(void) {
	usb_keyboard_poll();
	return usb_buffer_get();
}
