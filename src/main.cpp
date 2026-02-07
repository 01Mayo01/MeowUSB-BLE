#include <M5Cardputer.h>
#include <WiFi.h>
#include <SD.h>
#include <esp_wifi.h> // Required for esp_wifi_set_promiscuous
#include "USBHIDDevice.h"
#include "BluetoothHIDDevice.h"
#include "DuckyScriptParser.h"
#include "PayloadManager.h"
#include "ConfigManager.h"
#include "WiFiPentest.h"

#define PINK 0xFE19
#define GRAY 0x8410
#define FW_VERSION "v0.3.9-fix2"

// Global objects
MeowUSBDevice usbHid;
BluetoothHIDDevice btHid;
DuckyScriptParser duckyParser;
PayloadManager payloadManager;
ConfigManager configManager;
WiFiPentest wifiPentest;

// Device state
enum DeviceMode {
    MODE_MAIN_MENU,
    MODE_USB_HID,
    MODE_BT_HID,
    MODE_WIFI_MENU,
    MODE_WIFI_SCAN,
    MODE_WIFI_ATTACK_SELECT,
    MODE_WIFI_DEAUTH,
    MODE_WIFI_PORTAL,
    MODE_WIFI_KARMA,
    MODE_CONFIRM_EXECUTION,
    MODE_RENAME_BT,
    MODE_INPUT_SSID,
    MODE_SELECT_HTML, // Uses PayloadManager to browse
    MODE_HTML_SELECTED // Intermediate state
};

DeviceMode currentMode = MODE_MAIN_MENU;
DeviceMode nextModeAfterHTML = MODE_MAIN_MENU; 
bool isExecuting = false;
bool useBluetooth = false; 
String currentPayload = "";
APInfo selectedAP; 
String selectedHTMLFile = ""; 

// UI State
int selectedIndex = 0;
int scrollOffset = 0;
bool htmlBrowserInitialized = false; // Flag to reset browser for HTML selection

// Input State
String inputText = "";
String inputTitle = "";
int inputMaxLength = 32;
bool (*inputCallback)(String) = nullptr;

// Function declarations
void showBootScreen();
void showMainMenu();
void showWiFiMenu();
void showWiFiScan();
void showWiFiAttackSelect();
void showDeauthScreen();
void showPortalScreen();
void showKarmaScreen();
void showInputScreen();
void showModeSwitch(bool bluetooth);
void showHTMLSelectScreen();
void showError(String error);
void showConfirmationScreen(String payloadName);
void showExecutionScreen(String mode, String payloadName);
void showExecutionComplete();
void showRenameScreen();
void handleButtonA();
void handleKeyboardInput();
void moveSelectionUp(int maxItems);
void moveSelectionDown(int maxItems);
void executePayloadUSB();
void executePayloadBluetooth();
void drawBatteryStatus(); // Deprecated but kept for compat
void drawStatusBar(); // New status bar
void startManualPortal(String ssid);
void proceedWithPortal(); // Helper to start portal after HTML select

// Helper to list HTML files
std::vector<String> getHTMLFiles() {
    std::vector<String> files;
    File root = SD.open("/");
    if (!root || !root.isDirectory()) return files;
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = String(file.name());
            if (name.endsWith(".html") || name.endsWith(".HTML")) {
                files.push_back(name);
            }
        }
        file = root.openNextFile();
    }
    return files;
}

void stopWiFi() {
    if (WiFi.getMode() != WIFI_OFF) {
        wifiPentest.stopDeauth();
        wifiPentest.stopEvilPortal();
        wifiPentest.stopKarma();
        
        // Ensure promiscuous mode is off
        esp_wifi_set_promiscuous(false);
        
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_OFF);
        delay(100);
        Serial.println("WiFi Stopped");
    }
}

void stopBluetooth() {
    if (useBluetooth) {
        btHid.end();
        delay(500); // Wait for BLE stack to fully unload
        Serial.println("Bluetooth Stopped");
    }
}

