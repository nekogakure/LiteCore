この章では、USB プロトコルの定義について記述します。
実装されているファイルは[usb.h](../../../src/include/driver/usb/usb.h)です。

## 概要

このヘッダファイルは、USBプロトコルの標準定義を提供します。USB標準リクエスト、ディスクリプタタイプ、デバイスクラスコードなどが定義されています。

## 構造体

### `struct usb_setup_packet`
USBコントロール転送のSetup Packetを表す構造体です。

- `uint8_t bmRequestType`: リクエストタイプ（Direction、Type、Recipient）
- `uint8_t bRequest`: リクエスト番号
- `uint16_t wValue`: 値
- `uint16_t wIndex`: インデックス
- `uint16_t wLength`: データ長

### `struct usb_device_descriptor`
USB Device Descriptorを表す構造体です。

- `uint8_t bLength`: ディスクリプタ長（18バイト）
- `uint8_t bDescriptorType`: ディスクリプタタイプ（0x01）
- `uint16_t bcdUSB`: USB仕様番号（BCD）
- `uint8_t bDeviceClass`: デバイスクラス
- `uint8_t bDeviceSubClass`: デバイスサブクラス
- `uint8_t bDeviceProtocol`: デバイスプロトコル
- `uint8_t bMaxPacketSize0`: EP0の最大パケットサイズ
- `uint16_t idVendor`: ベンダーID
- `uint16_t idProduct`: プロダクトID
- `uint16_t bcdDevice`: デバイスリリース番号（BCD）
- `uint8_t iManufacturer`: 製造者文字列のインデックス
- `uint8_t iProduct`: 製品文字列のインデックス
- `uint8_t iSerialNumber`: シリアル番号文字列のインデックス
- `uint8_t bNumConfigurations`: Configuration数

### `struct usb_config_descriptor`
USB Configuration Descriptorを表す構造体です。

- `uint8_t bLength`: ディスクリプタ長（9バイト）
- `uint8_t bDescriptorType`: ディスクリプタタイプ（0x02）
- `uint16_t wTotalLength`: Configurationの総長
- `uint8_t bNumInterfaces`: Interface数
- `uint8_t bConfigurationValue`: Configuration値
- `uint8_t iConfiguration`: Configuration文字列のインデックス
- `uint8_t bmAttributes`: 属性（Self-powered、Remote Wakeupなど）
- `uint8_t bMaxPower`: 最大消費電力（2mA単位）

### `struct usb_interface_descriptor`
USB Interface Descriptorを表す構造体です。

- `uint8_t bLength`: ディスクリプタ長（9バイト）
- `uint8_t bDescriptorType`: ディスクリプタタイプ（0x04）
- `uint8_t bInterfaceNumber`: Interface番号
- `uint8_t bAlternateSetting`: Alternate Setting番号
- `uint8_t bNumEndpoints`: Endpoint数（EP0を除く）
- `uint8_t bInterfaceClass`: Interfaceクラス
- `uint8_t bInterfaceSubClass`: Interfaceサブクラス
- `uint8_t bInterfaceProtocol`: Interfaceプロトコル
- `uint8_t iInterface`: Interface文字列のインデックス

### `struct usb_endpoint_descriptor`
USB Endpoint Descriptorを表す構造体です。

- `uint8_t bLength`: ディスクリプタ長（7バイト）
- `uint8_t bDescriptorType`: ディスクリプタタイプ（0x05）
- `uint8_t bEndpointAddress`: エンドポイントアドレス（Direction + Number）
- `uint8_t bmAttributes`: 属性（Transfer Type）
- `uint16_t wMaxPacketSize`: 最大パケットサイズ
- `uint8_t bInterval`: ポーリング間隔

## 定数 / 定義

