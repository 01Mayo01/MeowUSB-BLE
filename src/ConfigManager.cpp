#include "ConfigManager.h"
#include <SD.h>
#include <LittleFS.h>

ConfigManager::ConfigManager() {
    configFilePath = "/config.json";
    bluetoothName = getDefaultBluetoothName(); // Default name
}

bool ConfigManager::loadConfig() {
    File configFile;
    
    // Try SD card first, then LittleFS
    if (SD.exists(configFilePath)) {
        configFile = SD.open(configFilePath, FILE_READ);
    } else if (LittleFS.exists(configFilePath)) {
        configFile = LittleFS.open(configFilePath, "r");
    } else {
        // No config file exists, use defaults
        bluetoothName = getDefaultBluetoothName();
        return true;
    }
    
    if (!configFile) {
        bluetoothName = getDefaultBluetoothName();
        return false;
    }
    
    String configContent = configFile.readString();
    configFile.close();
    
    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, configContent);
    
    if (error) {
        bluetoothName = getDefaultBluetoothName();
        return false;
    }
    
    // Load settings
    bluetoothName = doc["bluetooth_name"] | getDefaultBluetoothName();
    
    return true;
}

bool ConfigManager::saveConfig() {
    // Create JSON
    StaticJsonDocument<256> doc;
    doc["bluetooth_name"] = bluetoothName;
    
    // Try to save to SD card first
    if (SD.exists("/")) {
        File configFile = SD.open(configFilePath, FILE_WRITE);
        if (configFile) {
            if (serializeJson(doc, configFile) == 0) {
                configFile.close();
                return false;
            }
            configFile.close();
            return true;
        }
    }
    
    // Fallback to LittleFS
    if (LittleFS.exists("/")) {
        File configFile = LittleFS.open(configFilePath, "w");
        if (configFile) {
            if (serializeJson(doc, configFile) == 0) {
                configFile.close();
                return false;
            }
            configFile.close();
            return true;
        }
    }
    
    return false;
}