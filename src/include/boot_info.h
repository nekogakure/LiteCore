#ifndef KERNEL_BOOT_INFO_H
#define KERNEL_BOOT_INFO_H

#include <stdint.h>

// EFIメモリタイプ
typedef enum {
	EfiReservedMemoryType,
	EfiLoaderCode,
	EfiLoaderData,
	EfiBootServicesCode,
	EfiBootServicesData,
	EfiRuntimeServicesCode,
	EfiRuntimeServicesData,
	EfiConventionalMemory,
	EfiUnusableMemory,
	EfiACPIReclaimMemory,
	EfiACPIMemoryNVS,
	EfiMemoryMappedIO,
	EfiMemoryMappedIOPortSpace,
	EfiPalCode,
	EfiPersistentMemory,
	EfiMaxMemoryType
} EFI_MEMORY_TYPE;

// EFIメモリディスクリプタ
typedef struct {
	uint32_t Type;
	uint64_t PhysicalStart;
	uint64_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

// ブートローダーからカーネルに渡す情報
typedef struct {
	EFI_MEMORY_DESCRIPTOR *MemoryMap;
	uint64_t MemoryMapSize;
	uint64_t MemoryMapDescriptorSize;
	uint32_t MemoryMapDescriptorVersion;
	uint64_t FramebufferBase;
	uint64_t FramebufferSize;
	uint32_t HorizontalResolution;
	uint32_t VerticalResolution;
	uint32_t PixelsPerScanLine;
} BOOT_INFO;

#endif // KERNEL_BOOT_INFO_H
