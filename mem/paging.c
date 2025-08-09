#include "paging.h"
#include "memory.h"
#include "../kernel/util/vga.h"
#include "../kernel/kernel.h"

static page_table_t* pml4_table = NULL;

#define VMEM_START      0xFFFF800000000000
#define VMEM_END        0xFFFF900000000000

#define MAX_PHYS_MEMORY 0x10000000
#define MAX_FRAMES      (MAX_PHYS_MEMORY / PAGE_SIZE)
static uint32_t page_frames[MAX_FRAMES / 32];

static void set_frame(uint64_t frame_addr) {
    uint64_t frame = frame_addr / PAGE_SIZE;
    page_frames[frame / 32] |= (1 << (frame % 32));
}

static void clear_frame(uint64_t frame_addr) {
    uint64_t frame = frame_addr / PAGE_SIZE;
    page_frames[frame / 32] &= ~(1 << (frame % 32));
}

static int test_frame(uint64_t frame_addr) {
    uint64_t frame = frame_addr / PAGE_SIZE;
    return (page_frames[frame / 32] & (1 << (frame % 32))) != 0;
}

static uint64_t find_free_frame() {
    for (uint64_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (page_frames[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(page_frames[i] & (1 << j))) {
                    return (i * 32 + j) * PAGE_SIZE;
                }
            }
        }
    }
    printk("ERROR: No free page frames available!\n");
    return 0;
}

static void set_page_directory(uint64_t phys_addr) {
    __asm__ volatile("movq %0, %%cr3" : : "r" (phys_addr) : "memory");
}

static uint64_t get_page_directory() {
    uint64_t cr3;
    __asm__ volatile("movq %%cr3, %0" : "=r" (cr3) : : "memory");
    return cr3;
}

void flush_tlb(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r" (virt) : "memory");
}

void flush_tlb_all() {
    uint64_t cr3 = get_page_directory();
    set_page_directory(cr3);
}

uint64_t virt_to_phys(void* virt_addr) {
    uint64_t addr = (uint64_t)virt_addr;
    uint64_t pml4_index = (addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (addr >> 30) & 0x1FF;
    uint64_t pd_index = (addr >> 21) & 0x1FF;
    uint64_t pt_index = (addr >> 12) & 0x1FF;
    uint64_t offset = addr & 0xFFF;
    
    page_table_t* pml4 = (page_table_t*)KERNEL_PHYS_TO_VIRT(get_page_directory());
    if (!(pml4->entries[pml4_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_table_t* pdpt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pml4->entries[pml4_index] & ~0xFFF);
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_table_t* pd = (page_table_t*)KERNEL_PHYS_TO_VIRT(pdpt->entries[pdpt_index] & ~0xFFF);
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    if (pd->entries[pd_index] & PAGE_HUGE) {
        return (pd->entries[pd_index] & ~0x1FFFFF) + (addr & 0x1FFFFF);
    }
    
    page_table_t* pt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pd->entries[pd_index] & ~0xFFF);
    if (!(pt->entries[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (pt->entries[pt_index] & ~0xFFF) + offset;
}

void map_page(uint64_t phys, uint64_t virt, uint64_t flags) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index = (virt >> 21) & 0x1FF;
    uint64_t pt_index = (virt >> 12) & 0x1FF;
    
    if (!(pml4_table->entries[pml4_index] & PAGE_PRESENT)) {
        uint64_t pdpt_phys = find_free_frame();
        if (!pdpt_phys) {
            printk("ERROR: Failed to allocate PDPT\n");
            return;
        }
        
        set_frame(pdpt_phys);
        page_table_t* pdpt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pdpt_phys);
        
        for (int i = 0; i < PDPT_ENTRIES; i++) {
            pdpt->entries[i] = 0;
        }
        
        pml4_table->entries[pml4_index] = pdpt_phys | PAGE_PRESENT | PAGE_WRITE | flags;
    }
    
    page_table_t* pdpt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pml4_table->entries[pml4_index] & ~0xFFF);
    
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        uint64_t pd_phys = find_free_frame();
        if (!pd_phys) {
            printk("ERROR: Failed to allocate PD\n");
            return;
        }
        
        set_frame(pd_phys);
        page_table_t* pd = (page_table_t*)KERNEL_PHYS_TO_VIRT(pd_phys);
        
        for (int i = 0; i < PD_ENTRIES; i++) {
            pd->entries[i] = 0;
        }
        
        pdpt->entries[pdpt_index] = pd_phys | PAGE_PRESENT | PAGE_WRITE | flags;
    }
    
    page_table_t* pd = (page_table_t*)KERNEL_PHYS_TO_VIRT(pdpt->entries[pdpt_index] & ~0xFFF);
    
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        uint64_t pt_phys = find_free_frame();
        if (!pt_phys) {
            printk("ERROR: Failed to allocate PT\n");
            return;
        }
        
        set_frame(pt_phys);
        page_table_t* pt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pt_phys);
        
        for (int i = 0; i < PT_ENTRIES; i++) {
            pt->entries[i] = 0;
        }
        
        pd->entries[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITE | flags;
    }
    
    page_table_t* pt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pd->entries[pd_index] & ~0xFFF);
    pt->entries[pt_index] = phys | PAGE_PRESENT | flags;
    
    flush_tlb(virt);
}

