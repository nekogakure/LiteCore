/**
 * @file xhci.c
 * @brief xHCI (eXtensible Host Controller Interface) USB 3.0 Driver
 * 
 * xHCI仕様に基づくUSBホストコントローラドライバ
 * Phase 1: コントローラ検出と基本初期化
 */

#include <driver/usb/xhci.h>
#include <device/pci.h>
#include <util/console.h>
#include <util/io.h>
#include <driver/timer/timer.h>
#include <mem/manager.h>

/* デバッグ出力を有効化 */
#define XHCI_DEBUG 1

/* グローバルなxHCIコントローラインスタンス */
static struct xhci_hc g_xhci_controller;

/**
 * @brief 32bitレジスタ読み取り
 */
static inline uint32_t xhci_read32(volatile uint32_t *reg) {
	return *reg;
}

/**
 * @brief 32bitレジスタ書き込み
 */
static inline void xhci_write32(volatile uint32_t *reg, uint32_t value) {
	*reg = value;
}

/**
 * @brief 64bitレジスタ読み取り
 */
static inline uint64_t xhci_read64(volatile uint64_t *reg) {
	return *reg;
}

/**
 * @brief 64bitレジスタ書き込み
 */
static inline void xhci_write64(volatile uint64_t *reg, uint64_t value) {
	*reg = value;
}

/**
 * @brief xHCI Capability Registersの情報を表示
 */
static void xhci_dump_cap_regs(struct xhci_hc *hc) {
	uint32_t hcsparams1 = xhci_read32(&hc->cap_regs->hcsparams1);
	uint32_t hcsparams2 = xhci_read32(&hc->cap_regs->hcsparams2);

	hc->max_slots = (hcsparams1 & 0xFF);
	hc->max_intrs = ((hcsparams1 >> 8) & 0x7FF);
	hc->max_ports = ((hcsparams1 >> 24) & 0xFF);

#ifdef XHCI_DEBUG
	printk("xHCI: Capability Registers:\n");
	printk("  HCI Version: %x.%02x\n", hc->hci_version >> 8,
	       hc->hci_version & 0xFF);
	printk("  Max Device Slots: %u\n", hc->max_slots);
	printk("  Max Interrupters: %u\n", hc->max_intrs);
	printk("  Max Ports: %u\n", hc->max_ports);
	printk("  Max PSA Size: %u\n", (hcsparams2 >> 4) & 0xF);
#endif
}

/**
 * @brief xHCIコントローラをリセット
 */
