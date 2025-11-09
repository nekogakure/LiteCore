この章では、USBキーボードドライバについて記述します。
実装されているファイルは[usb_keyboard.c](../../../src/kernel/driver/usb/usb_keyboard.c)です。

## 概要

このドライバは、HID Boot ProtocolキーボードをサポートするUSBキーボードの入力処理を提供します。xHCIドライバと連携して、Interrupt IN Transferによるキーボードレポートの取得とASCII文字への変換を行います。

主な機能：
- HID Boot Protocolキーボードレポートの解析
- USBスキャンコードからASCII文字への変換
- 修飾キー（Shift、Ctrl、Alt）の処理
- キーリピート防止（前回キーとの差分検出）
- 循環バッファによる入力キューイング

## 関数 / API

### `void usb_keyboard_init(void)`
USBキーボードドライバを初期化します。

**引数**: なし

**戻り値**: なし

**詳細**:
- 入力バッファを初期化
- 前回のキー状態をクリア
- xHCIコントローラを初期化

**呼び出し元**: `kernel_main()`

---

### `char usb_keyboard_getchar(void)`
USBキーボードから1文字読み取ります（非ブロッキング）。

**引数**: なし

**戻り値**:
- 成功: 読み取った文字
- 失敗: `0`（入力バッファが空）

**詳細**:
- バッファに文字がある場合は即座に返す
- バッファが空の場合は0を返す（ブロックしない）

**使用例**:
```c
char c = usb_keyboard_getchar();
if (c != 0) {
    console_putchar(c);
}
```

---

### `void usb_keyboard_poll(void)`
USBキーボードをポーリングして入力を取得します。

**引数**: なし

**戻り値**: なし

**詳細**:
- xHCIドライバの`xhci_poll_keyboard()`を呼び出す
- 新しいキーボードレポートがあれば`process_keyboard_report()`で処理
- 継続的にInterrupt IN Transferを送信して入力を監視

**呼び出し元**: `shell_run()`のメインループ

**注意**: この関数は定期的に呼び出す必要があります（ポーリングベース）

---

### `static void process_keyboard_report(struct hid_keyboard_report *report)`
HIDキーボードレポートを解析して文字を入力バッファに追加します。

**引数**:
- `report`: HIDキーボードレポート（8バイト）

**戻り値**: なし

**詳細**:
1. 修飾キー（Shift、Ctrl、Alt）を検出
2. 前回のキー状態と比較して新しいキー押下を検出
3. 新しいキーのみを処理（リピート防止）
4. スキャンコードをASCII文字に変換
5. 特殊処理（Ctrl+C、Backspaceなど）
6. 入力バッファに文字を追加

**キーリピート防止**:
```c
static uint8_t previous_keys[6] = {0};

// 新しく押されたキーのみを検出
for (int i = 0; i < 6; i++) {
    uint8_t keycode = report->keycode[i];
    if (keycode == 0) continue;
    
    bool was_pressed = false;
    for (int j = 0; j < 6; j++) {
        if (previous_keys[j] == keycode) {
            was_pressed = true;
            break;
        }
    }
    
    if (!was_pressed) {
        // 新しいキー押下を処理
    }
}

// 状態を更新
memcpy(previous_keys, report->keycode, 6);
```

---

### `static char usb_scancode_to_ascii(uint8_t scancode, bool shift)`
USBスキャンコードをASCII文字に変換します。

**引数**:
- `scancode`: USBキーコード（HID Usage Tables）
- `shift`: Shiftキーが押されているか

**戻り値**:
- 成功: ASCII文字
- 失敗: `0`（変換不可能なキー）

**対応スキャンコード**:
- `0x04-0x1D`: a-z
- `0x1E-0x27`: 1-0
- `0x2C`: Space
- `0x28`: Enter
- `0x2A`: Backspace
- `0x2D-0x38`: 記号キー（-=[]など）

**Shift処理**:
- アルファベット: 小文字↔大文字
- 数字: 記号（1→!、2→@など）
- 記号: 対応する記号（,→<、.→>など）

