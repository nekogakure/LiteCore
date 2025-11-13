この章では、割り込みハンドラ管理について記述します。
実装されているファイルは[interrupt.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/interrupt/interrupt.c)です。

## 概要
割り込みハンドラ管理モジュールは、IRQハンドラの登録、FIFOキューによるイベント管理、非同期イベント処理を提供します。複数のハンドラを1つのIRQに登録でき、イベントをキューイングして後で処理できます。

## 関数 / API

#### `int interrupt_register(uint32_t irq, irq_handler_t handler, void *ctx)`
IRQ番号に対してハンドラを登録します。

引数:
  - irq(uint32_t): IRQ番号（0-15）
  - handler(irq_handler_t): ハンドラ関数
  - ctx(void*): ハンドラに渡すコンテキストポインタ

戻り値: 0=成功、-1=範囲外またはハンドラ数上限

#### `int interrupt_unregister(uint32_t irq)`
IRQハンドラを登録解除します。

引数:
  - irq(uint32_t): IRQ番号

戻り値: 0=成功、-1=エラー

#### `int interrupt_raise(uint32_t event)`
イベントをFIFOキューにプッシュします。ISRから呼び出されます。

引数:
  - event(uint32_t): イベントデータ

戻り値: 1=成功、0=FIFOが満杯

#### `int interrupt_dispatch_one(void)`
FIFOから1つのイベントをポップし、対応するハンドラを呼び出します。

引数: なし

戻り値: 処理したイベント数（0または1）

#### `void interrupt_dispatch_all(void)`
FIFOが空になるまで全てのイベントを処理します。

引数: なし

#### `void interrupt_init(void)`
割り込みサブシステムを初期化します。FIFOとハンドラテーブルをリセットします。

引数: なし

## 定数 / 定義

- `MAX_IRQS`: 16 - 最大IRQ数
- `MAX_HANDLERS_PER_IRQ`: 4 - 1つのIRQに登録できる最大ハンドラ数
- `FIFO_CAPACITY`: 64 - FIFOキューの容量

## 構造体

#### `typedef void (*irq_handler_t)(uint32_t irq, void *ctx)`
IRQハンドラの関数ポインタ型です。

引数:
  - irq(uint32_t): IRQ番号またはイベントデータ
  - ctx(void*): コンテキストポインタ

#### `struct fifo_t`（内部定義）
FIFOキューの構造体です。

- `buf[FIFO_CAPACITY](uint32_t)`: イベントバッファ
- `head(uint32_t)`: 読み取り位置
- `tail(uint32_t)`: 書き込み位置
- `count(uint32_t)`: キューに格納されているイベント数

### 割り込み処理の流れ

1. **ハードウェア割り込み発生**: デバイスがIRQを発行
2. **ISR実行**: 割り込みサービスルーチンが即座に実行
3. **イベントエンキュー**: `interrupt_raise()`でイベントをFIFOに追加
4. **メインループ**: `interrupt_dispatch_all()`でイベントを処理
5. **ハンドラ呼び出し**: 登録されたハンドラ関数を順番に呼び出し

### マルチハンドラサポート

1つのIRQに最大4つのハンドラを登録できます。全てのハンドラが順番に呼び出されます。

### スレッドセーフ

`irq_save()`と`irq_restore()`を使用してクリティカルセクションを保護します。
