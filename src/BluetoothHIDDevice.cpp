#include "BluetoothHIDDevice.h"
#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#endif
#include <BleKeyboard.h>

// Explicitly define modifier keys to avoid library conflicts
#define RAW_KEY_LEFT_CTRL   0x80
#define RAW_KEY_LEFT_SHIFT  0x81
#define RAW_KEY_LEFT_ALT    0x82
#define RAW_KEY_LEFT_GUI    0x83
#define RAW_KEY_RIGHT_CTRL  0x84
#define RAW_KEY_RIGHT_SHIFT 0x85
#define RAW_KEY_RIGHT_ALT   0x86
#define RAW_KEY_RIGHT_GUI   0x87

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
    // Use 100% battery to prevent some hosts from querying it constantly
    // Force a new name to ensure fresh enumeration on host
    String finalName = deviceName + "-v6"; 
    bleKeyboard = new BleKeyboard(finalName.c_str(), "MeowCorp", 100);
    
    // IMPORTANT: Delay to allow object creation to settle
    delay(100);

    // Initialize with comprehensive error handling
    bool success = false;
    try {
        bleKeyboard->begin();
        
        // Wait for BLE stack to stabilize
        delay(100);
        
        // STOP Advertising to apply settings safely
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        if (pAdvertising) {
            pAdvertising->stop();
        }

        // Adjust advertising settings for better connection reliability
        if (pAdvertising) {
            // Force advertising response to be true to help discovery
            pAdvertising->setScanResponse(true);
            // Increase advertising interval to be less aggressive (helps with some Androids/Windows)
            pAdvertising->setMinInterval(32); // 20ms
            pAdvertising->setMaxInterval(64); // 40ms
        }
        
        // RE-ENABLE Security: HID devices usually REQUIRE bonding/encryption
        // Set to "Just Works" mode (No Input No Output)
        NimBLEDevice::setSecurityAuth(true, true, true); // Auth, Encrypt, Bond
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        
        // Set Tx Power to Maximum (ESP_PWR_LVL_P9 = 9dBm)
        NimBLEDevice::setPower(ESP_PWR_LVL_P9); 
        
        // Restart Advertising with new settings
        if (pAdvertising) {
            pAdvertising->start();
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
    if (isStarted) {
        if (bleKeyboard) {
            bleKeyboard->end(); 
            delay(500); // Allow tasks to finish
            
            // NimBLE cleanup to free radio for WiFi
            #if defined(USE_NIMBLE)
            // Check if initialized before deinit to avoid crashes
            if (NimBLEDevice::getInitialized()) {
                NimBLEDevice::deinit(true);
            }
            #endif
            
            delete bleKeyboard;
            bleKeyboard = nullptr;
        }
        isStarted = false;
        Serial.println("BLE Keyboard stopped and de-initialized");
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
        // Use atomic pressRaw method to send Modifiers + Key in a single HID report
        // This avoids issues where modifiers are sent separately or timing issues occur
        
        Serial.println("BLE: Atomic sendKey: " + String(key, HEX) + " Mods: " + String(modifiers, HEX));
        bleKeyboard->pressRaw(key, modifiers);
        
        // Hold for a moment - increased for reliability
        delay(100);
        
        // Release everything
        bleKeyboard->releaseAll();
        delay(100);
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