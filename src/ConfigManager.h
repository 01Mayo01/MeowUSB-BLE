#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigManager {
private:
    String bluetoothName;
    String configFilePath;
    
public:
    ConfigManager();
    
    bool loadConfig();
    bool saveConfig();
    
    String getBluetoothName() { return bluetoothName; }
    void setBluetoothName(const String& name) { bluetoothName = name; }
    
    String getDefaultBluetoothName() { return "M5-Ducky"; }
};

#endif // CONFIG_MANAGER_H