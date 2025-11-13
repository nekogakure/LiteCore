この章では、IDTロード処理について記述します。
実装されているファイルは[load_idt.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/interrupt/load_idt.c)です。

## 概要
IDTロードモジュールは、IDT（Interrupt Descriptor Table）をCPUにロードするための低レベル関数を提供します。`lidt`命令のラッパーとして機能します。

## 関数 / API

#### `void load_idt(void *ptr, unsigned size)`
IDTポインタをCPUにロードします。`lidt`命令を実行してIDTレジスタを更新します。

引数:
  - ptr(void*): IDTポインタ構造体へのポインタ
  - size(unsigned): サイズパラメータ（未使用）

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

このファイルには構造体の定義はありません。

### lidt命令

x86の`lidt`命令は、IDTレジスタに新しいIDTの場所とサイズをロードします。IDTレジスタは以下の情報を保持します：

- **リミット**: IDTのサイズ-1（16ビット）
- **ベースアドレス**: IDTの先頭アドレス（32ビット）

### 使用方法

```c
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_ptr idtp;
idtp.limit = sizeof(idt) - 1;
idtp.base = (uint32_t)&idt;

load_idt(&idtp, 0);
```

この関数はIDT初期化時に一度だけ呼び出されます。
