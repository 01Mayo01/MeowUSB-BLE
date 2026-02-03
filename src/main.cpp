#include <M5Cardputer.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <LittleFS.h>
#include <vector>
#include <string>
#include "DuckyScriptParser.h"
#include "USBHIDDevice.h"
#include "BluetoothHIDDevice.h"
#include "PayloadManager.h"
#include "ConfigManager.h"

#define PINK 0xFE19

// Gray color for disabled text
#define GRAY 0x8410

#define FW_VERSION "v0.0.5"

// Global objects
MeowUSBDevice usbHid;
BluetoothHIDDevice btHid;
DuckyScriptParser duckyParser;
PayloadManager payloadManager;
ConfigManager configManager;

// Device state
enum DeviceMode {
    MODE_IDLE,
    MODE_USB_HID,
    MODE_BT_HID,
    MODE_CONFIRM_EXECUTION,
    MODE_RENAME_BT
};

DeviceMode currentMode = MODE_IDLE;
bool isExecuting = false;
bool useBluetooth = false; // Default to USB
String currentPayload = "";

// UI State
int selectedIndex = 0;
int scrollOffset = 0;

// Function declarations
void showBootScreen();
void showMainMenu();
void showError(String error);
void showConfirmationScreen(String payloadName);
void showExecutionScreen(String mode, String payloadName);
void showExecutionComplete();
void showRenameScreen();
void handleButtonA();
void handleKeyboardInput();
void moveSelectionUp();
void moveSelectionDown();
void executePayloadUSB();
void executePayloadBluetooth();
void drawBatteryStatus();

void setup() {
    // Initialize M5 Cardputer
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.setCursor(0, 0);
    
    // Show cat-themed boot screen
    showBootScreen();
    delay(2000); // Show boot screen for 2 seconds
    
    Serial.begin(115200);
    Serial.println("M5 Cardputer DuckyScript Executor v1.0");
    
    // Initialize SD card
    SPI.begin(40, 39, 14, 12);
    if (!SD.begin(12, SPI, 25000000)) {
        Serial.println("SD Card initialization failed!");
        // Don't return, allow LittleFS
    }
    
    // Initialize USB HID
    if (!usbHid.begin()) {
        Serial.println("USB HID initialization failed!");
        showError("USB HID Error");
        return;
    }
    
    // Initialize Bluetooth HID (but don't start advertising yet)
    // Bluetooth advertising will be controlled by Tab key toggle
    Serial.println("Bluetooth HID ready (use Tab to toggle)");
    
    // Show Bluetooth initialization status
    M5Cardputer.Display.setCursor(0, 100);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("Bluetooth ready...");
    
    // Initialize payload manager
    payloadManager.begin();
    
    // Load configuration
    configManager.loadConfig();
    
    // Show main menu
    showMainMenu();
    
    Serial.println("Setup complete!");
}

void loop() {
    M5Cardputer.update();
    
    // Handle button input
    if (M5Cardputer.BtnA.isPressed()) {
        handleButtonA();
        delay(200); // Debounce
    }
    
    // Handle keyboard input for navigation
    if (M5Cardputer.Keyboard.isPressed()) {
        handleKeyboardInput();
        // Delay handled in function
    }
    
    // Handle payload execution
    if (isExecuting) {
        // Add safety delay during Bluetooth execution
        if (currentMode == MODE_BT_HID) {
            delay(50); // Prevent rapid operations during BLE
        }
        
        // Process next line
        duckyParser.process();
        
        // Update display with current line
        String currentLine = duckyParser.getCurrentLine();
        if (currentLine.length() > 0) {
            // Clear previous line area (simple approach)
            M5Cardputer.Display.fillRect(0, 60, M5Cardputer.Display.width(), 20, BLACK);
            M5Cardputer.Display.setCursor(0, 60);
            M5Cardputer.Display.setTextColor(GREEN);
            M5Cardputer.Display.print("> ");
            M5Cardputer.Display.println(currentLine.substring(0, 30)); // Truncate if too long
        }
        
        // Check for completion
        if (duckyParser.isExecutionComplete()) {
            isExecuting = false;
            showExecutionComplete();
        }
        
        // Check for abort (ESC)
        if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.isKeyPressed(27)) {
            duckyParser.stopExecution();
            isExecuting = false;
            showExecutionComplete(); // Or show aborted screen
        }
    }
}

