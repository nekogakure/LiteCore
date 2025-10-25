[ORG 0x1000]            ; カーネルをロードするアドレス
[BITS 16]               ; 16-bit real

; entry point
start: 
        mov ax, 0               ; セグメントレジスタの初期化
        mov ds, ax              ; データセグメント？の設定（なお理解していない）
        mov es, ax              ; エクストラセグメント？の設定（なお理解していない）

        ; 文字列表示
        mov si, msg             ; msgアドレス
        call print_string       ; 文字列表示関数の呼び出し

        jmp halt                ; System stop

; 文字列表示
print_string: 
        mov ah, 0x0E            ; BIOS（テレタイプ）
.loop:
        lodsb                   ; SIから1バイトをALに読み込んでSIを進める
        cmp al, 0               ; NULLチェック
        je .done                ; 終わり
        int 0x10                ; BIOS割り込みで文字表示
        jmp .loop               ; 次の文字へ
.done:
        ret                     ; 呼び出したところへ戻る

halt:
        cli
        hlt
        jmp halt

msg db 'Welcome to LiteCore Kernel!', 0

times 512-($-$$) db 0