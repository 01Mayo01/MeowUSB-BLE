#ifndef PAYLOAD_MANAGER_H
#define PAYLOAD_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include <LittleFS.h>
#include <vector>

struct FileEntry {
    String name;
    bool isDir;
};

enum StorageType {
    STORAGE_ROOT_SELECT, // Virtual root to select drive
    STORAGE_SD,
    STORAGE_LITTLEFS
};

class PayloadManager {
private:
    StorageType currentStorage;
    String currentPath;
    std::vector<FileEntry> currentFiles;
    
    void scanDirectory(fs::FS &fs, const String& path);

public:
    PayloadManager();
    bool begin();
    
    // Navigation
    void navigateUp();
    bool navigateDown(const String& name);
    void selectStorage(StorageType type);
    
    // Getters
    std::vector<FileEntry> getFileList();
    String getCurrentPath();
    StorageType getCurrentStorage();
    
    // File Operations
    String loadFile(const String& filename);
    File openFile(const String& filename); // Open file for streaming
    
    void refresh();
};

#endif // PAYLOAD_MANAGER_H