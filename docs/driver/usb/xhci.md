この章では、xHCI (eXtensible Host Controller Interface) ホストコントローラドライバについて記述します。
実装されているファイルは[xhci.c](../../../src/kernel/driver/usb/xhci.c), [xhci.h](../../../src/include/driver/usb/xhci.h)です。

## 概要

xHCIドライバは、USB 3.0ホストコントローラの制御を行います。PCIバス上のxHCIコントローラを検出し、デバイスの列挙、アドレス割り当て、データ転送（コントロール転送、Interrupt転送）を実装しています。

主な機能：
- PCIバスからのxHCIコントローラ検出
- コントローラの初期化とリセット
- Command Ring / Event Ringの管理
- デバイスの列挙とアドレス割り当て
- コントロール転送（Setup/Data/Status）
- Interrupt IN転送（キーボード入力など）
- TRB (Transfer Request Block)の管理

## 構造体

### `struct xhci_cap_regs`
xHCIのCapability Registersを表す構造体です。

- `uint8_t caplength`: Capability Registers Length
- `uint8_t reserved`: 予約済み
- `uint16_t hciversion`: Interface Version Number
- `uint32_t hcsparams1`: Structural Parameters 1 (最大スロット数、ポート数など)
- `uint32_t hcsparams2`: Structural Parameters 2
- `uint32_t hcsparams3`: Structural Parameters 3
- `uint32_t hccparams1`: Capability Parameters 1
- `uint32_t dboff`: Doorbell Offset
- `uint32_t rtsoff`: Runtime Register Space Offset
- `uint32_t hccparams2`: Capability Parameters 2

### `struct xhci_op_regs`
xHCIのOperational Registersを表す構造体です。

- `uint32_t usbcmd`: USB Command (Run/Stop, Resetなど)
- `uint32_t usbsts`: USB Status (Halted, Controller Not Readyなど)
- `uint32_t pagesize`: Page Size
- `uint32_t dnctrl`: Device Notification Control
- `uint64_t crcr`: Command Ring Control Register
- `uint64_t dcbaap`: Device Context Base Address Array Pointer
- `uint32_t config`: Configure (MaxSlotsEnなど)
- `uint32_t portsc[]`: Port Status and Control (可変長配列)

### `struct xhci_intr_regs`
Runtime Registers内のInterrupter Registersを表す構造体です。

- `uint32_t iman`: Interrupt Management
- `uint32_t imod`: Interrupt Moderation
- `uint32_t erstsz`: Event Ring Segment Table Size
- `uint32_t reserved`: 予約済み
- `uint64_t erstba`: Event Ring Segment Table Base Address
- `uint64_t erdp`: Event Ring Dequeue Pointer

### `struct xhci_trb`
Transfer Request Block (TRB)を表す構造体です。全ての転送はTRBを使用して実行されます。

- `uint64_t parameter`: TRBパラメータ（アドレスやSetup Packetなど）
- `uint32_t status`: ステータス（Transfer Length、Completion Codeなど）
- `uint32_t control`: コントロールフィールド（TRB Type、Cycle Bitなど）

### `struct xhci_erst_entry`
Event Ring Segment Table Entryを表す構造体です。

- `uint64_t ring_segment_base`: Event Ringセグメントのベースアドレス
- `uint32_t ring_segment_size`: Event Ringセグメントのサイズ
- `uint32_t reserved`: 予約済み

### `struct xhci_slot_context`
Device ContextのSlot Contextを表す構造体です。

- `uint32_t dw0`: Route String、Speedなど
- `uint32_t dw1`: Root Hub Port Number、Context Entriesなど
- `uint32_t dw2`: TTポート関連
- `uint32_t dw3`: デバイスアドレスなど
- `uint32_t reserved[4]`: 予約済み

### `struct xhci_endpoint_context`
Device ContextのEndpoint Contextを表す構造体です。

- `uint32_t dw0`: Endpoint State、Intervalなど
- `uint32_t dw1`: Endpoint Type、Max Packet Sizeなど
- `uint64_t tr_dequeue_pointer`: Transfer Ring Dequeue Pointer
- `uint32_t dw4`: Average TRB Lengthなど
- `uint32_t reserved[3]`: 予約済み

### `struct xhci_device_context`
Device Contextを表す構造体です。

- `struct xhci_slot_context slot`: Slot Context
- `struct xhci_endpoint_context ep[31]`: Endpoint Context（最大31個）

