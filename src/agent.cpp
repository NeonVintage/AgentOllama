#include "agent.hpp"
#include <iostream>
#include <regex>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace ollama_agent {

Agent::Agent(OllamaClient& client, FileManager& fileManager)
    : client_(client), fileManager_(fileManager) {}

std::string Agent::getExistingFilesContext() const {
    std::ostringstream context;
    std::string workDir = fileManager_.getWorkingDirectory();
    
    // Extensions we care about
    static const std::set<std::string> codeExtensions = {
        "html", "htm", "css", "scss", "js", "jsx", "ts", "tsx",
        "py", "c", "cpp", "h", "hpp", "java", "rs", "go",
        "json", "xml", "yaml", "yml", "md", "txt", "sh", "bat"
    };
    
    std::vector<std::pair<std::string, std::string>> existingFiles;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                workDir, std::filesystem::directory_options::skip_permission_denied)) {
            
            if (!entry.is_regular_file()) continue;
            
            std::string filename = entry.path().filename().string();
            if (filename[0] == '.') continue;  // Skip hidden files
            
            // Get extension
            size_t dotPos = filename.rfind('.');
            if (dotPos == std::string::npos) continue;
            
            std::string ext = filename.substr(dotPos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (codeExtensions.find(ext) == codeExtensions.end()) continue;
            
            // Get relative path
            std::string relativePath = std::filesystem::relative(entry.path(), workDir).string();
            // Convert backslashes to forward slashes
            std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
            
            // Read content
            std::string content = fileManager_.readFile(relativePath);
            if (content.empty()) continue;
            
            // Limit file size
            if (content.length() > 30000) {
                content = content.substr(0, 1000) + "\n\n... [FILE TRUNCATED] ...\n";
            }
            
            existingFiles.push_back({relativePath, content});
            
            // Limit number of files
            if (existingFiles.size() >= 20) break;
        }
    } catch (...) {
        // Ignore errors
    }
    
    if (existingFiles.empty()) {
        return "";
    }
    
    context << "\n\n--- EXISTING FILES IN PROJECT ---\n";
    context << "These files already exist. When modifying, output the COMPLETE updated file.\n\n";
    
    for (const auto& [path, content] : existingFiles) {
        context << "FILE: " << path << "\n```\n" << content << "\n```\n\n";
    }
    
    context << "--- END EXISTING FILES ---\n";
    
    return context.str();
}

std::string Agent::buildSystemPrompt() const {
    return R"(You are a code generation assistant. You create and modify files.

CRITICAL: You MUST output files in this EXACT format - no exceptions:

FILE: index.html
```html
<!DOCTYPE html>
<html>
...content...
</html>
```

FILE: styles.css
```css
body {
    ...styles...
}
```

STRICT RULES:
1. EVERY file MUST start with "FILE: filename.ext" on its own line
2. The code block MUST come immediately after
3. Output the COMPLETE file content, not just changes
4. When modifying existing files, include ALL the original content plus your changes
5. If multiple files need changes, output ALL of them
6. Do NOT skip any files - if styles.css exists and needs updating, output it

Working directory: )" + fileManager_.getWorkingDirectory() + R"(

When existing files are provided, output updated versions of ALL files that need changes.)";
}

std::string Agent::trimString(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string Agent::extractFilenameFromText(const std::string& text) const {
    std::string cleaned = text;
    
    // Remove markdown formatting
    size_t pos = 0;
    while ((pos = cleaned.find('*')) != std::string::npos) cleaned.erase(pos, 1);
    while ((pos = cleaned.find('`')) != std::string::npos) cleaned.erase(pos, 1);
    while ((pos = cleaned.find('#')) != std::string::npos) cleaned.erase(pos, 1);
    
    // Remove "FILE:" prefix if present
    std::string upper = cleaned;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper.find("FILE:") == 0 || upper.find("FILE :") == 0) {
        size_t colonPos = cleaned.find(':');
        if (colonPos != std::string::npos) {
            cleaned = cleaned.substr(colonPos + 1);
        }
    }
    
    return trimString(cleaned);
}

