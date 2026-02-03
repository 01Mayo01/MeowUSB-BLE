#ifndef DUCKYSCRIPT_PARSER_H
#define DUCKYSCRIPT_PARSER_H

#include <Arduino.h>
#include <vector>
#include <map>

// HID Device interface
class HIDDevice {
public:
    virtual void sendKey(uint8_t key, uint8_t modifiers = 0) = 0;
    virtual void sendString(const String& text) = 0;
    virtual void sendKeySequence(const String& keys) = 0;
    virtual void moveMouse(int8_t x, int8_t y) = 0;
    virtual void clickMouse(uint8_t buttons) = 0;
    virtual void delay(uint32_t ms) = 0;
    virtual bool isConnected() = 0;
};

// HID Modes
enum HIDMode {
    HID_MODE_KEYBOARD,
    HID_MODE_MOUSE,
    HID_MODE_COMBO
};

class DuckyScriptParser {
private:
    HIDDevice* hidDevice;
    bool executionComplete;
    unsigned long commandDelay;
    
    // DuckyScript commands
    std::map<String, uint8_t> specialKeys;
    std::map<String, uint8_t> modifiers;
    
    // Parsing state
    std::vector<String> lines;
    int currentLine;
    bool inCommentBlock;
    
    // Command handlers
    void handleREM(const String& line);
    void handleREM_BLOCK(const String& line);
    void handleDELAY(const String& line);
    void handleSTRING(const String& line);
    void handleSTRINGLN(const String& line);
    void handleKEY(const String& line);
    void handleKEYS(const String& line);
    void handleMOUSE_MOVE(const String& line);
    void handleMOUSE_CLICK(const String& line);
    void handleDEFAULTDELAY(const String& line);
    
    // Utility functions
    String trim(const String& str);
    std::vector<String> split(const String& str, char delimiter);
    uint8_t parseKey(const String& keyName);
    uint8_t parseModifiers(const String& modifierStr);
    bool isWhitespace(char c);
    
public:
    DuckyScriptParser();
    
    void setHIDDevice(HIDDevice* device);
    void execute(const String& script);
    void process(); // Process next line
    String getCurrentLine(); // Get current line text
    void executeLine(const String& line);
    bool isExecutionComplete() { return executionComplete; }
    void stopExecution();
    
    // Command constants (Arduino Keyboard.h compatible)
    static const uint8_t DUCKY_ENTER = 0xB0;
    static const uint8_t DUCKY_ESC = 0xB1;
    static const uint8_t DUCKY_BACKSPACE = 0xB2;
    static const uint8_t DUCKY_TAB = 0xB3;
    static const uint8_t DUCKY_SPACE = ' ';
    static const uint8_t DUCKY_DELETE = 0xD4;
    static const uint8_t DUCKY_UP = 0xDA;
    static const uint8_t DUCKY_DOWN = 0xD9;
    static const uint8_t DUCKY_LEFT = 0xD8;
    static const uint8_t DUCKY_RIGHT = 0xD7;
    
    // Modifier constants (Bitmasks for internal use)
    static const uint8_t MOD_CTRL_LEFT   = 0x01;
    static const uint8_t MOD_SHIFT_LEFT  = 0x02;
    static const uint8_t MOD_ALT_LEFT    = 0x04;
    static const uint8_t MOD_GUI_LEFT    = 0x08;
    static const uint8_t MOD_CTRL_RIGHT  = 0x10;
    static const uint8_t MOD_SHIFT_RIGHT = 0x20;
    static const uint8_t MOD_ALT_RIGHT   = 0x40;
    static const uint8_t MOD_GUI_RIGHT   = 0x80;
};

#endif // DUCKYSCRIPT_PARSER_H