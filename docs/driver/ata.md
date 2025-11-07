この章では、ATAドライバについて記述します。
実装されているファイルは[ata.c](../../src/kernel/driver/ata.c), [ata.h](../../src/include/driver/ata.h)です。

## 概要
ATAドライバは、IDE/ATAハードディスクドライブへのアクセスを提供します。PIOモードを使用してセクタの読み書きを行い、LBA28アドレッシングをサポートします。プライマリおよびセカンダリバスの両方に対応し、マスター/スレーブドライブの検出と制御が可能です。

## 関数 / API

#### `int ata_init(void)`
ATAドライバを初期化し、接続されているATAデバイスを検出します。Primary Slave、Secondary Master、Primary Masterの順にデバイスを試行します。

引数: なし

戻り値: 成功時0、失敗時-1

#### `int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, void *buffer)`
ATAデバイスから指定されたセクタを読み取ります。

引数:
  - drive(uint8_t): ドライブ番号（0=Primary Master, 1=Primary Slave, 2=Secondary Master, 3=Secondary Slave）
  - lba(uint32_t): 読み取り開始LBA（Logical Block Address）
  - sectors(uint8_t): 読み取るセクタ数
  - buffer(void*): 読み取ったデータを格納するバッファ（sectors * 512バイト）

戻り値: 成功時0、失敗時-1

#### `int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sectors, const void *buffer)`
ATAデバイスへ指定されたセクタを書き込みます。

引数:
  - drive(uint8_t): ドライブ番号
  - lba(uint32_t): 書き込み開始LBA
  - sectors(uint8_t): 書き込むセクタ数
  - buffer(const void*): 書き込むデータのバッファ

戻り値: 成功時0、失敗時-1

## 定数 / 定義

### ATAレジスタ（Primary Bus）
- `ATA_PRIMARY_DATA`: 0x1F0 - データレジスタ
- `ATA_PRIMARY_ERROR`: 0x1F1 - エラーレジスタ（読み取り）
- `ATA_PRIMARY_FEATURES`: 0x1F1 - フィーチャーレジスタ（書き込み）
- `ATA_PRIMARY_SECCOUNT`: 0x1F2 - セクタカウントレジスタ
- `ATA_PRIMARY_LBALOW`: 0x1F3 - LBA Low
- `ATA_PRIMARY_LBAMID`: 0x1F4 - LBA Mid
- `ATA_PRIMARY_LBAHIGH`: 0x1F5 - LBA High
- `ATA_PRIMARY_DRIVE`: 0x1F6 - ドライブ/ヘッドレジスタ
- `ATA_PRIMARY_STATUS`: 0x1F7 - ステータスレジスタ（読み取り）
- `ATA_PRIMARY_COMMAND`: 0x1F7 - コマンドレジスタ（書き込み）

### ATAレジスタ（Secondary Bus）
- `ATA_SECONDARY_DATA`: 0x170 - データレジスタ
- `ATA_SECONDARY_ERROR`: 0x171 - エラーレジスタ
- `ATA_SECONDARY_FEATURES`: 0x171 - フィーチャーレジスタ
- `ATA_SECONDARY_SECCOUNT`: 0x172 - セクタカウントレジスタ
- `ATA_SECONDARY_LBALOW`: 0x173 - LBA Low
- `ATA_SECONDARY_LBAMID`: 0x174 - LBA Mid
- `ATA_SECONDARY_LBAHIGH`: 0x175 - LBA High
- `ATA_SECONDARY_DRIVE`: 0x176 - ドライブ/ヘッドレジスタ
- `ATA_SECONDARY_STATUS`: 0x177 - ステータスレジスタ
- `ATA_SECONDARY_COMMAND`: 0x177 - コマンドレジスタ

### ATAコマンド
- `ATA_CMD_READ_PIO`: 0x20 - PIO読み取りコマンド
- `ATA_CMD_WRITE_PIO`: 0x30 - PIO書き込みコマンド
- `ATA_CMD_IDENTIFY`: 0xEC - IDENTIFY DEVICEコマンド

### ATAステータスビット
- `ATA_SR_BSY`: 0x80 - ビジー（デバイスが処理中）
- `ATA_SR_DRDY`: 0x40 - ドライブ準備完了
- `ATA_SR_DF`: 0x20 - ドライブ書き込みエラー
- `ATA_SR_DSC`: 0x10 - ドライブシーク完了
- `ATA_SR_DRQ`: 0x08 - データ要求準備完了
- `ATA_SR_CORR`: 0x04 - 訂正されたデータ
- `ATA_SR_IDX`: 0x02 - インデックス
- `ATA_SR_ERR`: 0x01 - エラー

### ATAエラービット
- `ATA_ER_BBK`: 0x80 - 不良ブロック
- `ATA_ER_UNC`: 0x40 - 訂正不可能なデータ
- `ATA_ER_MC`: 0x20 - メディア変更
- `ATA_ER_IDNF`: 0x10 - IDが見つからない
- `ATA_ER_MCR`: 0x08 - メディア変更要求
- `ATA_ER_ABRT`: 0x04 - コマンド中止
- `ATA_ER_TK0NF`: 0x02 - トラック0が見つからない
- `ATA_ER_AMNF`: 0x01 - アドレスマークが見つからない

### ATAデバイス選択
- `ATA_MASTER`: 0xE0 - マスターデバイス
- `ATA_SLAVE`: 0xF0 - スレーブデバイス

## 構造体

このファイルには公開構造体の定義はありません。

### 内部実装

ATAドライバはポーリングベースのPIOモードを使用します：
- デバイスの準備完了を待機（BSYビットのクリア）
- データ要求準備完了を待機（DRQビットのセット）
- エラー検出とタイムアウト処理
- 割り込みイベントの定期的な処理でFIFOオーバーフローを防止
