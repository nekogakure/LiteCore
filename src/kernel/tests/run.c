#include <util/console.h>
#include <config.h>
#include <tests/define.h>

void run_test() {
        #ifdef MEM_TEST
        printk("> MEMORY TEST\n");
        mem_test();
        new_line();
        #endif

        #ifdef INTERRUPT_TEST
        printk("> INTERRUPT TEST\n");
        interrupt_test();
        new_line();
        #endif

        #ifdef INTERRUPT_VECTOR_TEST
        printk("> INTERRUPT VECTOR TEST\n");
        interrupt_vector_test();
        new_line();
        #endif

        #ifdef ALLOC_IRQ_TEST
        printk("> ALOOC IRQ TEST\n");
        alloc_irq_test();
        new_line();
        #endif

        #ifdef GDT_TEST
        printk("> GDT TEST");
        gdt_test();
        new_line();
        #endif

        #ifdef PAGING_TEST
        printk("> PAGING TEST\n");
        paging_test();
        new_line();
        #endif

        #ifdef VMEM_TEST
        printk("> VMEM TEST\n");
        vmem_test();
        new_line();
        #endif

        #ifdef FAT12_TEST
        printk("> FAT12 TEST");
        fat12_test();
        new_line();
        #endif
}