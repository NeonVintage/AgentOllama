@echo off
setlocal enabledelayedexpansion

echo ========================================
echo OllamaAgent Build Script for Windows
echo ========================================
echo.

:: Check for Visual Studio Developer Command Prompt
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo cl.exe not found in PATH, searching for Visual Studio...
    
    :: Try to find and run VsDevCmd
    set "VSDEVFOUND="
    for %%v in (18 2025 2024 2022 2019 2017) do (
        for %%e in (Enterprise Professional Community BuildTools) do (
            if exist "C:\Program Files\Microsoft Visual Studio\%%v\%%e\Common7\Tools\VsDevCmd.bat" (
                echo Found Visual Studio %%v %%e
                call "C:\Program Files\Microsoft Visual Studio\%%v\%%e\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
                set "VSDEVFOUND=1"
                goto :vs_found
            )
            if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%v\%%e\Common7\Tools\VsDevCmd.bat" (
                echo Found Visual Studio %%v %%e
                call "C:\Program Files (x86)\Microsoft Visual Studio\%%v\%%e\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
                set "VSDEVFOUND=1"
                goto :vs_found
            )
        )
    )
    
    :vs_found
    if not defined VSDEVFOUND (
        echo ERROR: Visual Studio not found!
        echo Please install Visual Studio with C++ tools.
        exit /b 1
    )
)

echo Compiler: cl.exe found

:: Find vcpkg - check environment variable first, then common locations
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\vcpkg.exe" (
        echo Found vcpkg from VCPKG_ROOT environment variable
        goto :vcpkg_found
    )
)

set "VCPKG_ROOT="

:: Check common locations
for %%p in (
    "C:\vcpkg"
    "C:\src\vcpkg"
    "%USERPROFILE%\vcpkg"
    "%USERPROFILE%\source\repos\vcpkg"
    "C:\dev\vcpkg"
    "C:\tools\vcpkg"
    "C:\Users\%USERNAME%\vcpkg"
    "D:\vcpkg"
    "D:\Programs\vcpkg"
    "D:\Programs\vcpkg-master"
    "%LOCALAPPDATA%\vcpkg"
    "%ProgramFiles%\vcpkg"
    "%ProgramFiles(x86)%\vcpkg"
) do (
    if exist "%%~p\vcpkg.exe" (
        set "VCPKG_ROOT=%%~p"
        goto :vcpkg_found
    )
)

:vcpkg_found
if not defined VCPKG_ROOT (
    echo ERROR: vcpkg not found!
    echo.
    echo Please install vcpkg from: https://github.com/microsoft/vcpkg
    echo.
    echo Common installation:
    echo   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
    echo   cd C:\vcpkg
    echo   bootstrap-vcpkg.bat
    echo.
    echo Or set VCPKG_ROOT environment variable to your vcpkg location.
    exit /b 1
)

echo Found vcpkg at: %VCPKG_ROOT%

:: Check if curl is installed in vcpkg
set "CURL_TRIPLET=x64-windows"
set "CURL_INSTALLED=%VCPKG_ROOT%\installed\%CURL_TRIPLET%"

if not exist "%CURL_INSTALLED%\include\curl\curl.h" (
    echo curl not found in vcpkg. Installing...
    "%VCPKG_ROOT%\vcpkg.exe" install curl:%CURL_TRIPLET%
    
    if %errorlevel% neq 0 (
        echo ERROR: Failed to install curl via vcpkg!
        exit /b 1
    )
)

set "CURL_INCLUDE=%CURL_INSTALLED%\include"
set "CURL_LIB=%CURL_INSTALLED%\lib"
set "CURL_BIN=%CURL_INSTALLED%\bin"

echo Found curl includes: %CURL_INCLUDE%
echo Found curl libs: %CURL_LIB%

:: Create build directory
if not exist build mkdir build

echo.
echo Building OllamaAgent...
echo.

:: Compile
cl /nologo /EHsc /std:c++17 /O2 ^
    /I include ^
    /I "%CURL_INCLUDE%" ^
    src\main.cpp ^
    src\agent.cpp ^
    src\file_manager.cpp ^
    src\json_parser.cpp ^
    src\ollama_client.cpp ^
    /Fe:build\ollama_agent.exe ^
    /Fo:build\ ^
    /link /LIBPATH:"%CURL_LIB%" libcurl.lib ws2_32.lib

if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED!
    exit /b 1
)

:: Copy required DLLs
echo.
echo Copying required DLLs...
if exist "%CURL_BIN%\libcurl.dll" (
    copy /Y "%CURL_BIN%\libcurl.dll" build\ >nul
    echo   Copied libcurl.dll
)
if exist "%CURL_BIN%\zlib1.dll" (
    copy /Y "%CURL_BIN%\zlib1.dll" build\ >nul
    echo   Copied zlib1.dll
)
if exist "%CURL_BIN%\libssl-3-x64.dll" (
    copy /Y "%CURL_BIN%\libssl-3-x64.dll" build\ >nul
    echo   Copied libssl-3-x64.dll
)
if exist "%CURL_BIN%\libcrypto-3-x64.dll" (
    copy /Y "%CURL_BIN%\libcrypto-3-x64.dll" build\ >nul
    echo   Copied libcrypto-3-x64.dll
)

:: Copy any other DLLs that might be needed
for %%f in ("%CURL_BIN%\*.dll") do (
    if not exist "build\%%~nxf" (
        copy /Y "%%f" build\ >nul 2>&1
    )
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Executable: build\ollama_agent.exe
echo.
echo To run:
echo   cd build
echo   ollama_agent.exe
echo.
echo Make sure Ollama is running: ollama serve
echo.

endlocal
