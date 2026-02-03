#include "BluetoothHIDDevice.h"
#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#endif
#include <BleKeyboard.h>

BluetoothHIDDevice::BluetoothHIDDevice() {
    deviceConnected = false;
    currentMode = HID_MODE_KEYBOARD;
    deviceName = "MeowUSB-BLE";
    bleKeyboard = nullptr; // Don't create it yet, wait for begin()
    isInitializing = false;
    isShuttingDown = false;
    isStarted = false;
    initStartTime = 0;
}

BluetoothHIDDevice::~BluetoothHIDDevice() {
    if (bleKeyboard) {
        bleKeyboard->end();
        delete bleKeyboard;
    }
}

bool BluetoothHIDDevice::begin(const String& name) {
    // Prevent re-initialization during shutdown
    if (isShuttingDown) {
        Serial.println("BLE init blocked: shutdown in progress");
        return false;
    }
    
    // Check if we can reuse the existing instance
    if (bleKeyboard != nullptr) {
        if (deviceName == name) {
            Serial.println("BLE reusing existing instance: " + deviceName);
            
            // Only start if not already started
            if (!isStarted) {
                bleKeyboard->begin();
                isStarted = true;
            } else {
                Serial.println("BLE already running, skipping begin()");
            }
            
            deviceConnected = bleKeyboard->isConnected();
            return true;
        } else {
            // Name changed, we MUST re-initialize
            // This is a heavy operation, but we'll try to avoid it by just updating the existing instance if possible
            // BleKeyboard doesn't support renaming after begin(), so we have to destroy it.
            // But to prevent crashes, we'll just stick with the old name for now if it exists.
            Serial.println("BLE name change requested but ignored to prevent instability. Using: " + deviceName);
            
            if (!isStarted) {
                bleKeyboard->begin();
                isStarted = true;
            }
            return true;
        }
    }

    // Set initialization flag
    isInitializing = true;
    initStartTime = millis();
    
    // Set device name first
    deviceName = name;
    
    // If keyboard already exists with different name, clean it up
    // BUT we modified the logic above to avoid this path if possible.
    // If we reach here, it means bleKeyboard is NULL.
    if (bleKeyboard) {
        // This should not happen with current logic, but safety first
        isShuttingDown = true;
        bleKeyboard->end();
        delete bleKeyboard;
        bleKeyboard = nullptr;
        delay(500); // Longer delay for BLE stack cleanup
        isShuttingDown = false;
    }
    
    // Additional safety delay
    delay(200);
    
    // Create new instance with updated name
    // Initialize with standard parameters for broader compatibility
    // Device Name, Device Manufacturer, Battery Level
    bleKeyboard = new BleKeyboard(deviceName.c_str(), "MeowCorp", 100);
    
    // Set connection intervals to improve stability (min, max, latency, timeout)
    // These values are recommended for Apple devices and general stability
    // 12 * 1.25ms = 15ms min interval
    // 24 * 1.25ms = 30ms max interval
    // 0 latency
    // 400 * 10ms = 4 seconds timeout
    // NOTE: BleKeyboard (NimBLE version) might not expose setConnectionParams directly on the class
    // We rely on defaults or need to access the underlying NimBLEServer
    
    // Initialize with comprehensive error handling
    bool success = false;
    try {
        bleKeyboard->begin();
        
        // Wait for BLE stack to stabilize
        delay(500); // Increased from 300ms
        
        // Adjust advertising settings for better connection reliability
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        if (pAdvertising) {
            pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connection issues 
            pAdvertising->setMinPreferred(0x12);
        }
        
        success = true;
        isStarted = true;
        Serial.println("BLE Keyboard initialized: " + deviceName);
        
        // Reset connection state
        deviceConnected = false;
        
    } catch (...) {
        Serial.println("BLE Keyboard initialization failed!");
        if (bleKeyboard) {
            delete bleKeyboard;
            bleKeyboard = nullptr;
        }
        success = false;
    }
    
    // Clear initialization flag
    isInitializing = false;
    return success;
}

