この章では、割り込み要求（IRQ）管理について記述します。
実装されているファイルは[irq.c](../../src/kernel/interrupt/irq.c), [irq.h](../../src/include/interrupt/irq.h), [interrupt.c](../../src/kernel/interrupt/interrupt.c)です。

## 概要
IRQ管理モジュールは、割り込みハンドラの登録、割り込みイベントのキューイング、割り込みフラグの保存/復元を提供します。FIFOキューを使用して割り込みイベントを非同期に処理し、割り込みコンテキストと通常コンテキストを分離します。

## 関数 / API

#### `uint32_t irq_save(void)`
EFLAGSレジスタを保存し、割り込みを無効化します。クリティカルセクションに入る前に使用します。

引数: なし

戻り値: 保存されたEFLAGS値

#### `void irq_restore(uint32_t flags)`
保存されたEFLAGSレジスタを復元します。クリティカルセクションを出る時に使用します。

引数:
  - flags(uint32_t): `irq_save()`で保存されたEFLAGS値

#### `int interrupt_register(uint32_t irq, void (*handler)(uint32_t, void *), void *ctx)`
割り込みハンドラを登録します。指定されたIRQ番号に対してハンドラ関数を設定します。

引数:
  - irq(uint32_t): IRQ番号
  - handler(void (*)(uint32_t, void*)): 割り込みハンドラ関数
  - ctx(void*): ハンドラに渡されるコンテキストデータ

戻り値: 0=成功、負値=エラー

#### `int interrupt_unregister(uint32_t irq)`
割り込みハンドラを登録解除します。

引数:
  - irq(uint32_t): IRQ番号

戻り値: 0=成功、負値=エラー

#### `int interrupt_raise(uint32_t event)`
割り込みイベントをFIFOキューにエンキューします。実際のハンドラ呼び出しは後で行われます。

引数:
  - event(uint32_t): イベントデータ（IRQ番号やカスタムペイロードを含む）

戻り値: 0=成功、負値=エラー（キューが満杯の場合）

#### `int interrupt_dispatch_one(void)`
FIFOキューから1つのイベントをデキューし、対応するハンドラを呼び出します。

引数: なし

戻り値: 処理したイベント数（0または1）

#### `void interrupt_dispatch_all(void)`
FIFOキューの全てのイベントを処理します。キューが空になるまでハンドラを呼び出します。

引数: なし

#### `void interrupt_init(void)`
割り込みサブシステムを初期化します。FIFOキューとハンドラテーブルを初期化します。

引数: なし

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

このファイルには公開構造体の定義はありません。

### 内部実装

割り込み処理は2段階で行われます：

1. **ISR（Interrupt Service Routine）**: 割り込みが発生した時に即座に呼び出される。最小限の処理を行い、イベントをFIFOにエンキューする
2. **イベントハンドラ**: 後で`interrupt_dispatch_*()`によって呼び出される。時間のかかる処理を実行できる

### FIFOキュー

- 割り込みイベントをバッファリングするリングバッファ
- オーバーフロー時はエラーを返す
- スピンロックで保護され、マルチコアやネストした割り込みから安全

### 使用例

```c
// ハンドラを登録
void my_handler(uint32_t event, void *ctx) {
    printk("IRQ received: %u\n", event);
}
interrupt_register(1, my_handler, NULL);

// ISRからイベントを発行
interrupt_raise(1);

// メインループでイベントを処理
while (1) {
    interrupt_dispatch_all();
}
```
