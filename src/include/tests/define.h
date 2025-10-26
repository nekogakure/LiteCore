#ifndef _TESTS_CONFIG_H
#define _TESTS_CONFIG_H

// テストを実行するかどうか
//#define TEST_TRUE

#ifdef TEST_TRUE
// メモリマップとメモリのテスト
#define MEM_TEST
// 割り込みのテスト
#define INTERRUPT_TEST
// 割り込みベクタのテスト
#define INTERRUPT_VECTOR_TEST
// 割り込みハンドラでメモリのテスト
#define ALLOC_IRQ_TEST
// GDT（セグメント）再構築テスト
#define GDT_TEST

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

#endif /* TEST_TRUE */

#endif /* _TESTS_CONFIG_H */