bool Agent::looksLikeFilename(const std::string& text) const {
    std::string trimmed = trimString(text);
    
    // Must have an extension (dot followed by letters)
    size_t dotPos = trimmed.rfind('.');
    if (dotPos == std::string::npos || dotPos == 0 || dotPos >= trimmed.length() - 1) {
        return false;
    }
    
    // Extension should be 1-5 alphanumeric characters
    std::string ext = trimmed.substr(dotPos + 1);
    if (ext.length() < 1 || ext.length() > 5) return false;
    
    for (char c : ext) {
        if (!std::isalnum(c)) return false;
    }
    
    // Filename part should be reasonable
    std::string name = trimmed.substr(0, dotPos);
    if (name.empty() || name.length() > 100) return false;
    
    // Should contain valid filename characters
    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '/' && c != '\\' && c != '.') {
            return false;
        }
    }
    
    return true;
}

std::vector<ParsedFile> Agent::parseFilesFromResponse(const std::string& response) const {
    std::vector<ParsedFile> files;
    
    std::istringstream stream(response);
    std::string line;
    std::string pendingFilename;
    std::string currentContent;
    std::string currentLang;
    bool inCodeBlock = false;
    int codeBlockCount = 0;
    std::vector<std::string> recentLines;  // Keep track of recent lines before code block
    
    // Language to extension mapping
    std::map<std::string, std::string> langToExt = {
        {"html", "html"}, {"htm", "html"},
        {"css", "css"}, {"scss", "scss"},
        {"javascript", "js"}, {"js", "js"}, {"jsx", "jsx"},
        {"typescript", "ts"}, {"ts", "ts"}, {"tsx", "tsx"},
        {"python", "py"}, {"py", "py"},
        {"cpp", "cpp"}, {"c++", "cpp"}, {"cxx", "cpp"},
        {"c", "c"}, {"java", "java"},
        {"rust", "rs"}, {"go", "go"},
        {"ruby", "rb"}, {"php", "php"},
        {"json", "json"}, {"xml", "xml"},
        {"yaml", "yaml"}, {"yml", "yml"},
        {"bash", "sh"}, {"sh", "sh"}, {"shell", "sh"},
        {"bat", "bat"}, {"cmd", "cmd"},
        {"sql", "sql"}, {"md", "md"}
    };
    
    auto generateFilename = [&](const std::string& lang, int index) -> std::string {
        std::string lowerLang = lang;
        std::transform(lowerLang.begin(), lowerLang.end(), lowerLang.begin(), ::tolower);
        
        auto it = langToExt.find(lowerLang);
        std::string ext = (it != langToExt.end()) ? it->second : lang;
        
        if (ext == "html") return (index == 0) ? "index.html" : "page" + std::to_string(index) + ".html";
        if (ext == "css") return (index == 0) ? "styles.css" : "styles" + std::to_string(index) + ".css";
        if (ext == "js") return (index == 0) ? "script.js" : "script" + std::to_string(index) + ".js";
        
        return "file" + std::to_string(index) + "." + ext;
    };
    
    // Helper to find filename in recent lines
    auto findFilenameInRecentLines = [&]() -> std::string {
        for (auto it = recentLines.rbegin(); it != recentLines.rend(); ++it) {
            std::string trimmed = trimString(*it);
            if (trimmed.empty()) continue;
            
            // Check various patterns
            std::string upper = trimmed;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            
            // FILE: pattern
            if (upper.find("FILE:") != std::string::npos || upper.find("FILE :") != std::string::npos) {
                size_t colonPos = trimmed.find(':');
                if (colonPos != std::string::npos) {
                    std::string possibleFile = extractFilenameFromText(trimmed.substr(colonPos + 1));
                    if (looksLikeFilename(possibleFile)) return possibleFile;
                }
            }
            
            // Direct filename patterns (with or without markdown)
            std::string extracted = extractFilenameFromText(trimmed);
            if (looksLikeFilename(extracted)) return extracted;
            
            // Pattern: "Updated index.html:" or "Here's the index.html file:"
            std::regex filenameRegex(R"(([a-zA-Z0-9_\-./]+\.[a-zA-Z0-9]{1,5}))");
            std::smatch match;
            if (std::regex_search(trimmed, match, filenameRegex)) {
                std::string found = match[1].str();
                if (looksLikeFilename(found)) return found;
            }
        }
        return "";
    };
    
    while (std::getline(stream, line)) {
        std::string trimmedLine = trimString(line);
        
        // Check for code block markers - be more flexible
        bool isCodeBlockMarker = false;
        size_t tickPos = line.find("```");
        if (tickPos != std::string::npos) {
            isCodeBlockMarker = true;
        }
        
        if (isCodeBlockMarker) {
            if (!inCodeBlock) {
                // Starting a code block
                inCodeBlock = true;
                currentContent.clear();
                
                // Extract language hint after ```
                size_t langStart = tickPos + 3;
                if (langStart < line.length()) {
                    currentLang = line.substr(langStart);
                    // Remove any trailing characters
                    size_t spacePos = currentLang.find(' ');
                    if (spacePos != std::string::npos) {
                        currentLang = currentLang.substr(0, spacePos);
                    }
                    currentLang = trimString(currentLang);
                }
                
                // Try to find filename from pending or recent lines
                if (pendingFilename.empty()) {
                    pendingFilename = findFilenameInRecentLines();
                }
            } else {
                // Ending a code block
                inCodeBlock = false;
                
                // Determine filename
                std::string filename;
                
                if (!pendingFilename.empty()) {
                    filename = pendingFilename;
                    pendingFilename.clear();
                } else if (!currentLang.empty()) {
                    filename = generateFilename(currentLang, codeBlockCount);
                }
                
                // Save the file if we have content
                std::string trimmedContent = trimString(currentContent);
                if (!filename.empty() && !trimmedContent.empty()) {
                    ParsedFile file;
                    file.filename = filename;
                    file.content = currentContent;
                    
                    // Trim trailing whitespace from content
                    size_t endPos = file.content.find_last_not_of(" \t\n\r");
                    if (endPos != std::string::npos) {
                        file.content = file.content.substr(0, endPos + 1);
                    }
                    
                    // Get extension as language
                    size_t dotPos = filename.rfind('.');
                    if (dotPos != std::string::npos) {
                        file.language = filename.substr(dotPos + 1);
                    }
                    
                    if (verbose_) {
                        std::cout << "[Parser] Found file: " << filename << " (" << file.content.length() << " bytes)" << std::endl;
                    }
                    
                    files.push_back(file);
                    codeBlockCount++;
                }
                
                currentContent.clear();
                currentLang.clear();
                recentLines.clear();  // Clear recent lines after processing a code block
            }
            continue;
        }
        
        if (inCodeBlock) {
            // Inside code block - accumulate content
            if (!currentContent.empty()) {
                currentContent += "\n";
            }
            currentContent += line;
        } else {
            // Outside code block - track recent lines and look for filename indicators
            recentLines.push_back(line);
            if (recentLines.size() > 5) {
                recentLines.erase(recentLines.begin());
            }
            
            std::string upperLine = trimmedLine;
            std::transform(upperLine.begin(), upperLine.end(), upperLine.begin(), ::toupper);
            
            // Check for "FILE: filename" pattern - priority
            if (upperLine.find("FILE:") != std::string::npos || upperLine.find("FILE :") != std::string::npos) {
                size_t colonPos = trimmedLine.find(':');
                if (colonPos != std::string::npos) {
                    std::string possibleFile = extractFilenameFromText(trimmedLine.substr(colonPos + 1));
                    if (looksLikeFilename(possibleFile)) {
                        pendingFilename = possibleFile;
                        if (verbose_) {
                            std::cout << "[Parser] Found FILE: marker -> " << pendingFilename << std::endl;
                        }
                    }
                }
            }
            // Check for **filename.ext** or `filename.ext` patterns
            else if (looksLikeFilename(extractFilenameFromText(trimmedLine))) {
                std::string extracted = extractFilenameFromText(trimmedLine);
                if (trimmedLine.length() < 100) {
                    pendingFilename = extracted;
                    if (verbose_) {
                        std::cout << "[Parser] Found filename pattern -> " << pendingFilename << std::endl;
                    }
                }
            }
        }
    }
    
    if (verbose_ && files.empty()) {
        std::cout << "[Parser] No files detected. Code blocks found: " << codeBlockCount << std::endl;
    }
    
    return files;
}

