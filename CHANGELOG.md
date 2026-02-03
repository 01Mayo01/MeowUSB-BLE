# Changelog

## v0.1.2
- **Fix:** Fixed Bluetooth firmware crashes caused by Watchdog Timer (WDT) timeouts during long string transmission. Added chunked sending with explicit task yielding.

## v0.1.1
- **Fix:** Switched Bluetooth stack to NimBLE-Arduino. This resolves persistent crash/instability issues associated with the default Bluedroid stack on ESP32-S3.
- **Improvement:** Reduced memory usage and improved BLE connection speed.

## v0.1
- **Fix:** Fixed critical Bluetooth stability issues. Implemented robust state tracking to prevent BLE stack re-initialization crashes.
- **Fix:** Bluetooth mode switching is now fully stable.

## v0.0.9
- **Fix:** Fixed DuckyScript parsing for commands with tabs or multiple spaces (e.g., `GUI<tab>r`). Added robust whitespace splitting logic.
- **Removed:** Removed all Mouse HID functionality to streamline the codebase and focus on Keyboard operations.
- **Debug:** Added detailed serial debug logging for HID key and modifier events.

## v0.0.8
- **Fix:** Fixed critical issue where `GUI` command (and other modifiers) would incorrectly trigger multiple modifier keys simultaneously due to bitmask overlap. Updated `DuckyScriptParser` to use unique bit flags for internal modifier tracking.

## v0.0.7
- **Fix:** Added explicit support for `GUI`, `WINDOWS`, `COMMAND`, and `ENTER` in DuckyScript parser to ensure reliable execution.

## v0.0.6
- **Build:** Maintenance release with updated filename convention.

## v0.0.5
- **Fix:** Increased USB HID string transmission delay to 20ms for better reliability on slower hosts.
- **Docs:** Added DIAGNOSTICS.md and TEST_PLAN.md.

## v0.0.4
- **Fix:** **CRITICAL** Bluetooth stability fix. Refactored `BluetoothHIDDevice` to use a single-instance pattern. The `BleKeyboard` instance is now preserved across mode switches to prevent heap fragmentation and crashes.
- **Fix:** Added 20ms delay to Bluetooth string transmission.
- **Fix:** Improved DuckyScript delay handling.

## v0.0.3
- **Fix:** Fixed HID report timing. Added 20ms hold time for keys and 20ms recovery time after release.
- **Fix:** Fixed issue where commands ran on device but did nothing on target.
- **Update:** Incremented version display.

## v0.0.2
- **Fix:** Fixed `Tab` key detection for mode switching. Changed from `isKeyPressed(KEY_TAB)` (macro conflict) to `keysState().tab`.
- **Update:** Version bump.

## v0.0.1
- **Feature:** Initial Release.
- **Fix:** Enabled TinyUSB (`ARDUINO_USB_MODE=0`) for proper HID support.
- **Fix:** Added `FW_VERSION` display to UI.
- **Fix:** Basic DuckyScript parsing improvements.
