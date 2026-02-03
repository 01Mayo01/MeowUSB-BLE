#include "USBHIDDevice.h"

MeowUSBDevice* MeowUSBDevice::instance = nullptr;

void MeowUSBDevice::usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                Serial.println("USB Started");
                if (instance) instance->setConnected(true); // Optimistic: assume connected when started
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                Serial.println("USB Stopped");
                if (instance) instance->setConnected(false);
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                Serial.println("USB Suspended");
                if (instance) instance->setConnected(false);
                break;
            case ARDUINO_USB_RESUME_EVENT:
                Serial.println("USB Resumed");
                if (instance) instance->setConnected(true);
                break;
            default:
                break;
        }
    }
}

MeowUSBDevice::MeowUSBDevice() {
    deviceConnected = false;
    currentMode = HID_MODE_KEYBOARD;
    instance = this;
}

bool MeowUSBDevice::begin() {
    // Initialize USB HID device
    USB.onEvent(usbEventCallback);
    Keyboard.begin();
    USB.begin();
    
    Serial.println("USB HID initialized");
    // deviceConnected will be set by callback
    return true;
}

void MeowUSBDevice::setMode(HIDMode mode) {
    currentMode = mode;
    Serial.println("USB HID mode set to: " + String(mode));
}

void MeowUSBDevice::sendKey(uint8_t key, uint8_t modifiers) {
    if (!deviceConnected) return;
    
    // Press modifiers
    if (modifiers & DuckyScriptParser::MOD_CTRL_LEFT) Keyboard.press(KEY_LEFT_CTRL);
    if (modifiers & DuckyScriptParser::MOD_SHIFT_LEFT) Keyboard.press(KEY_LEFT_SHIFT);
    if (modifiers & DuckyScriptParser::MOD_ALT_LEFT) Keyboard.press(KEY_LEFT_ALT);
    if (modifiers & DuckyScriptParser::MOD_GUI_LEFT) Keyboard.press(KEY_LEFT_GUI);
    if (modifiers & DuckyScriptParser::MOD_CTRL_RIGHT) Keyboard.press(KEY_RIGHT_CTRL);
    if (modifiers & DuckyScriptParser::MOD_SHIFT_RIGHT) Keyboard.press(KEY_RIGHT_SHIFT);
    if (modifiers & DuckyScriptParser::MOD_ALT_RIGHT) Keyboard.press(KEY_RIGHT_ALT);
    if (modifiers & DuckyScriptParser::MOD_GUI_RIGHT) Keyboard.press(KEY_RIGHT_GUI);
    
    // Press key
    if (key != 0) {
        Keyboard.press(key);
        delay(10);
        Keyboard.release(key);
    }
    
    // Release modifiers
    Keyboard.releaseAll();
    delay(10);
}

void MeowUSBDevice::sendString(const String& text) {
    if (!deviceConnected) return;
    Keyboard.print(text);
}

void MeowUSBDevice::sendKeySequence(const String& keys) {
    // For simplicity in this rewrite, we'll just handle basic sequences or ignore
    // Since sendKey handles modifiers + key, complex sequences might need parsing
    // But DuckyScript usually sends one key combo at a time which sendKey handles
    // If 'keys' contains multiple keys to be pressed simultaneously:
    
    // This is a placeholder for complex sequence handling if needed
    // For now, we assume sendKey covers the main use cases
}

void MeowUSBDevice::moveMouse(int8_t x, int8_t y) {
    // Mouse functionality disabled
}

void MeowUSBDevice::clickMouse(uint8_t buttons) {
    // Mouse functionality disabled
}

void MeowUSBDevice::delay(uint32_t ms) {
    ::delay(ms);
}

extern "C" bool tud_mounted(void);

bool MeowUSBDevice::isConnected() {
    return deviceConnected;
}