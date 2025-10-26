#include <interrupt/irq.h>

/**
 * @fn irq_save
 * 割り込みフラグを保存し、割り込みを無効化します。
 * 
 * @return 保存されたフラグ
 */
uint32_t irq_save(void) {
    uint32_t flags;
    asm volatile(
        "pushf\n\t"
        "pop %0\n\t"
        : "=r" (flags)
        :
        : "memory"
    );
    asm volatile("cli" ::: "memory");
    return flags;
}

/**
 * @brief 割り込みフラグを以前の状態に復元します。
 *
 * この関数は、指定された flags パラメータの値で CPU の割り込みフラグ（EFLAGS レジスタ）を復元します。
 * 主に割り込みを無効化した後などに、元の割り込み状態を再設定するために使用します。
 *
 * @param flags 復元する EFLAGS レジスタの値。
 *
 * @note この関数はインラインアセンブリを使用してプロセッサのフラグを操作します。
 *       誤った使用はシステムの安定性に影響を与える可能性があるため、注意して使用してください。
 */

void irq_restore(uint32_t flags) {
    asm volatile(
        "push %0\n\t"
        "popf\n\t"
        :
        : "r" (flags)
        : "memory"
    );
}
