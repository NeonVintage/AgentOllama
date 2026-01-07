#pragma once

#include "ollama_client.hpp"
#include "file_manager.hpp"
#include <string>
#include <vector>
#include <regex>
#include <functional>

namespace ollama_agent {

// Represents a parsed file from LLM response
struct ParsedFile {
    std::string filename;
    std::string content;
    std::string language;
};

// Callback type for output messages
using OutputCallback = std::function<void(const std::string& message)>;

class Agent {
public:
    Agent(OllamaClient& client, FileManager& fileManager);
    
    // Process a user request
    bool processRequest(const std::string& userRequest);
    
    // Get the last response from the agent
    std::string getLastResponse() const;
    
    // Get files created in last operation
    std::vector<std::string> getCreatedFiles() const;
    
    // Set verbose mode for debugging
    void setVerbose(bool verbose);
    
    // Set output callback for GUI integration
    void setOutputCallback(OutputCallback callback);
    
    // Get conversation history summary
    std::string getContextSummary() const;

private:
    OllamaClient& client_;
    FileManager& fileManager_;
    std::string lastResponse_;
    std::vector<std::string> createdFiles_;
    bool verbose_ = false;
    std::string contextSummary_;
    OutputCallback outputCallback_;
    
    // Build the system prompt for the agent
    std::string buildSystemPrompt() const;
    
    // Parse files from LLM response
    std::vector<ParsedFile> parseFilesFromResponse(const std::string& response) const;
    
    // Execute file creation from parsed response
    bool executeFileCreation(const std::vector<ParsedFile>& files);
    
    // Print status message
    void printStatus(const std::string& message) const;
    
    // Extract thinking/explanation from response
    std::string extractExplanation(const std::string& response) const;
    
    // Helper to trim whitespace from strings
    std::string trimString(const std::string& str) const;
    
    // Helper to extract filename from text with markdown formatting
    std::string extractFilenameFromText(const std::string& text) const;
    
    // Helper to check if a string looks like a valid filename
    bool looksLikeFilename(const std::string& text) const;
    
    // Read existing files and build context for the LLM
    std::string getExistingFilesContext() const;
    
    // Output a message (to callback if set, otherwise stdout)
    void outputMessage(const std::string& message) const;
};

} // namespace ollama_agent

