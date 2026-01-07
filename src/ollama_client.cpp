#include "ollama_client.hpp"
#include "json_parser.hpp"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

namespace ollama_agent {

// Callback for libcurl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

OllamaClient::OllamaClient(const OllamaConfig& config) : config_(config) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

OllamaClient::~OllamaClient() {
    curl_global_cleanup();
}

std::string OllamaClient::buildUrl(const std::string& endpoint) const {
    std::ostringstream url;
    url << "http://" << config_.host << ":" << config_.port << endpoint;
    return url.str();
}

std::string OllamaClient::httpPost(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (!curl) {
        lastError_ = "Failed to initialize CURL";
        return "";
    }
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeoutSeconds);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        lastError_ = std::string("CURL error: ") + curl_easy_strerror(res);
        response = "";
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return response;
}

std::string OllamaClient::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (!curl) {
        lastError_ = "Failed to initialize CURL";
        return "";
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);  // Short timeout for status checks
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        lastError_ = std::string("CURL error: ") + curl_easy_strerror(res);
        response = "";
    }
    
    curl_easy_cleanup(curl);
    
    return response;
}

bool OllamaClient::isAvailable() {
    std::string response = httpGet(buildUrl("/api/tags"));
    return !response.empty();
}

std::vector<std::string> OllamaClient::listModels() {
    std::vector<std::string> models;
    std::string response = httpGet(buildUrl("/api/tags"));
    
    if (response.empty()) {
        return models;
    }
    
    // Parse models from response - simple parsing
    size_t pos = 0;
    while ((pos = response.find("\"name\"", pos)) != std::string::npos) {
        size_t colonPos = response.find(':', pos);
        if (colonPos == std::string::npos) break;
        
        size_t quoteStart = response.find('"', colonPos + 1);
        if (quoteStart == std::string::npos) break;
        
        size_t quoteEnd = response.find('"', quoteStart + 1);
        if (quoteEnd == std::string::npos) break;
        
        std::string modelName = response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        models.push_back(modelName);
        
        pos = quoteEnd + 1;
    }
    
    return models;
}

void OllamaClient::setModel(const std::string& model) {
    config_.model = model;
}

std::string OllamaClient::getModel() const {
    return config_.model;
}

std::string OllamaClient::generate(const std::string& prompt) {
    std::string url = buildUrl("/api/generate");
    std::string body = JsonParser::buildRequest(config_.model, prompt, false);
    
    std::string response = httpPost(url, body);
    
    if (response.empty()) {
        return "";
    }
    
    // Extract the response content
    auto content = JsonParser::getString(response, "response");
    if (content.has_value()) {
        return content.value();
    }
    
    // Try parsing as streaming response
    return JsonParser::extractStreamContent(response);
}

std::string OllamaClient::chat(const std::string& systemPrompt, const std::string& userMessage) {
    std::string url = buildUrl("/api/chat");
    std::string body = JsonParser::buildChatRequest(config_.model, systemPrompt, userMessage, false);
    
    std::string response = httpPost(url, body);
    
    if (response.empty()) {
        return "";
    }
    
    // For chat API, content is nested in message object
    // First find the message object, then extract content
    size_t msgPos = response.find("\"message\"");
    if (msgPos != std::string::npos) {
        auto content = JsonParser::getString(response.substr(msgPos), "content");
        if (content.has_value()) {
            return content.value();
        }
    }
    
    // Fallback: try extracting content directly
    auto content = JsonParser::getString(response, "content");
    if (content.has_value()) {
        return content.value();
    }
    
    // Try parsing as streaming response
    return JsonParser::extractStreamContent(response);
}

void OllamaClient::generateStream(const std::string& prompt, StreamCallback callback) {
    // For streaming, we use the generate endpoint with stream=true
    // This is a simplified implementation
    std::string url = buildUrl("/api/generate");
    std::string body = JsonParser::buildRequest(config_.model, prompt, true);
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        lastError_ = "Failed to initialize CURL";
        return;
    }
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    std::string buffer;
    
    // Custom write callback for streaming
    auto streamWriter = +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        auto* data = static_cast<std::pair<std::string*, StreamCallback*>*>(userp);
        size_t totalSize = size * nmemb;
        std::string chunk(static_cast<char*>(contents), totalSize);
        
        data->first->append(chunk);
        
        // Process complete lines
        size_t pos;
        while ((pos = data->first->find('\n')) != std::string::npos) {
            std::string line = data->first->substr(0, pos);
            data->first->erase(0, pos + 1);
            
            if (!line.empty()) {
                auto content = JsonParser::getString(line, "response");
                if (content.has_value()) {
                    (*data->second)(content.value());
                }
            }
        }
        
        return totalSize;
    };
    
    std::pair<std::string*, StreamCallback*> userData{&buffer, &callback};
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streamWriter);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userData);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeoutSeconds);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        lastError_ = std::string("CURL error: ") + curl_easy_strerror(res);
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

std::string OllamaClient::getLastError() const {
    return lastError_;
}

} // namespace ollama_agent

