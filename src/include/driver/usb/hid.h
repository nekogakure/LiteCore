#ifndef _USB_HID_H
#define _USB_HID_H

#include <stdint.h>

/* HID Class Descriptor Types */
#define HID_DT_HID          0x21
#define HID_DT_REPORT       0x22
#define HID_DT_PHYSICAL     0x23

/* HID Class Requests */
#define HID_REQ_GET_REPORT      0x01
#define HID_REQ_GET_IDLE        0x02
#define HID_REQ_GET_PROTOCOL    0x03
#define HID_REQ_SET_REPORT      0x09
#define HID_REQ_SET_IDLE        0x0A
#define HID_REQ_SET_PROTOCOL    0x0B

/* HID Protocol */
#define HID_PROTOCOL_BOOT       0
#define HID_PROTOCOL_REPORT     1

/* Boot Protocol Keyboard Report */
struct hid_keyboard_report {
	uint8_t modifier;     /* Ctrl, Shift, Alt, GUI */
	uint8_t reserved;
	uint8_t keycode[6];   /* 同時押し可能なキー（最大6個） */
} __attribute__((packed));

/* Modifier bits */
#define HID_MOD_LEFT_CTRL   0x01
#define HID_MOD_LEFT_SHIFT  0x02
#define HID_MOD_LEFT_ALT    0x04
#define HID_MOD_LEFT_GUI    0x08
#define HID_MOD_RIGHT_CTRL  0x10
#define HID_MOD_RIGHT_SHIFT 0x20
#define HID_MOD_RIGHT_ALT   0x40
#define HID_MOD_RIGHT_GUI   0x80

#endif /* _USB_HID_H */
