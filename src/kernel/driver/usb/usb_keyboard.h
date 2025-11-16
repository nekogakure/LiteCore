#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t modifiers; // Ctrl, Shift, Alt など
	uint8_t reserved;
	uint8_t keys[6]; // 同時押し可能な6キー
} __attribute__((packed)) usb_keyboard_report_t;

int usb_keyboard_init(void);
void usb_keyboard_poll(void);
bool usb_keyboard_available(void);
char usb_keyboard_getc(void);
char usb_scancode_to_ascii(uint8_t scancode, bool shift);

#endif // USB_KEYBOARD_H