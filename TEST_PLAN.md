# Test Plan

## 1. USB Functionality

### Test Case 1.1: Detection
1.  **Setup:** Device powered off.
2.  **Action:** Plug device into PC via USB-C.
3.  **Expected Result:**
    *   Device boots up.
    *   Main menu shows "USB (Rdy)" in blue/pink.
    *   PC recognizes "MeowUSB-BLE" as a Keyboard.

### Test Case 1.2: Hot Plug
1.  **Setup:** Device on battery power (if applicable) or USB.
2.  **Action:** Unplug USB, wait 5 seconds, plug back in.
3.  **Expected Result:**
    *   Status changes from "(Rdy)" to "(Wait)" or "(Disc)" and back to "(Rdy)" reliably.

## 2. Bluetooth Stability

### Test Case 2.1: Pairing
1.  **Setup:** Device in USB mode.
2.  **Action:** Press `Tab` to switch to BLE.
3.  **Expected Result:**
    *   Screen shows "Bluetooth booting...".
    *   Screen shows "Bluetooth ready!".
    *   PC/Phone can discover "MeowUSB-BLE".
    *   Pairing succeeds.

### Test Case 2.2: Mode Switching (Stress Test)
1.  **Setup:** Device connected via BLE.
2.  **Action:** Press `Tab` to switch to USB, wait 2 seconds, Press `Tab` to switch back to BLE.
3.  **Repeat:** Do this 10 times.
4.  **Expected Result:**
    *   Device **DOES NOT CRASH**.
    *   Bluetooth reconnects automatically (or is available) each time.

## 3. DuckyScript Execution

### Test Case 3.1: Hello World
1.  **Payload:**
    ```
    DELAY 1000
    STRING Hello World
    ENTER
    ```
2.  **Action:** Execute payload via menu.
3.  **Expected Result:**
    *   Target opens text editor (focus required).
    *   "Hello World" types out.
    *   New line is entered.
    *   No missing characters.

### Test Case 3.2: Modifiers
1.  **Payload:**
    ```
    GUI r
    DELAY 500
    STRING notepad
    ENTER
    ```
2.  **Action:** Execute.
3.  **Expected Result:**
    *   Run dialog opens (Windows) or Spotlight (Mac - adjust key as needed).
    *   "notepad" is typed.
    *   Notepad opens.

### Test Case 3.3: Speed Test
1.  **Payload:** Long string of text (50+ chars).
2.  **Action:** Execute.
3.  **Expected Result:** All characters appear. No dropped keys.

## 4. System Verification

### Test Case 4.1: Version Check
1.  **Action:** Look at top right of screen.
2.  **Expected Result:** Battery % displayed, "v0.0.5" displayed below it.
