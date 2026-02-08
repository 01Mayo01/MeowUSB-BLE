#include "DuckyScriptParser.h"

DuckyScriptParser::DuckyScriptParser() {
    executionComplete = true;
    commandDelay = 100; // Increased default delay to 100ms for better reliability
    currentLine = 0;
    inCommentBlock = false;
    hidDevice = nullptr;
    
    // Initialize special keys mapping
    specialKeys["ENTER"] = DUCKY_ENTER;
    specialKeys["ESC"] = DUCKY_ESC;
    specialKeys["BACKSPACE"] = DUCKY_BACKSPACE;
    specialKeys["TAB"] = DUCKY_TAB;
    specialKeys["SPACE"] = DUCKY_SPACE;
    specialKeys["DELETE"] = DUCKY_DELETE;
    specialKeys["UP"] = DUCKY_UP;
    specialKeys["DOWN"] = DUCKY_DOWN;
    specialKeys["LEFT"] = DUCKY_LEFT;
    specialKeys["RIGHT"] = DUCKY_RIGHT;
    specialKeys["F1"] = 0x3A;
    specialKeys["F2"] = 0x3B;
    specialKeys["F3"] = 0x3C;
    specialKeys["F4"] = 0x3D;
    specialKeys["F5"] = 0x3E;
    specialKeys["F6"] = 0x3F;
    specialKeys["F7"] = 0x40;
    specialKeys["F8"] = 0x41;
    specialKeys["F9"] = 0x42;
    specialKeys["F10"] = 0x43;
    specialKeys["F11"] = 0x44;
    specialKeys["F12"] = 0x45;
    
    // Initialize modifiers mapping
    modifiers["CTRL"] = MOD_CTRL_LEFT;
    modifiers["SHIFT"] = MOD_SHIFT_LEFT;
    modifiers["ALT"] = MOD_ALT_LEFT;
    modifiers["GUI"] = MOD_GUI_LEFT;
    modifiers["WINDOWS"] = MOD_GUI_LEFT;
    modifiers["COMMAND"] = MOD_GUI_LEFT;
    modifiers["CTRL-LEFT"] = MOD_CTRL_LEFT;
    modifiers["CTRL-RIGHT"] = MOD_CTRL_RIGHT;
    modifiers["SHIFT-LEFT"] = MOD_SHIFT_LEFT;
    modifiers["SHIFT-RIGHT"] = MOD_SHIFT_RIGHT;
    modifiers["ALT-LEFT"] = MOD_ALT_LEFT;
    modifiers["ALT-RIGHT"] = MOD_ALT_RIGHT;
    modifiers["GUI-LEFT"] = MOD_GUI_LEFT;
    modifiers["GUI-RIGHT"] = MOD_GUI_RIGHT;
}

void DuckyScriptParser::setHIDDevice(HIDDevice* device) {
    hidDevice = device;
}

void DuckyScriptParser::execute(const String& script) {
    if (!hidDevice || !hidDevice->isConnected()) {
        Serial.println("HID device not available");
        return;
    }
    
    executionComplete = false;
    currentLine = 0;
    inCommentBlock = false;
    
    // Split script into lines
    lines.clear();
    int start = 0;
    int end = script.indexOf('\n');
    
    while (end != -1) {
        lines.push_back(script.substring(start, end));
        start = end + 1;
        end = script.indexOf('\n', start);
    }
    
    // Add last line if it doesn't end with newline
    if (start < script.length()) {
        lines.push_back(script.substring(start));
    }
    
    Serial.println("Starting DuckyScript execution");
    Serial.println("Lines: " + String(lines.size()));
}

void DuckyScriptParser::process() {
    if (executionComplete || !hidDevice) return;
    
    if (currentLine < lines.size()) {
        executeLine(lines[currentLine]);
        currentLine++;
    } else {
        executionComplete = true;
    }
}

String DuckyScriptParser::getCurrentLine() {
    if (currentLine < lines.size()) {
        return lines[currentLine];
    }
    return "";
}

