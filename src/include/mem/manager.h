#ifndef _MEM_MANAGER_H
#define _MEM_MANAGER_H

#include <config.h>

/**
 * @enum mem_type
 * @brief メモリのタイプを表す
 */
typedef enum mem_type {
	MEM_TYPE_HEAP = 0, // カーネルヒープ
	MEM_TYPE_FRAME = 1 // フレーム（4KB）の割当
} mem_type_t;

void mem_init(uint32_t st, uint32_t end);
void *kmalloc(uint32_t size);
void kfree(void *ptr);
int mem_has_space(mem_type_t type, uint32_t size);
void memory_init();
void *stack_alloc(uint32_t size);

/**
 * @brief スタック領域を解放する
 * @param top stack_alloc が返したトップアドレス（高位アドレス）
 * @param size stack_alloc に渡したサイズ（同じ値を渡すこと）
 */
void stack_free(void *top, uint32_t size);

#endif /* _MEM_MANAGER_H */
