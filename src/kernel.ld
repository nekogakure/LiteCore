ENTRY(kernel_entry)
OUTPUT_ARCH(i386)

SECTIONS {
        . = 0x10000;           /* カーネルのアドレス */

        .text : {
                KEEP(*(.text.kernel_entry))
                *(.text)
        }

        .rodata ALIGN(4) : {    /* 読み取り専用データ */
                *(.rodata)      /* 定数 */
        }

        .data ALIGN(4) : {      /* 初期化住みのデータ */
                *(.data)        /* グローバル変数 */
        }

        .bss ALIGN(4) : {       /* 未初期化データ */
                *(.bss)         /* 未初期化の変数 */
        }

        __end = .;              /* カーネルの終了アドレス */
}