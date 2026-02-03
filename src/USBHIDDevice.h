#ifndef MEOW_USB_DEVICE_H
#define MEOW_USB_DEVICE_H

#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>
#include <USBHIDKeyboard.h>
#include "DuckyScriptParser.h"

class MeowUSBDevice : public HIDDevice {
private:
    USBHIDKeyboard Keyboard;
    HIDMode currentMode;
    volatile bool deviceConnected;
    
    static MeowUSBDevice* instance;
    static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    
    // Keyboard report
    uint8_t keyReport[8];
    
    void sendKeyboardReport();
    
public:
    MeowUSBDevice();
    bool begin();
    void setMode(HIDMode mode);
    
    // HIDDevice interface implementation
    void sendKey(uint8_t key, uint8_t modifiers = 0) override;
    void sendString(const String& text) override;
    void sendKeySequence(const String& keys) override;
    void moveMouse(int8_t x, int8_t y) override;
    void clickMouse(uint8_t buttons) override;
    void delay(uint32_t ms) override;
    bool isConnected() override;
    
    void setConnected(bool connected) { deviceConnected = connected; }
};

#endif // MEOW_USB_DEVICE_H