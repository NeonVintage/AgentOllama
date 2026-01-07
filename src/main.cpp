#include "agent.hpp"
#include "ollama_client.hpp"
#include "file_manager.hpp"
#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

void printBanner() {
    std::cout << R"(
  ___  _ _                        _                    _   
 / _ \| | | __ _ _ __ ___   __ _ / \   __ _  ___ _ __ | |_ 
| | | | | |/ _` | '_ ` _ \ / _` / _ \ / _` |/ _ \ '_ \| __|
| |_| | | | (_| | | | | | | (_| / ___ \ (_| |  __/ | | | |_ 
 \___/|_|_|\__,_|_| |_| |_|\__,_/_/   \_\__, |\___|_| |_|\__|
                                        |___/                
)" << std::endl;
    std::cout << "  AI-Powered Code Generation Agent" << std::endl;
    std::cout << "  Using local Ollama at 127.0.0.1:11434" << std::endl;
    std::cout << "========================================" << std::endl;
}

void printHelp() {
    std::cout << "\nAvailable Commands:" << std::endl;
    std::cout << "  /help           - Show this help message" << std::endl;
    std::cout << "  /models         - List available Ollama models" << std::endl;
    std::cout << "  /model <name>   - Switch to a different model" << std::endl;
    std::cout << "  /dir <path>     - Change output directory" << std::endl;
    std::cout << "  /pwd            - Show current output directory" << std::endl;
    std::cout << "  /list           - List files in output directory" << std::endl;
    std::cout << "  /verbose        - Toggle verbose/debug mode" << std::endl;
    std::cout << "  /raw            - Show raw response from last request" << std::endl;
    std::cout << "  /clear          - Clear the screen" << std::endl;
    std::cout << "  /quit or /exit  - Exit the program" << std::endl;
    std::cout << "\nOr just type your request to generate code!" << std::endl;
    std::cout << "Example: \"make a webpage about dogs\"" << std::endl;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable UTF-8 output on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    printBanner();
    
    // Parse command line arguments
    std::string outputDir = ".";
    std::string model = "llama3.2";
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                outputDir = argv[++i];
            }
        } else if (arg == "--model" || arg == "-m") {
            if (i + 1 < argc) {
                model = argv[++i];
            }
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "\nUsage: ollama_agent [options]" << std::endl;
            std::cout << "\nOptions:" << std::endl;
            std::cout << "  -o, --output <dir>   Set output directory (default: current)" << std::endl;
            std::cout << "  -m, --model <name>   Set Ollama model (default: llama3.2)" << std::endl;
            std::cout << "  -v, --verbose        Enable verbose output" << std::endl;
            std::cout << "  -h, --help           Show this help" << std::endl;
            return 0;
        }
    }
    
    // Initialize components
    ollama_agent::OllamaConfig config;
    config.host = "127.0.0.1";
    config.port = 11434;
    config.model = model;
    config.timeoutSeconds = 300;  // 5 minutes for complex requests
    
    ollama_agent::OllamaClient client(config);
    ollama_agent::FileManager fileManager(outputDir);
    ollama_agent::Agent agent(client, fileManager);
    
    agent.setVerbose(verbose);
    
    // Check Ollama availability
    std::cout << "\nConnecting to Ollama..." << std::endl;
    if (!client.isAvailable()) {
        std::cerr << "ERROR: Cannot connect to Ollama at " << config.host << ":" << config.port << std::endl;
        std::cerr << "Make sure Ollama is running: ollama serve" << std::endl;
        std::cerr << "Error: " << client.getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "[OK] Connected to Ollama" << std::endl;
    
    // List available models and auto-select first one
    std::cout << "\nAvailable models:" << std::endl;
    auto models = client.listModels();
    if (models.empty()) {
        std::cerr << "ERROR: No models found!" << std::endl;
        std::cerr << "Please pull a model first: ollama pull llama3.2" << std::endl;
        return 1;
    }
    
    for (const auto& m : models) {
        std::cout << "  - " << m << std::endl;
    }
    
    // Use first model if no model was specified via command line
    if (model == "llama3.2") {  // Default wasn't changed
        client.setModel(models[0]);
    }
    
    std::cout << "\n[OK] Using model: " << client.getModel() << std::endl;
    std::cout << "[OK] Output directory: " << fileManager.getWorkingDirectory() << std::endl;
    
    printHelp();
    
    // Main interaction loop
    std::string input;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, input);
        
        if (std::cin.eof()) {
            break;
        }
        
        input = trim(input);
        if (input.empty()) {
            continue;
        }
        
        // Handle commands
        if (input[0] == '/') {
            std::string cmd = input;
            std::string arg;
            
            size_t spacePos = input.find(' ');
            if (spacePos != std::string::npos) {
                cmd = input.substr(0, spacePos);
                arg = trim(input.substr(spacePos + 1));
            }
            
            // Convert command to lowercase
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
            
            if (cmd == "/quit" || cmd == "/exit" || cmd == "/q") {
                std::cout << "Goodbye!" << std::endl;
                break;
            } else if (cmd == "/help" || cmd == "/?") {
                printHelp();
            } else if (cmd == "/models") {
                std::cout << "Available models:" << std::endl;
                auto models = client.listModels();
                if (models.empty()) {
                    std::cout << "  No models found or cannot retrieve list" << std::endl;
                } else {
                    for (const auto& m : models) {
                        std::string marker = (m == client.getModel()) ? " (current)" : "";
                        std::cout << "  - " << m << marker << std::endl;
                    }
                }
            } else if (cmd == "/model") {
                if (arg.empty()) {
                    std::cout << "Current model: " << client.getModel() << std::endl;
                } else {
                    client.setModel(arg);
                    std::cout << "Switched to model: " << client.getModel() << std::endl;
                }
            } else if (cmd == "/dir") {
                if (arg.empty()) {
                    std::cout << "Current directory: " << fileManager.getWorkingDirectory() << std::endl;
                } else {
                    fileManager.setWorkingDirectory(arg);
                    std::cout << "Output directory set to: " << fileManager.getWorkingDirectory() << std::endl;
                }
            } else if (cmd == "/pwd") {
                std::cout << "Current directory: " << fileManager.getWorkingDirectory() << std::endl;
            } else if (cmd == "/list" || cmd == "/ls") {
                std::cout << "Files in " << fileManager.getWorkingDirectory() << ":" << std::endl;
                auto files = fileManager.listFiles();
                if (files.empty()) {
                    std::cout << "  (empty)" << std::endl;
                } else {
                    for (const auto& f : files) {
                        std::cout << "  " << f << std::endl;
                    }
                }
            } else if (cmd == "/verbose") {
                verbose = !verbose;
                agent.setVerbose(verbose);
                std::cout << "Verbose mode: " << (verbose ? "ON" : "OFF") << std::endl;
            } else if (cmd == "/raw") {
                std::string lastResponse = agent.getLastResponse();
                if (lastResponse.empty()) {
                    std::cout << "No previous response." << std::endl;
                } else {
                    std::cout << "\n=== RAW RESPONSE ===" << std::endl;
                    std::cout << lastResponse << std::endl;
                    std::cout << "=== END RAW RESPONSE ===" << std::endl;
                }
            } else if (cmd == "/clear" || cmd == "/cls") {
                clearScreen();
                printBanner();
            } else {
                std::cout << "Unknown command: " << cmd << std::endl;
                std::cout << "Type /help for available commands" << std::endl;
            }
            continue;
        }
        
        // Process as a generation request
        std::cout << "\n[...] Thinking..." << std::endl;
        agent.processRequest(input);
    }
    
    return 0;
}