void handleKeyboardInput() {
    // Mode Toggle (Tab)
    // Debounce Tab key
    static unsigned long lastTabPress = 0;
    if (M5Cardputer.Keyboard.keysState().tab) {
        if (millis() - lastTabPress > 500) {
            lastTabPress = millis();
            useBluetooth = !useBluetooth;
            
            // Handle Bluetooth advertising toggle
            if (useBluetooth) {
                // Show Bluetooth booting status
                M5Cardputer.Display.fillRect(0, 80, M5Cardputer.Display.width(), 20, BLACK);
                M5Cardputer.Display.setCursor(0, 80);
                M5Cardputer.Display.setTextColor(BLUE);
                M5Cardputer.Display.println("Bluetooth booting...");
                M5Cardputer.Display.setTextColor(WHITE);
                
                // Force display update
                M5Cardputer.Display.display();
                
                // Start Bluetooth advertising with configured name if not already running
                String btName = configManager.getBluetoothName();
                bool success = btHid.begin(btName);
                
                if (success) {
                    Serial.println("Bluetooth advertising started: " + btName);
                    
                    // Update status
                    M5Cardputer.Display.fillRect(0, 80, M5Cardputer.Display.width(), 20, BLACK);
                    M5Cardputer.Display.setCursor(0, 80);
                    M5Cardputer.Display.setTextColor(GREEN);
                    M5Cardputer.Display.println("Bluetooth ready!");
                    M5Cardputer.Display.setTextColor(WHITE);
                    M5Cardputer.Display.display();
                    delay(500);
                } else {
                    Serial.println("Bluetooth initialization failed!");
                    showError("BT Init Failed");
                    delay(1000);
                    useBluetooth = false; // Revert to USB mode
                }
            } else {
                // Switching to USB Mode
                // DO NOT stop Bluetooth to prevent crash/instability
                Serial.println("Switched to USB Mode (BLE remains active in bg)");
            }
            
            showMainMenu();
            return;
        }
    }

    // Navigation
    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        moveSelectionUp();
        delay(150);
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        moveSelectionDown();
        delay(150);
    }
    // Rename Bluetooth (R key)
    else if (M5Cardputer.Keyboard.isKeyPressed('r')) {
        currentMode = MODE_RENAME_BT;
        showRenameScreen();
        delay(300);
    }
    // Enter directory or Execute
    else if (M5Cardputer.Keyboard.keysState().enter) {
        if (currentMode == MODE_CONFIRM_EXECUTION) {
            // Execute the payload
            if (useBluetooth) {
                executePayloadBluetooth();
            } else {
                executePayloadUSB();
            }
        } else {
            std::vector<FileEntry> files = payloadManager.getFileList();
            if (selectedIndex >= 0 && selectedIndex < files.size()) {
                if (files[selectedIndex].isDir) {
                    if (payloadManager.navigateDown(files[selectedIndex].name)) {
                        selectedIndex = 0;
                        scrollOffset = 0;
                        showMainMenu();
                    }
                } else {
                    // Check connection before execution
                    bool connected = false;
                    if (useBluetooth) {
                        connected = btHid.isConnected();
                        if (!connected) {
                            showError("BT Not Connected");
                            delay(1000);
                            showMainMenu();
                        }
                    } else {
                        connected = usbHid.isConnected();
                        if (!connected) {
                            showError("USB Not Connected");
                            delay(1000);
                            showMainMenu();
                        }
                    }
                    
                    if (connected) {
                        currentMode = MODE_CONFIRM_EXECUTION;
                        currentPayload = files[selectedIndex].name;
                        showConfirmationScreen(currentPayload);
                    }
                }
            }
        }
        delay(300);
    }
    // Go Up / Back
    else if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.isKeyPressed(27)) {
        if (currentMode == MODE_CONFIRM_EXECUTION) {
            currentMode = MODE_IDLE;
            showMainMenu();
        } else {
            payloadManager.navigateUp();
            selectedIndex = 0;
            scrollOffset = 0;
            showMainMenu();
        }
        delay(300);
    }
}

