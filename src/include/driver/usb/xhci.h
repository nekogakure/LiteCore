#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include <util/config.h>
#include <stdint.h>

/* xHCI USB速度定義 */
#define XHCI_SPEED_FULL     1  /* USB 1.1 Full Speed (12 Mbps) */
#define XHCI_SPEED_LOW      2  /* USB 1.1 Low Speed (1.5 Mbps) */
#define XHCI_SPEED_HIGH     3  /* USB 2.0 High Speed (480 Mbps) */
#define XHCI_SPEED_SUPER    4  /* USB 3.0 Super Speed (5 Gbps) */

/* xHCI Capability Registers (オフセット 0x00から) */
struct xhci_cap_regs {
	uint8_t  caplength;      /* 0x00: Capability Registers Length */
	uint8_t  reserved;
	uint16_t hciversion;     /* 0x02: Interface Version Number */
	uint32_t hcsparams1;     /* 0x04: Structural Parameters 1 */
	uint32_t hcsparams2;     /* 0x08: Structural Parameters 2 */
	uint32_t hcsparams3;     /* 0x0C: Structural Parameters 3 */
	uint32_t hccparams1;     /* 0x10: Capability Parameters 1 */
	uint32_t dboff;          /* 0x14: Doorbell Offset */
	uint32_t rtsoff;         /* 0x18: Runtime Register Space Offset */
	uint32_t hccparams2;     /* 0x1C: Capability Parameters 2 */
};

/* xHCI Operational Registers */
struct xhci_op_regs {
	uint32_t usbcmd;         /* 0x00: USB Command */
	uint32_t usbsts;         /* 0x04: USB Status */
	uint32_t pagesize;       /* 0x08: Page Size */
	uint32_t reserved1[2];
	uint32_t dnctrl;         /* 0x14: Device Notification Control */
	uint64_t crcr;           /* 0x18: Command Ring Control Register */
	uint32_t reserved2[4];
	uint64_t dcbaap;         /* 0x30: Device Context Base Address Array Pointer */
	uint32_t config;         /* 0x38: Configure */
};

/* xHCI Port Registers (Operational Registers領域内) */
struct xhci_port_regs {
	uint32_t portsc;         /* Port Status and Control */
	uint32_t portpmsc;       /* Port Power Management Status and Control */
	uint32_t portli;         /* Port Link Info */
	uint32_t porthlpmc;      /* Port Hardware LPM Control */
};

/* USBCMD Register bits */
#define XHCI_CMD_RUN         (1 << 0)   /* Run/Stop */
#define XHCI_CMD_HCRST       (1 << 1)   /* Host Controller Reset */
#define XHCI_CMD_INTE        (1 << 2)   /* Interrupter Enable */
#define XHCI_CMD_HSEE        (1 << 3)   /* Host System Error Enable */
#define XHCI_CMD_EWE         (1 << 10)  /* Enable Wrap Event */

/* USBSTS Register bits */
#define XHCI_STS_HCH         (1 << 0)   /* HC Halted */
#define XHCI_STS_HSE         (1 << 2)   /* Host System Error */
#define XHCI_STS_EINT        (1 << 3)   /* Event Interrupt */
#define XHCI_STS_PCD         (1 << 4)   /* Port Change Detect */
#define XHCI_STS_CNR         (1 << 11)  /* Controller Not Ready */
#define XHCI_STS_HCE         (1 << 12)  /* Host Controller Error */

/* PORTSC Register bits */
#define XHCI_PORTSC_CCS      (1 << 0)   /* Current Connect Status */
#define XHCI_PORTSC_PED      (1 << 1)   /* Port Enabled/Disabled */
#define XHCI_PORTSC_PR       (1 << 4)   /* Port Reset */
#define XHCI_PORTSC_PLS_MASK (0xF << 5) /* Port Link State */
#define XHCI_PORTSC_PP       (1 << 9)   /* Port Power */
#define XHCI_PORTSC_SPEED_MASK (0xF << 10) /* Port Speed */
#define XHCI_PORTSC_CSC      (1 << 17)  /* Connect Status Change */
#define XHCI_PORTSC_PEC      (1 << 18)  /* Port Enabled/Disabled Change */
#define XHCI_PORTSC_PRC      (1 << 21)  /* Port Reset Change */

/* xHCI Host Controller 構造体 */
struct xhci_hc {
	/* MMIO Base Address */
	uintptr_t base_addr;
	
	/* Register pointers */
	volatile struct xhci_cap_regs *cap_regs;
	volatile struct xhci_op_regs  *op_regs;
	volatile uint32_t             *doorbell_array;
	volatile void                 *runtime_regs;
	volatile struct xhci_port_regs *port_regs;
	
	/* Controller information */
	uint16_t hci_version;
	uint32_t max_slots;
	uint32_t max_ports;
	uint32_t max_intrs;
	
	/* PCI information */
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	
	/* State */
	int initialized;
};

/* 関数プロトタイプ */

/**
 * @brief PCIバスからxHCIコントローラを検出・初期化
 * @return 0: 成功, 負数: エラー
 */
int xhci_init(void);

/**
 * @brief xHCIコントローラをリセット
 * @param hc ホストコントローラ構造体
 * @return 0: 成功, 負数: エラー
 */
int xhci_reset_controller(struct xhci_hc *hc);

/**
 * @brief xHCIコントローラを起動
 * @param hc ホストコントローラ構造体
 * @return 0: 成功, 負数: エラー
 */
int xhci_start_controller(struct xhci_hc *hc);

/**
 * @brief xHCIポートの状態を取得
 * @param hc ホストコントローラ構造体
 * @param port_num ポート番号（1から開始）
 * @return ポートステータス
 */
uint32_t xhci_get_port_status(struct xhci_hc *hc, uint32_t port_num);

#endif /* _USB_XHCI_H */
