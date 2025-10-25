[ORG 0x7C00]                    ; Boot sector start address
[BITS 16]

; define consts
%include "src/boot/config.inc"

; entry
start:
        mov ax, 0               ; セグメントレジスタの初期化
        mov ds, ax              ; データセグメント？の設定（なお理解していない）
        mov es, ax              ; エクストラセグメント？の設定（なお理解していない）
        mov ss, ax              ; スタックセグメント？の設定（当たり前のように理解していない）
        mov sp, 0x7C00          ; スタックポインタの設定

        mov [boot_drive], dl    ; ブートドライブ番号の保存

        ; ディスクからカーネルをKERNEL_OFFSETに読みこむ
        mov ah, 0x02            ; BIOSのディスクの読み込み機能を選択

        ; 定数をすべて読み込む
        mov al, SECTOR_COUNT
        mov bx, KERNEL_OFFSET
        mov ch, CYLINDER_NUM
        mov cl, START_SECTOR
        mov dh, HEAD_NUM
        mov dl, [boot_drive]

        call read_disk          ; ディスクから読み込み

        ; カーネル起動準備
        cli                     ; 割り込みを無効にする
        mov ax, 0x2401          ; A20ラインとやらを有効にする
        int 0x15                ; BIOS割り込みでA20ラインを有効化
        sti                     ; 割り込みを再度有効化
        jc boot_error           ; エラーのフラグが立っていたらエラー処理へ移行

        jmp KERNEL_OFFSET       ; カーネルへジャンプ

boot_error: 
        mov si, boot_err_msg     ; エラーメッセージのアドレス
        call print_string       ; 文字列表示を呼び出し
        jmp halt                ; 止める

%include "src/boot/utils.inc"

; DATA
boot_drive      db 0            ; ブートドライブ番号
boot_err_msg    db "Boot failed! (A20 enable failed) :("

times 510-($-$$) db 0
dw 0xAA55