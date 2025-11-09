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
	hc->keyboard_slot_id = 0; /* 初期化 */
	hc->keyboard_endpoint = 0;
	hc->keyboard_buffer = NULL;

	/* Transfer Ring管理変数を初期化 */
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 32; j++) {
			hc->transfer_ring_index[i][j] = 0;
			hc->transfer_ring_cycle[i][j] = 1;
		}
	}

	/* BAR0を読み取る（MMIO Base Address） */
	bar0 = pci_read_config_dword(bus, device, function, 0x10);

#ifdef XHCI_DEBUG
	printk("xHCI: BAR0 = 0x%08x\n", bar0);
#endif

	/* Memory Spaceかチェック（bit 0 = 0） */
	if (bar0 & 1) {
		printk("xHCI: Error: BAR0 is not memory space\n");
		return -1;
	}

	/* 64-bit BARかチェック（bits 2:1 = 10b） */
	if ((bar0 & 0x6) == 0x4) {
		/* 64-bit BAR: BAR0とBAR1を合わせて読む */
		uint32_t bar1 =
			pci_read_config_dword(bus, device, function, 0x14);
		uint64_t bar64 = ((uint64_t)bar1 << 32) | (bar0 & 0xFFFFFFF0);
		base_addr = (uintptr_t)bar64;
#ifdef XHCI_DEBUG
		printk("xHCI: 64-bit BAR: BAR1 = 0x%08x, Address = 0x%08x%08x\n",
		       bar1, (uint32_t)(base_addr >> 32), (uint32_t)(base_addr & 0xFFFFFFFF));
#endif
	} else {
		/* 32-bit BAR */
		base_addr = bar0 & 0xFFFFFFF0;
#ifdef XHCI_DEBUG
		printk("xHCI: 32-bit BAR: Address = 0x%08x\n", base_addr);
#endif
	}

	hc->base_addr = base_addr;

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

	/* Phase 2: Command RingとEvent Ringを設定 */
	if (xhci_setup_command_ring(hc) != 0) {
		return -3;
	}

	if (xhci_setup_event_ring(hc) != 0) {
		return -4;
	}

	/* MaxSlotsEnを設定 */
	uint32_t config = xhci_read32(&hc->op_regs->config);
	config &= ~0xFF;
	config |= hc->max_slots;
	xhci_write32(&hc->op_regs->config, config);

#ifdef XHCI_DEBUG
	printk("xHCI: MaxSlotsEn set to %u\n", hc->max_slots);
#endif

	/* コントローラを起動 */
	if (xhci_start_controller(hc) != 0) {
		return -5;
	}

	hc->initialized = 1;

#ifdef XHCI_DEBUG
	printk("xHCI: Initialization complete (Phase 1-2)\n");
#endif

	/* ポート状態をチェック */
	printk("xHCI: Scanning %u ports for devices...\n", hc->max_ports);
	for (uint32_t port = 1; port <= hc->max_ports; port++) {
		uint32_t portsc = xhci_get_port_status(hc, port);
		printk("xHCI: Port %u status: 0x%08x (CCS=%d, PED=%d)\n", 
		       port, portsc, 
		       (portsc & XHCI_PORTSC_CCS) ? 1 : 0,
		       (portsc & XHCI_PORTSC_PED) ? 1 : 0);
		if (portsc & XHCI_PORTSC_CCS) {
			printk("xHCI: Device detected on port %u at init\n", port);
			xhci_handle_port_status_change(hc, port);
		}
	}

	return 0;
}

/**
 * @brief xHCIドライバの初期化
 */
