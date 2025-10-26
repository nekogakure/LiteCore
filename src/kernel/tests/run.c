#include <console.h>
#include <config.h>
#include <tests/define.h>

void run_test() {
        #ifdef MEM_TEST
        printk("> MEMORY TEST\n");
        mem_test();
        #endif

        new_line();

        #ifdef INTERRUPT_TEST
        printk("> INTERRUPT TEST\n");
        interrupt_test();
        #endif

        new_line();

        #ifdef INTERRUPT_VECTOR_TEST
        printk("> INTERRUPT VECTOR TEST\n");
        interrupt_vector_test();
        #endif

        new_line();

        #ifdef ALLOC_IRQ_TEST
        printk("> ALOOC IRQ TEST\n");
        alloc_irq_test();
        #endif

        new_line();

        #ifdef GDT_TEST
        gdt_test();
        #endif
}