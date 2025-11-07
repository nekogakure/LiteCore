この章では、入出力ユーティリティについて記述します。
実装されているファイルは[io.c](../../src/kernel/util/io.c), [io.h](../../src/include/util/io.h)です。

## 概要
入出力ユーティリティは、VGAテキストメモリへの低レベルアクセス、ポートI/O操作、CPU制御機能を提供します。画面クリア、直接メモリ書き込み、inb/outbなどのポートアクセス関数が含まれます。

## 関数 / API

#### `void _write(const char *string)`
文字列をVGAテキストメモリに直接書き込みます。カーソル位置の追跡は行いません。

引数:
  - string(const char*): 表示する文字列

#### `void clear_screen()`
VGA画面全体をスペースで埋め尽くしてクリアします。

引数: なし

#### `void cpu_halt(void)`
CPUを休止状態にします。次の割り込みまで待機します（STI; HLT命令）。

引数: なし

#### `void timer_handler(uint32_t payload, void *ctx)`
タイマー割り込みハンドラ（IRQ 0）。現在は何もしません。

引数:
  - payload(uint32_t): イベントペイロード（未使用）
  - ctx(void*): コンテキスト（未使用）

#### `uint8_t inb(uint16_t port)`
8ビット値をI/Oポートから読み取ります。

引数:
  - port(uint16_t): ポート番号

戻り値: 読み取った8ビット値

#### `void outb(uint16_t port, uint8_t value)`
8ビット値をI/Oポートに書き込みます。

引数:
  - port(uint16_t): ポート番号
  - value(uint8_t): 書き込む値

#### `uint16_t inw(uint16_t port)`
16ビット値をI/Oポートから読み取ります。

引数:
  - port(uint16_t): ポート番号

戻り値: 読み取った16ビット値

#### `void outw(uint16_t port, uint16_t value)`
16ビット値をI/Oポートに書き込みます。

引数:
  - port(uint16_t): ポート番号
  - value(uint16_t): 書き込む値

## 定数 / 定義

- `VIDEO_MEMORY`: 0xB8000 - VGAテキストメモリのベースアドレス
- `COLOR`: 0x07 - デフォルトの色属性（白文字、黒背景）

## 構造体

このファイルには構造体の定義はありません。

### ポートI/O

x86アーキテクチャでは、I/Oポートを使用してデバイスと通信します：
- **in命令**: ポートからデータを読み取る
- **out命令**: ポートにデータを書き込む

一般的なポート：
- 0x3F8-0x3FF: COM1（シリアルポート）
- 0x60: キーボードデータポート
- 0x64: キーボードステータス/コマンドポート
- 0x1F0-0x1F7: プライマリATAコントローラ
- 0x170-0x177: セカンダリATAコントローラ

### CPU休止

`cpu_halt()`は省電力のためにCPUを休止状態にします。割り込みが発生するまでCPUは停止し、電力を節約します。
