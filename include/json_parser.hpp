#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace ollama_agent {

// Simple JSON parser for handling Ollama API responses
class JsonParser {
public:
    // Parse a JSON string and extract a string value by key
    static std::optional<std::string> getString(const std::string& json, const std::string& key);
    
    // Parse a JSON string and extract a boolean value by key
    static std::optional<bool> getBool(const std::string& json, const std::string& key);
    
    // Parse streaming response to extract content
    static std::string extractStreamContent(const std::string& response);
    
    // Build a JSON object for Ollama API request
    static std::string buildRequest(const std::string& model, 
                                   const std::string& prompt,
                                   bool stream = false);
    
    // Build a chat request with system prompt
    static std::string buildChatRequest(const std::string& model,
                                        const std::string& systemPrompt,
                                        const std::string& userMessage,
                                        bool stream = false);

private:
    // Helper to escape JSON strings
    static std::string escapeJson(const std::string& input);
    
    // Helper to find value position after key
    static size_t findValueStart(const std::string& json, const std::string& key);
    
    // Extract string value starting at position
    static std::string extractString(const std::string& json, size_t startPos);
};

} // namespace ollama_agent