void unmap_page(uint64_t virt) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index = (virt >> 21) & 0x1FF;
    uint64_t pt_index = (virt >> 12) & 0x1FF;
    
    if (!(pml4_table->entries[pml4_index] & PAGE_PRESENT)) {
        return;
    }
    
    page_table_t* pdpt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pml4_table->entries[pml4_index] & ~0xFFF);
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        return;
    }
    
    page_table_t* pd = (page_table_t*)KERNEL_PHYS_TO_VIRT(pdpt->entries[pdpt_index] & ~0xFFF);
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        return;
    }
    
    if (pd->entries[pd_index] & PAGE_HUGE) {
        uint64_t phys = pd->entries[pd_index] & ~0x1FFFFF;
        clear_frame(phys);
        pd->entries[pd_index] = 0;
        flush_tlb(virt);
        return;
    }
    
    page_table_t* pt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pd->entries[pd_index] & ~0xFFF);
    if (!(pt->entries[pt_index] & PAGE_PRESENT)) {
        return;
    }
    
    uint64_t phys = pt->entries[pt_index] & ~0xFFF;
    clear_frame(phys);
    pt->entries[pt_index] = 0;
    
    int pt_empty = 1;
    for (int i = 0; i < PT_ENTRIES; i++) {
        if (pt->entries[i] & PAGE_PRESENT) {
            pt_empty = 0;
            break;
        }
    }
    
    if (pt_empty) {
        uint64_t pt_phys = pd->entries[pd_index] & ~0xFFF;
        clear_frame(pt_phys);
        pd->entries[pd_index] = 0;
        
        int pd_empty = 1;
        for (int i = 0; i < PD_ENTRIES; i++) {
            if (pd->entries[i] & PAGE_PRESENT) {
                pd_empty = 0;
                break;
            }
        }
        
        if (pd_empty) {
            uint64_t pd_phys = pdpt->entries[pdpt_index] & ~0xFFF;
            clear_frame(pd_phys);
            pdpt->entries[pdpt_index] = 0;
            
            int pdpt_empty = 1;
            for (int i = 0; i < PDPT_ENTRIES; i++) {
                if (pdpt->entries[i] & PAGE_PRESENT) {
                    pdpt_empty = 0;
                    break;
                }
            }
            
            if (pdpt_empty) {
                uint64_t pdpt_phys = pml4_table->entries[pml4_index] & ~0xFFF;
                clear_frame(pdpt_phys);
                pml4_table->entries[pml4_index] = 0;
            }
        }
    }
    
    flush_tlb(virt);
}