### `struct xhci_input_control_context`
Input ContextのControl Contextを表す構造体です。

- `uint32_t drop_flags`: Drop Context Flags
- `uint32_t add_flags`: Add Context Flags
- `uint32_t reserved[6]`: 予約済み

### `struct xhci_input_context`
Input Contextを表す構造体です。Address DeviceやConfigure Endpointで使用します。

- `struct xhci_input_control_context input_control`: Input Control Context
- `struct xhci_slot_context slot`: Slot Context
- `struct xhci_endpoint_context ep[31]`: Endpoint Context（最大31個）

### `struct xhci_port_regs`
Port Registersを表す構造体です。

- `uint32_t portsc`: Port Status and Control
- `uint32_t portpmsc`: Port Power Management Status and Control
- `uint32_t portli`: Port Link Info
- `uint32_t porthlpmc`: Port Hardware LPM Control

### `struct xhci_hc`
xHCIホストコントローラの状態を管理する構造体です。

- `uintptr_t base_addr`: MMIO Base Address
- `volatile struct xhci_cap_regs *cap_regs`: Capability Registersへのポインタ
- `volatile struct xhci_op_regs *op_regs`: Operational Registersへのポインタ
- `volatile uint32_t *doorbell_array`: Doorbell Arrayへのポインタ
- `volatile void *runtime_regs`: Runtime Registersへのポインタ
- `volatile struct xhci_port_regs *port_regs`: Port Registersへのポインタ
- `volatile struct xhci_intr_regs *intr_regs`: Interrupter Registersへのポインタ
- `uint16_t hci_version`: HCI Version
- `uint32_t max_slots`: 最大デバイススロット数
- `uint32_t max_ports`: 最大ポート数
- `uint32_t max_intrs`: 最大Interrupter数
- `uint8_t bus`: PCIバス番号
- `uint8_t device`: PCIデバイス番号
- `uint8_t function`: PCI Function番号
- `uint64_t *dcbaa`: Device Context Base Address Array
- `struct xhci_trb *command_ring`: Command Ring
- `struct xhci_trb *event_ring`: Event Ring
- `struct xhci_erst_entry *erst`: Event Ring Segment Table
- `uint32_t cmd_ring_index`: Command Ringのインデックス
- `uint32_t cmd_ring_cycle`: Command RingのCycle Bit
- `uint32_t event_ring_index`: Event Ringのインデックス
- `uint32_t event_ring_cycle`: Event RingのCycle Bit
- `struct xhci_device_context *device_contexts[256]`: Device Context配列
- `struct xhci_trb *transfer_rings[256][32]`: Transfer Ring配列 [slot][endpoint]
- `uint32_t transfer_ring_index[256][32]`: Transfer Ringインデックス
- `uint32_t transfer_ring_cycle[256][32]`: Transfer Ring Cycle Bit
- `uint8_t slot_allocated[256]`: スロット割り当て状態
- `uint8_t keyboard_slot_id`: キーボードデバイスのスロットID
- `uint8_t keyboard_endpoint`: キーボードのInterrupt INエンドポイント
- `void *keyboard_buffer`: キーボードレポート受信バッファ
- `int initialized`: 初期化済みフラグ

## 定数 / 定義

### USB Command Register (USBCMD) ビット定義
- `XHCI_CMD_RUN`: Run/Stop (1=Run, 0=Stop)
- `XHCI_CMD_HCRST`: Host Controller Reset
- `XHCI_CMD_INTE`: Interrupter Enable
- `XHCI_CMD_HSEE`: Host System Error Enable
- `XHCI_CMD_EWE`: Enable Wrap Event

### USB Status Register (USBSTS) ビット定義
- `XHCI_STS_HCH`: HC Halted
- `XHCI_STS_HSE`: Host System Error
- `XHCI_STS_EINT`: Event Interrupt
- `XHCI_STS_PCD`: Port Change Detect
- `XHCI_STS_CNR`: Controller Not Ready

### Port Status and Control Register (PORTSC) ビット定義
- `XHCI_PORTSC_CCS`: Current Connect Status
- `XHCI_PORTSC_PED`: Port Enabled/Disabled
- `XHCI_PORTSC_PR`: Port Reset
- `XHCI_PORTSC_PLS_MASK`: Port Link State Mask
- `XHCI_PORTSC_PP`: Port Power
- `XHCI_PORTSC_SPEED_MASK`: Port Speed Mask
- `XHCI_PORTSC_CSC`: Connect Status Change
- `XHCI_PORTSC_PEC`: Port Enabled/Disabled Change
- `XHCI_PORTSC_PRC`: Port Reset Change

