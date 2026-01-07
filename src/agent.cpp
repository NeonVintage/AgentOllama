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
    
    context << "\n\n=== EXISTING PROJECT FILES ===\n";
    context << "Below are the current files. To modify any file, you MUST output the COMPLETE updated content.\n";
    context << "Use the format: FILE: filename.ext followed by code block with FULL content.\n\n";
    
    for (const auto& [path, content] : existingFiles) {
        context << "CURRENT FILE: " << path << " (" << content.length() << " bytes)\n```\n" << content << "\n```\n\n";
    }
    
    context << "=== END EXISTING FILES ===\n";
    context << "IMPORTANT: When modifying files above, output the ENTIRE file with all changes included.\n";
    context << "When creating NEW files, use FILE: newfilename.ext format.\n";
    
    return context.str();
}

std::string Agent::buildSystemPrompt() const {
    return R"(You are a code generation assistant that creates and modifies files.

OUTPUT FORMAT - You MUST use this EXACT format for EVERY file:

FILE: index.html
```html
<!DOCTYPE html>
<html>
<head>...</head>
<body>...</body>
</html>
```

FILE: about.html
```html
<!DOCTYPE html>
...complete file content...
```

CRITICAL RULES:
1. Start each file with "FILE: filename.ext" on its own line
2. Follow immediately with a code block containing the COMPLETE file
3. NEVER output partial files or diffs - always the ENTIRE file content
4. When modifying: output the FULL updated file, not just the changed parts
5. When creating new pages: output EACH new file separately with FILE: marker
6. If user asks for links/navigation: update ALL pages that need the links

WHEN USER ASKS FOR NEW PAGES OR LINKS:
- Create each new HTML file with FILE: marker
- Update index.html to include links to new pages
- Include navigation in ALL pages
- Output ALL files that need changes

Working directory: )" + fileManager_.getWorkingDirectory() + R"(

Remember: Output COMPLETE files. Every file needs FILE: marker followed by code block.)";
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
    std::map<std::string, size_t> fileIndexByName;  // Track files by name to deduplicate
    
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
                    
                    // Check for duplicate filename - keep the latest version
                    auto it = fileIndexByName.find(filename);
                    if (it != fileIndexByName.end()) {
                        // Replace existing file with newer version
                        files[it->second] = file;
                        if (verbose_) {
                            outputMessage("[Parser] Updated file: " + filename + " (" + std::to_string(file.content.length()) + " bytes) - replacing previous version");
                        }
                    } else {
                        // New file
                        fileIndexByName[filename] = files.size();
                        files.push_back(file);
                        if (verbose_) {
                            outputMessage("[Parser] Found file: " + filename + " (" + std::to_string(file.content.length()) + " bytes)");
                        }
                    }
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
                            outputMessage("[Parser] Found FILE: marker -> " + pendingFilename);
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
                        outputMessage("[Parser] Found filename pattern -> " + pendingFilename);
                    }
                }
            }
        }
    }
    
    if (verbose_ && files.empty()) {
        outputMessage("[Parser] No files detected. Code blocks found: " + std::to_string(codeBlockCount));
    }
    
    // Check if model seems to be ignoring instructions
    if (verbose_ && !files.empty()) {
        outputMessage("[Parser] Total unique files: " + std::to_string(files.size()));
    }
    
    return files;
}

