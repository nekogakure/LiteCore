LiteCoreカーネルのドキュメントの目次

## 目次

### カーネルコア
- [カーネルメイン関数とエントリポイント](./kernel/main)
- [設定ファイル (config.h)](./config)

### メモリ管理
- [メモリ管理概要](./memory)
- [ヒープアロケータ](./mem/manager)
- [物理メモリマップ](./mem/map)
- [仮想メモリマッピング](./mem/vmem)
- [ページング機構](./mem/paging)
- [セグメント記述子テーブル（GDT）](./mem/segment)

### 割り込み処理
- [割り込み記述子テーブル（IDT）](./interrupt/idt)
- [IRQ管理とイベント処理](./interrupt/irq)
- [割り込みハンドラ管理](./interrupt/interrupt)
- [IDTロード処理](./interrupt/load_idt)

### デバイスドライバ
- [キーボードドライバ](./device/keyboard)
- [PCIバスドライバ](./device/pci)
- [ATAディスクドライバ](./driver/ata)

### ファイルシステム
- [ファイルシステム概要](./fs/index)
- [仮想ファイルシステム（VFS）](./fs/fs)
- [ブロックキャッシュ](./fs/block_cache)
- [FAT12ファイルシステム](./fs/fat/fat12)
- [ext2ファイルシステム](./fs/ext2)

### 同期機構
- [スピンロック](./sync/spinlock)

### ユーティリティ
- [コンソール出力](./util/console)
- [入出力ユーティリティ](./util/io)
- [シェル機能](./util/shell)
- [コマンドシステム](./util/commands)
- [拡張コマンド](./shell/extended_commands.h)
- [シェル統合](./util/shell_integration)
- [カーネル初期化](./util/init_msg)

### テスト
- [テストシステム](./tests/tests)

### 開発ツール
- [開発ツール](./tools/tools)

### その他
- [ビルド方法](./build)
- [コマンドシステム詳細](./command_system)
- [ディレクトリ構造](./tree)
- [エージェント情報](./AGENTS)
