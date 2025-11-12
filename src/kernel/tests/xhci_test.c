#include <stddef.h>
#include <util/console.h>
#include <driver/usb/xhci.h>
#include <device/pci.h>
#include <mem/manager.h>

/**
 * @brief xHCI USBホストコントローラのテスト
 */
void xhci_test() {
	/* PCI設定空間をスキャンしてxHCIデバイスを探す */
	printk("xhci_test: Scanning PCI bus for xHCI controllers...\n");

	int found = 0;
	for (int bus = 0; bus < 8;
	     bus++) { // バス0-7をスキャン（テスト用に範囲を狭める）
		for (int dev = 0; dev < 32; dev++) {
			for (int func = 0; func < 8; func++) {
				uint32_t vendor_id = pci_read_config_dword(
					bus, dev, func, 0x00);
				if ((vendor_id & 0xFFFF) == 0xFFFF) {
					continue; // デバイスなし
				}

				/* クラスコードを取得 */
				uint32_t class_reg = pci_read_config_dword(
					bus, dev, func, 0x08);
				uint32_t class_code = (class_reg >> 8) &
						      0xFFFFFF;
				uint8_t prog_if = (class_reg >> 8) & 0xFF;

				/* USB xHCIコントローラを検出（Class 0x0C03, PI 0x30） */
				if (class_code == 0x0C0330) {
					printk("  Found xHCI at Bus %d, Device %d, Function %d\n",
					       bus, dev, func);
					printk("  Vendor:Device = 0x%x:0x%x\n",
					       vendor_id & 0xFFFF,
					       (vendor_id >> 16) & 0xFFFF);
					printk("  Class Code = 0x%x, PI = 0x%x\n",
					       class_code >> 8, prog_if);

					/* BAR0を読み取る（MMIOベースアドレス） */
					uint32_t bar0 = pci_read_config_dword(
						bus, dev, func, 0x10);
					printk("  BAR0 = 0x%x\n", bar0);

					if ((bar0 & 0x1) == 0) { // Memory Space
						uintptr_t base_addr;

						/* 64-bit BARかチェック（bits 2:1 = 10b） */
						if ((bar0 & 0x6) == 0x4) {
							/* 64-bit BAR: BAR0とBAR1を合わせて読む */
							uint32_t bar1 =
								pci_read_config_dword(
									bus,
									dev,
									func,
									0x14);
							uint64_t bar64 =
								((uint64_t)bar1
								 << 32) |
								(bar0 &
								 0xFFFFFFF0);
							base_addr = (uintptr_t)
								bar64;
							printk("  64-bit BAR: BAR1 = 0x%x, Address = 0x%08x%08x\n",
							       bar1,
							       (uint32_t)(base_addr >>
									  32),
							       (uint32_t)(base_addr &
									  0xFFFFFFFF));
						} else {
							/* 32-bit BAR */
							base_addr = bar0 &
								    0xFFFFFFF0;
							printk("  32-bit BAR: Address = 0x%x\n",
							       (uint32_t)
								       base_addr);
						}

						printk("  MMIO Base Address = 0x%08x%08x\n",
						       (uint32_t)(base_addr >>
								  32),
						       (uint32_t)(base_addr &
								  0xFFFFFFFF)); /* Capabilityレジスタを読み取る */
						volatile struct xhci_cap_regs
							*cap_regs =
								(volatile struct xhci_cap_regs
									 *)
									base_addr;

						uint8_t caplength =
							cap_regs->caplength;
						uint16_t hciversion =
							cap_regs->hciversion;
						uint32_t hcsparams1 =
							cap_regs->hcsparams1;

						printk("  Capability Length = 0x%x\n",
						       caplength);
						printk("  HCI Version = 0x%x\n",
						       hciversion);
						printk("  Max Device Slots = %u\n",
						       hcsparams1 & 0xFF);
						printk("  Max Interrupters = %u\n",
						       (hcsparams1 >> 8) &
							       0x7FF);
						printk("  Max Ports = %u\n",
						       (hcsparams1 >> 24) &
							       0xFF);

						/* Operationalレジスタを読み取る */
						volatile struct xhci_op_regs *op_regs =
							(volatile struct xhci_op_regs
								 *)(base_addr +
								    caplength);

						uint32_t usbsts =
							op_regs->usbsts;
						printk("  USB Status Register (USBSTS) = 0x%x\n",
						       usbsts);
						printk("    HCHalted = %u\n",
						       (usbsts & (1 << 0)) ? 1 :
									     0);
						printk("    Controller Not Ready = %u\n",
						       (usbsts & (1 << 11)) ?
							       1 :
							       0);

						found = 1;
					}
					printk("\n");
				}
			}
		}
	}

	if (!found) {
		printk("xhci_test: No xHCI controller found\n");
		printk("Note: Make sure QEMU is started with USB 3.0 support:\n");
		printk("      -device qemu-xhci,id=xhci\n");
	} else {
		printk("xhci_test: xHCI detection successful!\n");
	}
}