bool Agent::executeFileCreation(const std::vector<ParsedFile>& files) {
    createdFiles_.clear();
    bool allSuccess = true;
    
    for (const auto& file : files) {
        printStatus("Creating file: " + file.filename);
        
        if (fileManager_.createFile(file.filename, file.content)) {
            createdFiles_.push_back(file.filename);
            std::cout << "  [+] Created: " << file.filename << std::endl;
        } else {
            std::cerr << "  [!] Failed to create: " << file.filename << " - " << fileManager_.getLastError() << std::endl;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::string Agent::extractExplanation(const std::string& response) const {
    std::string explanation;
    std::istringstream stream(response);
    std::string line;
    bool inCodeBlock = false;
    
    while (std::getline(stream, line)) {
        if (line.find("```") == 0 || trimString(line).find("```") == 0) {
            inCodeBlock = !inCodeBlock;
            continue;
        }
        
        if (!inCodeBlock && !line.empty()) {
            // Skip lines that look like file headers
            std::string upper = line;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper.find("FILE:") != std::string::npos) continue;
            
            explanation += line + "\n";
        }
    }
    
    return explanation;
}

void Agent::printStatus(const std::string& message) const {
    if (verbose_) {
        std::cout << "[Agent] " << message << std::endl;
    }
}

bool Agent::processRequest(const std::string& userRequest) {
    printStatus("Processing request: " + userRequest);
    
    // Build system prompt
    std::string systemPrompt = buildSystemPrompt();
    
    // Get existing files context
    std::string existingFiles = getExistingFilesContext();
    
    // Combine user request with existing files
    std::string fullRequest = userRequest;
    if (!existingFiles.empty()) {
        fullRequest = userRequest + existingFiles;
        std::cout << "[i] Including existing project files in context..." << std::endl;
    }
    
    printStatus("Sending request to Ollama...");
    std::string response = client_.chat(systemPrompt, fullRequest);
    
    if (response.empty()) {
        lastResponse_ = "Error: Failed to get response from Ollama. " + client_.getLastError();
        std::cerr << lastResponse_ << std::endl;
        return false;
    }
    
    lastResponse_ = response;
    printStatus("Received response from Ollama");
    
    // Debug: show raw response in verbose mode
    if (verbose_) {
        std::cout << "\n=== RAW RESPONSE ===\n" << response << "\n=== END RESPONSE ===\n" << std::endl;
    }
    
    // Parse files from response
    std::vector<ParsedFile> files = parseFilesFromResponse(response);
    
    if (files.empty()) {
        std::cout << "\n" << response << std::endl;
        std::cout << "\n[No files detected in response]" << std::endl;
        
        // Count code blocks to help debug
        int codeBlocks = 0;
        size_t pos = 0;
        while ((pos = response.find("```", pos)) != std::string::npos) {
            codeBlocks++;
            pos += 3;
        }
        codeBlocks /= 2;  // Opening and closing
        
        if (codeBlocks > 0) {
            std::cout << "[Debug] Found " << codeBlocks << " code block(s) but couldn't extract filenames." << std::endl;
            std::cout << "[Debug] The model may not be using the expected format." << std::endl;
        }
        std::cout << "Tip: Use /verbose for detailed debug output." << std::endl;
        return true;
    }
    
    // Print explanation
    std::string explanation = extractExplanation(response);
    if (!explanation.empty()) {
        std::cout << "\n" << explanation << std::endl;
    }
    
    // Execute file creation
    std::cout << "\nCreating " << files.size() << " file(s)..." << std::endl;
    bool success = executeFileCreation(files);
    
    if (success) {
        std::cout << "\n[OK] All files created successfully!" << std::endl;
        std::cout << "Location: " << fileManager_.getWorkingDirectory() << std::endl;
    }
    
    // Update context summary
    contextSummary_ = "Created files: ";
    for (size_t i = 0; i < createdFiles_.size(); ++i) {
        if (i > 0) contextSummary_ += ", ";
        contextSummary_ += createdFiles_[i];
    }
    
    return success;
}

std::string Agent::getLastResponse() const {
    return lastResponse_;
}

std::vector<std::string> Agent::getCreatedFiles() const {
    return createdFiles_;
}

void Agent::setVerbose(bool verbose) {
    verbose_ = verbose;
}

std::string Agent::getContextSummary() const {
    return contextSummary_;
}

} // namespace ollama_agent