void setup() {
    // Initialize M5 Cardputer
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    
    // Ensure clean radio state on boot
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    btHid.end(); // Ensure BLE is off
    
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.setCursor(0, 0);
    
    // Show boot screen (Reverted to v0.2.3 style)
    showBootScreen();
    delay(2000); 
    
    Serial.begin(115200);
    Serial.println("M5 Cardputer MeowTool v0.3.4");
    
    // Initialize SD card
    SPI.begin(40, 39, 14, 12);
    if (!SD.begin(12, SPI, 25000000)) {
        Serial.println("SD Card initialization failed!");
    }
    
    // Initialize USB HID
    if (!usbHid.begin()) {
        Serial.println("USB HID initialization failed!");
        showError("USB HID Error");
        return;
    }
    
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
    
    // Handle background tasks
    if (wifiPentest.isDeauthActive()) {
        wifiPentest.updateDeauth();
    }
    if (wifiPentest.isPortalActive()) {
        wifiPentest.handlePortal();
    }
    
    // Handle button input
    if (M5Cardputer.BtnA.isPressed()) {
        handleButtonA();
        delay(200); // Debounce
    }
    
    // Handle keyboard input for navigation
    if (M5Cardputer.Keyboard.isPressed()) {
        handleKeyboardInput();
    }
    
    // Handle payload execution
    if (isExecuting) {
        if (currentMode == MODE_BT_HID) {
            delay(50); 
        }
        
        duckyParser.process();
        
        String currentLine = duckyParser.getCurrentLine();
        if (currentLine.length() > 0) {
            M5Cardputer.Display.fillRect(0, 60, M5Cardputer.Display.width(), 20, BLACK);
            M5Cardputer.Display.setCursor(0, 60);
            M5Cardputer.Display.setTextColor(GREEN);
            M5Cardputer.Display.print("> ");
            M5Cardputer.Display.println(currentLine.substring(0, 30)); 
        }
        
        if (duckyParser.isExecutionComplete()) {
            isExecuting = false;
            showExecutionComplete();
        }
        
        if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.isKeyPressed(27)) {
            duckyParser.stopExecution();
            isExecuting = false;
            showExecutionComplete(); 
        }
    }
}

