#ifndef BLUETOOTH_HID_DEVICE_H
#define BLUETOOTH_HID_DEVICE_H

#include <Arduino.h>
#include "DuckyScriptParser.h"

class BleKeyboard;

class BluetoothHIDDevice : public HIDDevice {
private:
    BleKeyboard* bleKeyboard;
    HIDMode currentMode;
    bool deviceConnected;
    String deviceName;
    bool isInitializing;
    bool isShuttingDown;
    unsigned long initStartTime;
    
public:
    BluetoothHIDDevice();
    ~BluetoothHIDDevice();
    bool begin(const String& name);
    void end();
    void setMode(HIDMode mode);
    
    // HIDDevice interface implementation
    void sendKey(uint8_t key, uint8_t modifiers = 0) override;
    void sendString(const String& text) override;
    void sendKeySequence(const String& keys) override;
    void delay(uint32_t ms) override;
    bool isConnected() override;
    
    // Bluetooth specific
    void handleConnection();
    String getDeviceName() { return deviceName; }
};

#endif // BLUETOOTH_HID_DEVICE_H