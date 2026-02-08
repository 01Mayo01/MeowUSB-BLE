# Changelog

## v0.2.5
- **Fix:** Fixed USB "instability" by removing the optimistic connection flag in `ARDUINO_USB_STARTED_EVENT`. The device now waits for explicit host enumeration (`tud_mounted()`) or Resume events before considering itself connected.
- **Fix:** Handled USB Suspend/Resume events to correctly update connection status when the host sleeps or wakes up.

## v0.2.4
- **Fix:** Fixed USB connection inconsistency by strictly relying on `tud_mounted()` for connection state checks. This prevents the device from attempting to send keystrokes before the host has fully enumerated the USB HID device.
- **Fix:** Added `releaseAll()` before every USB key transmission to ensure a clean state.

## v0.2.3
- **Fix:** Addressed Bluetooth mode switching instability. Now explicitly stops advertising before applying security/power settings and restarts it, ensuring proper BLE stack configuration.
- **Improvement:** Optimized BLE initialization timing to reduce main loop blocking.

## v0.2.2
- **Fix:** Reduced directory file limit to 100 to prevent freezes on large folders.
- **Fix:** Added explicit `file.close()` in scanning loop to prevent file handle leaks.
- **Fix:** Increased directory scanning delay to 10ms for better WDT compliance.

## v0.2.1
- **Fix:** Fixed firmware freeze when opening directories with many files. Added `delay(1)` yield during directory scanning to prevent Watchdog Timer (WDT) resets.
- **Improvement:** Implemented a safety limit of 250 files per directory to prevent Out-Of-Memory (OOM) crashes.

## v0.2.0
- **Feature:** Major Bluetooth HID reliability upgrade. Implemented `pressRaw()` in `BleKeyboard` library to support Atomic HID Reports.
- **Fix:** Fixed `GUI` (Windows) command failure by sending Modifiers and Keys in a single report, eliminating state desynchronization issues on Windows hosts.
- **Improvement:** Reduced HID report chatter by consolidating separate modifier and key events into atomic operations.

## v0.1.10
- **Fix:** Enhanced Bluetooth HID `GUI` command reliability. Added `releaseAll()` call before every key sequence to ensure clean HID state.
- **Fix:** Improved DuckyScript parameter parsing by trimming extra whitespace from parameters.
- **Debug:** Added verbose serial logging for BLE modifier and key press events to assist in troubleshooting.

## v0.1.9
- **Fix:** Addressed inconsistent `GUI` command behavior in Bluetooth mode by significantly increasing timing delays.
- **Improvement:** Increased modifier press delay (20ms -> 100ms), key hold time (60ms -> 100ms), and release delay (30ms -> 100ms) to ensure reliable registration on all hosts.

## v0.1.8
- **Fix:** Fixed `GUI` (Windows/Command) key functionality in Bluetooth mode by using raw HID modifier codes (0x80-0x87) to eliminate library conflicts.
- **Improvement:** Increased key hold timing (60ms) and added explicit delays (20ms) between modifier press and key press to ensure reliable registration on slower Bluetooth hosts.

## v0.1.7
- **Fix:** Fixed DuckyScript `GUI` command handling in Bluetooth mode. Previously, commands like `GUI r` were not correctly combining the modifier and the key. Updated parser to strictly associate parameters with the GUI modifier.

## v0.1.6
- **Fix:** Re-enabled standard Bluetooth Security (Bonding/Encryption) with "Just Works" pairing. This fixes disconnection loops on hosts (Windows/Android) that require secure HID connections.
- **Fix:** Set Bluetooth Tx Power to Maximum (9dBm) to rule out signal integrity issues.
- **Change:** Updated device name suffix to "-v6" to force hosts to re-enumerate services and clear stale cache.

## v0.1.5
- **Fix:** Fixed rapid Bluetooth disconnect/reconnect loops by forcing "No Security" mode to bypass complex bonding requirements that fail on some hosts.
- **Fix:** Adjusted BLE advertising intervals to be less aggressive (20-40ms), improving compatibility with Android and Windows.
- **Improvement:** Removed battery service updates to prevent potential conflicts.

## v0.1.4
- **Fix:** Fixed Bluetooth reconnection loops by relaxing connection interval requirements and allowing the host (PC/Phone) to negotiate its preferred parameters.
- **Fix:** Removed hardcoded "Min Preferred" advertising settings which were causing some hosts to reject the connection.

## v0.1.3
- **Fix:** Improved Bluetooth connection stability by adjusting advertising parameters (min preferred connection intervals) to better support Apple and modern Windows devices.
- **Fix:** Increased BLE stack stabilization delay during initialization.

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