void handleKeyboardInput() {
    // Use isChange() to ensure we catch every key event exactly once
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

            // === GLOBAL SHORTCUTS ===
            
            // TAB: Toggle USB/BLE
            if (status.tab) {
                if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID || currentMode == MODE_MAIN_MENU) {
                    useBluetooth = !useBluetooth;
                    // If in HID mode, switch the mode variable
                    if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) {
                        currentMode = useBluetooth ? MODE_BT_HID : MODE_USB_HID;
                    }
                    
                    if (useBluetooth) {
                        stopWiFi(); // Ensure WiFi is off before starting BLE
                        String btName = configManager.getBluetoothName();
                        btHid.begin(btName);
                    } else {
                        stopBluetooth(); // Stop BLE when switching to USB
                    }
                    showModeSwitch(useBluetooth);
                    return; 
                }
            }

            // ESC: Back / Cancel
            // Checking both physical ESC key mapping if available or ` key which is often ESC on Cardputer
            bool escPressed = false;
            for (auto c : status.word) {
                if (c == 27 || c == '`') escPressed = true;
            }

            if (escPressed) {
                // Handle Back Logic
                if (currentMode == MODE_WIFI_DEAUTH) {
                    wifiPentest.stopDeauth();
                    currentMode = MODE_WIFI_ATTACK_SELECT;
                    showWiFiAttackSelect();
                } else if (currentMode == MODE_WIFI_PORTAL) {
                    wifiPentest.stopEvilPortal();
                    if (selectedAP.ssid.length() > 0 && selectedAP.bssid[0] != 0) { // Scan origin
                         currentMode = MODE_WIFI_ATTACK_SELECT;
                         showWiFiAttackSelect();
                    } else { // Manual origin
                        currentMode = MODE_WIFI_MENU;
                        showWiFiMenu();
                    }
                } else if (currentMode == MODE_WIFI_KARMA) {
                    wifiPentest.stopKarma();
                    currentMode = MODE_WIFI_MENU;
                    showWiFiMenu();
                } else if (currentMode == MODE_INPUT_SSID) {
                    currentMode = MODE_WIFI_MENU;
                    showWiFiMenu();
                } else if (currentMode == MODE_SELECT_HTML) {
                    if (selectedAP.bssid[0] != 0) {
                         currentMode = MODE_WIFI_ATTACK_SELECT;
                         showWiFiAttackSelect();
                    } else {
                         currentMode = MODE_WIFI_MENU;
                         showWiFiMenu();
                    }
                } else if (currentMode == MODE_CONFIRM_EXECUTION) {
                    currentMode = useBluetooth ? MODE_BT_HID : MODE_USB_HID;
                    showMainMenu();
                } else if (currentMode != MODE_MAIN_MENU) {
                    if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) {
                        if (payloadManager.getCurrentPath() == "/") {
                            currentMode = MODE_MAIN_MENU;
                            selectedIndex = 0;
                            showMainMenu();
                        } else {
                            payloadManager.navigateUp();
                            selectedIndex = 0;
                            showMainMenu();
                        }
                    } else if (currentMode == MODE_WIFI_SCAN || currentMode == MODE_WIFI_ATTACK_SELECT) {
                        currentMode = MODE_WIFI_MENU;
                        selectedIndex = 0;
                        stopWiFi(); // Ensure WiFi is off when leaving sub-menus
                        showWiFiMenu();
                    } else if (currentMode == MODE_WIFI_MENU) { // Back from WiFi Menu
                        stopWiFi();
                        currentMode = MODE_MAIN_MENU;
                        selectedIndex = 0;
                        showMainMenu();
                    } else {
                        currentMode = MODE_MAIN_MENU;
                        selectedIndex = 0;
                        showMainMenu();
                    }
                }
                return;
            }

            // === CONTEXT SPECIFIC INPUT ===

            // 1. Text Input (SSID)
            if (currentMode == MODE_INPUT_SSID) {
                if (status.enter) {
                    if (inputCallback && inputCallback(inputText)) {
                        // Success
                    }
                    return;
                }
                if (status.del) {
                    if (inputText.length() > 0) {
                        inputText.remove(inputText.length() - 1);
                        showInputScreen();
                    }
                    return;
                }
                for (auto c : status.word) {
                    if (inputText.length() < inputMaxLength) {
                        inputText += c;
                    }
                }
                showInputScreen();
                return;
            }

            // 2. Navigation (Menus & Lists)
            bool up = false;
            bool down = false;
            
            // Check Arrow Keys (Fn + ;/.) or just ;/.
            for (auto c : status.word) {
                if (c == ';') up = true;
                if (c == '.') down = true;
            }
            
            if (up) {
                int maxItems = 0;
                if (currentMode == MODE_MAIN_MENU) maxItems = 2;
                else if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) maxItems = payloadManager.getFileList().size();
                else if (currentMode == MODE_WIFI_MENU) maxItems = 4;
                else if (currentMode == MODE_WIFI_SCAN) maxItems = wifiPentest.getResults().size();
                else if (currentMode == MODE_WIFI_ATTACK_SELECT) maxItems = 3;
                else if (currentMode == MODE_SELECT_HTML) maxItems = payloadManager.getFileList(".html").size() + 1; // +1 for "Default"

                moveSelectionUp(maxItems);
                return;
            }

            if (down) {
                int maxItems = 0;
                if (currentMode == MODE_MAIN_MENU) maxItems = 2;
                else if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) maxItems = payloadManager.getFileList().size();
                else if (currentMode == MODE_WIFI_MENU) maxItems = 4;
                else if (currentMode == MODE_WIFI_SCAN) maxItems = wifiPentest.getResults().size();
                else if (currentMode == MODE_WIFI_ATTACK_SELECT) maxItems = 3;
                else if (currentMode == MODE_SELECT_HTML) maxItems = payloadManager.getFileList(".html").size() + 1;

                moveSelectionDown(maxItems);
                return;
            }

            // 3. Enter / Select
            if (status.enter) {
                if (currentMode == MODE_MAIN_MENU) {
                    if (selectedIndex == 0) { // BadUSB/BLE
                        currentMode = MODE_USB_HID; // Start with USB (can toggle)
                        useBluetooth = false;
                        showMainMenu();
                    } else if (selectedIndex == 1) { // WiFi
                        currentMode = MODE_WIFI_MENU;
                        selectedIndex = 0;
                        showWiFiMenu();
                    }
                }
                else if (currentMode == MODE_WIFI_MENU) {
                    if (selectedIndex == 0) { // Scan
                        currentMode = MODE_WIFI_SCAN;
                        M5Cardputer.Display.clear();
                        M5Cardputer.Display.setCursor(0,0);
                        M5Cardputer.Display.println("Scanning...");
                        wifiPentest.scanNetworks();
                        selectedIndex = 0;
                        showWiFiScan();
                    } else if (selectedIndex == 1) { // Manual Portal
                        currentMode = MODE_INPUT_SSID;
                        inputTitle = "Enter Portal SSID:";
                        inputText = "Free WiFi";
                        inputCallback = [](String result) -> bool {
                            selectedAP.ssid = result;
                            memset(selectedAP.bssid, 0, 6);
                            
                            // Setup HTML Browser
                            currentMode = MODE_SELECT_HTML;
                            nextModeAfterHTML = MODE_WIFI_PORTAL;
                            selectedIndex = 0;
                            htmlBrowserInitialized = false; 
                            
                            showHTMLSelectScreen();
                            return true;
                        };
                        showInputScreen();
                    } else if (selectedIndex == 2) { // Karma
                        currentMode = MODE_WIFI_KARMA;
                        wifiPentest.startKarma();
                        showKarmaScreen();
                    } else { // Back
                        currentMode = MODE_MAIN_MENU;
                        selectedIndex = 0;
                        showMainMenu();
                    }
                }
                else if (currentMode == MODE_SELECT_HTML) {
                    // Use PayloadManager for HTML navigation
                    if (selectedIndex == 0) {
                        // Default Selected
                        selectedHTMLFile = ""; 
                        if (nextModeAfterHTML == MODE_WIFI_PORTAL) proceedWithPortal();
                        else { currentMode = MODE_WIFI_MENU; showWiFiMenu(); }
                    } else {
                        // File/Folder Selected
                        std::vector<FileEntry> files = payloadManager.getFileList(".html");
                        int fileIndex = selectedIndex - 1;
                        if (fileIndex < files.size()) {
                            if (files[fileIndex].isDir) {
                                if (payloadManager.navigateDown(files[fileIndex].name)) {
                                    selectedIndex = 0;
                                    scrollOffset = 0;
                                    showHTMLSelectScreen();
                                }
                            } else {
                                // It's a file, select it
                                selectedHTMLFile = payloadManager.getCurrentPath();
                                if (selectedHTMLFile != "/") selectedHTMLFile += "/";
                                selectedHTMLFile += files[fileIndex].name;
                                
                                if (nextModeAfterHTML == MODE_WIFI_PORTAL) proceedWithPortal();
                                else { currentMode = MODE_WIFI_MENU; showWiFiMenu(); }
                            }
                        }
                    }
                }
                else if (currentMode == MODE_WIFI_SCAN) {
                    std::vector<APInfo> results = wifiPentest.getResults();
                    if (selectedIndex < results.size()) {
                        selectedAP = results[selectedIndex];
                        currentMode = MODE_WIFI_ATTACK_SELECT;
                        selectedIndex = 0;
                        showWiFiAttackSelect();
                    }
                }
                else if (currentMode == MODE_WIFI_ATTACK_SELECT) {
                    if (selectedIndex == 0) { // Deauth
                        currentMode = MODE_WIFI_DEAUTH;
                        wifiPentest.startDeauth(selectedAP.bssid, selectedAP.channel);
                        showDeauthScreen();
                    } else if (selectedIndex == 1) { // Clone Portal
                        currentMode = MODE_SELECT_HTML;
                        nextModeAfterHTML = MODE_WIFI_PORTAL;
                        selectedIndex = 0;
                        htmlBrowserInitialized = false;
                        showHTMLSelectScreen();
                    } else { // Back
                        currentMode = MODE_WIFI_SCAN;
                        showWiFiScan();
                    }
                }
                else if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) {
                    std::vector<FileEntry> files = payloadManager.getFileList();
                    if (selectedIndex >= 0 && selectedIndex < files.size()) {
                        if (files[selectedIndex].isDir) {
                            if (payloadManager.navigateDown(files[selectedIndex].name)) {
                                selectedIndex = 0;
                                scrollOffset = 0;
                                showMainMenu();
                            }
                        } else {
                            currentMode = MODE_CONFIRM_EXECUTION;
                            currentPayload = files[selectedIndex].name;
                            showConfirmationScreen(currentPayload);
                        }
                    }
                }
                else if (currentMode == MODE_CONFIRM_EXECUTION) {
                    if (useBluetooth) executePayloadBluetooth();
                    else executePayloadUSB();
                }
                return;
            }
        }
    }
}

