/**
 * @file uefi.h
 * @brief UEFI関連の定義とブート情報構造体
 * @details UEFIブートローダーから受け取る情報の定義
 */

#ifndef LITECORE_BOOT_UEFI_H
#define LITECORE_BOOT_UEFI_H

#include <stdint.h>

/**
 * @brief EFI Memory Descriptor
 * UEFIから提供されるメモリマップエントリ
 */
typedef struct {
        uint32_t type;
        uint64_t physical_start;
        uint64_t virtual_start;
        uint64_t page_count;
        uint64_t attribute;
} efi_memory_descriptor_t;

/**
 * @brief Boot Information Structure
 * UEFIブートローダーからカーネルに渡される情報
 */
typedef struct {
        uint64_t memory_map_size;
        uint64_t memory_map_descriptor_size;
        efi_memory_descriptor_t* memory_map;
        uint64_t framebuffer_base;
        uint32_t framebuffer_width;
        uint32_t framebuffer_height;
        uint32_t framebuffer_pitch;
        uint32_t framebuffer_bpp;
} boot_info_t;

#endif /* LITECORE_BOOT_UEFI_H */