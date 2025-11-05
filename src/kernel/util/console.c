#include <config.h>
#include <util/io.h>
#include <stdarg.h>
#include <stdint.h>
#include <interrupt/irq.h>

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/* unused function lol
static void serial_init(void) {
        outb(0x3f8 + 1, 0x00); // すべての割り込みを無効化
        outb(0x3f8 + 3, 0x80); // DLABを有効化
        outb(0x3f8 + 0, 0x03); // 分周値下位バイト (38400ボーレート)
        outb(0x3f8 + 1, 0x00); // 分周値上位バイト
        outb(0x3f8 + 3, 0x03); // 8ビット、パリティなし、ストップビット1
        outb(0x3f8 + 2, 0xC7); // FIFO有効化
        outb(0x3f8 + 4, 0x0B); // IRQ有効化、RTS/DSRセット
}
*/
static void serial_putc(char c) {
	while ((inb(0x3f8 + 5) & 0x20) == 0) {
	}
	outb(0x3f8, (uint8_t)c);
}

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

/**
 * @def N_HISTORY
 * @brief コンソールの最大保存容量
 */
#define N_HISTORY 100
static char history[N_HISTORY][80];
static int history_lines = 0;
static int history_offset = 0;

void console_init() {
	clear_screen();
	cursor_row = 0;
	cursor_col = 0;
	history_lines = 0;
	history_offset = 0;
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
		uint8_t *video = (uint8_t *)VIDEO_MEMORY;
		int last = (CONSOLE_ROWS - 1) * CONSOLE_COLS;
		char linebuf[CONSOLE_COLS];
		for (int c = 0; c < CONSOLE_COLS; c++) {
			linebuf[c] = (char)video[(last + c) * 2];
		}

		if (history_lines < N_HISTORY) {
			for (int i = 0; i < CONSOLE_COLS; ++i)
				history[history_lines][i] = linebuf[i];
			history_lines++;
		} else {
			for (int i = 0; i < N_HISTORY - 1; ++i) {
				for (int c = 0; c < CONSOLE_COLS; ++c)
					history[i][c] = history[i + 1][c];
			}
			for (int c = 0; c < CONSOLE_COLS; ++c)
				history[N_HISTORY - 1][c] = linebuf[c];
		}

		for (int r = 0; r < CONSOLE_ROWS - 1; r++) {
			for (int c = 0; c < CONSOLE_COLS; c++) {
				int dst = (r * CONSOLE_COLS + c) * 2;
				int src = ((r + 1) * CONSOLE_COLS + c) * 2;
				video[dst] = video[src];
				video[dst + 1] = video[src + 1];
			}
		}

		uint8_t attr = COLOR;
		for (int c = 0; c < CONSOLE_COLS; c++) {
			video[(last + c) * 2] = ' ';
			video[(last + c) * 2 + 1] = attr;
		}
		cursor_row = CONSOLE_ROWS - 1;
		history_offset = (history_lines > CONSOLE_ROWS) ?
					 history_lines - CONSOLE_ROWS :
					 0;
	}
}

/**
 * @fn console_putc
 * @brief 位置文字表示
 */
static void console_putc(char ch) {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	uint8_t attr = COLOR;

	if (ch == '\n') {
		new_line();
		serial_putc('\n');
		return;
	}

	int pos = (cursor_row * CONSOLE_COLS + cursor_col) * 2;
	video[pos] = (uint8_t)ch;
	video[pos + 1] = attr;
	cursor_col++;
	serial_putc(ch);
	if (cursor_col >= CONSOLE_COLS) {
		new_line();
	}
}

/* history_offsetからコンソールを再描画 */
static void redraw_from_history(void) {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	/* clear screen */
	uint8_t attr = COLOR;
	for (int r = 0; r < CONSOLE_ROWS; ++r) {
		for (int c = 0; c < CONSOLE_COLS; ++c) {
			int pos = (r * CONSOLE_COLS + c) * 2;
			video[pos] = ' ';
			video[pos + 1] = attr;
		}
	}

	int start = history_offset;
	for (int r = 0; r < CONSOLE_ROWS; ++r) {
		int idx = start + r;
		if (idx < 0 || idx >= history_lines)
			continue;
		for (int c = 0; c < CONSOLE_COLS; ++c) {
			int pos = (r * CONSOLE_COLS + c) * 2;
			video[pos] = (uint8_t)history[idx][c];
			video[pos + 1] = attr;
		}
	}
}