void handleButtonA() {
    // Basic execute functionality on button press if valid
    if (currentMode == MODE_CONFIRM_EXECUTION) {
        if (useBluetooth) executePayloadBluetooth();
        else executePayloadUSB();
    }
}

void moveSelectionUp(int maxItems) {
    if (maxItems > 0) {
        selectedIndex--;
        if (selectedIndex < 0) selectedIndex = maxItems - 1;
        
        // Refresh appropriate screen
        if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID || currentMode == MODE_MAIN_MENU) showMainMenu();
        else if (currentMode == MODE_WIFI_MENU) showWiFiMenu();
        else if (currentMode == MODE_WIFI_SCAN) showWiFiScan();
        else if (currentMode == MODE_WIFI_ATTACK_SELECT) showWiFiAttackSelect();
        else if (currentMode == MODE_SELECT_HTML) showHTMLSelectScreen();
    }
}

void moveSelectionDown(int maxItems) {
    if (maxItems > 0) {
        selectedIndex++;
        if (selectedIndex >= maxItems) selectedIndex = 0;
        
        // Refresh appropriate screen
        if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID || currentMode == MODE_MAIN_MENU) showMainMenu();
        else if (currentMode == MODE_WIFI_MENU) showWiFiMenu();
        else if (currentMode == MODE_WIFI_SCAN) showWiFiScan();
        else if (currentMode == MODE_WIFI_ATTACK_SELECT) showWiFiAttackSelect();
        else if (currentMode == MODE_SELECT_HTML) showHTMLSelectScreen();
    }
}

