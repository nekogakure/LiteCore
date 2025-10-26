#ifndef _TESTS_CONFIG_H
#define _TESTS_CONFIG_H

// テストを実行するかどうか
#define TEST_TRUE

#ifdef TEST_TRUE
// メモリマップとメモリのテスト
#define MEM_TEST


/* -------- 宣言 -------- */
#ifdef MEM_TEST
void mem_test();
#endif /* MEM_TEST */

#endif /* TEST_TRUE */

#endif /* _TESTS_CONFIG_H */