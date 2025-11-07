この章では、PCIバスドライバについて記述します。
実装されているファイルは[pci.c](../../src/kernel/device/pci.c), [pci.h](../../src/include/device/pci.h)です。

## 概要
PCIドライバは、PCIバス上のデバイスを検出し、コンフィギュレーション空間にアクセスする機能を提供します。標準的なPCIコンフィギュレーション空間アクセスメカニズムを使用して、バス、デバイス、ファンクションを列挙します。

## 関数 / API

#### `uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)`
PCIコンフィギュレーション空間から32ビット値を読み取ります。

引数:
  - bus(uint8_t): PCIバス番号（0-255）
  - device(uint8_t): PCIデバイス番号（0-31）
  - func(uint8_t): PCIファンクション番号（0-7）
  - offset(uint8_t): コンフィギュレーション空間内のオフセット（4バイト境界）

戻り値: 読み取った32ビット値

#### `void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value)`
PCIコンフィギュレーション空間へ32ビット値を書き込みます。

引数:
  - bus(uint8_t): PCIバス番号
  - device(uint8_t): PCIデバイス番号
  - func(uint8_t): PCIファンクション番号
  - offset(uint8_t): コンフィギュレーション空間内のオフセット
  - value(uint32_t): 書き込む値

#### `void pci_enumerate(void)`
システム上の全PCIデバイスを列挙し、情報を出力します。全バス（0-255）、全デバイス（0-31）、全ファンクション（0-7）をスキャンします。

引数: なし

## 定数 / 定義

- `0xCF8`: PCIコンフィギュレーションアドレスポート
- `0xCFC`: PCIコンフィギュレーションデータポート

## 構造体

このファイルには公開構造体の定義はありません。

### PCIコンフィギュレーション空間

PCIコンフィギュレーション空間の主要なオフセット：
- 0x00: ベンダーID (16ビット) とデバイスID (16ビット)
- 0x08: クラスコード、サブクラス、プログラミングインターフェース、リビジョンID
- 0x0C: ヘッダタイプ、マルチファンクションフラグ

### ベンダーID

- 0xFFFF: デバイスが存在しない
- その他: 有効なPCIデバイス
