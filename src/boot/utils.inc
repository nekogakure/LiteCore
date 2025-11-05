; ディスク読み込み
read_disk:
        pusha                   ; レジスタ保存
        mov ah, 0x02           ; BIOSのディスク読み込み機能を選択
        int 0x13               ; BIOS割り込みでディスク読み込み
        jc .error              ; エラーのフラグが立っていたらエラー処理へ
        popa                   ; レジスタ復元
        ret                    ; 戻る

.error:
        mov si, disk_err_msg   ; エラーメッセージのアドレスを設定
        call print_string      ; 文字列表示呼び出し
        jmp halt               ; システム停止

; 文字列表示
print_string:
        pusha                  ; レジスタ保存
        mov ah, 0x0E          ; テレタイプモード
.loop:
        lodsb                  ; SI->AL->SI++
        cmp al, 0             ; NULL終端チェック
        je .done              ; 終端なら終了
        int 0x10              ; BIOS割り込みで文字を表示
        jmp .loop             ; 次の文字へ
.done:
        popa                   ; レジスタ復元
        ret                    ; 戻る

; システム停止
halt:
        cli
        hlt
        jmp halt

disk_err_msg: db "LBoot: Disk read error! :(", 0