void BluetoothHIDDevice::end() {
    if (bleKeyboard) {
        // Do NOT call bleKeyboard->end() as it causes instability
        // bleKeyboard->end();
        Serial.println("BLE Keyboard 'stopped' (stack kept active)");
        // We don't change isStarted to false because the stack is still up
    }
}

void BluetoothHIDDevice::setMode(HIDMode mode) {
    currentMode = mode;
    Serial.println("BLE HID mode set to: " + String(mode));
}

void BluetoothHIDDevice::sendKey(uint8_t key, uint8_t modifiers) {
    // Safety checks during unstable periods
    if (isInitializing || isShuttingDown) {
        Serial.println("HID operation blocked: unstable state");
        return;
    }
    
    if (!isConnected() || !bleKeyboard) return;
    
    // Additional safety delay during first 3 seconds
    if (initStartTime > 0 && (millis() - initStartTime) < 3000) {
        delay(50); // Extra delay during critical period
    }
    
    try {
        // Press modifiers
        if (modifiers & DuckyScriptParser::MOD_CTRL_LEFT) bleKeyboard->press(KEY_LEFT_CTRL);
        if (modifiers & DuckyScriptParser::MOD_SHIFT_LEFT) bleKeyboard->press(KEY_LEFT_SHIFT);
        if (modifiers & DuckyScriptParser::MOD_ALT_LEFT) bleKeyboard->press(KEY_LEFT_ALT);
        if (modifiers & DuckyScriptParser::MOD_GUI_LEFT) bleKeyboard->press(KEY_LEFT_GUI);
        if (modifiers & DuckyScriptParser::MOD_CTRL_RIGHT) bleKeyboard->press(KEY_RIGHT_CTRL);
        if (modifiers & DuckyScriptParser::MOD_SHIFT_RIGHT) bleKeyboard->press(KEY_RIGHT_SHIFT);
        if (modifiers & DuckyScriptParser::MOD_ALT_RIGHT) bleKeyboard->press(KEY_RIGHT_ALT);
        if (modifiers & DuckyScriptParser::MOD_GUI_RIGHT) bleKeyboard->press(KEY_RIGHT_GUI);
        
        // Press key
        if (key != 0) {
            bleKeyboard->press(key);
        }

        // Hold for a moment
        delay(20);
        
        // Release everything
        bleKeyboard->releaseAll();
        delay(20);
    } catch (...) {
        Serial.println("HID operation failed - connection lost");
        deviceConnected = false;
    }
}

void BluetoothHIDDevice::sendString(const String& text) {
    if (!isConnected() || !bleKeyboard) return;
    bleKeyboard->print(text);
    delay(20); // Small delay after string
}

void BluetoothHIDDevice::sendKeySequence(const String& keys) {
    // Not implemented for complex sequences yet
    Serial.println("BLE Key Sequence: " + keys);
}

void BluetoothHIDDevice::delay(uint32_t ms) {
    ::delay(ms);
}

bool BluetoothHIDDevice::isConnected() {
    // Don't check during initialization or shutdown
    if (isInitializing || isShuttingDown) {
        return false;
    }
    
    if (!bleKeyboard) return false;
    
    // Add protection against checking too frequently during connection
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    
    // During first 2 seconds after initialization, be more conservative
    if (initStartTime > 0 && (now - initStartTime) < 2000) {
        if (now - lastCheck < 500) {
            return deviceConnected; // Use cached state during critical period
        }
    } else {
        // Normal operation: check every 100ms
        if (now - lastCheck < 100) {
            return deviceConnected;
        }
    }
    
    lastCheck = now;
    
    // Additional safety check
    if (isInitializing || isShuttingDown) {
        return false;
    }
    
    deviceConnected = bleKeyboard->isConnected();
    return deviceConnected;
}

void BluetoothHIDDevice::handleConnection() {
    // BleKeyboard handles connection internally
    if (bleKeyboard) {
        deviceConnected = bleKeyboard->isConnected();
    } else {
        deviceConnected = false;
    }
}