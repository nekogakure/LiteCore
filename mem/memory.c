#include "memory.h"
#include "../kernel/util/vga.h"
#include "config.h"
#include "../kernel/kernel.h"

static memory_block_t* free_list = NULL;
static size_t total_memory = 0;
static size_t used_memory = 0;

void memory_init() {
    free_list = (memory_block_t*)HEAP_START;
    
    free_list->size = HEAP_SIZE - sizeof(memory_block_t);
    free_list->is_free = 1;
    free_list->next = NULL;
    
    total_memory = HEAP_SIZE - sizeof(memory_block_t);
    used_memory = 0;
    
    printk("Memory management initialized\n");
    printk("Heap start: 0x");
    printk_hex("%x", HEAP_START);
    printk(", Heap end: 0x");
    printk_hex("%x", HEAP_END);
    printk("\n");
    printk("Total available memory: ");
    printk_int("%d", total_memory);
    printk(" bytes\n");
}

static void split_block(memory_block_t* block, size_t size) {
    memory_block_t* new_block = (memory_block_t*)((uint8_t*)block + sizeof(memory_block_t) + size);
    new_block->size = block->size - size - sizeof(memory_block_t);
    new_block->is_free = 1;
    new_block->next = block->next;
    
    block->size = size;
    block->is_free = 0;
    block->next = new_block;
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = (size + 7) & ~7;
    
    memory_block_t* current = free_list;
    memory_block_t* previous = NULL;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size > size + sizeof(memory_block_t) + 8) {
                split_block(current, size);
            } else {
                current->is_free = 0;
            }
            
            used_memory += current->size;
            return (void*)((uint8_t*)current + sizeof(memory_block_t));
        }
        
        previous = current;
        current = current->next;
    }
    
    text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
    printk("ERROR: Out of memory! Failed to allocate ");
    printk_int("%d", size);
    printk(" bytes\n");
    text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
    return NULL;
}

static void merge_blocks() {
    memory_block_t* current = free_list;
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
            continue;
        }
        
        current = current->next;
    }
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    if ((uint64_t)block < HEAP_START || (uint64_t)block >= HEAP_END) {
        text_set_color(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
        printk("ERROR: Invalid free address: 0x");
        printk_hex("%x", (uint64_t)ptr);
        printk("\n");
        text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
        return;
    }
    
    if (block->is_free) {
        text_set_color(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK);
        printk("WARNING: Double free detected at 0x");
        printk_hex("%x", (uint64_t)ptr);
        printk("\n");
        text_set_color(TEXT_COLOR_LIGHT_GREY, TEXT_COLOR_BLACK);
        return;
    }
    
    block->is_free = 1;
    used_memory -= block->size;
    
    merge_blocks();
}

size_t get_free_memory() {
    return total_memory - used_memory;
}

void memory_print_stats() {
    size_t free_mem = get_free_memory();
    float usage_percentage = ((float)used_memory / total_memory) * 100;
    
    printk("\n==== Memory Statistics ====\n");
    printk("Total memory: ");
    printk_int("%d", total_memory);
    printk(" bytes\n");
    
    printk("Used memory:  ");
    printk_int("%d", used_memory);
    printk(" bytes (");
    printk_int("%d", (int)usage_percentage);
    printk("%%)\n");
    
    printk("Free memory:  ");
    printk_int("%d", free_mem);
    printk(" bytes\n");
    
    printk("\nMemory Map:\n");
    
    memory_block_t* current = free_list;
    int block_num = 0;
    
    while (current != NULL) {
        printk("[");
        printk_int("%d", block_num++);
        printk("] Address: 0x");
        printk_hex("%x", (uint64_t)current);
        printk(", Size: ");
        printk_int("%d", current->size);
        printk(" bytes, Status: ");
        printk(current->is_free ? "FREE" : "USED");
        printk("\n");
        
        current = current->next;
    }
    
    printk("=========================\n");
}