### USB Request Types
- `USB_DIR_OUT`: Host-to-Device (0x00)
- `USB_DIR_IN`: Device-to-Host (0x80)
- `USB_TYPE_STANDARD`: Standard Request (0x00)
- `USB_TYPE_CLASS`: Class Request (0x20)
- `USB_TYPE_VENDOR`: Vendor Request (0x40)
- `USB_RECIP_DEVICE`: デバイス宛 (0x00)
- `USB_RECIP_INTERFACE`: インターフェース宛 (0x01)
- `USB_RECIP_ENDPOINT`: エンドポイント宛 (0x02)

### USB Standard Requests
- `USB_REQ_GET_STATUS`: Get Status (0x00)
- `USB_REQ_CLEAR_FEATURE`: Clear Feature (0x01)
- `USB_REQ_SET_FEATURE`: Set Feature (0x03)
- `USB_REQ_SET_ADDRESS`: Set Address (0x05)
- `USB_REQ_GET_DESCRIPTOR`: Get Descriptor (0x06)
- `USB_REQ_SET_DESCRIPTOR`: Set Descriptor (0x07)
- `USB_REQ_GET_CONFIGURATION`: Get Configuration (0x08)
- `USB_REQ_SET_CONFIGURATION`: Set Configuration (0x09)
- `USB_REQ_GET_INTERFACE`: Get Interface (0x0A)
- `USB_REQ_SET_INTERFACE`: Set Interface (0x0B)

### Descriptor Types
- `USB_DT_DEVICE`: Device Descriptor (0x01)
- `USB_DT_CONFIG`: Configuration Descriptor (0x02)
- `USB_DT_STRING`: String Descriptor (0x03)
- `USB_DT_INTERFACE`: Interface Descriptor (0x04)
- `USB_DT_ENDPOINT`: Endpoint Descriptor (0x05)

### USB Device Class Codes
- `USB_CLASS_AUDIO`: Audio (0x01)
- `USB_CLASS_COMM`: Communications and CDC Control (0x02)
- `USB_CLASS_HID`: Human Interface Device (0x03)
- `USB_CLASS_PRINTER`: Printer (0x07)
- `USB_CLASS_MASS_STORAGE`: Mass Storage (0x08)
- `USB_CLASS_HUB`: Hub (0x09)
- `USB_CLASS_DATA`: CDC-Data (0x0A)
- `USB_CLASS_VENDOR_SPEC`: Vendor Specific (0xFF)

### HID Subclass Codes
- `HID_SUBCLASS_NONE`: No Subclass (0x00)
- `HID_SUBCLASS_BOOT`: Boot Interface Subclass (0x01)

### HID Protocol Codes
- `HID_PROTOCOL_NONE`: None (0x00)
- `HID_PROTOCOL_KEYBOARD`: Keyboard (0x01)
- `HID_PROTOCOL_MOUSE`: Mouse (0x02)

### HID Class Requests
- `HID_REQ_GET_REPORT`: Get Report (0x01)
- `HID_REQ_GET_IDLE`: Get Idle (0x02)
- `HID_REQ_GET_PROTOCOL`: Get Protocol (0x03)
- `HID_REQ_SET_REPORT`: Set Report (0x09)
- `HID_REQ_SET_IDLE`: Set Idle (0x0A)
- `HID_REQ_SET_PROTOCOL`: Set Protocol (0x0B)

## 使用例

### Setup Packetの構築

```c
struct usb_setup_packet setup;

// GET_DESCRIPTOR (Device)
setup.bmRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
setup.bRequest = USB_REQ_GET_DESCRIPTOR;
setup.wValue = (USB_DT_DEVICE << 8) | 0;
setup.wIndex = 0;
setup.wLength = 18;

// SET_CONFIGURATION
setup.bmRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
setup.bRequest = USB_REQ_SET_CONFIGURATION;
setup.wValue = config_value;
setup.wIndex = 0;
setup.wLength = 0;

// SET_PROTOCOL (Boot Protocol)
setup.bmRequestType = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
setup.bRequest = HID_REQ_SET_PROTOCOL;
setup.wValue = 0; // Boot Protocol
setup.wIndex = interface_number;
setup.wLength = 0;
```