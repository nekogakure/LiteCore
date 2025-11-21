#ifndef _TESTS_CONFIG_H
#define _TESTS_CONFIG_H

// テストを実行するかどうか
#define TEST_TRUE

//#ifdef TEST_TRUE
// メモリマップとメモリのテスト
//#define MEM_TEST
// 割り込みのテスト
//#define INTERRUPT_TEST
// 割り込みベクタのテスト
//#define INTERRUPT_VECTOR_TEST
// 割り込みハンドラでメモリのテスト
//#define ALLOC_IRQ_TEST
// GDT（セグメント）再構築テスト
//#define GDT_TEST
// ページングのテスト
//#define PAGING_TEST
// 仮想メモリのテスト
//#define VMEM_TEST
// FAT12 test
//#define FAT12_TEST
// ext2 test
//#define EXT2_TEST
// ACPI Timer test
// #define ACPI_TIMER_TEST
// APIC Timer test
// #define APIC_TIMER_TEST
// Multi Task test
#define MULTI_TASK_TEST

/* -------- 宣言 -------- */
#ifdef MEM_TEST
void mem_test();
#endif /* MEM_TEST */

#ifdef INTERRUPT_TEST
void interrupt_test();
#endif /* INTERRUPT_TEST */

#ifdef INTERRUPT_VECTOR_TEST
void interrupt_vector_test();
#endif /* INTERRUPT_VECTOR_TEST */

#ifdef ALLOC_IRQ_TEST
void alloc_irq_test();
#endif /* ALLOC_IRQ_TEST */

#ifdef GDT_TEST
void gdt_test();
#endif /* GDT_TEST */

#ifdef PAGING_TEST
void paging_test();
#endif /* PAGING_TEST */

#ifdef VMEM_TEST
void vmem_test();
#endif /* VMEM_TEST */

#ifdef FAT12_TEST
void fat12_test();
#endif

#ifdef EXT2_TEST
void ext2_test();
#endif

#ifdef ACPI_TIMER_TEST
void acpi_timer_test();
#endif

#ifdef APIC_TIMER_TEST
void apic_timer_test();
#endif

#ifdef MULTI_TASK_TEST
void multi_task_test();
#endif

#endif /* TEST_TRUE */

#endif /* _TESTS_CONFIG_H */