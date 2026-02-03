# MeowUSB-BLE: M5 Cardputer DuckyScript Executor

A firmware for M5 Cardputer that executes DuckyScript payloads via USB and Bluetooth, featuring a cat-themed interface.

## Features

- **USB HID Support**: Emulate keyboard and mouse over USB
- **Bluetooth HID Support**: Wireless payload execution
- **DuckyScript Parser**: Compatible with standard DuckyScript commands
- **SD Card Storage**: Store and manage unlimited payloads
- **Web Interface**: Upload and manage payloads via WiFi
- **Cat-Themed UI**: Meow! 🐱

## Installation

1. Open the project in VS Code with PlatformIO extension
2. Connect your M5 Cardputer via USB
3. Click "Upload" to flash the firmware

## Usage

### Main Menu
- Use **Arrow Keys** to navigate the payload list
- Press **ENTER** to execute the selected payload via USB
- Press **Btn B** to execute via Bluetooth
- Press **Btn C** to enter Web Configuration mode

### Adding Payloads
1. **Via SD Card**: Save your `.txt` DuckyScript files to the `/payloads` folder on the SD card
2. **Via Web Interface**: 
   - Select "Web Config" from the main menu
   - Connect to WiFi AP "M5-Ducky" (Password: `ducky123`)
   - Open `http://192.168.4.1` in your browser
   - Upload your script files

### Supported DuckyScript Commands
- `REM`: Comment
- `DELAY [ms]`: Wait for specified milliseconds
- `STRING [text]`: Type text
- `STRINGLN [text]`: Type text and press Enter
- `KEY [key]`: Press a specific key (e.g., `KEY ENTER`, `KEY F1`)
- `KEYS [sequence]`: Press a key sequence
- `MOUSE_MOVE [x] [y]`: Move mouse cursor
- `MOUSE_CLICK [LEFT|RIGHT|MIDDLE]`: Click mouse button
- `DEFAULTDELAY [ms]`: Set default delay between commands

## Hardware Requirements
- M5Stack Cardputer (ESP32-S3)
- Micro SD Card (formatted FAT32)

## License
MIT
