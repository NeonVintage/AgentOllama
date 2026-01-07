#include "file_manager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace ollama_agent {

FileManager::FileManager(const std::string& workingDir) {
    workingDir_ = std::filesystem::absolute(workingDir);
    if (!std::filesystem::exists(workingDir_)) {
        std::filesystem::create_directories(workingDir_);
    }
}

void FileManager::setWorkingDirectory(const std::string& path) {
    workingDir_ = std::filesystem::absolute(path);
    if (!std::filesystem::exists(workingDir_)) {
        std::filesystem::create_directories(workingDir_);
    }
}

std::string FileManager::getWorkingDirectory() const {
    return workingDir_.string();
}

std::filesystem::path FileManager::resolvePath(const std::string& relativePath) const {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path;
    }
    return workingDir_ / path;
}

bool FileManager::ensureParentDirs(const std::filesystem::path& path) {
    try {
        std::filesystem::path parentDir = path.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create parent directories: ") + e.what();
        return false;
    }
}

bool FileManager::createFile(const std::string& relativePath, const std::string& content) {
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        
        if (!ensureParentDirs(fullPath)) {
            return false;
        }
        
        std::ofstream file(fullPath);
        if (!file.is_open()) {
            lastError_ = "Failed to open file for writing: " + fullPath.string();
            return false;
        }
        
        file << content;
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create file: ") + e.what();
        return false;
    }
}

std::string FileManager::readFile(const std::string& relativePath) const {
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        
        if (!std::filesystem::exists(fullPath)) {
            return "";
        }
        
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            return "";
        }
        
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    } catch (const std::exception&) {
        return "";
    }
}

bool FileManager::fileExists(const std::string& relativePath) const {
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        return std::filesystem::exists(fullPath);
    } catch (const std::exception&) {
        return false;
    }
}

bool FileManager::createDirectory(const std::string& relativePath) {
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        if (std::filesystem::exists(fullPath)) {
            return true;
        }
        return std::filesystem::create_directories(fullPath);
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create directory: ") + e.what();
        return false;
    }
}

std::vector<std::string> FileManager::listFiles(const std::string& relativePath) const {
    std::vector<std::string> files;
    
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        
        if (!std::filesystem::exists(fullPath) || !std::filesystem::is_directory(fullPath)) {
            return files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
            files.push_back(entry.path().filename().string());
        }
    } catch (const std::exception&) {
        // Return empty list on error
    }
    
    return files;
}

bool FileManager::executeOperations(const std::vector<FileOperation>& operations) {
    bool allSuccess = true;
    
    for (const auto& op : operations) {
        if (!createFile(op.path, op.content)) {
            allSuccess = false;
            std::cerr << "Failed to create file: " << op.path << std::endl;
        }
    }
    
    return allSuccess;
}

std::string FileManager::getLastError() const {
    return lastError_;
}

bool FileManager::deleteFile(const std::string& relativePath) {
    try {
        std::filesystem::path fullPath = resolvePath(relativePath);
        if (std::filesystem::exists(fullPath)) {
            return std::filesystem::remove(fullPath);
        }
        return true;  // File doesn't exist, consider it deleted
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to delete file: ") + e.what();
        return false;
    }
}

} // namespace ollama_agent

