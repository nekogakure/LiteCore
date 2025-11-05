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

        ; BIOS拡張読み込み(LBA)のサポートをチェック
        mov ah, 0x41
        mov bx, 0x55AA
        mov dl, [boot_drive]
        int 0x13
        jc .use_chs             ; 拡張機能非対応ならCHSモードへ
        cmp bx, 0xAA55
        jne .use_chs

        ; LBAモードで読み込み
        mov si, dap             ; Disk Address Packetのアドレス
        mov ah, 0x42
        mov dl, [boot_drive]
        int 0x13
        jc disk_error
        jmp .boot_continue

.use_chs:
        mov bx, KERNEL_OFFSET>>4
        mov es, bx
        xor bx, bx
        mov ah, 0x02
        mov al, SECTOR_COUNT    ; 63以下を想定
        mov ch, CYLINDER_NUM
        mov cl, START_SECTOR
        mov dh, HEAD_NUM
        mov dl, [boot_drive]
        int 0x13
        jc disk_error

.boot_continue:

        ; プロテクトモードへの移行準備
        cli                     ; 割り込みを無効にする
        mov ax, 0x2401          ; A20ラインとやらを有効にする
        int 0x15                ; BIOS割り込みでA20ラインを有効化
        jc boot_error           ; エラーのフラグが立っていたらエラー処理へ移行
        lgdt [gdt_descriptor]   ; GDTをロード
        mov eax, cr0            ; 保護モードの準備
        or eax, 1               ; 保護モードのビット設定
        mov cr0, eax            ; 保護モードの有効化

        jmp CODE_SEGMENT:pm_start       ; 保護モードへジャンプ

disk_error:
        mov si, disk_err_msg
        call print_string
        jmp halt

boot_error: 
        mov si, boot_err_msg    ; エラーメッセージのアドレス
        call print_string       ; 文字列表示を呼び出し
        jmp halt                ; 止める

%include "src/boot/utils.inc"

; 32-bit mode
[BITS 32]
pm_start:
        mov ax, DATA_SEGMENT    ; データセグメントの設定
        mov ds, ax
        mov es, ax
        mov ss, ax
        mov esp, 0x90000        ; スタックポインタを再度設定

        jmp KERNEL_OFFSET       ; カーネルにジャンプ

; GDT define（何も考えるな、感じろ精神）
gdt_start:
        ; Nullディスクリプタ
        dd 0x00000000
        dd 0x00000000

        ; code seg (0x08)
        dw 0xFFFF               ; セグメントリミット（下）
        dw 0x0000               ; ベースアドレス（下）
        db 0x00                 ; ベースアドレス（中）
        db 0x9A                 ; アクセスバイト（実行、読み取り、コード可能）
        db 0xCF                 ; フラグとリミット（上）
        db 0x00                 ; ベースアドレス（上）

        ; data seg (0x10)
        dw 0xFFFF               ; セグメントリミット（下）
        dw 0x0000               ; ベースアドレス（下）
        db 0x00                 ; ベースアドレス（中）
        db 0x92                 ; アクセスバイト（データ、書き込み可能）
        db 0xCF                 ; フラグとリミット（上）
        db 0x00                 ; ベースアドレス（上）
gdt_end:

gdt_descriptor:
        dw gdt_end - gdt_start - 1      ; GDTのサイズ
        dd gdt_start                    ; GDTの開始アドレス


; DATA
boot_drive      db 0            ; ブートドライブ番号
boot_err_msg    db "LBoot: Boot failed! (A20 enable failed) :("

; Disk Address Packet (DAP) for LBA mode
align 4
dap:
        db 0x10                 ; DAP size (16 bytes)
        db 0                    ; Reserved
        dw SECTOR_COUNT         ; Number of sectors to read
        dw KERNEL_OFFSET        ; Offset
        dw 0                    ; Segment (0x0000 = absolute address)
        dd 1                    ; LBA start (sector 1 = after boot sector)
        dd 0                    ; Upper 32 bits of LBA (0 for <2TB disks)

times 510-($-$$) db 0
dw 0xAA55