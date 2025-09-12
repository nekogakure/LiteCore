/**
 * @file multiboot.h
 * @brief Multiboot header definitions
 * @details
 * Contains constants and structures related to the Multiboot specification
 * used by GRUB to boot the kernel.
 * @author nekogakure
 */

#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <stdint.h>

/** @brief Magic number for Multiboot verification */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/**
 * @brief Multiboot information structure
 * @details Structure containing boot information passed by GRUB
 */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
};

#endif /* _MULTIBOOT_H */
