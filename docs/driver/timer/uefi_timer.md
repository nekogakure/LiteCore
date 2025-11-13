この章では、UEFI Timer（PIT）ドライバについて記述します。
実装されているファイルは[uefi_timer.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/driver/timer/uefi_timer.c), [uefi_timer.h](https://github.com/nekogakure/LiteCore/blob/main/src/include/driver/timer/uefi_timer.h)です。

## 概要
UEFI環境用のタイマードライバです。PIT (Programmable Interval Timer) を使用してタイマー割り込みを生成し、システム時刻の管理とスリープ機能を提供します。UEFI環境ではAPIC Timerの代わりにこのドライバが使用されます。

## 関数 / API

#### `int uefi_timer_init(void)`
UEFI環境用のタイマーを初期化します。PITを100Hz（10ms間隔）で動作するように設定し、IRQ 0のマスクを解除します。

引数: なし

戻り値: 0=成功、-1=失敗

#### `void uefi_timer_tick(uint32_t irq, void *context)`
タイマー割り込みハンドラです。10ms毎に呼ばれ、ティックカウントを増加させます。

引数:
  - irq(uint32_t): 割り込み番号
  - context(void*): コンテキスト（未使用）

#### `void uefi_sleep_ms(uint32_t ms)`
指定されたミリ秒だけスリープします。タイマー割り込みベースでCPUを休止状態にします。

引数:
  - ms(uint32_t): スリープ時間（ミリ秒）

#### `uint64_t uefi_get_uptime_ms(void)`
起動からの経過時間をミリ秒で取得します。

引数: なし

戻り値: 経過時間（ミリ秒）

#### `void uefi_wait_us(uint32_t us)`
指定されたマイクロ秒だけビジーウェイトで待機します。タイマーに依存しない単純なループベースの実装です。

引数:
  - us(uint32_t): 待機時間（マイクロ秒、最大10000us）

## 定数 / 定義

- `PIT_FREQUENCY`: 1193182 - PITの基本周波数（約1.193MHz）
- `PIT_CHANNEL0`: 0x40 - PITチャンネル0のI/Oポート
- `PIT_COMMAND`: 0x43 - PITコマンドレジスタのI/Oポート
- `PIT_MODE`: 0x36 - Mode 3: 方形波生成モード

## 構造体

このファイルには公開構造体の定義はありません。

## 実装の詳細

### PITの設定
PITは約1.193MHzで動作するハードウェアタイマーです。分周比を設定することで任意の周波数の割り込みを生成できます。このドライバでは100Hz（10ms間隔）で動作するように設定されています。

### タイマー割り込み
IRQ 0（PITタイマー）の割り込みハンドラが10msごとに呼ばれ、`timer_ticks`を増加させます。この値を基に経過時間やスリープ機能を実装します。

### スリープ機能
`uefi_sleep_ms()`は、目標ティック数に達するまで`hlt`命令でCPUを休止状態にします。これにより無駄なCPU使用を避けます。

### マイクロ秒待機
`uefi_wait_us()`は、タイマーに依存しないビジーウェイト実装です。`pause`命令を使用してCPUの負荷を軽減しながら、指定されたマイクロ秒だけループします。最大10msに制限されています。

### UEFI環境での使用
このドライバは`UEFI_MODE`が定義されている場合に使用されます。APIC Timerが利用できないUEFI環境でも、基本的なタイマー機能を提供します。