void moveSelectionUp() {
    std::vector<FileEntry> files = payloadManager.getFileList();
    if (!files.empty()) {
        selectedIndex--;
        if (selectedIndex < 0) selectedIndex = files.size() - 1;
        showMainMenu();
    }
}

void moveSelectionDown() {
    std::vector<FileEntry> files = payloadManager.getFileList();
    if (!files.empty()) {
        selectedIndex++;
        if (selectedIndex >= files.size()) selectedIndex = 0;
        showMainMenu();
    }
}

void handleButtonA() {
    // Button A: Execute based on current mode
    if (useBluetooth) {
        if (btHid.isConnected()) {
            executePayloadBluetooth();
        } else {
            showError("BT Not Connected");
            delay(1000);
            showMainMenu();
        }
    } else {
        if (usbHid.isConnected()) {
            executePayloadUSB();
        } else {
            showError("USB Not Connected");
            delay(1000);
            showMainMenu();
        }
    }
}

void executePayloadUSB() {
    std::vector<FileEntry> files = payloadManager.getFileList();
    if (selectedIndex >= files.size()) return;
    
    // Only execute files
    if (files[selectedIndex].isDir) return;
    
    String payloadName = files[selectedIndex].name;
    String payloadContent = payloadManager.loadFile(payloadName);
    
    if (payloadContent.isEmpty()) {
        showError("Empty/Failed Load");
        return;
    }
    
    // Switch to USB HID mode
    currentMode = MODE_USB_HID;
    usbHid.setMode(HID_MODE_KEYBOARD);
    
    // Show execution screen
    showExecutionScreen("USB", payloadName);
    
    // Parse and execute DuckyScript
    duckyParser.setHIDDevice(&usbHid);
    duckyParser.execute(payloadContent);
    isExecuting = true;
}

void executePayloadBluetooth() {
    std::vector<FileEntry> files = payloadManager.getFileList();
    if (selectedIndex >= files.size()) return;
    
    if (files[selectedIndex].isDir) return;
    
    String payloadName = files[selectedIndex].name;
    String payloadContent = payloadManager.loadFile(payloadName);
    
    if (payloadContent.isEmpty()) {
        showError("Empty/Failed Load");
        return;
    }
    
    // Extended connection verification with multiple checks
    unsigned long connectStart = millis();
    bool connectionVerified = false;
    
    // Wait up to 3 seconds for stable connection
    while (millis() - connectStart < 3000) {
        if (btHid.isConnected()) {
            // Additional verification delay
            delay(500);
            if (btHid.isConnected()) {
                connectionVerified = true;
                break;
            }
        }
        delay(100);
    }
    
    if (!connectionVerified) {
        showError("BT Connection Unstable");
        delay(1500);
        showMainMenu();
        return;
    }
    
    // Switch to Bluetooth HID mode
    currentMode = MODE_BT_HID;
    btHid.setMode(HID_MODE_KEYBOARD);
    
    // Show execution screen
    showExecutionScreen("Bluetooth", payloadName);
    
    // Parse and execute DuckyScript
    duckyParser.setHIDDevice(&btHid);
    duckyParser.execute(payloadContent);
    isExecuting = true;
}

