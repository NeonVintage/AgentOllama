#include "json_parser.hpp"
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace ollama_agent {

std::string JsonParser::escapeJson(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '\\': ss << "\\\\"; break;
            case '"':  ss << "\\\""; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) 
                       << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

size_t JsonParser::findValueStart(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return std::string::npos;
    
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return std::string::npos;
    
    // Skip whitespace after colon
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && std::isspace(json[valueStart])) {
        valueStart++;
    }
    
    return valueStart;
}

std::string JsonParser::extractString(const std::string& json, size_t startPos) {
    if (startPos >= json.length() || json[startPos] != '"') {
        return "";
    }
    
    std::ostringstream result;
    size_t pos = startPos + 1;  // Skip opening quote
    
    while (pos < json.length()) {
        if (json[pos] == '\\' && pos + 1 < json.length()) {
            // Handle escape sequences
            char next = json[pos + 1];
            switch (next) {
                case 'n':  result << '\n'; pos += 2; break;
                case 'r':  result << '\r'; pos += 2; break;
                case 't':  result << '\t'; pos += 2; break;
                case '\\': result << '\\'; pos += 2; break;
                case '"':  result << '"'; pos += 2; break;
                case '/':  result << '/'; pos += 2; break;
                case 'b':  result << '\b'; pos += 2; break;
                case 'f':  result << '\f'; pos += 2; break;
                case 'u':
                    // Unicode escape \uXXXX - decode it properly
                    if (pos + 5 < json.length()) {
                        std::string hexStr = json.substr(pos + 2, 4);
                        try {
                            unsigned int codePoint = std::stoul(hexStr, nullptr, 16);
                            if (codePoint < 0x80) {
                                // ASCII character
                                result << static_cast<char>(codePoint);
                            } else if (codePoint < 0x800) {
                                // 2-byte UTF-8
                                result << static_cast<char>(0xC0 | (codePoint >> 6));
                                result << static_cast<char>(0x80 | (codePoint & 0x3F));
                            } else {
                                // 3-byte UTF-8
                                result << static_cast<char>(0xE0 | (codePoint >> 12));
                                result << static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                                result << static_cast<char>(0x80 | (codePoint & 0x3F));
                            }
                        } catch (...) {
                            // If parsing fails, just skip
                        }
                        pos += 6;  // Skip \uXXXX
                    } else {
                        pos += 2;
                    }
                    break;
                default:
                    result << next;
                    pos += 2;
            }
        } else if (json[pos] == '"') {
            // End of string
            break;
        } else {
            result << json[pos];
            pos++;
        }
    }
    
    return result.str();
}

std::optional<std::string> JsonParser::getString(const std::string& json, const std::string& key) {
    size_t valueStart = findValueStart(json, key);
    if (valueStart == std::string::npos) {
        return std::nullopt;
    }
    
    return extractString(json, valueStart);
}

std::optional<bool> JsonParser::getBool(const std::string& json, const std::string& key) {
    size_t valueStart = findValueStart(json, key);
    if (valueStart == std::string::npos) {
        return std::nullopt;
    }
    
    if (json.substr(valueStart, 4) == "true") {
        return true;
    } else if (json.substr(valueStart, 5) == "false") {
        return false;
    }
    
    return std::nullopt;
}

std::string JsonParser::extractStreamContent(const std::string& response) {
    std::ostringstream fullContent;
    std::istringstream stream(response);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        
        auto content = getString(line, "response");
        if (content.has_value()) {
            fullContent << content.value();
        }
        
        // Also check for "content" key (chat API)
        auto chatContent = getString(line, "content");
        if (chatContent.has_value()) {
            fullContent << chatContent.value();
        }
    }
    
    return fullContent.str();
}

std::string JsonParser::buildRequest(const std::string& model, 
                                     const std::string& prompt,
                                     bool stream) {
    std::ostringstream json;
    json << "{";
    json << "\"model\":\"" << escapeJson(model) << "\",";
    json << "\"prompt\":\"" << escapeJson(prompt) << "\",";
    json << "\"stream\":" << (stream ? "true" : "false");
    json << "}";
    return json.str();
}

std::string JsonParser::buildChatRequest(const std::string& model,
                                         const std::string& systemPrompt,
                                         const std::string& userMessage,
                                         bool stream) {
    std::ostringstream json;
    json << "{";
    json << "\"model\":\"" << escapeJson(model) << "\",";
    json << "\"messages\":[";
    json << "{\"role\":\"system\",\"content\":\"" << escapeJson(systemPrompt) << "\"},";
    json << "{\"role\":\"user\",\"content\":\"" << escapeJson(userMessage) << "\"}";
    json << "],";
    json << "\"stream\":" << (stream ? "true" : "false");
    json << "}";
    return json.str();
}

} // namespace ollama_agent

