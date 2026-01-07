# OllamaAgent

A C++ AI-powered code generation semi-agent, using a local Ollama instance to generate complete software projects from natural language descriptions.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/macOS-000000?logo=apple&logoColor=white)
![C++17](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus&logoColor=white)

## Platform Support

| Platform | CLI | GUI | Build Script |
|----------|-----|-----|--------------|
| Windows 10/11 | Yes | Yes | `build.bat` / `build_gui.bat` |
| Linux (Ubuntu, Fedora, Arch) | Yes | No | `build.sh` |
| macOS (Intel & Apple Silicon) | Yes | No | `build.sh` |

## Features

- **Local AI** - Uses Ollama running locally, no cloud API keys needed
- **Natural Language** - Describe what you want: "make a webpage about dogs"
- **Auto File Creation** - Automatically creates files and directories
- **Project Awareness** - Reads existing files to make modifications
- **Multi-Model Support** - Use any model available in Ollama
- **GUI and CLI** - Windows GUI application or command-line interface

## Requirements

- **Ollama** - Local LLM runtime
- **C++17 compiler** (MSVC, GCC, or Clang)
- **libcurl** - For HTTP requests
- **CMake 3.15+** (optional)

---

## Ollama Setup

### Download Ollama

| Platform | Download Link |
|----------|---------------|
| Windows | [Download for Windows](https://ollama.com/download/windows) |
| macOS | [Download for macOS](https://ollama.com/download/mac) |
| Linux | `curl -fsSL https://ollama.com/install.sh \| sh` |

### Ollama Documentation

- **Official Website**: https://ollama.com
- **Documentation**: https://github.com/ollama/ollama/blob/main/README.md
- **API Reference**: https://github.com/ollama/ollama/blob/main/docs/api.md
- **Model Library**: https://ollama.com/library

### Quick Start with Ollama

1. **Install Ollama** from the links above

2. **Start Ollama server**:
   ```bash
   ollama serve
   ```

3. **Pull a model** (recommended: llama3.2 or codellama):
   ```bash
   ollama pull llama3.2
   ```
   
   Other good models for coding:
   ```bash
   ollama pull codellama
   ollama pull deepseek-coder
   ollama pull qwen2.5-coder
   ```

4. **Verify it's running**:
   ```bash
   curl http://127.0.0.1:11434/api/tags
   ```

---

## Building OllamaAgent

### Windows (with vcpkg)

1. **Install vcpkg** (if not already installed):
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   bootstrap-vcpkg.bat
   ```

2. **Install curl**:
   ```cmd
   vcpkg install curl:x64-windows
   ```

3. **Build CLI version**:
   ```cmd
   build.bat
   ```

4. **Build GUI version** (Windows only):
   ```cmd
   build_gui.bat
   ```

5. **Run**:
   ```cmd
   cd build
   ollama_agent.exe       # Command-line version
   ollama_agent_gui.exe   # GUI version (double-click)
   ```

### Linux / macOS

1. **Install dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt install libcurl4-openssl-dev build-essential
   
   # Fedora
   sudo dnf install libcurl-devel gcc-c++
   
   # Arch Linux
   sudo pacman -S curl base-devel
   
   # macOS (Homebrew)
   brew install curl
   ```

2. **Build**:
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **Run**:
   ```bash
   ./build/ollama_agent
   ```

### Using CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## Usage

### GUI Version (Windows)

Double-click `ollama_agent_gui.exe` to launch the graphical interface.

**GUI Features:**
- Theme selector - choose from 3 built-in themes
- Model selector dropdown - choose from available Ollama models
- Directory browser - select output folder for generated files
- Input text area - type your requests
- Output display - see responses and status
- File list - view files in the current directory
- Verbose mode checkbox - enable debug output

**Available Themes:**
| Theme | Description |
|-------|-------------|
| Agent Ollama | Dark modern theme with blue accents |
| Xenos | Purple alien theme with different layout (sidebar left, input top) |
| Deluxe | Golden luxury theme with elegant fonts |

### CLI Version

```bash
ollama_agent.exe
```

Or with options:
```bash
ollama_agent.exe -o ./my_project -m codellama -v
```

| Option | Description |
|--------|-------------|
| `-o, --output <dir>` | Set output directory (default: current) |
| `-m, --model <name>` | Set Ollama model (default: auto-select first) |
| `-v, --verbose` | Enable verbose/debug output |
| `-h, --help` | Show help |

### Interactive Commands

| Command | Description |
|---------|-------------|
| `/help` | Show help message |
| `/models` | List available Ollama models |
| `/model <name>` | Switch to a different model |
| `/dir <path>` | Change output directory |
| `/pwd` | Show current output directory |
| `/list` | List files in output directory |
| `/verbose` | Toggle verbose/debug mode |
| `/raw` | Show raw response from last request |
| `/clear` | Clear the screen |
| `/quit` | Exit the program |

### Example Session

```
> /dir my_website
Output directory set to: C:\projects\my_website

> make a simple webpage about dogs with HTML and CSS

[i] Including existing project files in context...
[...] Thinking...

Creating 2 file(s)...
  [+] Created: index.html
  [+] Created: styles.css

[OK] All files created successfully!
Location: C:\projects\my_website

> add a navigation bar with links to Home, About, and Contact

[i] Including existing project files in context...
[...] Thinking...

Creating 1 file(s)...
  [+] Created: index.html

[OK] All files created successfully!
```

---

## How It Works

1. **User Input** → You describe what you want to build
2. **Context Gathering** → Agent reads existing files in the working directory
3. **LLM Processing** → Request is sent to Ollama with file context
4. **Response Parsing** → Agent extracts file definitions from the response
5. **File Creation** → Files are automatically created/updated on disk

### File Format

The agent instructs the LLM to output files in this format:

```
FILE: index.html
```html
<!DOCTYPE html>
<html>
...
</html>
```

FILE: styles.css
```css
body {
    font-family: sans-serif;
}
```
```

---

## Project Structure

```
OllamaAgent/
├── CMakeLists.txt          # CMake build configuration
├── build.bat               # Windows CLI build script
├── build_gui.bat           # Windows GUI build script
├── build.sh                # Linux/macOS build script
├── README.md               # This file
├── LICENSE                 # MIT License
├── include/
│   ├── agent.hpp           # Main agent logic
│   ├── file_manager.hpp    # File operations
│   ├── json_parser.hpp     # JSON handling
│   └── ollama_client.hpp   # Ollama API client
└── src/
    ├── main.cpp            # CLI entry point
    ├── gui_main.cpp        # GUI entry point (Windows)
    ├── agent.cpp           # Agent implementation
    ├── file_manager.cpp    # File operations
    ├── json_parser.cpp     # JSON parsing
    └── ollama_client.cpp   # HTTP client
```

---

## Troubleshooting

### Cannot connect to Ollama

```
ERROR: Cannot connect to Ollama at 127.0.0.1:11434
```

**Solution:**
1. Make sure Ollama is running: `ollama serve`
2. Check if port 11434 is available
3. Verify with: `curl http://127.0.0.1:11434/api/tags`

### No models found

```
ERROR: No models found!
```

**Solution:**
Pull a model first:
```bash
ollama pull llama3.2
```

### Files not being created

If you see `[No files detected in response]`:
1. Run `/verbose` to enable debug mode
2. Try again
3. Run `/raw` to see the LLM's response
4. The model may not be following the expected format - try a different model

### CURL errors on Windows

Make sure libcurl DLLs are in the same folder as the executable or in your PATH.

---

## Recommended Models

| Model | Best For | Command |
|-------|----------|---------|
| llama3.2 | General purpose | `ollama pull llama3.2` |
| codellama | Code generation | `ollama pull codellama` |
| deepseek-coder | Complex code | `ollama pull deepseek-coder` |
| qwen2.5-coder | Fast coding | `ollama pull qwen2.5-coder` |

---

## Configuration

The agent connects to Ollama at:
- **Host**: 127.0.0.1
- **Port**: 11434
- **Timeout**: 5 minutes (for complex requests)

These are currently hardcoded but can be modified in `src/main.cpp`.

---

## License

MIT License - Feel free to use, modify, and distribute.

---

## Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest features
- Submit pull requests

---

## Acknowledgments

- [Ollama](https://ollama.com) - Local LLM runtime
- [libcurl](https://curl.se/libcurl/) - HTTP client library
