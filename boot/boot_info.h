#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <Uefi.h>

typedef struct {
	EFI_MEMORY_DESCRIPTOR *MemoryMap;
	UINTN MemoryMapSize;
	UINTN MemoryMapDescriptorSize;
	UINT32 MemoryMapDescriptorVersion;
	EFI_PHYSICAL_ADDRESS FramebufferBase;
	UINTN FramebufferSize;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	UINT32 PixelsPerScanLine;
} BOOT_INFO;

#endif // BOOT_INFO_H