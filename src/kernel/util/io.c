#include <util/config.h>
#include <util/io.h>

void _write(const char *string) {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	uint8_t attr = COLOR;
	while (*string) {
		*video++ = *string++;
		*video++ = attr;
	}
}

void clear_screen() {
	uint8_t *video = (uint8_t *)VIDEO_MEMORY;
	uint8_t attr = COLOR;
	for (int i = 0; i < 80 * 25; i++) {
		*video++ = ' ';
		*video++ = attr;
	}
}

void cpu_halt(void) {
	__asm__ volatile("sti; hlt");
}

void timer_handler(uint32_t payload, void *ctx) {
	(void)payload;
	(void)ctx;
	// TODO: プリエンプティブマルチタスクする
	// extern void task_schedule(void);
	// task_schedule();
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

void outb(uint16_t port, uint8_t value) {
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

void outw(uint16_t port, uint16_t value) {
	__asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

void outl(uint16_t port, uint32_t value) {
	__asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}
