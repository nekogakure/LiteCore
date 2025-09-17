/**
 * @file util/gdt.h
 * @brief Global Descriptor Table definitions
 * @details
 * Provides definitions and initialization functions for the GDT for x86_64.
 * Used for segmentation settings.
 * @author nekogakure
 * @version 0.1
 * @date 2025-09-12
 */

#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

/** @defgroup gdt GDT Management
 *  @brief Global Descriptor Table management functions
 *  @{
 */

/**
 * @brief GDT entry structure
 */
struct gdt_entry {
    uint16_t limit_low;    /**< Segment limit 0-15 bits */
    uint16_t base_low;     /**< Base address 0-15 bits */
    uint8_t  base_middle;  /**< Base address 16-23 bits */
    uint8_t  access;       /**< Access rights */
    uint8_t  granularity;  /**< Granularity */
    uint8_t  base_high;    /**< Base address 24-31 bits */
} __attribute__((packed));

/**
 * @brief GDTR structure
 */
struct gdt_ptr {
    uint16_t limit;        /**< Size of GDT - 1 */
    uint32_t base;         /**< Base address of GDT */
} __attribute__((packed));

/**
 * @brief GDT segment access flag definitions
 */
enum gdt_access_flags {
    GDT_PRESENT     = 0x80, /**< Segment present flag */
    GDT_RING0       = 0x00, /**< Privilege level 0 (kernel) */
    GDT_RING3       = 0x60, /**< Privilege level 3 (user) */
    GDT_SYSTEM      = 0x10, /**< System segment */
    GDT_EXECUTABLE  = 0x08, /**< Executable segment */
    GDT_RW          = 0x02, /**< Read/Write enabled */
    GDT_ACCESSED    = 0x01  /**< Accessed flag */
};

/**
 * @brief GDT granularity flag definitions
 */
enum gdt_granularity_flags {
    GDT_GRANULARITY = 0x80, /**< 4KB block granularity */
    GDT_32BIT       = 0x40, /**< 32-bit segment */
    GDT_64BIT       = 0x20  /**< 64-bit segment (not used in 32-bit mode) */
};

/** @} */ /* end of gdt group */

#endif /* _GDT_H */
