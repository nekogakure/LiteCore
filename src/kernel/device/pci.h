#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>

uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t func,
			       uint8_t offset);
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t func,
			    uint8_t offset, uint32_t value);
void pci_enumerate(void);

#endif /* _PCI_H */
