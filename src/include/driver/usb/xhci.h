#ifndef _USB_XHCI_H
#define _USB_XHCI_H

#include <util/config.h>
#include <stdint.h>

/* xHCI USB速度定義 */
#define XHCI_SPEED_FULL 1 /* USB 1.1 Full Speed (12 Mbps) */
#define XHCI_SPEED_LOW 2 /* USB 1.1 Low Speed (1.5 Mbps) */
#define XHCI_SPEED_HIGH 3 /* USB 2.0 High Speed (480 Mbps) */
#define XHCI_SPEED_SUPER 4 /* USB 3.0 Super Speed (5 Gbps) */

/* xHCI Capability Registers (オフセット 0x00から) */
struct xhci_cap_regs {
	uint8_t caplength; /* 0x00: Capability Registers Length */
	uint8_t reserved;
	uint16_t hciversion; /* 0x02: Interface Version Number */
	uint32_t hcsparams1; /* 0x04: Structural Parameters 1 */
	uint32_t hcsparams2; /* 0x08: Structural Parameters 2 */
	uint32_t hcsparams3; /* 0x0C: Structural Parameters 3 */
	uint32_t hccparams1; /* 0x10: Capability Parameters 1 */
	uint32_t dboff; /* 0x14: Doorbell Offset */
	uint32_t rtsoff; /* 0x18: Runtime Register Space Offset */
	uint32_t hccparams2; /* 0x1C: Capability Parameters 2 */
};

/* xHCI Operational Registers */
struct xhci_op_regs {
	uint32_t usbcmd; /* 0x00: USB Command */
	uint32_t usbsts; /* 0x04: USB Status */
	uint32_t pagesize; /* 0x08: Page Size */
	uint32_t reserved1[2];
	uint32_t dnctrl; /* 0x14: Device Notification Control */
	uint64_t crcr; /* 0x18: Command Ring Control Register */
	uint32_t reserved2[4];
	uint64_t dcbaap; /* 0x30: Device Context Base Address Array Pointer */
	uint32_t config; /* 0x38: Configure */
};

/* xHCI Port Registers (Operational Registers領域内) */
struct xhci_port_regs {
	uint32_t portsc; /* Port Status and Control */
	uint32_t portpmsc; /* Port Power Management Status and Control */
	uint32_t portli; /* Port Link Info */
	uint32_t porthlpmc; /* Port Hardware LPM Control */
};

/* USBCMD Register bits */
#define XHCI_CMD_RUN (1 << 0) /* Run/Stop */
#define XHCI_CMD_HCRST (1 << 1) /* Host Controller Reset */
#define XHCI_CMD_INTE (1 << 2) /* Interrupter Enable */
#define XHCI_CMD_HSEE (1 << 3) /* Host System Error Enable */
#define XHCI_CMD_EWE (1 << 10) /* Enable Wrap Event */

/* USBSTS Register bits */
#define XHCI_STS_HCH (1 << 0) /* HC Halted */
#define XHCI_STS_HSE (1 << 2) /* Host System Error */
#define XHCI_STS_EINT (1 << 3) /* Event Interrupt */
#define XHCI_STS_PCD (1 << 4) /* Port Change Detect */
#define XHCI_STS_CNR (1 << 11) /* Controller Not Ready */
#define XHCI_STS_HCE (1 << 12) /* Host Controller Error */

/* PORTSC Register bits */
#define XHCI_PORTSC_CCS (1 << 0) /* Current Connect Status */
#define XHCI_PORTSC_PED (1 << 1) /* Port Enabled/Disabled */
#define XHCI_PORTSC_PR (1 << 4) /* Port Reset */
#define XHCI_PORTSC_PLS_MASK (0xF << 5) /* Port Link State */
#define XHCI_PORTSC_PP (1 << 9) /* Port Power */
#define XHCI_PORTSC_SPEED_MASK (0xF << 10) /* Port Speed */
#define XHCI_PORTSC_CSC (1 << 17) /* Connect Status Change */
#define XHCI_PORTSC_PEC (1 << 18) /* Port Enabled/Disabled Change */
#define XHCI_PORTSC_PRC (1 << 21) /* Port Reset Change */

/* CRCR Register bits */
#define XHCI_CRCR_RCS (1 << 0) /* Ring Cycle State */
#define XHCI_CRCR_CS (1 << 1) /* Command Stop */
#define XHCI_CRCR_CA (1 << 2) /* Command Abort */
#define XHCI_CRCR_CRR (1 << 3) /* Command Ring Running */

