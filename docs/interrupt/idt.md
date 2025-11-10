この章では、割り込み記述子テーブル（IDT）について記述します。
実装されているファイルは[idt.c](../../src/kernel/interrupt/idt.c), [idt.h](../../src/include/interrupt/idt.h)です。

## 概要
IDT（Interrupt Descriptor Table）モジュールは、x86の割り込みと例外の処理を初期化します。256個のIDTエントリを設定し、PIC（Programmable Interrupt Controller）を再マッピングして、割り込みベクタをカーネルが使用する範囲（32-255）に配置します。

## 関数 / API

#### `void idt_init(void)`
IDTを初期化します。256個のエントリを設定し、PICを再マッピングし、IDTをCPUにロードします。

引数: なし

## 定数 / 定義

- `PIC1_COMMAND`: 0x20 - マスターPICコマンドポート
- `PIC1_DATA`: 0x21 - マスターPICデータポート
- `PIC2_COMMAND`: 0xA0 - スレーブPICコマンドポート
- `PIC2_DATA`: 0xA1 - スレーブPICデータポート
- `IDT_ENTRIES`: 256 - IDTエントリ数

## 構造体

#### `struct idt_entry`（内部定義）
IDTエントリを表す構造体です（64ビット形式）。

- `base_lo(uint16_t)`: ハンドラアドレスの下位16ビット（bits 0-15）
- `sel(uint16_t)`: コードセグメントセレクタ（0x08=カーネルコード）
- `ist(uint8_t)`: Interrupt Stack Table（通常は0）
- `flags(uint8_t)`: フラグバイト（0x8E=割り込みゲート、DPL=0、Present）
- `base_mid(uint16_t)`: ハンドラアドレスの中位16ビット（bits 16-31）
- `base_hi(uint32_t)`: ハンドラアドレスの上位32ビット（bits 32-63）
- `reserved(uint32_t)`: 予約領域（0でなければならない）

#### `struct idt_ptr`（内部定義）
IDTポインタ構造体です。

- `limit(uint16_t)`: IDTのサイズ-1
- `base(uint64_t)`: IDTのベースアドレス（64ビット）

### PICの再マッピング

デフォルトでは、PICは割り込みをベクタ0-15にマッピングしますが、これはCPU例外（0-31）と競合します。そのため、以下のように再マッピングします：

- **マスターPIC**: IRQ 0-7をベクタ32-39にマッピング
- **スレーブPIC**: IRQ 8-15をベクタ40-47にマッピング

### 割り込みベクタの割り当て

- **0-31**: CPU例外（ディバイドエラー、ページフォルトなど）
- **32-47**: ハードウェア割り込み（PICから）
- **48-255**: ソフトウェア割り込み/予約