void console_scroll_page_up(void) {
	if (history_lines <= CONSOLE_ROWS)
		return;
	history_offset -= CONSOLE_ROWS;
	if (history_offset < 0)
		history_offset = 0;
	redraw_from_history();
}

void console_scroll_page_down(void) {
	if (history_lines <= CONSOLE_ROWS)
		return;
	int max_offset = history_lines - CONSOLE_ROWS;
	history_offset += CONSOLE_ROWS;
	if (history_offset > max_offset)
		history_offset = max_offset;
	redraw_from_history();
}

/**
 * @fn console_write
 * @brief 文字列を書き込む
 */
static void console_write(const char *s) {
	while (*s) {
		console_putc(*s++);
	}
}

/**
 * @fn printk
 * @brief printfのようにVGAに出力
 */
int printk(const char *fmt, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, fmt);
	int i = 0, j = 0;

	while (fmt[i] && j < (int)sizeof(buffer) - 1) {
		if (fmt[i] == '%' && fmt[i + 1]) {
			i++;
			// widthとパディングの簡易サポート
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
					while (pad-- > 0 &&
					       j < (int)sizeof(buffer) - 1)
						buffer[j++] = pad_zero ? '0' :
									 ' ';
				}
				for (k = n - 1;
				     k >= 0 && j < (int)sizeof(buffer) - 1; k--)
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
					while (pad-- > 0 &&
					       j < (int)sizeof(buffer) - 1)
						buffer[j++] = pad_zero ? '0' :
									 ' ';
				}
				for (k = n - 1;
				     k >= 0 && j < (int)sizeof(buffer) - 1; k--)
					buffer[j++] = numbuf[k];
			} else if (spec == 'x' || spec == 'X') {
				unsigned int val = va_arg(args, unsigned int);
				char numbuf[32];
				int n = 0, k = 0;
				const char *hex = (spec == 'x') ?
							  "0123456789abcdef" :
							  "0123456789ABCDEF";
				do {
					numbuf[n++] = hex[val & 0xF];
					val >>= 4;
				} while (val && n < (int)sizeof(numbuf));
				if (width > n) {
					int pad = width - n;
					while (pad-- > 0 &&
					       j < (int)sizeof(buffer) - 1)
						buffer[j++] = pad_zero ? '0' :
									 ' ';
				}
				for (k = n - 1;
				     k >= 0 && j < (int)sizeof(buffer) - 1; k--)
					buffer[j++] = numbuf[k];
			} else if (spec == 's') {
				const char *s = va_arg(args, const char *);
				while (*s && j < (int)sizeof(buffer) - 1)
					buffer[j++] = *s++;
			} else if (spec == 'c') {
				char c = (char)va_arg(args, int);
				buffer[j++] = c;
			} else if (spec == 'p') {
				void *ptr = va_arg(args, void *);
				uintptr_t val = (uintptr_t)ptr;
				if (j < (int)sizeof(buffer) - 1)
					buffer[j++] = '0';
				if (j < (int)sizeof(buffer) - 1)
					buffer[j++] = 'x';
				const char *hex = "0123456789abcdef";
				int nibbles = (int)sizeof(uintptr_t) * 2;
				for (int k = nibbles - 1; k >= 0; k--) {
					int shift = k * 4;
					uint8_t d = (val >> shift) & 0xF;
					if (j < (int)sizeof(buffer) - 1)
						buffer[j++] = hex[d];
				}
			} else {
				buffer[j++] = '%';
				buffer[j++] = spec;
			}
		} else if (fmt[i] == '\\' && fmt[i + 1] == 'n') {
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

	uint32_t _irq_flags = irq_save();
	console_write(buffer);
	irq_restore(_irq_flags);
	return j;
}