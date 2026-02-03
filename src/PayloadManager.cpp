#include "PayloadManager.h"

PayloadManager::PayloadManager() {
    currentStorage = STORAGE_ROOT_SELECT;
    currentPath = "/";
}

bool PayloadManager::begin() {
    // SD initialized in main.cpp
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    } else {
        Serial.println("LittleFS Mounted");
    }
    
    refresh();
    return true;
}

void PayloadManager::scanDirectory(fs::FS &fs, const String& path) {
    currentFiles.clear();
    
    File root = fs.open(path);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open directory: " + path);
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        FileEntry entry;
        String fileName = String(file.name());
        
        // Handle full paths if returned by FS
        int lastSlash = fileName.lastIndexOf('/');
        if (lastSlash >= 0) fileName = fileName.substring(lastSlash + 1);
        
        entry.name = fileName;
        entry.isDir = file.isDirectory();
        
        currentFiles.push_back(entry);
        file = root.openNextFile();
    }
    root.close();
}

void PayloadManager::refresh() {
    currentFiles.clear();
    
    if (currentStorage == STORAGE_ROOT_SELECT) {
        currentFiles.push_back({"SD Card", true});
        currentFiles.push_back({"Internal Storage", true});
    } else if (currentStorage == STORAGE_SD) {
        scanDirectory(SD, currentPath);
    } else if (currentStorage == STORAGE_LITTLEFS) {
        scanDirectory(LittleFS, currentPath);
    }
}

void PayloadManager::navigateUp() {
    if (currentStorage == STORAGE_ROOT_SELECT) return;
    
    if (currentPath == "/") {
        currentStorage = STORAGE_ROOT_SELECT;
        currentPath = "/";
    } else {
        int lastSlash = currentPath.lastIndexOf('/');
        if (lastSlash == 0) {
            currentPath = "/";
        } else if (lastSlash > 0) {
            currentPath = currentPath.substring(0, lastSlash);
        }
    }
    refresh();
}

bool PayloadManager::navigateDown(const String& name) {
    if (currentStorage == STORAGE_ROOT_SELECT) {
        if (name == "SD Card") {
            currentStorage = STORAGE_SD;
            currentPath = "/";
            refresh();
            return true;
        } else if (name == "Internal Storage") {
            currentStorage = STORAGE_LITTLEFS;
            currentPath = "/";
            refresh();
            return true;
        }
        return false;
    }
    
    String newPath = currentPath;
    if (newPath != "/") newPath += "/";
    newPath += name;
    
    fs::FS* fs = (currentStorage == STORAGE_SD) ? (fs::FS*)&SD : (fs::FS*)&LittleFS;
    File f = fs->open(newPath);
    if (f && f.isDirectory()) {
        currentPath = newPath;
        f.close();
        refresh();
        return true;
    }
    if (f) f.close();
    return false;
}

void PayloadManager::selectStorage(StorageType type) {
    currentStorage = type;
    currentPath = "/";
    refresh();
}

std::vector<FileEntry> PayloadManager::getFileList() {
    return currentFiles;
}

String PayloadManager::getCurrentPath() {
    if (currentStorage == STORAGE_ROOT_SELECT) return "Select Drive";
    return currentPath;
}

StorageType PayloadManager::getCurrentStorage() {
    return currentStorage;
}

String PayloadManager::loadFile(const String& filename) {
    if (currentStorage == STORAGE_ROOT_SELECT) return "";
    
    String fullPath = currentPath;
    if (fullPath != "/") fullPath += "/";
    fullPath += filename;
    
    fs::FS* fs = (currentStorage == STORAGE_SD) ? (fs::FS*)&SD : (fs::FS*)&LittleFS;
    
    File file = fs->open(fullPath, FILE_READ);
    if (!file) return "";
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    return content;
}