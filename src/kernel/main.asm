[ORG 0x1000]                    ; kernel start address
[BITS 32]                       ; 32ビットプロテクトモード

; プログラムの開始点
start:
    ; 画面クリア
    mov edi, 0xB8000            ; ビデオメモリの開始アドレス
    mov ax, 0x0720              ; al: スペース（0x20）、ah: 属性（0x07: 白文字/黒背景）
    mov ecx, 2000               ; カウンタレジスタを設定（80x25文字）
    rep stosw                   ; AXをECX回EDIに書き込み、EDIをインクリメント

    ; 文字列表示
    mov edi, 0xB8000            ; ビデオメモリの開始アドレス
    mov esi, msg                ; msgアドレスを設定
    call print_string           ; 文字列表示を呼び出し

    jmp halt                    ; システム停止

; 文字列表示
print_string:
    mov ah, 0x07                ; 白文字黒背景
.loop:
    lodsb                       ; SI -> AL -> SI++
    cmp al, 0                   ; NULL終端をチェック
    je .done                    ; NULL終端だったら終了
    mov [edi], ax               ; ビデオメモリに書き込み
    add edi, 2                  ; Next移動
    jmp .loop                   ; 次の文字へ
.done:
    ret                         ; 呼び出し元に戻る

; システム停止
halt:
    cli
    hlt
    jmp halt

; DATA
msg db 'LiteCore kernel Loaded!', 0

times 512-($-$$) db 0