int is_page_mapped(uint64_t virt) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index = (virt >> 21) & 0x1FF;
    uint64_t pt_index = (virt >> 12) & 0x1FF;
    
    if (!(pml4_table->entries[pml4_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_table_t* pdpt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pml4_table->entries[pml4_index] & ~0xFFF);
    if (!(pdpt->entries[pdpt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_table_t* pd = (page_table_t*)KERNEL_PHYS_TO_VIRT(pdpt->entries[pdpt_index] & ~0xFFF);
    if (!(pd->entries[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    if (pd->entries[pd_index] & PAGE_HUGE) {
        return 1;
    }
    
    page_table_t* pt = (page_table_t*)KERNEL_PHYS_TO_VIRT(pd->entries[pd_index] & ~0xFFF);
    return (pt->entries[pt_index] & PAGE_PRESENT) != 0;
}

void* valloc(size_t size) {
    if (size == 0) return NULL;
    
    size = PAGE_ALIGN_UP(size);
    uint64_t pages = size / PAGE_SIZE;
    
    for (uint64_t vaddr = VMEM_START; vaddr < VMEM_END; vaddr += PAGE_SIZE) {
        int region_free = 1;
        
        for (uint64_t i = 0; i < pages; i++) {
            if (is_page_mapped(vaddr + i * PAGE_SIZE)) {
                region_free = 0;
                vaddr += i * PAGE_SIZE;
                break;
            }
        }
        
        if (region_free) {
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t phys = find_free_frame();
                if (!phys) {
                    for (uint64_t j = 0; j < i; j++) {
                        unmap_page(vaddr + j * PAGE_SIZE);
                    }
                    return NULL;
                }
                
                set_frame(phys);
                map_page(phys, vaddr + i * PAGE_SIZE, PAGE_WRITE);
            }
            
            return (void*)vaddr;
        }
    }
    
    return NULL;
}

void vfree(void* ptr) {
    if (!ptr) return;
    
    uint64_t vaddr = (uint64_t)ptr;
    
    if (vaddr < VMEM_START || vaddr >= VMEM_END) {
        printk("ERROR: Invalid virtual address in vfree: 0x");
        printk_hex("%x", vaddr);
        printk("\n");
        return;
    }
    
    vaddr = PAGE_ALIGN(vaddr);
    
    while (vaddr < VMEM_END && is_page_mapped(vaddr)) {
        unmap_page(vaddr);
        vaddr += PAGE_SIZE;
    }
}

void* vmmap(uint64_t phys_addr, size_t size, uint64_t flags) {
    if (size == 0) return NULL;
    
    uint64_t phys_start = PAGE_ALIGN(phys_addr);
    uint64_t offset = phys_addr - phys_start;
    
    size = PAGE_ALIGN_UP(size + offset);
    uint64_t pages = size / PAGE_SIZE;
    
    for (uint64_t vaddr = VMEM_START; vaddr < VMEM_END; vaddr += PAGE_SIZE) {
        int region_free = 1;
        
        for (uint64_t i = 0; i < pages; i++) {
            if (is_page_mapped(vaddr + i * PAGE_SIZE)) {
                region_free = 0;
                vaddr += i * PAGE_SIZE;
                break;
            }
        }
        
        if (region_free) {
            for (uint64_t i = 0; i < pages; i++) {
                map_page(phys_start + i * PAGE_SIZE, vaddr + i * PAGE_SIZE, flags);
            }
            
            return (void*)(vaddr + offset);
        }
    }
    
    return NULL;
}

void vmunmap(void* virt_addr, size_t size) {
    if (!virt_addr || size == 0) return;
    
    uint64_t vaddr = PAGE_ALIGN((uint64_t)virt_addr);
    size = PAGE_ALIGN_UP(size + ((uint64_t)virt_addr - vaddr));
    uint64_t pages = size / PAGE_SIZE;
    
    for (uint64_t i = 0; i < pages; i++) {
        if (is_page_mapped(vaddr + i * PAGE_SIZE)) {
            unmap_page(vaddr + i * PAGE_SIZE);
        }
    }
}

void paging_init() {
    printk("Initializing paging system...\n");
    
    for (int i = 0; i < MAX_FRAMES / 32; i++) {
        page_frames[i] = 0;
    }
    
    for (uint64_t addr = 0; addr < 0x1000000; addr += PAGE_SIZE) {
        set_frame(addr);
    }
    
    uint64_t pml4_phys = find_free_frame();
    if (!pml4_phys) {
        printk("ERROR: Failed to allocate PML4\n");
        return;
    }
    
    set_frame(pml4_phys);
    pml4_table = (page_table_t*)KERNEL_PHYS_TO_VIRT(pml4_phys);
    
    for (int i = 0; i < PML4_ENTRIES; i++) {
        pml4_table->entries[i] = 0;
    }
    
    for (uint64_t addr = KERNEL_PHYS_BASE; addr < KERNEL_PHYS_BASE + 0x1000000; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_WRITE);
    }
    
    for (uint64_t addr = KERNEL_PHYS_BASE; addr < KERNEL_PHYS_BASE + 0x1000000; addr += PAGE_SIZE) {
        map_page(addr, KERNEL_PHYS_TO_VIRT(addr), PAGE_WRITE);
    }
    
    printk("Setting page directory to 0x");
    printk_hex("%x", pml4_phys);
    printk("\n");
    
    set_page_directory(pml4_phys);
    
    printk("Paging initialized successfully.\n");
    printk("Kernel virtual base: 0x");
    printk_hex("%x", KERNEL_BASE);
    printk("\n");
}

void paging_print_info() {
    uint64_t free_frames = 0;
    for (uint64_t i = 0; i < MAX_FRAMES; i++) {
        if (!test_frame(i * PAGE_SIZE)) {
            free_frames++;
        }
    }
    
    printk("\n==== Paging Information ====\n");
    printk("Page size: 4KB\n");
    printk("Total page frames: ");
    printk_int("%d", MAX_FRAMES);
    printk("\n");
    printk("Free page frames: ");
    printk_int("%d", free_frames);
    printk(" (");
    printk_int("%d", free_frames * 4);
    printk("KB)\n");
    printk("Used page frames: ");
    printk_int("%d", MAX_FRAMES - free_frames);
    printk(" (");
    printk_int("%d", (MAX_FRAMES - free_frames) * 4);
    printk("KB)\n");
    printk("Current page directory: 0x");
    printk_hex("%x", get_page_directory());
    printk("\n");
    printk("============================\n");
}