### Command Ring Control Register (CRCR) ビット定義
- `XHCI_CRCR_RCS`: Ring Cycle State
- `XHCI_CRCR_CS`: Command Stop
- `XHCI_CRCR_CA`: Command Abort
- `XHCI_CRCR_CRR`: Command Ring Running

### Interrupt Management Register (IMAN) ビット定義
- `XHCI_IMAN_IP`: Interrupt Pending
- `XHCI_IMAN_IE`: Interrupt Enable

### TRB Type定義
- `TRB_TYPE_NORMAL`: Normal TRB (1)
- `TRB_TYPE_SETUP`: Setup Stage TRB (2)
- `TRB_TYPE_DATA`: Data Stage TRB (3)
- `TRB_TYPE_STATUS`: Status Stage TRB (4)
- `TRB_TYPE_LINK`: Link TRB (6)
- `TRB_TYPE_EVENT_DATA`: Event Data TRB (7)
- `TRB_TYPE_NOOP_TRANSFER`: No Op Transfer TRB (8)
- `TRB_TYPE_ENABLE_SLOT`: Enable Slot Command (9)
- `TRB_TYPE_DISABLE_SLOT`: Disable Slot Command (10)
- `TRB_TYPE_ADDRESS_DEVICE`: Address Device Command (11)
- `TRB_TYPE_CONFIG_EP`: Configure Endpoint Command (12)
- `TRB_TYPE_EVAL_CONTEXT`: Evaluate Context Command (13)
- `TRB_TYPE_RESET_EP`: Reset Endpoint Command (14)
- `TRB_TYPE_STOP_EP`: Stop Endpoint Command (15)
- `TRB_TYPE_SET_TR_DEQUEUE`: Set TR Dequeue Pointer Command (16)
- `TRB_TYPE_RESET_DEVICE`: Reset Device Command (17)
- `TRB_TYPE_NOOP_COMMAND`: No Op Command (23)
- `TRB_TYPE_TRANSFER_EVENT`: Transfer Event (32)
- `TRB_TYPE_COMMAND_COMPLETION`: Command Completion Event (33)
- `TRB_TYPE_PORT_STATUS_CHANGE`: Port Status Change Event (34)

### TRB Control ビット定義
- `TRB_CYCLE`: Cycle Bit
- `TRB_ENT`: Evaluate Next TRB
- `TRB_ISP`: Interrupt on Short Packet
- `TRB_CH`: Chain bit
- `TRB_IOC`: Interrupt On Completion
- `TRB_IDT`: Immediate Data
- `TRB_DIR_IN`: Direction (Data Stage)

### Endpoint Type定義
- `EP_TYPE_CONTROL`: Control Endpoint (4)
- `EP_TYPE_ISOCH_OUT`: Isochronous OUT (1)
- `EP_TYPE_BULK_OUT`: Bulk OUT (2)
- `EP_TYPE_INTERRUPT_OUT`: Interrupt OUT (3)
- `EP_TYPE_ISOCH_IN`: Isochronous IN (5)
- `EP_TYPE_BULK_IN`: Bulk IN (6)
- `EP_TYPE_INTERRUPT_IN`: Interrupt IN (7)

### Ring Size定義
- `COMMAND_RING_SIZE`: Command Ringのサイズ (256)
- `EVENT_RING_SIZE`: Event Ringのサイズ (256)
- `TRANSFER_RING_SIZE`: Transfer Ringのサイズ (256)

### USB Speed定義
- `XHCI_SPEED_FULL`: USB 1.1 Full Speed (12 Mbps)
- `XHCI_SPEED_LOW`: USB 1.1 Low Speed (1.5 Mbps)
- `XHCI_SPEED_HIGH`: USB 2.0 High Speed (480 Mbps)
- `XHCI_SPEED_SUPER`: USB 3.0 Super Speed (5 Gbps)

## 関数 / API

### 初期化関数

#### `int xhci_init(void)`
xHCIドライバを初期化します。PCIバスをスキャンしてxHCIコントローラを検出し、初期化します。

戻り値:
  - 0: 成功
  - 負数: エラー