/* Interrupt Management Register bits */
#define XHCI_IMAN_IP (1 << 0) /* Interrupt Pending */
#define XHCI_IMAN_IE (1 << 1) /* Interrupt Enable */

/* TRB Types */
#define TRB_TYPE_NORMAL 1
#define TRB_TYPE_SETUP 2
#define TRB_TYPE_DATA 3
#define TRB_TYPE_STATUS 4
#define TRB_TYPE_LINK 6
#define TRB_TYPE_EVENT_DATA 7
#define TRB_TYPE_NOOP_TRANSFER 8
#define TRB_TYPE_ENABLE_SLOT 9
#define TRB_TYPE_DISABLE_SLOT 10
#define TRB_TYPE_ADDRESS_DEVICE 11
#define TRB_TYPE_CONFIG_EP 12
#define TRB_TYPE_EVAL_CONTEXT 13
#define TRB_TYPE_RESET_EP 14
#define TRB_TYPE_STOP_EP 15
#define TRB_TYPE_SET_TR_DEQUEUE 16
#define TRB_TYPE_RESET_DEVICE 17
#define TRB_TYPE_NOOP_COMMAND 23
#define TRB_TYPE_TRANSFER_EVENT 32
#define TRB_TYPE_COMMAND_COMPLETION 33
#define TRB_TYPE_PORT_STATUS_CHANGE 34

/* TRB Control bits */
#define TRB_CYCLE (1 << 0)
#define TRB_ENT (1 << 1) /* Evaluate Next TRB */
#define TRB_ISP (1 << 2) /* Interrupt on Short Packet */
#define TRB_CH (1 << 4) /* Chain bit */
#define TRB_IOC (1 << 5) /* Interrupt On Completion */
#define TRB_IDT (1 << 6) /* Immediate Data */
#define TRB_DIR_IN (1 << 16) /* Direction (Data Stage) */

/* Endpoint Types */
#define EP_TYPE_CONTROL 4
#define EP_TYPE_ISOCH_OUT 1
#define EP_TYPE_BULK_OUT 2
#define EP_TYPE_INTERRUPT_OUT 3
#define EP_TYPE_ISOCH_IN 5
#define EP_TYPE_BULK_IN 6
#define EP_TYPE_INTERRUPT_IN 7

/* Ring sizes */
#define COMMAND_RING_SIZE 256
#define EVENT_RING_SIZE 256
#define TRANSFER_RING_SIZE 256

/* xHCI Transfer Request Block (TRB) */
struct xhci_trb {
	uint64_t parameter;
	uint32_t status;
	uint32_t control;
} __attribute__((packed));

/* Event Ring Segment Table Entry */
struct xhci_erst_entry {
	uint64_t ring_segment_base;
	uint32_t ring_segment_size;
	uint32_t reserved;
} __attribute__((packed));

/* Runtime Registers - Interrupter */
struct xhci_intr_regs {
	uint32_t iman; /* Interrupt Management */
	uint32_t imod; /* Interrupt Moderation */
	uint32_t erstsz; /* Event Ring Segment Table Size */
	uint32_t reserved;
	uint64_t erstba; /* Event Ring Segment Table Base Address */
	uint64_t erdp; /* Event Ring Dequeue Pointer */
} __attribute__((packed));

/* Device Context - Slot Context */
struct xhci_slot_context {
	uint32_t dw0;
	uint32_t dw1;
	uint32_t dw2;
	uint32_t dw3;
	uint32_t reserved[4];
} __attribute__((packed));

/* Device Context - Endpoint Context */
struct xhci_endpoint_context {
	uint32_t dw0;
	uint32_t dw1;
	uint64_t tr_dequeue_pointer;
	uint32_t dw4;
	uint32_t reserved[3];
} __attribute__((packed));

/* Device Context */
struct xhci_device_context {
	struct xhci_slot_context slot;
	struct xhci_endpoint_context ep[31];
} __attribute__((packed));

/* Input Context Control Context */
struct xhci_input_control_context {
	uint32_t drop_flags;
	uint32_t add_flags;
	uint32_t reserved[6];
} __attribute__((packed));

/* Input Context */
struct xhci_input_context {
	struct xhci_input_control_context input_control;
	struct xhci_slot_context slot;
	struct xhci_endpoint_context ep[31];
} __attribute__((packed));

