#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace ollama_agent {

// Represents a file to be created/modified
struct FileOperation {
    std::string path;
    std::string content;
    bool isNew = true;  // true = create, false = modify
};

class FileManager {
public:
    explicit FileManager(const std::string& workingDir = ".");
    
    // Set the working directory for file operations
    void setWorkingDirectory(const std::string& path);
    
    // Get current working directory
    std::string getWorkingDirectory() const;
    
    // Create a file with content
    bool createFile(const std::string& relativePath, const std::string& content);
    
    // Read a file's content
    std::string readFile(const std::string& relativePath) const;
    
    // Check if file exists
    bool fileExists(const std::string& relativePath) const;
    
    // Create directory (and parent directories if needed)
    bool createDirectory(const std::string& relativePath);
    
    // List files in directory
    std::vector<std::string> listFiles(const std::string& relativePath = ".") const;
    
    // Execute multiple file operations
    bool executeOperations(const std::vector<FileOperation>& operations);
    
    // Get last error message
    std::string getLastError() const;
    
    // Delete a file
    bool deleteFile(const std::string& relativePath);

private:
    std::filesystem::path workingDir_;
    std::string lastError_;
    
    // Resolve relative path to absolute
    std::filesystem::path resolvePath(const std::string& relativePath) const;
    
    // Ensure parent directories exist
    bool ensureParentDirs(const std::filesystem::path& path);
};

} // namespace ollama_agent

