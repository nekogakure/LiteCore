この章では、APIC Timerドライバについて記述します。
実装されているファイルは[apic.c](https://github.com/nekogakure/LiteCore/blob/main/src/kernel/driver/timer/apic.c), [apic.h](https://github.com/nekogakure/LiteCore/blob/main/src/include/driver/timer/apic.h)です。

## 概要
APIC (Advanced Programmable Interrupt Controller) Timerは、x86_64プロセッサに内蔵された高精度タイマーです。このドライバは、APIC Timerを使用してシステム時刻の管理、タイマー割り込みの生成、および遅延処理を実装します。ACPI PM Timerを使用してキャリブレーションを行い、正確な周波数を計算します。

## 関数 / API

#### `int apic_timer_init(void)`
APIC Timerを初期化します。CPUのAPICサポートを確認し、ベースアドレスを取得し、タイマー周波数をキャリブレーションして、1000Hz (1ms間隔) で動作するように設定します。

引数: なし

戻り値: 0=成功、-1=APICサポートなし

#### `int apic_timer_available(void)`
APIC Timerが初期化済みで利用可能かチェックします。

引数: なし

戻り値: 1=利用可能、0=利用不可

#### `void apic_timer_tick(uint32_t irq, void *context)`
タイマー割り込みハンドラです。1ms毎に呼ばれ、ティックカウントを増加させます。EOI (End of Interrupt) を送信して割り込みを終了します。

引数:
  - irq(uint32_t): 割り込み番号
  - context(void*): コンテキスト（未使用）

#### `uint64_t apic_get_uptime_us(void)`
システム起動からの経過時間をマイクロ秒で取得します。

引数: なし

戻り値: 経過時間（マイクロ秒）

#### `uint64_t apic_get_uptime_ms(void)`
システム起動からの経過時間をミリ秒で取得します。

引数: なし

戻り値: 経過時間（ミリ秒）

#### `uint32_t apic_timer_get_frequency(void)`
APIC Timerの周波数を取得します。

引数: なし

戻り値: タイマー周波数（Hz）

#### `void apic_timer_delay_us(uint32_t us)`
指定されたマイクロ秒だけビジーウェイトで遅延します。

引数:
  - us(uint32_t): 遅延時間（マイクロ秒）

#### `void apic_timer_delay_ms(uint32_t ms)`
指定されたミリ秒だけビジーウェイトで遅延します。

引数:
  - ms(uint32_t): 遅延時間（ミリ秒）

## 定数 / 定義

- `APIC_BASE_DEFAULT`: 0xFEE00000 - APICレジスタのデフォルトベースアドレス
- `APIC_ID`: 0x020 - Local APIC IDレジスタオフセット
- `APIC_VERSION`: 0x030 - Local APIC Versionレジスタオフセット
- `APIC_TPR`: 0x080 - Task Priority Registerオフセット
- `APIC_EOI`: 0x0B0 - End Of Interruptレジスタオフセット
- `APIC_LDR`: 0x0D0 - Logical Destination Registerオフセット
- `APIC_DFR`: 0x0E0 - Destination Format Registerオフセット
- `APIC_SPURIOUS`: 0x0F0 - Spurious Interrupt Vector Registerオフセット
- `APIC_ESR`: 0x280 - Error Status Registerオフセット
- `APIC_TIMER_LVT`: 0x320 - LVT Timer Registerオフセット
- `APIC_TIMER_INIT`: 0x380 - Timer Initial Countレジスタオフセット
- `APIC_TIMER_CURRENT`: 0x390 - Timer Current Countレジスタオフセット
- `APIC_TIMER_DIV`: 0x3E0 - Timer Divide Configurationレジスタオフセット
- `APIC_TIMER_MODE_ONESHOT`: 0x00000000 - ワンショットモード
- `APIC_TIMER_MODE_PERIODIC`: 0x00020000 - 周期モード
- `APIC_TIMER_MODE_TSCDEADLINE`: 0x00040000 - TSCデッドラインモード
- `APIC_TIMER_DIV_1`: 0x0B - 分周比1
- `APIC_TIMER_DIV_2`: 0x00 - 分周比2
- `APIC_TIMER_DIV_4`: 0x01 - 分周比4
- `APIC_TIMER_DIV_8`: 0x02 - 分周比8
- `APIC_TIMER_DIV_16`: 0x03 - 分周比16
- `APIC_TIMER_DIV_32`: 0x08 - 分周比32
- `APIC_TIMER_DIV_64`: 0x09 - 分周比64
- `APIC_TIMER_DIV_128`: 0x0A - 分周比128
- `ACPI_PM_TIMER_PORT`: 0x608 - ACPI PM Timerのポートアドレス
- `ACPI_PM_TIMER_FREQ`: 3579545 - ACPI PM Timerの固定周波数（3.579545 MHz）

## 構造体

このファイルには公開構造体の定義はありません。

## 実装の詳細

### キャリブレーション
APIC Timerの周波数は CPU によって異なるため、ACPI PM Timer（固定3.579545 MHz）を基準にしてキャリブレーションを行います。10ms間のAPICタイマーのカウント数を測定し、実際の周波数を計算します。

### 動作モード
APIC TimerはPeriodicモードで動作し、1000Hz（1ms間隔）で割り込みを生成します。割り込みベクタは48番が使用されます。

### 時刻管理
`current_tick_count`変数で1ms単位のティック数を管理し、これを基にマイクロ秒/ミリ秒単位の経過時間を提供します。

### 遅延処理
`apic_timer_delay_us()`および`apic_timer_delay_ms()`は、アップタイムを監視してビジーウェイトで遅延を実装します。CPUの負荷を避けるため`pause`命令を使用します。
