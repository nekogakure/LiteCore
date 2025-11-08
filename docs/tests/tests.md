この章では、テストシステムについて記述します。
実装されているファイルは[run.c](../../src/kernel/tests/run.c), [run.h](../../src/include/tests/run.h), [define.h](../../src/include/tests/define.h)と各種テストファイルです。

## 概要
テストシステムは、カーネルの各サブシステムの機能を検証するためのユニットテストを提供します。メモリ管理、ページング、割り込み、ファイルシステムなどのテストが含まれます。テストは条件付きコンパイルで有効化/無効化できます。

## テストファイル

### run.c / run.h
テストランナーです。有効化されたテストを順番に実行します。

#### `void run_test(void)`
すべての有効化されたテストを実行します。

引数: なし

### define.h
テストの有効化/無効化を制御するマクロ定義ファイルです。

#### テスト制御マクロ
- `TEST_TRUE`: 定義するとテストシステムを有効化
- `MEMORY_TEST`: メモリ管理テストを有効化
- `VMEM_TEST`: 仮想メモリテストを有効化
- `PAGING_TEST`: ページングテストを有効化
- `GDT_TEST`: GDTテストを有効化
- `INTERRUPT_TEST`: 割り込みテストを有効化
- `INTERRUPT_VECTOR_TEST`: 割り込みベクタテストを有効化
- `ALLOC_IRQ_TEST`: IRQ割り当てテストを有効化
- `FS_TEST`: ファイルシステムテストを有効化
- `EXT2_TEST`: ext2ファイルシステムテストを有効化

### 個別テストファイル

#### memory_test.c
メモリ管理（ヒープアロケータ）のテストです。`kmalloc`と`kfree`の動作を検証します。

#### vmem_test.c
仮想メモリマッピングのテストです。物理アドレスと仮想アドレスの変換を検証します。

#### paging_test.c
ページングシステムのテストです。ページマッピング、アンマッピング、範囲マッピングを検証します。

#### gdt_test.c
GDT（Global Descriptor Table）のテストです。セグメント記述子の設定を検証します。

#### interrupt_test.c
基本的な割り込み処理のテストです。割り込みハンドラの登録と実行を検証します。

#### interrupt_vector_test.c
割り込みベクタテーブルのテストです。複数の割り込みハンドラの動作を検証します。

#### alloc_irq_test.c
IRQ割り当てシステムのテストです。割り込み要求の登録とディスパッチを検証します。

#### fs_test.c
ファイルシステムの基本的なテストです。VFSレイヤーの動作を検証します。

#### ext2_test.c
ext2ファイルシステムのテストです。マウント、ディレクトリ列挙、ファイル読み取りを検証します。

## テストの実行

### テストの有効化
`src/include/tests/define.h`でマクロを定義します：

```c
#define TEST_TRUE          // テストシステムを有効化
#define MEMORY_TEST        // メモリテストを有効化
#define PAGING_TEST        // ページングテストを有効化
// ... 他のテスト
```

### ビルドと実行
```bash
make clean && make all
make run
```

### 出力例
```
====== TESTS ======
Running memory test...
Memory test passed!
Running paging test...
Paging test passed!
...
All tests completed!
```

## テストの追加

新しいテストを追加する手順：

1. `src/kernel/tests/`に新しいテストファイルを作成
2. `src/include/tests/define.h`に制御マクロを追加
3. `src/kernel/tests/run.c`にテスト関数呼び出しを追加
4. 
### テスト関数の形式

```c
#include <util/config.h>
#include <tests/define.h>
#include <util/console.h>

void my_new_test() {
    // テストコード    
}
```

## 注意事項

- テストは起動時に一度だけ実行されます
- 一部のテストは相互に依存する場合があります（例: ページングテストはメモリ管理が初期化済みであることを前提とする）
- テストの失敗はシステムの不安定化を引き起こす可能性があります
- 本番環境では`TEST_TRUE`を未定義にしてテストを無効化してください
