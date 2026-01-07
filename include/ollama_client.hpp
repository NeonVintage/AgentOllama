#pragma once

#include <string>
#include <functional>
#include <vector>

namespace ollama_agent {

// Configuration for Ollama connection
struct OllamaConfig {
    std::string host = "127.0.0.1";
    int port = 11434;
    std::string model = "llama3.2";  // Default model
    int timeoutSeconds = 120;
};

// Callback for streaming responses
using StreamCallback = std::function<void(const std::string& chunk)>;

class OllamaClient {
public:
    explicit OllamaClient(const OllamaConfig& config = OllamaConfig{});
    ~OllamaClient();
    
    // Check if Ollama is available
    bool isAvailable();
    
    // List available models
    std::vector<std::string> listModels();
    
    // Set the model to use
    void setModel(const std::string& model);
    
    // Get current model
    std::string getModel() const;
    
    // Send a prompt and get response (blocking)
    std::string generate(const std::string& prompt);
    
    // Send a chat message with system prompt
    std::string chat(const std::string& systemPrompt, const std::string& userMessage);
    
    // Send a prompt with streaming callback
    void generateStream(const std::string& prompt, StreamCallback callback);
    
    // Get last error message
    std::string getLastError() const;

private:
    OllamaConfig config_;
    std::string lastError_;
    
    // Build the base URL
    std::string buildUrl(const std::string& endpoint) const;
    
    // Perform HTTP POST request
    std::string httpPost(const std::string& url, const std::string& body);
    
    // Perform HTTP GET request
    std::string httpGet(const std::string& url);
};

} // namespace ollama_agent

