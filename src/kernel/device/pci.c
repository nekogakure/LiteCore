#include <config.h>
#include <console.h>
#include <device/pci.h>

#include <stdint.h>

/* I/O ポートアクセス */
static inline void outl(uint16_t port, uint32_t val) {
        __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
        uint32_t ret;
        __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
}

/**
 * @fn pci_cfg_addr
 * @brief PCI コンフィグ空間アクセス用のアドレスを作成します
 */
static inline uint32_t pci_cfg_addr(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
        return (uint32_t)((1U << 31) |
                          ((uint32_t)bus << 16) |
                          ((uint32_t)device << 11) |
                          ((uint32_t)func << 8) |
                          (offset & 0xFC));
}

/**
 * @fn pci_read_config_dword
 * @brief PCI コンフィグ空間から32bitを読み出します
 */
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
        uint32_t address = pci_cfg_addr(bus, device, func, offset);
        outl(0xCF8, address);
        return inl(0xCFC);
}

/**
 * @fn pci_write_config_dword
 * @brief PCI コンフィグ空間へ32bitを書き込みます
 */
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
        uint32_t address = pci_cfg_addr(bus, device, func, offset);
        outl(0xCF8, address);
        outl(0xCFC, value);
}

/**
 * @fn pci_enumerate
 * @brief システム上の PCI デバイスを列挙してprintkで出力
 *
 * （簡易実装として bus 0..255, device 0..31, func 0..7 を探索）
 */
void pci_enumerate(void) {
        printk("Scanning PCI bus...\n");

        for (uint16_t bus = 0; bus < 256; ++bus) {
                for (uint8_t device = 0; device < 32; ++device) {
                        for (uint8_t func = 0; func < 8; ++func) {
                                uint32_t data = pci_read_config_dword((uint8_t)bus, device, func, 0);
                                uint16_t vendor = (uint16_t)(data & 0xFFFF);
                                if (vendor == 0xFFFF) {
                                        continue;       // なんやそれしらんぞっていうデバイス用
                                }

                                uint16_t device_id = (uint16_t)((data >> 16) & 0xFFFF);
                                uint32_t class_rev = pci_read_config_dword((uint8_t)bus, device, func, 0x08);
                                uint8_t base_class = (class_rev >> 24) & 0xFF;
                                uint8_t sub_class  = (class_rev >> 16) & 0xFF;
                                uint8_t prog_if    = (class_rev >> 8)  & 0xFF;
                                uint8_t revision   = class_rev & 0xFF;

                                uint32_t header = pci_read_config_dword((uint8_t)bus, device, func, 0x0C);
                                uint8_t header_type = (header >> 16) & 0x7F;

                                printk("PCI device found: bus=%d dev=%d func=%d vendor=0x%04x device=0x%04x class=0x%02x subclass=0x%02x prog-if=0x%02x rev=0x%02x hdr=0x%02x\n",
                                       bus, device, func, vendor, device_id, base_class, sub_class, prog_if, revision, header_type);

                                // マルチファンクションデバイスでなければ funcループを抜ける
                                if (func == 0) {
                                        uint32_t hdr0 = pci_read_config_dword((uint8_t)bus, device, func, 0x0C);
                                        if (((hdr0 >> 16) & 0x80) == 0) {
                                                break; // single function device
                                        }
                                }
                        }
                }
        }

        printk("PCI scan complete\n");
}
