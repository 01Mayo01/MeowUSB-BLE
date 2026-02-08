#include "USBHIDDevice.h"

MeowUSBDevice* MeowUSBDevice::instance = nullptr;

void MeowUSBDevice::usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                Serial.println("USB Started");
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
    return true;
}

void MeowUSBDevice::setMode(HIDMode mode) {
    currentMode = mode;
    Serial.println("USB HID mode set to: " + String(mode));
}

void MeowUSBDevice::sendKey(uint8_t key, uint8_t modifiers) {
    if (!isConnected()) return;

    Serial.println("DEBUG: USB sendKey key=" + String(key, HEX) + " mods=" + String(modifiers, HEX));
    
    // Press modifiers
    if (modifiers & DuckyScriptParser::MOD_CTRL_LEFT) Keyboard.press(KEY_LEFT_CTRL);
    if (modifiers & DuckyScriptParser::MOD_SHIFT_LEFT) Keyboard.press(KEY_LEFT_SHIFT);
    if (modifiers & DuckyScriptParser::MOD_ALT_LEFT) Keyboard.press(KEY_LEFT_ALT);
    if (modifiers & DuckyScriptParser::MOD_GUI_LEFT) {
        Keyboard.press(KEY_LEFT_GUI);
        Serial.println("DEBUG: Pressed KEY_LEFT_GUI");
    }
    if (modifiers & DuckyScriptParser::MOD_CTRL_RIGHT) Keyboard.press(KEY_RIGHT_CTRL);
    if (modifiers & DuckyScriptParser::MOD_SHIFT_RIGHT) Keyboard.press(KEY_RIGHT_SHIFT);
    if (modifiers & DuckyScriptParser::MOD_ALT_RIGHT) Keyboard.press(KEY_RIGHT_ALT);
    if (modifiers & DuckyScriptParser::MOD_GUI_RIGHT) Keyboard.press(KEY_RIGHT_GUI);
    
    // Press key
    if (key != 0) {
        Keyboard.press(key);
        Serial.println("DEBUG: Pressed key " + String(key, HEX));
    }

    // Hold for a moment to ensure host registers it
    delay(20);
    
    // Release everything
    Keyboard.releaseAll();
    delay(20);
}

void MeowUSBDevice::sendString(const String& text) {
    if (!isConnected()) return;
    Keyboard.print(text);
    delay(20); // Small delay after string to ensure host processing
}

void MeowUSBDevice::sendKeySequence(const String& keys) {
    // For simplicity in this rewrite, we'll just handle basic sequences or ignore
    // Since sendKey handles modifiers + key, complex sequences might need parsing
    // But DuckyScript usually sends one key combo at a time which sendKey handles
    // If 'keys' contains multiple keys to be pressed simultaneously:
    
    // This is a placeholder for complex sequence handling if needed
    // For now, we assume sendKey covers the main use cases
}

void MeowUSBDevice::delay(uint32_t ms) {
    ::delay(ms);
}

extern "C" bool tud_mounted(void);

bool MeowUSBDevice::isConnected() {
    // Strictly rely on TinyUSB mounted status
    // deviceConnected flag is only an indication that the USB stack has started, 
    // but not necessarily that we are enumerated and ready to talk to the host.
    return tud_mounted();
}