int xhci_reset_controller(struct xhci_hc *hc) {
	uint32_t cmd, status;
	int timeout = 100; /* 100ms timeout */

#ifdef XHCI_DEBUG
	printk("xHCI: Resetting controller...\n");
#endif

	/* Run/Stopビットをクリアしてコントローラを停止 */
	cmd = xhci_read32(&hc->op_regs->usbcmd);
	cmd &= ~XHCI_CMD_RUN;
	xhci_write32(&hc->op_regs->usbcmd, cmd);

	/* コントローラが停止するまで待機 (HCH=1) */
	while (timeout-- > 0) {
		status = xhci_read32(&hc->op_regs->usbsts);
		if (status & XHCI_STS_HCH) {
			break;
		}
		kwait(1000); /* 1ms待機 */
	}

	if (timeout <= 0) {
		printk("xHCI: Error: Controller did not halt\n");
		return -1;
	}

	/* コントローラリセット */
	cmd = xhci_read32(&hc->op_regs->usbcmd);
	cmd |= XHCI_CMD_HCRST;
	xhci_write32(&hc->op_regs->usbcmd, cmd);

	/* リセット完了まで待機 (HCRST=0) */
	timeout = 500; /* 500ms timeout */
	while (timeout-- > 0) {
		cmd = xhci_read32(&hc->op_regs->usbcmd);
		status = xhci_read32(&hc->op_regs->usbsts);

		/* リセット完了かつCNR=0 */
		if (!(cmd & XHCI_CMD_HCRST) && !(status & XHCI_STS_CNR)) {
			break;
		}
		kwait(1000); /* 1ms待機 */
	}

	if (timeout <= 0) {
		printk("xHCI: Error: Controller reset timeout\n");
		return -2;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Controller reset completed\n");
#endif

	return 0;
}

/**
 * @brief xHCIコントローラを起動
 */
int xhci_start_controller(struct xhci_hc *hc) {
	uint32_t cmd, status;
	int timeout = 100; /* 100ms timeout */

#ifdef XHCI_DEBUG
	printk("xHCI: Starting controller...\n");
#endif

	/* Run/Stopビットをセット */
	cmd = xhci_read32(&hc->op_regs->usbcmd);
	cmd |= XHCI_CMD_RUN;
	xhci_write32(&hc->op_regs->usbcmd, cmd);

	/* コントローラが起動するまで待機 (HCH=0) */
	while (timeout-- > 0) {
		status = xhci_read32(&hc->op_regs->usbsts);
		if (!(status & XHCI_STS_HCH)) {
			break;
		}
		kwait(1000); /* 1ms待機 */
	}

	if (timeout <= 0) {
		printk("xHCI: Error: Controller did not start\n");
		return -1;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Controller started successfully\n");
#endif

	return 0;
}

/**
 * @brief xHCIポートの状態を取得
 */
uint32_t xhci_get_port_status(struct xhci_hc *hc, uint32_t port_num) {
	if (port_num == 0 || port_num > hc->max_ports) {
		return 0;
	}

	/* ポートレジスタは1-based indexing */
	volatile struct xhci_port_regs *port = &hc->port_regs[port_num - 1];

	return xhci_read32(&port->portsc);
}

/**
 * @brief PCIデバイスからxHCIコントローラを初期化
 */
static int xhci_probe_pci_device(uint8_t bus, uint8_t device,
				 uint8_t function) {
	struct xhci_hc *hc = &g_xhci_controller;
	uint32_t bar0;
	uintptr_t base_addr;
	uint8_t caplength;

#ifdef XHCI_DEBUG
	printk("xHCI: Found controller at PCI %02x:%02x.%x\n", bus, device,
	       function);
#endif

	/* PCI情報を保存 */
	hc->bus = bus;
	hc->device = device;
	hc->function = function;

	/* BAR0を読み取る（MMIO Base Address） */
	bar0 = pci_read_config_dword(bus, device, function, 0x10);

	/* Memory Spaceかチェック（bit 0 = 0） */
	if (bar0 & 1) {
		printk("xHCI: Error: BAR0 is not memory space\n");
		return -1;
	}

	/* Base Addressを取得（下位4bitはマスク） */
	base_addr = bar0 & 0xFFFFFFF0;
	hc->base_addr = base_addr;

#ifdef XHCI_DEBUG
	printk("xHCI: Base Address = 0x%08x\n", base_addr);
#endif

	/* Capability Registersをマップ */
	hc->cap_regs = (volatile struct xhci_cap_regs *)base_addr;

	/* Capability Lengthを取得 */
	caplength = hc->cap_regs->caplength;
	hc->hci_version = hc->cap_regs->hciversion;

	/* Operational Registersをマップ */
	hc->op_regs = (volatile struct xhci_op_regs *)(base_addr + caplength);

	/* Doorbell Arrayをマップ */
	uint32_t dboff = xhci_read32(&hc->cap_regs->dboff);
	hc->doorbell_array = (volatile uint32_t *)(base_addr + dboff);

	/* Runtime Registersをマップ */
	uint32_t rtsoff = xhci_read32(&hc->cap_regs->rtsoff);
	hc->runtime_regs = (volatile void *)(base_addr + rtsoff);

	/* Port Registersをマップ */
	hc->port_regs =
		(volatile struct xhci_port_regs *)((uintptr_t)hc->op_regs +
						   0x400);

	/* Capability情報を表示 */
	xhci_dump_cap_regs(hc);

	/* コントローラをリセット */
	if (xhci_reset_controller(hc) != 0) {
		return -2;
	}

	/* TODO: Phase 2でメモリ構造を割り当てる */

	/* コントローラを起動（Phase 2で有効化） */
	/* if (xhci_start_controller(hc) != 0) {
		return -3;
	} */

	hc->initialized = 1;

#ifdef XHCI_DEBUG
	printk("xHCI: Initialization complete (Phase 1)\n");
#endif

	return 0;
}

/**
 * @brief xHCIドライバの初期化
 */
int xhci_init(void) {
	uint32_t bus, device, function;
	uint32_t class_code, prog_if;

	printk("xHCI: Scanning PCI bus for USB 3.0 controllers...\n");

	/* PCIバスをスキャン */
	for (bus = 0; bus < 256; bus++) {
		for (device = 0; device < 32; device++) {
			for (function = 0; function < 8; function++) {
				/* ベンダーIDをチェック */
				uint16_t vendor_id =
					pci_read_config_dword(bus, device,
							      function, 0) &
					0xFFFF;
				if (vendor_id == 0xFFFF) {
					continue; /* デバイスなし */
				}

				/* Class Code取得 */
				uint32_t class_reg = pci_read_config_dword(
					bus, device, function, 0x08);
				class_code = (class_reg >> 8) & 0xFFFFFF;
				prog_if = class_reg & 0xFF;

				/* USB Controller (Class 0x0C03) かつ xHCI (PI 0x30) */
				if ((class_code >> 8) == 0x0C03 &&
				    prog_if == 0x30) {
					/* xHCIコントローラ発見 */
					return xhci_probe_pci_device(
						bus, device, function);
				}
			}
		}
	}

	printk("xHCI: No USB 3.0 controller found\n");
	return -1;
}
