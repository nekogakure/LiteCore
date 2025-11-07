この章では、スピンロック機構について記述します。
実装されているファイルは[spinlock.c](../../src/kernel/sync/spinlock.c), [spinlock.h](../../src/include/sync/spinlock.h)です。

## 概要
スピンロックは、マルチコアシステムや割り込みコンテキストでの排他制御を提供する同期プリミティブです。アトミックな命令（xchg）を使用してロックの取得と解放を行い、ビジーウェイトで実装されています。割り込みフラグの保存と復元をサポートし、割り込みハンドラ内でも安全に使用できます。

## 関数 / API

#### `void spin_lock(spinlock_t *lock)`
スピンロックを取得します。ロックが取得できるまでビジーウェイトします。

引数:
  - lock(spinlock_t*): 取得対象のスピンロック構造体へのポインタ

#### `void spin_unlock(spinlock_t *lock)`
スピンロックを解放します。

引数:
  - lock(spinlock_t*): 解放するスピンロックへのポインタ

#### `void spin_lock_irqsave(spinlock_t *lock, uint32_t *flags)`
スピンロックを取得し、割り込みフラグを保存します。割り込みを無効化してからロックを取得するため、割り込みハンドラとの競合を防ぎます。

引数:
  - lock(spinlock_t*): 取得するスピンロックへのポインタ
  - flags(uint32_t*): 割り込みフラグを保存するためのポインタ

#### `void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags)`
スピンロックを解放し、保存された割り込みフラグを復元します。

引数:
  - lock(spinlock_t*): 解放するスピンロックへのポインタ
  - flags(uint32_t): 復元する割り込みフラグ

## 定数 / 定義

このファイルには定数定義はありません。

## 構造体

#### `struct spinlock` / `spinlock_t`
スピンロックの状態を保持する構造体です。

- `lock(volatile uint32_t)`: ロックの状態（0=解放、1=取得済み）

### 使用例

```c
#include <sync/spinlock.h>

spinlock_t my_lock = {0};

// 基本的な使用
spin_lock(&my_lock);
// クリティカルセクション
spin_unlock(&my_lock);

// 割り込みフラグの保存/復元付き
uint32_t flags;
spin_lock_irqsave(&my_lock, &flags);
// クリティカルセクション（割り込みから保護）
spin_unlock_irqrestore(&my_lock, flags);
```

### 実装の詳細

- **xchg命令**: x86のアトミック交換命令を使用してロックを実装
- **メモリバリア**: `memory` clobberを使用してコンパイラの最適化を制限
- **ビジーウェイト**: ロックが取得できるまでループで待機
- **割り込み制御**: `irq_save()`と`irq_restore()`を使用して割り込みフラグを管理