#### `struct xhci_hc *xhci_find_controller(void)`
xHCIコントローラを検索します。すでに初期化されている場合はそのポインタを返し、未初期化の場合は初期化を試みます。

戻り値:
  - コントローラ構造体へのポインタ（成功）
  - NULL（失敗）

#### `int xhci_reset_controller(struct xhci_hc *hc)`
xHCIコントローラをリセットします。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_start_controller(struct xhci_hc *hc)`
xHCIコントローラを起動します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - 0: 成功
  - 負数: エラー

### Command/Event Ring管理関数

#### `int xhci_setup_command_ring(struct xhci_hc *hc)`
Command Ringを初期化します。64-byteアライメントでメモリを確保し、Link TRBでリング構造を構築します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_setup_event_ring(struct xhci_hc *hc)`
Event Ringを初期化します。Event Ring Segment Table (ERST)とEvent Ringを確保し、Interrupter 0に設定します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - 0: 成功
  - 負数: エラー

#### `void xhci_ring_doorbell(struct xhci_hc *hc, uint8_t slot_id, uint8_t target)`
Doorbellを鳴らしてコントローラに処理を開始させます。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID（0の場合はHost Controller Command）
  - target (uint8_t): ターゲット（DCIまたは0）

#### `int xhci_wait_for_event(struct xhci_hc *hc, struct xhci_trb *event_trb, uint32_t timeout_ms)`
Event Ringからイベントを待機します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - event_trb (struct xhci_trb *): イベントTRBを格納するバッファ（NULLも可）
  - timeout_ms (uint32_t): タイムアウト時間（ミリ秒、0の場合は即座に戻る）

戻り値:
  - 0: イベント取得成功
  - 負数: タイムアウトまたはエラー

#### `void xhci_handle_events(struct xhci_hc *hc)`
Event Ringのイベントを処理します。Port Status Change、Command Completion、Transfer Eventなどを処理します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

### デバイス列挙関数

#### `int xhci_reset_port(struct xhci_hc *hc, uint8_t port)`
ポートをリセットします。Port Reset Changeビットが立つまで待機します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - port (uint8_t): ポート番号（1-based）

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_enable_slot(struct xhci_hc *hc)`
Enable Slot Commandを実行してスロットを有効化します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - スロットID（正数）: 成功
  - 負数: エラー

#### `int xhci_address_device(struct xhci_hc *hc, uint8_t slot_id, uint8_t port)`
Address Device Commandを実行してデバイスにアドレスを割り当てます。Input ContextとDevice Contextを作成し、DCBAAに設定します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - port (uint8_t): ポート番号

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_configure_endpoint(struct xhci_hc *hc, uint8_t slot_id)`
Configure Endpoint Commandを実行してエンドポイントを設定します（未完全実装）。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID

戻り値:
  - 0: 成功
  - 負数: エラー

#### `void xhci_handle_port_status_change(struct xhci_hc *hc, uint8_t port)`
ポート状態変化イベントを処理します。デバイスの接続/切断を検出し、自動的に列挙処理を実行します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - port (uint8_t): ポート番号

#### `uint32_t xhci_get_port_status(struct xhci_hc *hc, uint32_t port_num)`
ポートのステータスを取得します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - port_num (uint32_t): ポート番号（1-based）

戻り値:
  - ポートステータス（PORTSC値）

### USB転送関数

#### `int xhci_control_transfer(struct xhci_hc *hc, uint8_t slot_id, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, void *data, uint16_t length)`
コントロール転送を実行します。Setup/Data/Statusの3ステージを実装しています。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - request_type (uint8_t): リクエストタイプ（bmRequestType）
  - request (uint8_t): リクエスト（bRequest）
  - value (uint16_t): 値（wValue）
  - index (uint16_t): インデックス（wIndex）
  - data (void *): データバッファ（NULLも可）
  - length (uint16_t): データ長

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_get_descriptor(struct xhci_hc *hc, uint8_t slot_id, uint8_t desc_type, uint8_t desc_index, void *buffer, uint16_t length)`
GET_DESCRIPTORリクエストを実行します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - desc_type (uint8_t): ディスクリプタタイプ
  - desc_index (uint8_t): ディスクリプタインデックス
  - buffer (void *): 受信バッファ
  - length (uint16_t): 受信バッファサイズ

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_set_configuration(struct xhci_hc *hc, uint8_t slot_id, uint8_t config_value)`
SET_CONFIGURATIONリクエストを実行します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - config_value (uint8_t): Configuration値

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_submit_interrupt_in(struct xhci_hc *hc, uint8_t slot_id, uint8_t endpoint, void *buffer, uint16_t length)`
Interrupt IN Transferを送信します。Transfer Ringに Normal TRBを追加し、Doorbellを鳴らします。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - endpoint (uint8_t): エンドポイント番号
  - buffer (void *): 受信バッファ
  - length (uint16_t): バッファサイズ

戻り値:
  - 0: 成功
  - 負数: エラー

### HIDキーボード関数

#### `int xhci_set_boot_protocol(struct xhci_hc *hc, uint8_t slot_id, uint8_t interface)`
HID Boot Protocolを設定します。SET_PROTOCOLリクエストを実行します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - interface (uint8_t): インターフェース番号

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_configure_keyboard(struct xhci_hc *hc, uint8_t slot_id)`
キーボードデバイスを設定します。Device Descriptor、Configuration Descriptorを取得し、SET_CONFIGURATIONとSET_PROTOCOLを実行します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID

戻り値:
  - 0: 成功
  - 負数: エラー

#### `uint8_t xhci_get_keyboard_slot(struct xhci_hc *hc)`
キーボードデバイスのスロットIDを取得します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ

戻り値:
  - スロットID（0の場合はキーボード未検出）

#### `int xhci_setup_keyboard_polling(struct xhci_hc *hc, uint8_t slot_id)`
キーボードポーリングをセットアップします。キーボードを設定し、最初のInterrupt IN Transferを送信します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID

戻り値:
  - 0: 成功
  - 負数: エラー

#### `int xhci_poll_keyboard(struct xhci_hc *hc, uint8_t slot_id, uint8_t *report_buffer)`
キーボードからのレポートをポーリングします。Event RingをチェックしてTransfer Eventがあればキーボードレポートをコピーし、次のTransferを送信します。

引数:
  - hc (struct xhci_hc *): コントローラ構造体へのポインタ
  - slot_id (uint8_t): スロットID
  - report_buffer (uint8_t *): レポート受信バッファ（8バイト）

戻り値:
  - 0: 成功（データあり）
  - 負数: エラーまたはデータなし

### ユーティリティ関数

#### `void *xhci_alloc_aligned(uint32_t size, uint32_t alignment)`
アライメントされたメモリを確保します。

引数:
  - size (uint32_t): サイズ
  - alignment (uint32_t): アライメント

戻り値:
  - メモリへのポインタ（成功）
  - NULL（失敗）

#### `void xhci_free_aligned(void *ptr)`
アライメントされたメモリを解放します。

引数:
  - ptr (void *): 解放するメモリへのポインタ

## 実装の詳細

### メモリアライメント要件

xHCIでは、各データ構造に厳密なアライメント要件があります：

- Command Ring: 64-byte
- Event Ring: 64-byte
- ERST: 64-byte
- Device Context: 64-byte
- Input Context: 64-byte
- Transfer Ring: 16-byte

### Cycle Bit管理

リング構造では、Producer Cycle BitとConsumer Cycle Bitを使用して、TRBの処理状態を管理します：

- Producer（ソフトウェア）がTRBを追加する際、現在のCycle Bitを設定
- リング境界で折り返す際、Cycle Bitを反転
- Consumer（ハードウェア）はCycle Bitが一致するTRBのみ処理

### コントロール転送の実装

コントロール転送は3ステージで構成されます：

1. **Setup Stage**: Setup Packetを送信（TRB_TYPE_SETUP、IDT、TRT設定）
2. **Data Stage**: データを送受信（TRB_TYPE_DATA、Direction bit設定）
3. **Status Stage**: ステータスを受信（TRB_TYPE_STATUS、IOC、Direction逆転）

### Interrupt IN Transferの実装

Interrupt IN Transferは以下の流れで動作します：

1. Transfer Ringを確保（初回のみ）
2. Normal TRBを作成（IOC、ISP設定）
3. Doorbellを鳴らして転送開始
4. Transfer Eventを待機
5. データ受信後、次のTransferを自動送信（ポーリング継続）

### デバイス列挙プロセス

1. Port Status Changeイベント検出
2. ポートリセット
3. Enable Slot Command実行
4. Address Device Command実行（Input Context、Device Context作成）
5. キーボードの場合、自動的に設定（GET_DESCRIPTOR、SET_CONFIGURATION、SET_PROTOCOL）