void startManualPortal(String ssid) {
    // Legacy function, replaced by proceedWithPortal
}

void proceedWithPortal() {
    currentMode = MODE_WIFI_PORTAL;
    
    // Check for custom HTML
    String htmlContent = "";
    // If a specific file was selected
    if (selectedHTMLFile.length() > 0) {
        if (SD.exists(selectedHTMLFile)) {
            File f = SD.open(selectedHTMLFile, FILE_READ);
            if (f) {
                htmlContent = f.readString();
                f.close();
            }
        }
    } 
    // Fallback to legacy behavior: check for portal.html if no selection made
    else if (SD.exists("/portal.html")) {
        File f = SD.open("/portal.html", FILE_READ);
        if (f) {
            htmlContent = f.readString();
            f.close();
        }
    }
    
    wifiPentest.startEvilPortal(selectedAP.ssid, htmlContent);
    showPortalScreen();
}

void showInputScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println(inputTitle);
    M5Cardputer.Display.println("--------------------");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println(inputText + "_"); // Cursor
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("ENTER: Confirm");
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("ESC: Cancel");
}

void showHTMLSelectScreen() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("=== Select HTML ===");
    
    std::vector<String> files = getHTMLFiles();
    // Item 0 is "Default"
    
    int maxItems = 5;
    if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    if (selectedIndex >= scrollOffset + maxItems) scrollOffset = selectedIndex - maxItems + 1;
    
    // Render "Default" option
    if (scrollOffset == 0) {
        M5Cardputer.Display.setTextColor(selectedIndex == 0 ? GREEN : WHITE);
        M5Cardputer.Display.println(selectedIndex == 0 ? "> Default / None" : "  Default / None");
    }

    // Render files
    for (int i = 0; i < files.size(); i++) {
        int menuIndex = i + 1;
        if (menuIndex >= scrollOffset && menuIndex < scrollOffset + maxItems) {
            if (menuIndex == selectedIndex) M5Cardputer.Display.setTextColor(GREEN);
            else M5Cardputer.Display.setTextColor(WHITE);
            
            M5Cardputer.Display.print(menuIndex == selectedIndex ? "> " : "  ");
            M5Cardputer.Display.println(files[i]);
        }
    }
}

void showModeSwitch(bool bluetooth) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(20, 30);
    M5Cardputer.Display.setTextSize(2);
    if (bluetooth) {
        M5Cardputer.Display.setTextColor(BLUE);
        M5Cardputer.Display.println("BLE MODE");
    } else {
        M5Cardputer.Display.setTextColor(GREEN);
        M5Cardputer.Display.println("USB MODE");
    }
    M5Cardputer.Display.setTextSize(1);
    delay(1000);
    showMainMenu();
}

