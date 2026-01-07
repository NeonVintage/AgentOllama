#!/bin/bash

echo "========================================"
echo "OllamaAgent Build Script for Linux/macOS"
echo "========================================"
echo

# Check for g++ or clang++
if command -v g++ &> /dev/null; then
    CXX=g++
elif command -v clang++ &> /dev/null; then
    CXX=clang++
else
    echo "ERROR: No C++ compiler found!"
    echo "Please install g++ or clang++"
    echo
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  Fedora: sudo dnf install gcc-c++"
    echo "  macOS: xcode-select --install"
    exit 1
fi

echo "Using compiler: $CXX"

# Check for curl
CURL_FLAGS=""
if pkg-config --exists libcurl 2>/dev/null; then
    CURL_FLAGS=$(pkg-config --cflags --libs libcurl)
    echo "Found libcurl via pkg-config"
elif [ -f /usr/include/curl/curl.h ]; then
    CURL_FLAGS="-lcurl"
    echo "Found libcurl in /usr/include"
elif [ -f /usr/local/include/curl/curl.h ]; then
    CURL_FLAGS="-I/usr/local/include -L/usr/local/lib -lcurl"
    echo "Found libcurl in /usr/local"
elif [ -f /opt/homebrew/include/curl/curl.h ]; then
    # macOS with Homebrew on Apple Silicon
    CURL_FLAGS="-I/opt/homebrew/include -L/opt/homebrew/lib -lcurl"
    echo "Found libcurl via Homebrew (Apple Silicon)"
elif [ -f /usr/local/opt/curl/include/curl/curl.h ]; then
    # macOS with Homebrew on Intel
    CURL_FLAGS="-I/usr/local/opt/curl/include -L/usr/local/opt/curl/lib -lcurl"
    echo "Found libcurl via Homebrew (Intel)"
else
    echo "WARNING: libcurl not found, trying -lcurl anyway..."
    CURL_FLAGS="-lcurl"
fi

# Create build directory
mkdir -p build

echo
echo "Building OllamaAgent..."
echo

# Compile
$CXX -std=c++17 -O2 \
    -I include \
    src/main.cpp \
    src/agent.cpp \
    src/file_manager.cpp \
    src/json_parser.cpp \
    src/ollama_client.cpp \
    $CURL_FLAGS \
    -o build/ollama_agent

if [ $? -ne 0 ]; then
    echo
    echo "BUILD FAILED!"
    echo
    echo "If curl is not found, install it:"
    echo "  Ubuntu/Debian: sudo apt install libcurl4-openssl-dev"
    echo "  Fedora: sudo dnf install libcurl-devel"
    echo "  macOS: brew install curl"
    echo "  Arch: sudo pacman -S curl"
    exit 1
fi

echo
echo "========================================"
echo "BUILD SUCCESSFUL!"
echo "========================================"
echo
echo "Executable: build/ollama_agent"
echo
echo "To run:"
echo "  ./build/ollama_agent"
echo
echo "Make sure Ollama is running: ollama serve"
echo