void showConfirmationScreen(String payloadName) {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("=== CONFIRM ===");
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("File: " + payloadName);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Device Connected.");
    M5Cardputer.Display.println("Press ENTER to Execute");
    M5Cardputer.Display.println("Press ESC to Cancel");
}

void showExecutionScreen(String mode, String payloadName) {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("=== EXECUTING ===");
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("Mode: " + mode);
    M5Cardputer.Display.println("File: " + payloadName);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Press ESC to stop");
}

void showExecutionComplete() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("=== COMPLETE ===");
    M5Cardputer.Display.println("Execution finished!");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Press any key...");
    
    currentMode = MODE_IDLE;
}

void showError(String error) {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("=== ERROR ===");
    M5Cardputer.Display.println(error);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Press any key...");
}

void showBootScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(PINK);
    
    // Cat ASCII art (Larger and more detailed)
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println("      |\\      _,,,---,,_");
    M5Cardputer.Display.println("ZZZzz /,`.-'`'    -.  ;-;;,_");
    M5Cardputer.Display.println("     |,4-  ) )-,_. ,\\ (  `'-'");
    M5Cardputer.Display.println("    '---''(_/--'  `-'\\_)");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println("");
    
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("  MeowUSB&BLE");
    M5Cardputer.Display.println("  File Browser");
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("  Loading...");
}

void showMainMenu() {
    M5Cardputer.Display.clear();
    drawBatteryStatus(); // Draw battery percentage in top right
    M5Cardputer.Display.setCursor(0, 0);
    
    // Header Cat (Centered)
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("          /\\_/\\");
    M5Cardputer.Display.println("         ( o.o )");
    
    // Mode Header
    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.print("Mode: ");
    if (useBluetooth) {
        M5Cardputer.Display.setTextColor(BLUE);
        M5Cardputer.Display.print("BT ");
        if (btHid.isConnected()) {
            M5Cardputer.Display.setTextColor(PINK);
            M5Cardputer.Display.println("(Conn)");
        } else {
            M5Cardputer.Display.setTextColor(RED);
            M5Cardputer.Display.println("(Disc)");
        }
    } else {
        M5Cardputer.Display.setTextColor(BLUE);
        M5Cardputer.Display.print("USB ");
        if (usbHid.isConnected()) {
            M5Cardputer.Display.setTextColor(PINK);
            M5Cardputer.Display.println("(Rdy)");
        } else {
            M5Cardputer.Display.setTextColor(RED);
            M5Cardputer.Display.println("(Wait)");
        }
    }

    M5Cardputer.Display.setTextColor(CYAN);
    M5Cardputer.Display.print("Path: ");
    M5Cardputer.Display.println(payloadManager.getCurrentPath());
    M5Cardputer.Display.println("--------------------");
    
    std::vector<FileEntry> files = payloadManager.getFileList();
    
    if (files.empty()) {
        M5Cardputer.Display.setTextColor(RED);
        M5Cardputer.Display.println("Empty directory");
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.println("ESC: Back");
    } else {
        int maxItems = 5; // Reduced from 7 to fit header cat
        
        // Adjust scroll offset
        if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
        if (selectedIndex >= scrollOffset + maxItems) scrollOffset = selectedIndex - maxItems + 1;
        
        // Ensure scrollOffset is valid
        if (scrollOffset < 0) scrollOffset = 0;
        if (scrollOffset >= files.size()) scrollOffset = files.size() - 1;
        
        for (int i = scrollOffset; i < files.size() && i < scrollOffset + maxItems; i++) {
            if (i == selectedIndex) {
                M5Cardputer.Display.setTextColor(PINK);
                M5Cardputer.Display.print("> ");
            } else {
                M5Cardputer.Display.setTextColor(WHITE);
                M5Cardputer.Display.print("  ");
            }
            
            if (files[i].isDir) {
                M5Cardputer.Display.print("[D] ");
            } else {
                M5Cardputer.Display.print("    ");
            }
            M5Cardputer.Display.println(files[i].name);
        }
    }
    
    // Footer
    // M5Cardputer.Display.setCursor(0, M5Cardputer.Display.height() - 10);
    // M5Cardputer.Display.setTextColor(YELLOW);
    // M5Cardputer.Display.print("ENT:Run/Open ESC:Back");
    
    // Add help text for R key
    M5Cardputer.Display.setCursor(0, M5Cardputer.Display.height() - 20);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.print("R:Rename BT ");
    if (useBluetooth) {
        M5Cardputer.Display.setTextColor(PINK);
        M5Cardputer.Display.println(configManager.getBluetoothName());
    } else {
        M5Cardputer.Display.setTextColor(GRAY);
        M5Cardputer.Display.println("(USB Mode)");
    }
}

