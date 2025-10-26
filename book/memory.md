# メモリ

このドキュメントはLiteCoreカーネルで実装されているメモリ管理（フレーム割当、メモリマップ、カーネルヒープ）の設計と使い方をまとめています。

## 概要

LiteCoreはメモリ管理を次の要素で提供します。

- memmap: 物理メモリフレームの管理情報を保持する構造体（`memmap_t`）。
- bitmap: フレーム割当を実装するためのビットマップ（1 ビット = 1 フレーム）。
- frame allocator: ビットマップを使ったフレーム単位（通常 4KiB）の割当/解放。
- kernel heap: カーネル内での動的メモリアロケータ（簡易 free-list, `kmalloc`/`kfree`）。

これらは `src/kernel/mem/` と `src/include/mem/` 以下に実装されています。

## メモリレイアウトと初期化

起動時に以下を行います。

1. `memmap_init(start_addr, end_addr)` を呼んで memmap と bitmap を初期化します。
	 - `start_addr` / `end_addr` はフレーム割当の扱う物理領域を表します（例: 0x100000 - 0x500000）。
2. カーネルのバイナリ領域の終端はリンカシンボル `__end` で参照できます。
3. ヒープ開始 (`heap_start`) は次のルールで安全に決定します:
	 - `heap_start` = max(`&__end`, `bitmap_end`) を取り、ページ（4KiB）境界に切り上げる。
	 - これにより静的領域（`memmap` や `bitmap`）とヒープ領域の重複を防ぎます。
4. `mem_init(heap_start, heap_end)` を呼んでカーネルヒープを初期化します。
5. `memmap_reserve(heap_start, heap_end)` でヒープ領域に対応するフレームを memmap 上で予約（割当不可）にします。

## `memmap_t` とビットマップ

`memmap_t` は次の情報を持ちます（実装に依存しますが概念は以下の通りです）：

- `start_addr`: 管理領域の先頭アドレス
- `end_addr`: 管理領域の末尾アドレス
- `start_frame`: 管理領域に対応する最初のフレーム番号
- `max_frames`: 管理可能なフレーム数
- `bitmap`: ビットマップの先頭（バイト単位）

ビットマップは 1 ビット = 1 フレーム を表し、0 が空き、1 が使用中を示します。

API（主要なもの）:

- `memmap_init(uint32_t start, uint32_t end)`
	- memmap を初期化し、ビットマップをゼロクリアします。
- `alloc_frame()` / `free_frame(frame_number)`
	- フレームの割当 / 解放。割当は連続するフレームが欲しい場合は複数回呼ぶか別実装を検討。
- `memmap_reserve(start, end)`
	- 指定範囲をビットマップ上で予約（1 にする）して以後割当対象外にする。
- `memmap_get()`
	- 現在の `memmap_t` へのポインタを返す。`main.c` やデバッグ用出力で利用される。

## カーネルヒープ（free-list）

ヒープは簡易的な free-list ベースのアロケータです。実装の要点:

- 各ブロックはヘッダを持ち、サイズと使用フラグを保持します。
- アラインは 8 バイト（または実装上の要件）で揃えます。
- `kmalloc(size)` は最初に見つかった十分な大きさのフリーブロックを分割して返します。
- `kfree(ptr)` は隣接するフリーブロックとマージ（coalesce）します。
- 小規模/教育目的の実装であり、スレッドセーフではありません。

API（主要なもの）:

- `memory_init()`
        - ヒープを初期化します。`mem_init`やその他諸々をいい感じに呼び出します。
- `mem_init(uint32_t start, uint32_t end)`
	- ヒープ領域を初期化します（`start` と `end` は仮想/物理の設計に従う）。
- `void *kmalloc(uint32_t size)`
	- 指定バイト数のメモリを返します。
- `void kfree(void *ptr)`
	- `kmalloc` で確保した領域を解放します。
- `bool mem_has_space(mem_type_t type, uint32_t size)`
	- 指定したメモリタイプ（ヒープ or フレーム）で指定サイズを確保できるかをチェックするユーティリティ。

## TASKS

- ヒープのスレッドセーフ化（スピンロック等）
- フレーム割当で連続フレームを高速に割当できるように最適化
- ページテーブル / 仮想メモリの導入後に、物理フレームと仮想アドレス空間の管理を分離