bool Agent::executeFileCreation(const std::vector<ParsedFile>& files) {
    createdFiles_.clear();
    bool allSuccess = true;
    std::string workDir = fileManager_.getWorkingDirectory();
    
    outputMessage("[Write] Target directory: " + workDir);
    
    for (const auto& file : files) {
        std::string fullPath = workDir + "/" + file.filename;
        // Normalize path separators for Windows
        std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
        
        // Check if file exists and get old size
        std::string existingContent = fileManager_.readFile(file.filename);
        bool fileExists = !existingContent.empty();
        
        outputMessage("[Write] " + file.filename + ":");
        outputMessage("        Full path: " + fullPath);
        outputMessage("        New content: " + std::to_string(file.content.length()) + " bytes");
        if (fileExists) {
            outputMessage("        Old content: " + std::to_string(existingContent.length()) + " bytes");
            if (existingContent == file.content) {
                outputMessage("        WARNING: Content unchanged!");
            }
        } else {
            outputMessage("        (new file)");
        }
        
        if (verbose_) {
            // Show first 200 chars of content being written
            std::string preview = file.content.substr(0, 200);
            if (file.content.length() > 200) preview += "...";
            outputMessage("        Preview: " + preview);
        }
        
        if (fileManager_.createFile(file.filename, file.content)) {
            createdFiles_.push_back(file.filename);
            
            // Verify the write
            std::string verifyContent = fileManager_.readFile(file.filename);
            if (verifyContent == file.content) {
                outputMessage("  [+] SUCCESS: " + file.filename + " (" + std::to_string(file.content.length()) + " bytes written)");
            } else if (verifyContent.empty()) {
                outputMessage("  [!] VERIFY FAILED: File appears empty after write!");
                allSuccess = false;
            } else {
                outputMessage("  [?] PARTIAL: Written " + std::to_string(verifyContent.length()) + " of " + std::to_string(file.content.length()) + " bytes");
            }
        } else {
            outputMessage("  [!] FAILED: " + file.filename + " - " + fileManager_.getLastError());
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

void Agent::setOutputCallback(OutputCallback callback) {
    outputCallback_ = callback;
}

void Agent::outputMessage(const std::string& message) const {
    if (outputCallback_) {
        outputCallback_(message);
    } else {
        std::cout << message << std::endl;
    }
}

void Agent::printStatus(const std::string& message) const {
    if (verbose_) {
        outputMessage("[Agent] " + message);
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
        outputMessage("[i] Including existing project files in context...");
        if (verbose_) {
            outputMessage("[i] Context size: " + std::to_string(existingFiles.length()) + " bytes");
        }
    } else {
        if (verbose_) {
            outputMessage("[i] No existing files found in: " + fileManager_.getWorkingDirectory());
        }
    }
    
    printStatus("Sending request to Ollama...");
    std::string response = client_.chat(systemPrompt, fullRequest);
    
    if (response.empty()) {
        lastResponse_ = "Error: Failed to get response from Ollama. " + client_.getLastError();
        outputMessage(lastResponse_);
        return false;
    }
    
    lastResponse_ = response;
    printStatus("Received response from Ollama");
    
    // Debug: show raw response in verbose mode
    if (verbose_) {
        outputMessage("\n=== RAW RESPONSE ===\n" + response + "\n=== END RESPONSE ===\n");
    }
    
    // Parse files from response
    std::vector<ParsedFile> files = parseFilesFromResponse(response);
    
    if (files.empty()) {
        outputMessage("\n" + response);
        outputMessage("\n[No files detected in response]");
        
        // Count code blocks to help debug
        int codeBlocks = 0;
        size_t pos = 0;
        while ((pos = response.find("```", pos)) != std::string::npos) {
            codeBlocks++;
            pos += 3;
        }
        codeBlocks /= 2;  // Opening and closing
        
        if (codeBlocks > 0) {
            outputMessage("[Debug] Found " + std::to_string(codeBlocks) + " code block(s) but couldn't extract filenames.");
            outputMessage("[Debug] The model may not be using the expected format.");
        }
        outputMessage("Tip: Enable verbose mode for detailed debug output.");
        return true;
    }
    
    // Print explanation
    std::string explanation = extractExplanation(response);
    if (!explanation.empty()) {
        outputMessage("\n" + explanation);
    }
    
    // Execute file creation
    outputMessage("\nCreating " + std::to_string(files.size()) + " file(s)...");
    bool success = executeFileCreation(files);
    
    if (success) {
        outputMessage("\n[OK] All files created successfully!");
        outputMessage("Location: " + fileManager_.getWorkingDirectory());
    }
    
    // Check if model might have ignored parts of the request
    std::string lowerRequest = userRequest;
    std::transform(lowerRequest.begin(), lowerRequest.end(), lowerRequest.begin(), ::tolower);
    
    bool expectedCss = (lowerRequest.find("css") != std::string::npos || 
                        lowerRequest.find("style") != std::string::npos);
    bool expectedNewPages = (lowerRequest.find("page") != std::string::npos && 
                            (lowerRequest.find("new") != std::string::npos || 
                             lowerRequest.find("create") != std::string::npos ||
                             lowerRequest.find("add") != std::string::npos));
    bool expectedNav = (lowerRequest.find("nav") != std::string::npos || 
                       lowerRequest.find("menu") != std::string::npos ||
                       lowerRequest.find("link") != std::string::npos);
    
    bool hasCss = false;
    bool hasMultipleHtml = false;
    int htmlCount = 0;
    for (const auto& f : createdFiles_) {
        if (f.find(".css") != std::string::npos) hasCss = true;
        if (f.find(".html") != std::string::npos || f.find(".htm") != std::string::npos) htmlCount++;
    }
    hasMultipleHtml = (htmlCount > 1);
    
    // Warn if model seems to have ignored the request
    bool mightHaveIgnored = false;
    if (expectedCss && !hasCss) {
        outputMessage("\n[!] WARNING: You asked for CSS/styles but no .css file was created.");
        mightHaveIgnored = true;
    }
    if (expectedNewPages && !hasMultipleHtml && htmlCount <= 1) {
        outputMessage("[!] WARNING: You asked for new pages but only " + std::to_string(htmlCount) + " HTML file(s) created.");
        mightHaveIgnored = true;
    }
    
    if (mightHaveIgnored) {
        outputMessage("[!] The model may not have followed your instructions properly.");
        outputMessage("[!] TIP: Try a larger model (llama3, mistral, codellama) for better results.");
        outputMessage("[!] Small models like gemma:1b often struggle with multi-step requests.");
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