void drawBatteryStatus() {
    // Get battery level from power management
    int32_t batteryLevel = M5Cardputer.Power.getBatteryLevel();
    
    // Only display if battery level is valid (0-100)
    if (batteryLevel >= 0 && batteryLevel <= 100) {
        // Set position to top right corner
        int16_t x = M5Cardputer.Display.width() - 40; // Adjust based on text width
        int16_t y = 2;
        
        M5Cardputer.Display.setCursor(x, y);
        M5Cardputer.Display.setTextColor(WHITE);
        
        // Draw battery icon and percentage
        M5Cardputer.Display.print("[");
        if (batteryLevel >= 50) {
            M5Cardputer.Display.setTextColor(GREEN);
        } else if (batteryLevel >= 25) {
            M5Cardputer.Display.setTextColor(YELLOW);
        } else {
            M5Cardputer.Display.setTextColor(RED);
        }
        M5Cardputer.Display.print(batteryLevel);
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print("%]");
        
        // Draw Version
        M5Cardputer.Display.setCursor(x, y + 10);
        M5Cardputer.Display.setTextColor(GRAY);
        M5Cardputer.Display.print(FW_VERSION);
    }
}

void showRenameScreen() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("=== RENAME BT ===");
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("Current: " + configManager.getBluetoothName());
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Enter new name:");
    M5Cardputer.Display.println("(max 16 chars)");
    
    String newName = "";
    bool done = false;
    unsigned long lastCursorUpdate = 0;
    bool cursorVisible = true;
    
    while (!done) {
        M5Cardputer.update();
        
        // Handle keyboard input
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                auto& status = M5Cardputer.Keyboard.keysState();
                
                // Enter to confirm
                if (status.enter) {
                    if (newName.length() > 0 && newName.length() <= 16) {
                        configManager.setBluetoothName(newName);
                        configManager.saveConfig();
                        done = true;
                    }
                    delay(300);
                }
                // Backspace
                else if (status.del) {
                    if (newName.length() > 0) {
                        newName.remove(newName.length() - 1);
                    }
                    delay(150);
                }
                // ESC to cancel
                else if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.isKeyPressed(27)) {
                    done = true;
                    delay(300);
                }
                // Regular keys
                else {
                    // Add typed characters
                    for (auto& c : status.word) {
                        if (newName.length() < 16) {
                            newName += c;
                        }
                    }
                }
            }
        }
        
        // Update cursor blinking
        if (millis() - lastCursorUpdate > 500) {
            cursorVisible = !cursorVisible;
            lastCursorUpdate = millis();
        }
        
        // Update display
        M5Cardputer.Display.fillRect(0, 60, M5Cardputer.Display.width(), 20, BLACK);
        M5Cardputer.Display.setCursor(0, 60);
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.print("> ");
        M5Cardputer.Display.print(newName);
        
        // Show blinking cursor
        if (cursorVisible) {
            M5Cardputer.Display.print("_");
        }
        
        delay(50); // Small delay for responsiveness
    }
    
    // Return to main menu
    currentMode = MODE_IDLE;
    showMainMenu();
}