/* xHCI Host Controller 構造体 */
struct xhci_hc {
	/* MMIO Base Address */
	uintptr_t base_addr;

	/* Register pointers */
	volatile struct xhci_cap_regs *cap_regs;
	volatile struct xhci_op_regs *op_regs;
	volatile uint32_t *doorbell_array;
	volatile void *runtime_regs;
	volatile struct xhci_port_regs *port_regs;
	volatile struct xhci_intr_regs *intr_regs;

	/* Controller information */
	uint16_t hci_version;
	uint32_t max_slots;
	uint32_t max_ports;
	uint32_t max_intrs;

	/* PCI information */
	uint8_t bus;
	uint8_t device;
	uint8_t function;

	/* Memory structures */
	uint64_t *dcbaa; /* Device Context Base Address Array */
	struct xhci_trb *command_ring;
	struct xhci_trb *event_ring;
	struct xhci_erst_entry *erst;

	/* Ring management */
	uint32_t cmd_ring_index;
	uint32_t cmd_ring_cycle;
	uint32_t event_ring_index;
	uint32_t event_ring_cycle;

	/* Device contexts */
	struct xhci_device_context *device_contexts[256];
	struct xhci_trb *transfer_rings[256][32]; /* [slot][endpoint] */
	uint32_t transfer_ring_index[256][32]; /* [slot][endpoint] */
	uint32_t transfer_ring_cycle[256][32]; /* [slot][endpoint] */

	/* Slot management */
	uint8_t slot_allocated[256];
	uint8_t keyboard_slot_id; /* キーボードデバイスのスロットID */
	uint8_t keyboard_endpoint; /* キーボードのInterrupt INエンドポイント */
	void *keyboard_buffer; /* キーボードレポート受信バッファ */

	/* State */
	int initialized;
};

typedef struct xhci_hc xhci_controller;

struct xhci_hc *xhci_find_controller(void);
int xhci_init(void);
int xhci_reset_controller(struct xhci_hc *hc);
int xhci_start_controller(struct xhci_hc *hc);
uint32_t xhci_get_port_status(struct xhci_hc *hc, uint32_t port_num);

/* Phase 2: Command/Event Ring functions */
int xhci_setup_command_ring(struct xhci_hc *hc);
int xhci_setup_event_ring(struct xhci_hc *hc);
void xhci_ring_doorbell(struct xhci_hc *hc, uint8_t slot_id, uint8_t target);
int xhci_wait_for_event(struct xhci_hc *hc, struct xhci_trb *event_trb,
			uint32_t timeout_ms);
void xhci_handle_events(struct xhci_hc *hc);

/* Phase 3: Device Enumeration functions */
int xhci_enable_slot(struct xhci_hc *hc);
int xhci_address_device(struct xhci_hc *hc, uint8_t slot_id, uint8_t port);
int xhci_configure_endpoint(struct xhci_hc *hc, uint8_t slot_id);
int xhci_reset_port(struct xhci_hc *hc, uint8_t port);
void xhci_handle_port_status_change(struct xhci_hc *hc, uint8_t port);

/* Phase 4: USB Protocol functions */
int xhci_control_transfer(struct xhci_hc *hc, uint8_t slot_id,
			  uint8_t request_type, uint8_t request, uint16_t value,
			  uint16_t index, void *data, uint16_t length);
int xhci_get_descriptor(struct xhci_hc *hc, uint8_t slot_id, uint8_t desc_type,
			uint8_t desc_index, void *buffer, uint16_t length);
int xhci_set_configuration(struct xhci_hc *hc, uint8_t slot_id,
			   uint8_t config_value);

/* Helper functions */
void *xhci_alloc_aligned(uint32_t size, uint32_t alignment);
void xhci_free_aligned(void *ptr);
uint8_t xhci_get_keyboard_slot(struct xhci_hc *hc);
int xhci_setup_keyboard_polling(struct xhci_hc *hc, uint8_t slot_id);
int xhci_poll_keyboard(struct xhci_hc *hc, uint8_t slot_id,
		       uint8_t *report_buffer);

/* HID Keyboard specific */
int xhci_configure_keyboard(struct xhci_hc *hc, uint8_t slot_id);
int xhci_set_boot_protocol(struct xhci_hc *hc, uint8_t slot_id,
			   uint8_t interface);
int xhci_submit_interrupt_in(struct xhci_hc *hc, uint8_t slot_id,
			     uint8_t endpoint, void *buffer, uint16_t length);

#endif /* _USB_XHCI_H */