int xhci_init(void) {
	uint32_t bus, device, function;
	uint32_t class_code;

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

				/* USB xHCI Controller (Class 0x0C0330) */
				if (class_code == 0x0C0330) {
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

/**
 * @brief PCIバスからxHCIコントローラを検索
 * @return コントローラ構造体へのポインタ、見つからない場合NULL
 */
struct xhci_hc *xhci_find_controller(void) {
	if (g_xhci_controller.initialized) {
		return &g_xhci_controller;
	}

	// 初期化されていない場合は検索
	if (xhci_init() == 0) {
		return &g_xhci_controller;
	}

	return NULL;
}

/**
 * @brief アライメントされたメモリを確保
 */
void *xhci_alloc_aligned(uint32_t size, uint32_t alignment) {
	/* ヒープからメモリを確保 */
	void *ptr = kmalloc(size + alignment - 1);
	if (!ptr) {
		return NULL;
	}

	/* アライメント調整 */
	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
	
	return (void *)aligned_addr;
}

/**
 * @brief アライメントされたメモリを解放
 */
void xhci_free_aligned(void *ptr) {
	if (ptr) {
		kfree(ptr);
	}
}

/**
 * @brief Command Ringを初期化
 */
int xhci_setup_command_ring(struct xhci_hc *hc) {
#ifdef XHCI_DEBUG
	printk("xHCI: Setting up Command Ring...\n");
#endif

	/* Command Ringのメモリを確保 (64-byteアライメント) */
	hc->command_ring = (struct xhci_trb *)xhci_alloc_aligned(
		COMMAND_RING_SIZE * sizeof(struct xhci_trb), 64);
	
	if (!hc->command_ring) {
		printk("xHCI: Error: Failed to allocate Command Ring\n");
		return -1;
	}

	/* Command Ringを初期化 */
	for (uint32_t i = 0; i < COMMAND_RING_SIZE; i++) {
		hc->command_ring[i].parameter = 0;
		hc->command_ring[i].status = 0;
		hc->command_ring[i].control = 0;
	}

	/* Link TRBを最後に配置してリング構造にする */
	struct xhci_trb *link_trb = &hc->command_ring[COMMAND_RING_SIZE - 1];
	link_trb->parameter = (uint64_t)(uintptr_t)hc->command_ring;
	link_trb->status = 0;
	link_trb->control = (TRB_TYPE_LINK << 10) | TRB_CYCLE;

	/* リング管理変数を初期化 */
	hc->cmd_ring_index = 0;
	hc->cmd_ring_cycle = 1;

	/* CRCR レジスタに設定 (RCS=1) */
	uint64_t crcr = (uint64_t)(uintptr_t)hc->command_ring | XHCI_CRCR_RCS;
	xhci_write64(&hc->op_regs->crcr, crcr);

#ifdef XHCI_DEBUG
	printk("xHCI: Command Ring initialized at 0x%08x%08x\n",
	       (uint32_t)((uintptr_t)hc->command_ring >> 32),
	       (uint32_t)((uintptr_t)hc->command_ring & 0xFFFFFFFF));
#endif

	return 0;
}

/**
 * @brief Event Ringを初期化
 */
int xhci_setup_event_ring(struct xhci_hc *hc) {
#ifdef XHCI_DEBUG
	printk("xHCI: Setting up Event Ring...\n");
#endif

	/* Event Ring Segment Table (ERST) を確保 (64-byteアライメント) */
	hc->erst = (struct xhci_erst_entry *)xhci_alloc_aligned(
		sizeof(struct xhci_erst_entry), 64);
	
	if (!hc->erst) {
		printk("xHCI: Error: Failed to allocate ERST\n");
		return -1;
	}

	/* Event Ringを確保 (64-byteアライメント) */
	hc->event_ring = (struct xhci_trb *)xhci_alloc_aligned(
		EVENT_RING_SIZE * sizeof(struct xhci_trb), 64);
	
	if (!hc->event_ring) {
		printk("xHCI: Error: Failed to allocate Event Ring\n");
		xhci_free_aligned(hc->erst);
		return -2;
	}

	/* Event Ringを初期化 */
	for (uint32_t i = 0; i < EVENT_RING_SIZE; i++) {
		hc->event_ring[i].parameter = 0;
		hc->event_ring[i].status = 0;
		hc->event_ring[i].control = 0;
	}

	/* ERSTエントリを設定 */
	hc->erst[0].ring_segment_base = (uint64_t)(uintptr_t)hc->event_ring;
	hc->erst[0].ring_segment_size = EVENT_RING_SIZE;
	hc->erst[0].reserved = 0;

	/* リング管理変数を初期化 */
	hc->event_ring_index = 0;
	hc->event_ring_cycle = 1;

	/* Runtime Registers の Interrupter 0 に設定 */
	hc->intr_regs = (volatile struct xhci_intr_regs *)
		((uintptr_t)hc->runtime_regs + 0x20);

	/* ERSTSZ: Event Ring Segment Tableのサイズ (offset 0x08) */
	volatile uint32_t *erstsz_ptr = (volatile uint32_t *)((uintptr_t)hc->intr_regs + 0x08);
	xhci_write32(erstsz_ptr, 1);

	/* ERSTBA: Event Ring Segment Tableのベースアドレス (offset 0x10) */
	volatile uint64_t *erstba_ptr = (volatile uint64_t *)((uintptr_t)hc->intr_regs + 0x10);
	xhci_write64(erstba_ptr, (uint64_t)(uintptr_t)hc->erst);

	/* ERDP: Event Ring Dequeue Pointer (offset 0x18) */
	volatile uint64_t *erdp_ptr = (volatile uint64_t *)((uintptr_t)hc->intr_regs + 0x18);
	xhci_write64(erdp_ptr, (uint64_t)(uintptr_t)hc->event_ring);

	/* IMAN: Interrupt Management (offset 0x00) */
	volatile uint32_t *iman_ptr = (volatile uint32_t *)((uintptr_t)hc->intr_regs + 0x00);
	xhci_write32(iman_ptr, XHCI_IMAN_IE | XHCI_IMAN_IP);

#ifdef XHCI_DEBUG
	printk("xHCI: Event Ring initialized at 0x%08x%08x\n",
	       (uint32_t)((uintptr_t)hc->event_ring >> 32),
	       (uint32_t)((uintptr_t)hc->event_ring & 0xFFFFFFFF));
#endif

	return 0;
}

/**
 * @brief Doorbellを鳴らす
 */
void xhci_ring_doorbell(struct xhci_hc *hc, uint8_t slot_id, uint8_t target) {
	/* Doorbell Array: doorbell_array[slot_id] に target を書き込む */
	xhci_write32(&hc->doorbell_array[slot_id], target);
}

/**
 * @brief Event Ringからイベントを取得
 */
int xhci_wait_for_event(struct xhci_hc *hc, struct xhci_trb *event_trb, uint32_t timeout_ms) {
	uint32_t elapsed = 0;

	while (elapsed < timeout_ms) {
		struct xhci_trb *trb = &hc->event_ring[hc->event_ring_index];
		uint32_t cycle = trb->control & TRB_CYCLE;

		/* Cycle Bitが一致すればイベント発生 */
		if (cycle == hc->event_ring_cycle) {
			/* イベントをコピー */
			if (event_trb) {
				event_trb->parameter = trb->parameter;
				event_trb->status = trb->status;
				event_trb->control = trb->control;
			}

			/* インデックスを進める */
			hc->event_ring_index++;
			if (hc->event_ring_index >= EVENT_RING_SIZE) {
				hc->event_ring_index = 0;
				hc->event_ring_cycle ^= 1;
			}

			/* ERDP を更新 (offset 0x18) */
			uint64_t erdp = (uint64_t)(uintptr_t)&hc->event_ring[hc->event_ring_index];
			volatile uint64_t *erdp_ptr = (volatile uint64_t *)((uintptr_t)hc->intr_regs + 0x18);
			xhci_write64(erdp_ptr, erdp | (1 << 3)); /* EHB=1 */

			return 0;
		}

		kwait(1000); /* 1ms待機 */
		elapsed++;
	}

	return -1; /* タイムアウト */
}

/**
 * @brief イベントハンドラ
 */
void xhci_handle_events(struct xhci_hc *hc) {
	struct xhci_trb event;

	while (xhci_wait_for_event(hc, &event, 0) == 0) {
		uint32_t trb_type = (event.control >> 10) & 0x3F;

		switch (trb_type) {
		case TRB_TYPE_PORT_STATUS_CHANGE:
		{
			uint32_t port_id = (event.parameter >> 24) & 0xFF;
#ifdef XHCI_DEBUG
			printk("xHCI: Port %u status change\n", port_id);
#endif
			xhci_handle_port_status_change(hc, port_id);
			break;
		}
		case TRB_TYPE_COMMAND_COMPLETION:
#ifdef XHCI_DEBUG
			printk("xHCI: Command completion event\n");
#endif
			break;
		case TRB_TYPE_TRANSFER_EVENT:
#ifdef XHCI_DEBUG
			printk("xHCI: Transfer event\n");
#endif
			break;
		default:
#ifdef XHCI_DEBUG
			printk("xHCI: Unknown event type: %u\n", trb_type);
#endif
			break;
		}
	}
}

/**
 * @brief ポートリセット
 */
int xhci_reset_port(struct xhci_hc *hc, uint8_t port) {
	if (port == 0 || port > hc->max_ports) {
		return -1;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Resetting port %u...\n", port);
#endif

	volatile struct xhci_port_regs *port_reg = &hc->port_regs[port - 1];
	uint32_t portsc = xhci_read32(&port_reg->portsc);

	/* Port Resetビットをセット */
	portsc |= XHCI_PORTSC_PR;
	xhci_write32(&port_reg->portsc, portsc);

	/* リセット完了まで待機 (Port Reset Change ビット) */
	uint32_t timeout = 500; /* 500ms */
	while (timeout-- > 0) {
		portsc = xhci_read32(&port_reg->portsc);
		if (portsc & XHCI_PORTSC_PRC) {
			/* Port Reset Change: リセット完了 */
			/* PRCビットをクリア (W1C: Write 1 to Clear) */
			xhci_write32(&port_reg->portsc, portsc | XHCI_PORTSC_PRC);
#ifdef XHCI_DEBUG
			printk("xHCI: Port %u reset completed\n", port);
#endif
			return 0;
		}
		kwait(1000); /* 1ms待機 */
	}

	printk("xHCI: Error: Port %u reset timeout\n", port);
	return -2;
}

/**
 * @brief Enable Slot Command
 */
int xhci_enable_slot(struct xhci_hc *hc) {
#ifdef XHCI_DEBUG
	printk("xHCI: Enabling slot...\n");
#endif

	/* Command TRBを作成 */
	struct xhci_trb *cmd_trb = &hc->command_ring[hc->cmd_ring_index];
	cmd_trb->parameter = 0;
	cmd_trb->status = 0;
	cmd_trb->control = (TRB_TYPE_ENABLE_SLOT << 10) | 
	                   (hc->cmd_ring_cycle ? TRB_CYCLE : 0);

	/* インデックスを進める */
	hc->cmd_ring_index++;
	if (hc->cmd_ring_index >= COMMAND_RING_SIZE - 1) {
		hc->cmd_ring_index = 0;
		hc->cmd_ring_cycle ^= 1;
	}

	/* Doorbell 0を鳴らす (Host Controller Command) */
	xhci_ring_doorbell(hc, 0, 0);

	/* Command Completion Eventを待機（他のイベントはスキップ） */
	struct xhci_trb event;
	uint32_t attempts = 0;
	while (attempts < 1000) {
		if (xhci_wait_for_event(hc, &event, 1) != 0) {
			attempts++;
			continue;
		}

		uint32_t trb_type = (event.control >> 10) & 0x3F;
		
		/* Port Status Changeイベントは無視（後で処理） */
		if (trb_type == TRB_TYPE_PORT_STATUS_CHANGE) {
#ifdef XHCI_DEBUG
			printk("xHCI: Port status change during slot enable (ignored)\n");
#endif
			continue;
		}
		
		/* Command Completionを待つ */
		if (trb_type == TRB_TYPE_COMMAND_COMPLETION) {
			break;
		}
		
		/* その他の予期しないイベント */
#ifdef XHCI_DEBUG
		printk("xHCI: Unexpected event type %u during slot enable\n", trb_type);
#endif
		attempts++;
	}

	if (attempts >= 1000) {
		printk("xHCI: Error: Enable Slot command timeout\n");
		return -1;
	}

	/* Completion Codeを確認 */
	uint32_t completion_code = (event.status >> 24) & 0xFF;
	if (completion_code != 1) { /* 1 = Success */
		printk("xHCI: Error: Enable Slot failed with code %u\n", completion_code);
		return -3;
	}

	/* Slot IDを取得 */
	uint8_t slot_id = (event.control >> 24) & 0xFF;

#ifdef XHCI_DEBUG
	printk("xHCI: Slot %u enabled\n", slot_id);
#endif

	hc->slot_allocated[slot_id] = 1;
	return slot_id;
}

/**
 * @brief Address Device Command
 */
int xhci_address_device(struct xhci_hc *hc, uint8_t slot_id, uint8_t port) {
	if (slot_id == 0 || slot_id > hc->max_slots) {
		return -1;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Addressing device on slot %u, port %u...\n", slot_id, port);
#endif

	/* Input Contextを作成 (64-byteアライメント) */
	struct xhci_input_context *input_ctx = 
		(struct xhci_input_context *)xhci_alloc_aligned(
			sizeof(struct xhci_input_context), 64);
	
	if (!input_ctx) {
		printk("xHCI: Error: Failed to allocate Input Context\n");
		return -2;
	}

	/* Input Contextを初期化 */
	for (uint32_t i = 0; i < sizeof(struct xhci_input_context) / 4; i++) {
		((uint32_t *)input_ctx)[i] = 0;
	}

	/* Input Control Contextを設定 */
	input_ctx->input_control.add_flags = 0x03; /* Slot Context と EP0 Context を追加 */

	/* Slot Contextを設定 */
	input_ctx->slot.dw0 = (1 << 27); /* Context Entries = 1 (EP0のみ) */
	input_ctx->slot.dw1 = (port << 16); /* Root Hub Port Number */

	/* Endpoint 0 Contextを設定 (コントロールパイプ) */
	/* Transfer Ringを確保 (16-byteアライメント) */
	struct xhci_trb *transfer_ring = (struct xhci_trb *)xhci_alloc_aligned(
		TRANSFER_RING_SIZE * sizeof(struct xhci_trb), 16);
	
	if (!transfer_ring) {
		printk("xHCI: Error: Failed to allocate Transfer Ring\n");
		xhci_free_aligned(input_ctx);
		return -3;
	}

	/* Transfer Ringを初期化 */
	for (uint32_t i = 0; i < TRANSFER_RING_SIZE; i++) {
		transfer_ring[i].parameter = 0;
		transfer_ring[i].status = 0;
		transfer_ring[i].control = 0;
	}

	hc->transfer_rings[slot_id][0] = transfer_ring;

	/* EP0 Context */
	input_ctx->ep[0].dw0 = 0; /* Interval, MaxPStreams, Mult, etc. */
	input_ctx->ep[0].dw1 = (EP_TYPE_CONTROL << 3) | (8 << 16); /* EP Type=Control, Max Packet Size=8 */
	input_ctx->ep[0].tr_dequeue_pointer = (uint64_t)(uintptr_t)transfer_ring | 1; /* DCS=1 */
	input_ctx->ep[0].dw4 = (8 << 16); /* Average TRB Length=8 */

	/* Device Contextを確保 */
	struct xhci_device_context *device_ctx = 
		(struct xhci_device_context *)xhci_alloc_aligned(
			sizeof(struct xhci_device_context), 64);
	
	if (!device_ctx) {
		printk("xHCI: Error: Failed to allocate Device Context\n");
		xhci_free_aligned(transfer_ring);
		xhci_free_aligned(input_ctx);
		return -4;
	}

	hc->device_contexts[slot_id] = device_ctx;

	/* DCBAAPにDevice Contextを設定 */
	if (!hc->dcbaa) {
		/* DCBAARを確保 (64-byteアライメント) */
		hc->dcbaa = (uint64_t *)xhci_alloc_aligned(
			(hc->max_slots + 1) * sizeof(uint64_t), 64);
		
		if (!hc->dcbaa) {
			printk("xHCI: Error: Failed to allocate DCBAA\n");
			xhci_free_aligned(device_ctx);
			xhci_free_aligned(transfer_ring);
			xhci_free_aligned(input_ctx);
			return -5;
		}

		/* DCBAAPをクリア */
		for (uint32_t i = 0; i <= hc->max_slots; i++) {
			hc->dcbaa[i] = 0;
		}

		/* DCBAAPレジスタに設定 */
		xhci_write64(&hc->op_regs->dcbaap, (uint64_t)(uintptr_t)hc->dcbaa);
	}

	hc->dcbaa[slot_id] = (uint64_t)(uintptr_t)device_ctx;

	/* Address Device Commandを発行 */
	struct xhci_trb *cmd_trb = &hc->command_ring[hc->cmd_ring_index];
	cmd_trb->parameter = (uint64_t)(uintptr_t)input_ctx;
	cmd_trb->status = 0;
	cmd_trb->control = (TRB_TYPE_ADDRESS_DEVICE << 10) | 
	                   (slot_id << 24) |
	                   (hc->cmd_ring_cycle ? TRB_CYCLE : 0);

	/* インデックスを進める */
	hc->cmd_ring_index++;
	if (hc->cmd_ring_index >= COMMAND_RING_SIZE - 1) {
		hc->cmd_ring_index = 0;
		hc->cmd_ring_cycle ^= 1;
	}

	/* Doorbell 0を鳴らす */
	xhci_ring_doorbell(hc, 0, 0);

	/* Command Completion Eventを待機（他のイベントはスキップ） */
	struct xhci_trb event;
	uint32_t attempts = 0;
	while (attempts < 1000) {
		if (xhci_wait_for_event(hc, &event, 1) != 0) {
			attempts++;
			continue;
		}

		uint32_t trb_type = (event.control >> 10) & 0x3F;
		
		/* Port Status Changeイベントは無視 */
		if (trb_type == TRB_TYPE_PORT_STATUS_CHANGE) {
#ifdef XHCI_DEBUG
			printk("xHCI: Port status change during address device (ignored)\n");
#endif
			continue;
		}
		
		/* Command Completionを待つ */
		if (trb_type == TRB_TYPE_COMMAND_COMPLETION) {
			break;
		}
		
		attempts++;
	}

	if (attempts >= 1000) {
		printk("xHCI: Error: Address Device command timeout\n");
		xhci_free_aligned(input_ctx);
		return -6;
	}

	/* Completion Codeを確認 */
	uint32_t completion_code = (event.status >> 24) & 0xFF;
	if (completion_code != 1) { /* 1 = Success */
		printk("xHCI: Error: Address Device failed with code %u\n", completion_code);
		xhci_free_aligned(input_ctx);
		return -7;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Device addressed on slot %u\n", slot_id);
#endif

	xhci_free_aligned(input_ctx);
	return 0;
}

/**
 * @brief ポート状態変化ハンドラ
 */
void xhci_handle_port_status_change(struct xhci_hc *hc, uint8_t port) {
	if (port == 0 || port > hc->max_ports) {
		return;
	}

	uint32_t portsc = xhci_get_port_status(hc, port);

#ifdef XHCI_DEBUG
	printk("xHCI: Port %u status = 0x%08x\n", port, portsc);
#endif

	/* デバイスが接続されているか確認 */
	if (portsc & XHCI_PORTSC_CCS) {
		/* デバイス接続 */
#ifdef XHCI_DEBUG
		printk("xHCI: Device connected on port %u\n", port);
#endif

		/* ポートが有効化されているか確認 */
		if (!(portsc & XHCI_PORTSC_PED)) {
			/* ポートリセットが必要 */
			if (xhci_reset_port(hc, port) != 0) {
				return;
			}
		}

		/* デバイスを列挙 */
		int slot_id = xhci_enable_slot(hc);
		if (slot_id < 0) {
			printk("xHCI: Error: Failed to enable slot\n");
			return;
		}

		if (xhci_address_device(hc, slot_id, port) != 0) {
			printk("xHCI: Error: Failed to address device\n");
			return;
		}

		/* 最初に列挙されたデバイスをキーボードとして扱う（簡易実装） */
		if (hc->keyboard_slot_id == 0) {
			hc->keyboard_slot_id = slot_id;
			printk("xHCI: Device on slot %u registered as keyboard\n", slot_id);
		} else {
			printk("xHCI: Device on slot %u enumerated (keyboard already registered on slot %u)\n", 
			       slot_id, hc->keyboard_slot_id);
		}

		printk("xHCI: Device enumeration completed on port %u\n", port);
	} else {
		/* デバイス切断 */
#ifdef XHCI_DEBUG
		printk("xHCI: Device disconnected on port %u\n", port);
#endif
	}

	/* Connect Status Changeビットをクリア */
	if (portsc & XHCI_PORTSC_CSC) {
		volatile struct xhci_port_regs *port_reg = &hc->port_regs[port - 1];
		xhci_write32(&port_reg->portsc, portsc | XHCI_PORTSC_CSC);
	}
}

/**
 * @brief コントロール転送を実行
 */
int xhci_control_transfer(struct xhci_hc *hc, uint8_t slot_id,
	uint8_t request_type, uint8_t request,
	uint16_t value, uint16_t index,
	void *data, uint16_t length) {
	
	if (slot_id == 0 || slot_id > hc->max_slots || !hc->slot_allocated[slot_id]) {
		return -1;
	}

	/* Transfer Ringを取得 (EP0 = DCI 1) */
	struct xhci_trb *transfer_ring = hc->transfer_rings[slot_id][0];
	if (!transfer_ring) {
		return -2;
	}

	uint32_t tr_index = hc->transfer_ring_index[slot_id][0];
	uint32_t cycle = hc->transfer_ring_cycle[slot_id][0];

	/* Setup Packetを作成 */
	struct {
		uint8_t bmRequestType;
		uint8_t bRequest;
		uint16_t wValue;
		uint16_t wIndex;
		uint16_t wLength;
	} __attribute__((packed)) setup_packet;

	setup_packet.bmRequestType = request_type;
	setup_packet.bRequest = request;
	setup_packet.wValue = value;
	setup_packet.wIndex = index;
	setup_packet.wLength = length;

	/* Setup Stage TRB */
	struct xhci_trb *setup_trb = &transfer_ring[tr_index];
	setup_trb->parameter = *(uint64_t *)&setup_packet;
	setup_trb->status = 8; /* Transfer Length = 8 bytes */
	setup_trb->control = (TRB_TYPE_SETUP << 10) | TRB_IDT | (cycle ? TRB_CYCLE : 0);
	
	/* TRT (Transfer Type): 0=No Data, 2=OUT, 3=IN */
	if (length > 0) {
		if (request_type & 0x80) {
			setup_trb->control |= (3 << 16); /* IN */
		} else {
			setup_trb->control |= (2 << 16); /* OUT */
		}
	}

	tr_index++;
	if (tr_index >= TRANSFER_RING_SIZE - 1) {
		tr_index = 0;
		cycle ^= 1;
	}

	/* Data Stage TRB (データがある場合) */
	if (length > 0 && data) {
		struct xhci_trb *data_trb = &transfer_ring[tr_index];
		data_trb->parameter = (uint64_t)(uintptr_t)data;
		data_trb->status = length | (0 << 22); /* TD Size = 0 */
		data_trb->control = (TRB_TYPE_DATA << 10) | (cycle ? TRB_CYCLE : 0);
		
		if (request_type & 0x80) {
			data_trb->control |= TRB_DIR_IN; /* IN */
		}

		tr_index++;
		if (tr_index >= TRANSFER_RING_SIZE - 1) {
			tr_index = 0;
			cycle ^= 1;
		}
	}

	/* Status Stage TRB */
	struct xhci_trb *status_trb = &transfer_ring[tr_index];
	status_trb->parameter = 0;
	status_trb->status = 0;
	status_trb->control = (TRB_TYPE_STATUS << 10) | TRB_IOC | (cycle ? TRB_CYCLE : 0);
	
	/* Direction: Setup Stageの逆方向 */
	if (length == 0) {
		status_trb->control |= TRB_DIR_IN; /* No Data -> Status IN */
	} else if (request_type & 0x80) {
		/* Data IN -> Status OUT (DIR=0) */
	} else {
		status_trb->control |= TRB_DIR_IN; /* Data OUT -> Status IN */
	}

	tr_index++;
	if (tr_index >= TRANSFER_RING_SIZE - 1) {
		tr_index = 0;
		cycle ^= 1;
	}

	/* Transfer Ring管理変数を更新 */
	hc->transfer_ring_index[slot_id][0] = tr_index;
	hc->transfer_ring_cycle[slot_id][0] = cycle;

	/* Doorbell 1を鳴らす (EP0 = DCI 1) */
	xhci_ring_doorbell(hc, slot_id, 1);

	/* Transfer Eventを待機 */
	struct xhci_trb event;
	uint32_t attempts = 0;
	while (attempts < 1000) {
		if (xhci_wait_for_event(hc, &event, 1) != 0) {
			attempts++;
			continue;
		}

		uint32_t trb_type = (event.control >> 10) & 0x3F;
		
		/* Port Status Changeイベントは無視 */
		if (trb_type == TRB_TYPE_PORT_STATUS_CHANGE) {
			continue;
		}
		
		/* Transfer Eventを待つ */
		if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
			/* Completion Codeを確認 */
			uint32_t completion_code = (event.status >> 24) & 0xFF;
			if (completion_code == 1 || completion_code == 13) { /* Success or Short Packet */
#ifdef XHCI_DEBUG
				printk("xHCI: Control transfer completed\n");
#endif
				return 0;
			} else {
				printk("xHCI: Control transfer failed with code %u\n", completion_code);
				return -3;
			}
		}
		
		attempts++;
	}

	printk("xHCI: Control transfer timeout\n");
	return -4;
}

/**
 * @brief GET_DESCRIPTORを実行
 */
int xhci_get_descriptor(struct xhci_hc *hc, uint8_t slot_id,
	uint8_t desc_type, uint8_t desc_index,
	void *buffer, uint16_t length) {
	
	return xhci_control_transfer(hc, slot_id,
		0x80, /* Device-to-Host, Standard, Device */
		0x06, /* GET_DESCRIPTOR */
		(desc_type << 8) | desc_index,
		0,
		buffer, length);
}

/**
 * @brief SET_CONFIGURATIONを実行
 */
int xhci_set_configuration(struct xhci_hc *hc, uint8_t slot_id, uint8_t config_value) {
	return xhci_control_transfer(hc, slot_id,
		0x00, /* Host-to-Device, Standard, Device */
		0x09, /* SET_CONFIGURATION */
		config_value,
		0,
		NULL, 0);
}

/**
 * @brief キーボードスロットIDを取得
 */
uint8_t xhci_get_keyboard_slot(struct xhci_hc *hc) {
	if (!hc) {
		return 0;
	}
	return hc->keyboard_slot_id;
}

/**
 * @brief キーボードポーリングのセットアップ
 */
int xhci_setup_keyboard_polling(struct xhci_hc *hc, uint8_t slot_id) {
	if (!hc || slot_id == 0 || slot_id > hc->max_slots) {
		return -1;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Setting up keyboard polling for slot %u\n", slot_id);
#endif

	/* キーボードを設定 */
	if (xhci_configure_keyboard(hc, slot_id) != 0) {
		printk("xHCI: Failed to configure keyboard\n");
		return -2;
	}

	/* キーボードレポート受信バッファを確保 */
	hc->keyboard_buffer = xhci_alloc_aligned(8, 64);
	if (!hc->keyboard_buffer) {
		printk("xHCI: Failed to allocate keyboard buffer\n");
		return -3;
	}

	/* Interrupt INエンドポイントを設定（通常はEP1 IN） */
	hc->keyboard_endpoint = 1;

	/* 最初のInterrupt IN Transferを送信 */
	if (xhci_submit_interrupt_in(hc, slot_id, hc->keyboard_endpoint, 
	                              hc->keyboard_buffer, 8) != 0) {
		printk("xHCI: Failed to submit interrupt IN\n");
		return -4;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Keyboard polling setup complete\n");
#endif
	
	return 0;
}

/**
 * @brief キーボードからのレポートをポーリング
 * @return 0:成功（データあり）、負数:エラーまたはデータなし
 */
int xhci_poll_keyboard(struct xhci_hc *hc, uint8_t slot_id, uint8_t *report_buffer) {
	if (!hc || slot_id == 0 || !report_buffer || !hc->keyboard_buffer) {
		return -1;
	}

	/* Event Ringをチェック */
	struct xhci_trb event;
	if (xhci_wait_for_event(hc, &event, 0) != 0) {
		return -2; /* イベントなし */
	}

	uint32_t trb_type = (event.control >> 10) & 0x3F;
	
	/* Port Status Changeは無視 */
	if (trb_type == TRB_TYPE_PORT_STATUS_CHANGE) {
		return -2;
	}
	
	/* Transfer Eventを処理 */
	if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
		uint32_t completion_code = (event.status >> 24) & 0xFF;
		uint8_t event_slot_id = (event.control >> 24) & 0xFF;

		/* このスロットのイベントか確認 */
		if (event_slot_id != slot_id) {
			return -2;
		}

		/* 成功またはShort Packet */
		if (completion_code == 1 || completion_code == 13) {
			/* キーボードレポートをコピー */
			for (int i = 0; i < 8; i++) {
				report_buffer[i] = ((uint8_t *)hc->keyboard_buffer)[i];
			}

			/* 次のInterrupt IN Transferを送信 */
			xhci_submit_interrupt_in(hc, slot_id, hc->keyboard_endpoint,
			                         hc->keyboard_buffer, 8);

			return 0; /* データあり */
		} else {
#ifdef XHCI_DEBUG
			printk("xHCI: Transfer event with code %u\n", completion_code);
#endif
			/* エラーの場合も次のTransferを送信 */
			xhci_submit_interrupt_in(hc, slot_id, hc->keyboard_endpoint,
			                         hc->keyboard_buffer, 8);
			return -3;
		}
	}
	
	return -2; /* 関係ないイベント */
}

/**
 * @brief HID Boot Protocolを設定
 */
int xhci_set_boot_protocol(struct xhci_hc *hc, uint8_t slot_id, uint8_t interface) {
#ifdef XHCI_DEBUG
	printk("xHCI: Setting Boot Protocol for slot %u, interface %u\n", slot_id, interface);
#endif

	/* SET_PROTOCOL (Boot Protocol) */
	return xhci_control_transfer(hc, slot_id,
		0x21, /* Class, Interface, Host-to-Device */
		0x0B, /* SET_PROTOCOL */
		0,    /* 0 = Boot Protocol */
		interface,
		NULL, 0);
}

/**
 * @brief キーボードを設定（Configuration + Boot Protocol）
 */
int xhci_configure_keyboard(struct xhci_hc *hc, uint8_t slot_id) {
	if (!hc || slot_id == 0) {
		return -1;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Configuring keyboard on slot %u\n", slot_id);
#endif

	/* Device Descriptorを取得（簡易版：8バイトのみ） */
	uint8_t device_desc[18];
	if (xhci_get_descriptor(hc, slot_id, 0x01, 0, device_desc, 8) != 0) {
		printk("xHCI: Failed to get device descriptor\n");
		return -2;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Device descriptor received\n");
#endif

	/* Configuration Descriptorを取得（簡易版：9バイトのみ） */
	uint8_t config_desc[9];
	if (xhci_get_descriptor(hc, slot_id, 0x02, 0, config_desc, 9) != 0) {
		printk("xHCI: Failed to get config descriptor\n");
		return -3;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Config descriptor received, bConfigurationValue = %u\n", config_desc[5]);
#endif

	/* SET_CONFIGURATIONを実行 */
	uint8_t config_value = config_desc[5];
	if (xhci_set_configuration(hc, slot_id, config_value) != 0) {
		printk("xHCI: Failed to set configuration\n");
		return -4;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Configuration set to %u\n", config_value);
#endif

	/* Boot Protocolを設定（Interface 0を仮定） */
	if (xhci_set_boot_protocol(hc, slot_id, 0) != 0) {
		printk("xHCI: Failed to set boot protocol\n");
		return -5;
	}

#ifdef XHCI_DEBUG
	printk("xHCI: Boot Protocol set successfully\n");
#endif

	return 0;
}

/**
 * @brief Interrupt IN Transferを送信
 */
int xhci_submit_interrupt_in(struct xhci_hc *hc, uint8_t slot_id, uint8_t endpoint, void *buffer, uint16_t length) {
	if (!hc || slot_id == 0 || !buffer) {
		return -1;
	}

	/* EndpointをDCIに変換 (IN endpoint = endpoint*2 + 1) */
	uint8_t dci = endpoint * 2 + 1;
	if (dci >= 32) {
		return -2;
	}

	/* Transfer Ringを取得 */
	struct xhci_trb *transfer_ring = hc->transfer_rings[slot_id][dci];
	if (!transfer_ring) {
		/* Transfer Ringを新規作成 */
		transfer_ring = (struct xhci_trb *)xhci_alloc_aligned(
			TRANSFER_RING_SIZE * sizeof(struct xhci_trb), 16);
		
		if (!transfer_ring) {
			printk("xHCI: Failed to allocate Transfer Ring for EP%u\n", endpoint);
			return -3;
		}

		/* 初期化 */
		for (uint32_t i = 0; i < TRANSFER_RING_SIZE; i++) {
			transfer_ring[i].parameter = 0;
			transfer_ring[i].status = 0;
			transfer_ring[i].control = 0;
		}

		hc->transfer_rings[slot_id][dci] = transfer_ring;
		hc->transfer_ring_index[slot_id][dci] = 0;
		hc->transfer_ring_cycle[slot_id][dci] = 1;

		/* TODO: Configure Endpoint Commandを実行 */
	}

	uint32_t tr_index = hc->transfer_ring_index[slot_id][dci];
	uint32_t cycle = hc->transfer_ring_cycle[slot_id][dci];

	/* Normal TRB */
	struct xhci_trb *trb = &transfer_ring[tr_index];
	trb->parameter = (uint64_t)(uintptr_t)buffer;
	trb->status = length | (0 << 22); /* TD Size = 0 */
	trb->control = (TRB_TYPE_NORMAL << 10) | TRB_IOC | TRB_ISP | (cycle ? TRB_CYCLE : 0);

	tr_index++;
	if (tr_index >= TRANSFER_RING_SIZE - 1) {
		tr_index = 0;
		cycle ^= 1;
	}

	hc->transfer_ring_index[slot_id][dci] = tr_index;
	hc->transfer_ring_cycle[slot_id][dci] = cycle;

	/* Doorbellを鳴らす */
	xhci_ring_doorbell(hc, slot_id, dci);

	return 0;
}