**使用例**:
```c
uint8_t keycode = 0x04; // 'a'
bool shift = true;
char c = usb_scancode_to_ascii(keycode, shift); // 'A'
```

---

### `static void usb_buffer_put(char c)`
文字を入力バッファに追加します。

**引数**:
- `c`: 追加する文字

**戻り値**: なし

**詳細**:
- 循環バッファに文字を格納
- バッファフル時は古い文字を上書き

---

### `static char usb_buffer_get(void)`
入力バッファから文字を取得します。

**引数**: なし

**戻り値**:
- 成功: バッファから取得した文字
- 失敗: `0`（バッファが空）

**詳細**:
- バッファから1文字を取り出す
- FIFO方式（先入れ先出し）

## 内部状態

### 入力バッファ
```c
#define USB_BUFFER_SIZE 64
static char usb_buffer[USB_BUFFER_SIZE];
static int usb_buffer_head = 0; // 書き込み位置
static int usb_buffer_tail = 0; // 読み取り位置
```

循環バッファで実装されており、最大64文字まで保持できます。

### 前回のキー状態
```c
static uint8_t previous_keys[6] = {0};
```

キーリピートを防止するため、前回のレポートで押されていたキーを記憶します。

## 修飾キーの処理

### Shiftキー
アルファベットと記号の変換に影響します：

| キー | 通常 | Shift |
|------|------|-------|
| a-z  | 小文字 | 大文字 |
| 1    | 1    | !     |
| 2    | 2    | @     |
| ,    | ,    | <     |
| .    | .    | >     |

### Ctrlキー
特殊な制御文字を生成します：

| キー | 動作 |
|------|------|
| Ctrl+C | 割り込み（将来実装） |
| Ctrl+D | EOF（将来実装） |
| Ctrl+Z | サスペンド（将来実装） |

現在の実装では、Ctrlキーは無視されます（将来の拡張用）。

### Altキー
現在の実装では、Altキーは無視されます（将来の拡張用）。

## 特殊キーの処理

### Backspace（0x2A）
特殊文字`\b`（0x08）として処理され、シェルで1文字削除に使用されます。

```c
if (c == '\b') {
    if (cmd_length > 0) {
        cmd_length--;
        console_putchar('\b');
        console_putchar(' ');
        console_putchar('\b');
    }
}
```

### Enter（0x28）
改行文字`\n`として処理され、コマンドの実行をトリガーします。

## キーボードレポート処理フロー

```
1. usb_keyboard_poll() 呼び出し
   ↓
2. xhci_poll_keyboard() でレポート取得
   ↓
3. Transfer Event待機
   ↓
4. レポート受信成功
   ↓
5. process_keyboard_report() 呼び出し
   ↓
6. 修飾キー検出（Shift、Ctrl、Alt）
   ↓
7. 前回キーとの差分検出
   ↓
8. 新しいキーのみ処理
   ↓
9. スキャンコード→ASCII変換
   ↓
10. 入力バッファに追加
    ↓
11. 次のInterrupt IN Transfer送信
    ↓
12. 1に戻る（継続ポーリング）
```

## 使用例

### シェルでの入力処理

```c
void shell_run(void) {
    char cmd[256];
    int cmd_length = 0;
    
    while (1) {
        // USBキーボードをポーリング
        usb_keyboard_poll();
        
        // 入力を取得
        char c = usb_keyboard_getchar();
        if (c == 0) {
            continue; // 入力なし
        }
        
        if (c == '\n') {
            // コマンド実行
            cmd[cmd_length] = '\0';
            execute_command(cmd);
            cmd_length = 0;
        } else if (c == '\b') {
            // Backspace処理
            if (cmd_length > 0) {
                cmd_length--;
                console_putchar('\b');
                console_putchar(' ');
                console_putchar('\b');
            }
        } else {
            // 文字追加
            cmd[cmd_length++] = c;
            console_putchar(c);
        }
    }
}
```