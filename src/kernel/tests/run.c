#include <console.h>
#include <config.h>
#include <tests/define.h>

void run_test() {
        #ifdef MEM_TEST
        mem_test();
        #endif
}