void DuckyScriptParser::executeLine(const String& line) {
    if (executionComplete || !hidDevice) return;
    
    String trimmedLine = trim(line);
    
    // Skip empty lines
    if (trimmedLine.length() == 0) return;
    
    // Handle comment blocks
    if (inCommentBlock) {
        if (trimmedLine.startsWith("REM_BLOCK") && trimmedLine.indexOf("END") != -1) {
            inCommentBlock = false;
        }
        return;
    }
    
    // Skip single line comments
    if (trimmedLine.startsWith("REM")) {
        return;
    }
    
    // Parse command
    int spaceIndex = trimmedLine.indexOf(' ');
    String command = (spaceIndex != -1) ? trimmedLine.substring(0, spaceIndex) : trimmedLine;
    String parameters = (spaceIndex != -1) ? trimmedLine.substring(spaceIndex + 1) : "";
    parameters = trim(parameters);
    
    // Execute command
    if (command == "DELAY") {
        handleDELAY(parameters);
    } else if (command == "STRING") {
        handleSTRING(parameters);
    } else if (command == "STRINGLN") {
        handleSTRINGLN(parameters);
    } else if (command == "KEY") {
        handleKEY(parameters);
    } else if (command == "KEYS") {
        handleKEYS(parameters);
    } else if (command == "DEFAULTDELAY") {
        handleDEFAULTDELAY(parameters);
    } else if (command == "REM_BLOCK") {
        handleREM_BLOCK(parameters);
    } else if (command == "GUI" || command == "WINDOWS" || command == "COMMAND") {
        // Explicitly handle GUI command which often has parameters like "GUI r"
        if (parameters.length() > 0) {
             // Treat "GUI r" as "GUI" modifier + "r" key
             String newCmd = "GUI " + parameters;
             handleKEY(newCmd);
        } else {
             // Just GUI key press
             handleKEY("GUI");
        }
    } else if (command == "ENTER") {
        // Explicitly handle ENTER
        handleKEY("ENTER");
    } else if (specialKeys.find(command.c_str()) != specialKeys.end() || modifiers.find(command.c_str()) != modifiers.end()) {
        // Implicit key command (e.g., "CTRL c")
        handleKEY(line);
    } else {
        Serial.println("Unknown command: " + command);
    }
    
    // Apply default delay
    if (commandDelay > 0) {
        hidDevice->delay(commandDelay);
    }
}

void DuckyScriptParser::handleREM(const String& line) {
    // Comments are ignored
    Serial.println("Comment: " + line);
}

void DuckyScriptParser::handleREM_BLOCK(const String& line) {
    if (line.indexOf("END") == -1) {
        inCommentBlock = true;
    }
}

void DuckyScriptParser::handleDELAY(const String& line) {
    int delayMs = line.toInt();
    if (delayMs > 0) {
        hidDevice->delay(delayMs);
        Serial.println("Delay: " + String(delayMs) + "ms");
    }
}

void DuckyScriptParser::handleSTRING(const String& line) {
    hidDevice->sendString(line);
    Serial.println("String: " + line);
}

void DuckyScriptParser::handleSTRINGLN(const String& line) {
    hidDevice->sendString(line);
    hidDevice->sendKey(DUCKY_ENTER);
    Serial.println("StringLN: " + line);
}

void DuckyScriptParser::handleKEY(const String& line) {
    std::vector<String> keyParts = splitByWhitespace(line);
    uint8_t key = 0;
    uint8_t keyMods = 0;
    
    Serial.println("DEBUG: handleKEY processing line: '" + line + "'");

    for (const String& part : keyParts) {
        Serial.println("DEBUG: Processing part: '" + part + "'");
        if (modifiers.find(part.c_str()) != modifiers.end()) {
            keyMods |= modifiers[part.c_str()];
            Serial.println("DEBUG: Found modifier: " + part + ", val: " + String(modifiers[part.c_str()], HEX) + ", keyMods now: " + String(keyMods, HEX));
        } else if (specialKeys.find(part.c_str()) != specialKeys.end()) {
            key = specialKeys[part.c_str()];
            Serial.println("DEBUG: Found special key: " + part + ", val: " + String(key, HEX));
        } else if (part.length() == 1) {
            // Single character
            key = part[0];
            Serial.println("DEBUG: Found char key: " + part + ", val: " + String(key, HEX));
        } else {
            Serial.println("DEBUG: Unknown part: " + part);
        }
    }
    
    Serial.println("DEBUG: Calling sendKey with key: " + String(key, HEX) + ", mods: " + String(keyMods, HEX));
    hidDevice->sendKey(key, keyMods);
    Serial.println("Key: " + line);
}

void DuckyScriptParser::handleKEYS(const String& line) {
    hidDevice->sendKeySequence(line);
    Serial.println("Keys: " + line);
}

void DuckyScriptParser::handleDEFAULTDELAY(const String& line) {
    int delayMs = line.toInt();
    if (delayMs > 0) {
        commandDelay = delayMs;
        Serial.println("Default delay: " + String(delayMs) + "ms");
    }
}

void DuckyScriptParser::stopExecution() {
    executionComplete = true;
    currentLine = 0;
    lines.clear();
}

String DuckyScriptParser::trim(const String& str) {
    int start = 0;
    int end = str.length() - 1;
    
    while (start <= end && isWhitespace(str[start])) start++;
    while (end >= start && isWhitespace(str[end])) end--;
    
    return str.substring(start, end - start + 1);
}

std::vector<String> DuckyScriptParser::splitByWhitespace(const String& str) {
    std::vector<String> result;
    int length = str.length();
    int start = 0;
    bool inWord = false;
    
    for (int i = 0; i < length; i++) {
        if (isWhitespace(str[i])) {
            if (inWord) {
                result.push_back(str.substring(start, i));
                inWord = false;
            }
        } else {
            if (!inWord) {
                start = i;
                inWord = true;
            }
        }
    }
    
    if (inWord) {
        result.push_back(str.substring(start));
    }
    
    return result;
}

bool DuckyScriptParser::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}