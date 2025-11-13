この章では、HID (Human Interface Device) の定義について記述します。
実装されているファイルは[hid.h](https://github.com/nekogakure/LiteCore/blob/main/src/include/driver/usb/hid.h)です。

## 概要

このヘッダファイルは、HIDクラスデバイス（キーボード、マウスなど）の制御に必要な定義を提供します。特にBoot Protocolキーボードのサポートに焦点を当てています。

## 構造体

### `struct hid_keyboard_report`
HID Boot Protocolキーボードレポートを表す構造体です。キーボードから送信される8バイトのレポートフォーマットです。

- `uint8_t modifier`: 修飾キー（Ctrl、Shift、Alt、GUI）のビットマスク
- `uint8_t reserved`: 予約済み（常に0）
- `uint8_t keycode[6]`: 同時押し可能なキーコード（最大6個）

## 定数 / 定義

### HID Class Descriptor Types
- `HID_DT_HID`: HID Descriptor (0x21)
- `HID_DT_REPORT`: Report Descriptor (0x22)
- `HID_DT_PHYSICAL`: Physical Descriptor (0x23)

### HID Class Requests
- `HID_REQ_GET_REPORT`: Get Report (0x01) - HIDレポートを取得
- `HID_REQ_GET_IDLE`: Get Idle (0x02) - アイドルレートを取得
- `HID_REQ_GET_PROTOCOL`: Get Protocol (0x03) - 現在のプロトコルを取得
- `HID_REQ_SET_REPORT`: Set Report (0x09) - HIDレポートを設定
- `HID_REQ_SET_IDLE`: Set Idle (0x0A) - アイドルレートを設定
- `HID_REQ_SET_PROTOCOL`: Set Protocol (0x0B) - プロトコルを設定

### HID Protocol
- `HID_PROTOCOL_BOOT`: Boot Protocol (0) - シンプルなレポートフォーマット
- `HID_PROTOCOL_REPORT`: Report Protocol (1) - 複雑なレポートフォーマット

### Modifier Bits（修飾キー）
- `HID_MOD_LEFT_CTRL`: 左Ctrlキー (0x01)
- `HID_MOD_LEFT_SHIFT`: 左Shiftキー (0x02)
- `HID_MOD_LEFT_ALT`: 左Altキー (0x04)
- `HID_MOD_LEFT_GUI`: 左GUIキー（Windowsキーなど） (0x08)
- `HID_MOD_RIGHT_CTRL`: 右Ctrlキー (0x10)
- `HID_MOD_RIGHT_SHIFT`: 右Shiftキー (0x20)
- `HID_MOD_RIGHT_ALT`: 右Altキー (0x40)
- `HID_MOD_RIGHT_GUI`: 右GUIキー (0x80)

## HID Boot Protocolキーボード

### レポートフォーマット

Boot Protocolキーボードは、8バイトの固定フォーマットでレポートを送信します：

```
Byte 0: modifier (修飾キービットマスク)
Byte 1: reserved (常に0)
Byte 2-7: keycode[0-5] (押されているキーのスキャンコード)
```

### キーコード

キーコードはUSB HID Usage Tables（Keyboard/Keypad Page）で定義されています：

- 0x00: No Event
- 0x04-0x1D: a-z
- 0x1E-0x27: 1-0
- 0x28: Enter
- 0x29: Escape
- 0x2A: Backspace
- 0x2B: Tab
- 0x2C: Space
- 0x2D-0x38: 記号キー
- 0x3A-0x45: F1-F12

### 同時押し検出

最大6個のキーを同時に押すことができます。keycodeフィールドが0の場合、そのスロットは空です。

### 使用例

```c
struct hid_keyboard_report report;

// レポートを受信したと仮定

// Shiftキーが押されているか確認
bool shift_pressed = (report.modifier & (HID_MOD_LEFT_SHIFT | HID_MOD_RIGHT_SHIFT)) != 0;

// Ctrlキーが押されているか確認
bool ctrl_pressed = (report.modifier & (HID_MOD_LEFT_CTRL | HID_MOD_RIGHT_CTRL)) != 0;

// 押されているキーをスキャン
for (int i = 0; i < 6; i++) {
    uint8_t keycode = report.keycode[i];
    if (keycode == 0) {
        continue; // 空のスロット
    }
    
    // キーコードを処理
    // 例: 0x04 (a) -> 'a' または 'A' (shiftによる)
}
```

## Boot Protocol vs Report Protocol

### Boot Protocol
- **シンプル**: 固定8バイトフォーマット
- **互換性**: BIOSでも動作可能
- **制限**: 基本的な機能のみ（マルチメディアキーなどは非対応）
- **用途**: システム起動時やOSロード前

### Report Protocol
- **柔軟**: カスタマイズ可能なレポートフォーマット
- **高機能**: マルチメディアキー、LEDなど全機能サポート
- **複雑**: Report Descriptorの解析が必要
- **用途**: OS完全起動後

現在の実装では、Boot Protocolのみをサポートしています。

## SET_PROTOCOL の実行

Boot Protocolを設定するには、以下のコントロール転送を実行します：

```c
// SET_PROTOCOL (Boot Protocol)
struct usb_setup_packet setup;
setup.bmRequestType = 0x21; // Class, Interface, Host-to-Device
setup.bRequest = HID_REQ_SET_PROTOCOL;
setup.wValue = HID_PROTOCOL_BOOT;
setup.wIndex = interface_number;
setup.wLength = 0;

xhci_control_transfer(hc, slot_id, 
    setup.bmRequestType, setup.bRequest,
    setup.wValue, setup.wIndex,
    NULL, 0);
```

## キーリピート処理

キーボードドライバは、前回のレポートと比較して新しいキー押下を検出する必要があります：

```c
static uint8_t previous_keys[6] = {0};

void process_keyboard_report(struct hid_keyboard_report *report) {
    // 新しく押されたキーを検出
    for (int i = 0; i < 6; i++) {
        uint8_t keycode = report->keycode[i];
        if (keycode == 0) continue;
        
        // 前回押されていなかったキーか確認
        bool was_pressed = false;
        for (int j = 0; j < 6; j++) {
            if (previous_keys[j] == keycode) {
                was_pressed = true;
                break;
            }
        }
        
        if (!was_pressed) {
            // 新しいキー押下を処理
            handle_key_press(keycode, report->modifier);
        }
    }
    
    // 前回の状態を更新
    memcpy(previous_keys, report->keycode, 6);
}
```