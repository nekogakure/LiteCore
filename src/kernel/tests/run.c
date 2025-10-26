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
        #ifdef INTERRUPT_VECTOR_TEST
        printk("> INTERRUPT VECTOR TEST\n");
        interrupt_vector_test();
        #endif
}