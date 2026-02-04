# MeowUSB-BLE: M5 Cardputer DuckyScript Executor

Firmware for M5 Cardputer that executes DuckyScript payloads via USB and Bluetooth, featuring a cat-themed interface.

## Features

- **USB HID Support**: Emulate keyboard over USB
- **Bluetooth HID Support**: Wireless payload execution
- **DuckyScript Parser**: Compatible with standard DuckyScript commands
- **SD Card Storage**: Store and manage unlimited payloads
- **Cat-Themed UI**: Meeowwww :3

## Installation

Will be uploaded to M5 Burner

## Usage

### Main Menu
- Use **Arrow Keys** , **Enter Key**, and **ESC Key** to navigate the payload list
- Use **ENTER** to execute the selected payload via USB
- Use **TAB** to switch between USB and BLE


### Adding Payloads
1. **Via SD Card**: Save your `.txt` DuckyScript files to your SD card


### Supported DuckyScript Commands
- `REM`: Comment
- `DELAY [ms]`: Wait for specified milliseconds
- `STRING [text]`: Type text
- `STRINGLN [text]`: Type text and press Enter
- `KEY [key]`: Press a specific key (e.g., `KEY ENTER`, `KEY F1`)
- `KEYS [sequence]`: Press a key sequence
- `DEFAULTDELAY [ms]`: Set default delay between commands
- `GUI`: Emulates Windows key

## Hardware Requirements
- M5Stack Cardputer (ESP32-S3)
- Micro SD Card (formatted FAT32)

## License
MIT