void showMainMenu() {
    M5Cardputer.Display.clear();
    drawStatusBar();
    M5Cardputer.Display.setCursor(0, 12); // Offset for Status Bar
    
    // Top Cat Header (Reverted to text header if requested, but user asked for Boot Screen revert)
    // Keeping Cat Header for Main Menu as "Cat Themed Main Menu" was v0.3.0 request
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("   /\\_/\\  MeowTool");
    M5Cardputer.Display.println("  ( o.o ) " FW_VERSION);
    M5Cardputer.Display.println("--------------------");

    if (currentMode == MODE_MAIN_MENU) {
        M5Cardputer.Display.setTextColor(selectedIndex == 0 ? PINK : WHITE);
        M5Cardputer.Display.println(selectedIndex == 0 ? "> BadUSB / BLE HID" : "  BadUSB / BLE HID");
        
        M5Cardputer.Display.setTextColor(selectedIndex == 1 ? PINK : WHITE);
        M5Cardputer.Display.println(selectedIndex == 1 ? "> WiFi Pentest" : "  WiFi Pentest");
    } 
    else if (currentMode == MODE_USB_HID || currentMode == MODE_BT_HID) {
        // Status is now in Top Bar, show Path here
        M5Cardputer.Display.setTextColor(CYAN);
        M5Cardputer.Display.println("Path: " + payloadManager.getCurrentPath());
        
        std::vector<FileEntry> files = payloadManager.getFileList();
        int maxItems = 5;
        if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
        if (selectedIndex >= scrollOffset + maxItems) scrollOffset = selectedIndex - maxItems + 1;
        
        for (int i = scrollOffset; i < files.size() && i < scrollOffset + maxItems; i++) {
            if (i == selectedIndex) {
                M5Cardputer.Display.setTextColor(PINK);
                M5Cardputer.Display.print("> ");
            } else {
                M5Cardputer.Display.setTextColor(WHITE);
                M5Cardputer.Display.print("  ");
            }
            if (files[i].isDir) M5Cardputer.Display.print("[D] ");
            else M5Cardputer.Display.print("    ");
            M5Cardputer.Display.println(files[i].name);
        }
    }
}

void showWiFiMenu() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("=== WiFi Pentest ===");
    
    M5Cardputer.Display.setTextColor(selectedIndex == 0 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 0 ? "> Scan Networks" : "  Scan Networks");
    
    M5Cardputer.Display.setTextColor(selectedIndex == 1 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 1 ? "> Manual Portal" : "  Manual Portal");
    
    M5Cardputer.Display.setTextColor(selectedIndex == 2 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 2 ? "> Karma Attack" : "  Karma Attack");

    M5Cardputer.Display.setTextColor(selectedIndex == 3 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 3 ? "> Select HTML" : "  Select HTML");

    M5Cardputer.Display.setTextColor(selectedIndex == 4 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 4 ? "> Back" : "  Back");
}

void showWiFiScan() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.println("=== Scan Results ===");
    
    std::vector<APInfo> results = wifiPentest.getResults();
    int maxItems = 5;
    if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    if (selectedIndex >= scrollOffset + maxItems) scrollOffset = selectedIndex - maxItems + 1;
    
    if (results.empty()) {
        M5Cardputer.Display.println("No networks found.");
        return;
    }

    for (int i = scrollOffset; i < results.size() && i < scrollOffset + maxItems; i++) {
        if (i == selectedIndex) M5Cardputer.Display.setTextColor(GREEN);
        else M5Cardputer.Display.setTextColor(WHITE);
        
        M5Cardputer.Display.print(i == selectedIndex ? "> " : "  ");
        M5Cardputer.Display.print(results[i].ssid.substring(0, 14));
        M5Cardputer.Display.print(" ");
        M5Cardputer.Display.println(results[i].rssi);
    }
}

void showWiFiAttackSelect() {
    M5Cardputer.Display.clear();
    drawBatteryStatus();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("Target: " + selectedAP.ssid);
    
    M5Cardputer.Display.setTextColor(selectedIndex == 0 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 0 ? "> Deauth Attack" : "  Deauth Attack");
    
    M5Cardputer.Display.setTextColor(selectedIndex == 1 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 1 ? "> Clone Portal" : "  Clone Portal");
    
    M5Cardputer.Display.setTextColor(selectedIndex == 2 ? GREEN : WHITE);
    M5Cardputer.Display.println(selectedIndex == 2 ? "> Back" : "  Back");
}

void showDeauthScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("!!! DEAUTHING !!!");
    M5Cardputer.Display.println(selectedAP.ssid);
    M5Cardputer.Display.println("Channel: " + String(selectedAP.channel));
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Press ESC to stop");
}

void showPortalScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(BLUE);
    M5Cardputer.Display.println("=== EVIL PORTAL ===");
    M5Cardputer.Display.println("SSID: " + selectedAP.ssid);
    M5Cardputer.Display.println("IP: 192.168.4.1");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Waiting for login...");
    M5Cardputer.Display.println("Press ESC to stop");
}

void showKarmaScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(MAGENTA);
    M5Cardputer.Display.println("=== KARMA ATTACK ===");
    M5Cardputer.Display.println("Mode: Passive Monitor");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println("Check Serial Monitor");
    M5Cardputer.Display.println("for Probe Requests");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.println("Press ESC to stop");
}

void showBootScreen() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(PINK);
    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.println("      |\\      _,,,---,,_");
    M5Cardputer.Display.println("ZZZzz /,`.-'`'    -.  ;-;;,_");
    M5Cardputer.Display.println("     |,4-  ) )-,_. ,\\ (  `'-'");
    M5Cardputer.Display.println("    '---''(_/--'  `-'\\_)");
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(40, 80);
    M5Cardputer.Display.println("MeowTool");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(60, 105);
    M5Cardputer.Display.println(FW_VERSION);
}

void drawStatusBar() {
    M5Cardputer.Display.fillRect(0, 0, M5Cardputer.Display.width(), 10, BLACK);
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextSize(1);
    
    // Connection Status
    if (useBluetooth) {
        M5Cardputer.Display.setTextColor(CYAN);
        M5Cardputer.Display.print("BLE: ");
        M5Cardputer.Display.print(btHid.isConnected() ? "Conn" : "Disc");
    } else {
        M5Cardputer.Display.setTextColor(CYAN);
        M5Cardputer.Display.print("USB: ");
        M5Cardputer.Display.print(usbHid.isConnected() ? "Conn" : "Disc");
    }
    
    // Battery Status (Right Aligned)
    int bat = M5Cardputer.Power.getBatteryLevel();
    int xPos = M5Cardputer.Display.width() - 32;
    M5Cardputer.Display.setCursor(xPos, 0);
    if (bat > 60) M5Cardputer.Display.setTextColor(GREEN);
    else if (bat > 20) M5Cardputer.Display.setTextColor(YELLOW);
    else M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.print(bat);
    M5Cardputer.Display.println("%");
    
    // Separator line below status bar
    M5Cardputer.Display.drawLine(0, 11, M5Cardputer.Display.width(), 11, GRAY);
}

void drawBatteryStatus() {
    // Deprecated, use drawStatusBar instead, but kept for compatibility if needed
    drawStatusBar();
}

void showConfirmationScreen(String payloadName) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(YELLOW);
    M5Cardputer.Display.println("Execute Payload?");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println(payloadName);
    M5Cardputer.Display.println("");
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("ENTER: Yes");
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("ESC: Cancel");
}

void showExecutionScreen(String mode, String payloadName) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("Running " + mode + "...");
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.println(payloadName);
    M5Cardputer.Display.println("Press ESC to abort");
}

void showExecutionComplete() {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.println("Done!");
    delay(1000);
    showMainMenu();
}

void showError(String error) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.println("ERROR:");
    M5Cardputer.Display.println(error);
    delay(2000);
    showMainMenu();
}

void executePayloadUSB() {
    String content = payloadManager.loadFile(currentPayload);
    if (content.length() > 0) {
        showExecutionScreen("USB", currentPayload);
        duckyParser.setHIDDevice(&usbHid);
        duckyParser.execute(content);
        isExecuting = true;
    } else {
        showError("Empty Payload");
    }
}

void executePayloadBluetooth() {
    if (!btHid.isConnected()) {
        showError("BT Not Connected");
        return;
    }
    String content = payloadManager.loadFile(currentPayload);
    if (content.length() > 0) {
        showExecutionScreen("BT", currentPayload);
        duckyParser.setHIDDevice(&btHid);
        duckyParser.execute(content);
        isExecuting = true;
    } else {
        showError("Empty Payload");
    }
}
