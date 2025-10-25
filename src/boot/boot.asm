[ORG 0x7C00]                    ; Boot sector start address
[BITS 16]

start: 
        mov ax, 0               ; セグメントレジスタの初期化
        mov ds, ax              ; データセグメント？の設定（なお理解していない）

        ; クリア
        mov ax, 0xB800          ; VGAseg
        mov es, ax              ; エクストラセグメント？の設定（理解していない）
        mov bx, 0x0000          ; オフセットの初期化
        mov ax, 0x0720          ; 0x20: スペース, 0x07: 白文字黒背景
        mov cx, 2000            ; レジスタの設定（80x25）

clean_loop:
        mov [es:bx], ax         ; ビデオメモリへの書き込み
        add bx, 2               ; Next位置へ
        loop clean_loop         ; 0になるまで

        ; 文字列の表示
        mov ax, 0xB800          ; VGAseg
        mov es, ax              ; エクストラセグメント？の設定（理解していない）
        mov bx, 0x0000          ; オフセットの初期化
        mov si, msg             ; msgアドレスの指定
        mov ah, 0x07            ; 白文字黒背景（0x07）

print_loop: 
        lodsb                   ; SIから1byteをALに読み込んでSI進める
        cmp al, 0               ; NULL終端チェック
        je halt                 ; exit
        mov [es:bx], ax         ; ビデオメモリに書き込み
        add bx, 2               ; 次の文字へ
        jmp print_loop          ; 〃

halt:
        cli                     ; pause
        hlt                     ; stop
        jmp halt                ; 再起動に備えてjmp

msg:
        db "Welcome to LiteCore kernel!", 0

times 510-($-$$) db 0
dw 0xAA55