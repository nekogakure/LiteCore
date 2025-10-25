[ORG 0x7C00]                    ; Boot sector start address
[BITS 16]

; define const
KERNEL_OFFSET EQU 0x1000        ; カーネルをロードするアドレス
SECTOR_COUNT  EQU 1             ; 読み込むセクタの数
START_SECTOR  EQU 2             ; 開始するセクタ番号
CYLINDER_NUM  EQU 0             ; シリンダ番号
HEAD_NUM      EQU 0             ; ヘッド番号

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

        int 0x13                ; BIOS割り込みでディスクを読み込む
        jc disk_error           ; エラーのフラグ（キャリーフラグ？というらしい）が立っていたらエラー処理へ移行

        ; カーネル起動準備
        cli                     ; 割り込みを無効にする
        mov ax, 0x2401          ; A20ラインとやらを有効にする
        int 0x15                ; BIOS割り込みでA20ラインを有効化
        sti                     ; 割り込みを再度有効化
        jc boot_error           ; エラーのフラグが立っていたらエラー処理へ移行

        jmp KERNEL_OFFSET       ; カーネルへジャンプ

; ディスク読み込み失敗時の処理
disk_error: 
        mov si, disk_err_msg    ; エラーメッセージのアドレス
        call print_string       ; 文字列表示を呼び出し
        jmp halt                ; 止める

; ブート（A20）失敗時の処理
boot_error: 
        mov si, boot_err_msg     ; エラーメッセージのアドレス
        call print_string       ; 文字列表示を呼び出し
        jmp halt                ; 止める

; 文字列表示
print_string:
        mov ah, 0x0E            ; テレタイプモード
.loop:
        lodsb                   ; SI 1byte -> AL -> SI++
        cmp al, 0               ; NULL終端チェック
        je .done                ; 終端なら終了
        int 0x10                ; BIOS割り込みで表示
        jmp .loop               ; 次の文字へ
.done:
        ret                     ; 呼び出しもとへ戻る

; システム停止
halt:
        cli
        hlt
        jmp halt

; DATA
boot_drive      db 0            ; ブートドライブ番号
disk_err_msg    db "Disk read error! :("
boot_err_msg    db "Boot failed! (A20 enable failed) :("

times 510-($-$$) db 0
dw 0xAA55