#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>

#include "agent.hpp"
#include "ollama_client.hpp"
#include "file_manager.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

// Theme structure
struct Theme {
    std::wstring name;
    COLORREF background;
    COLORREF surface;
    COLORREF surfaceLight;
    COLORREF border;
    COLORREF text;
    COLORREF textDim;
    COLORREF accent;
    COLORREF accentHover;
    COLORREF inputBg;
    COLORREF titleColor;
    std::wstring fontName;
    std::wstring monoFontName;
    int fontSize;
    int titleSize;
    bool sidebarLeft;      // Sidebar on left or right
    bool inputAtTop;       // Input at top or bottom
    bool compactMode;      // Compact spacing
    int cornerRadius;      // Button corner radius
};

// Available themes
Theme g_themes[] = {
    // Agent Ollama - Dark modern theme
    {
        L"Agent Ollama",
        RGB(30, 30, 30),       // background
        RGB(45, 45, 45),       // surface
        RGB(60, 60, 60),       // surfaceLight
        RGB(70, 70, 70),       // border
        RGB(230, 230, 230),    // text
        RGB(160, 160, 160),    // textDim
        RGB(100, 149, 237),    // accent (cornflower blue)
        RGB(130, 170, 255),    // accentHover
        RGB(25, 25, 25),       // inputBg
        RGB(100, 149, 237),    // titleColor
        L"Segoe UI",           // fontName
        L"Cascadia Code",      // monoFontName
        15,                    // fontSize
        22,                    // titleSize
        false,                 // sidebarLeft
        false,                 // inputAtTop
        false,                 // compactMode
        8                      // cornerRadius
    },
    // Xenos - Purple alien theme
    {
        L"Xenos",
        RGB(15, 5, 25),        // background (deep purple-black)
        RGB(35, 15, 55),       // surface (dark purple)
        RGB(55, 25, 85),       // surfaceLight
        RGB(120, 50, 180),     // border (bright purple)
        RGB(200, 180, 255),    // text (light purple)
        RGB(150, 100, 200),    // textDim
        RGB(180, 80, 255),     // accent (neon purple)
        RGB(220, 120, 255),    // accentHover
        RGB(20, 8, 35),        // inputBg
        RGB(180, 80, 255),     // titleColor
        L"Consolas",           // fontName (alien/tech feel)
        L"Consolas",           // monoFontName
        14,                    // fontSize
        26,                    // titleSize
        true,                  // sidebarLeft (different layout)
        true,                  // inputAtTop
        false,                 // compactMode
        4                      // cornerRadius (sharper)
    },
    // Deluxe - Golden luxury theme
    {
        L"Deluxe",
        RGB(20, 18, 15),       // background (dark brown-black)
        RGB(40, 35, 25),       // surface (dark gold-brown)
        RGB(60, 50, 35),       // surfaceLight
        RGB(180, 150, 80),     // border (gold)
        RGB(255, 245, 220),    // text (cream)
        RGB(180, 160, 120),    // textDim
        RGB(218, 165, 32),     // accent (goldenrod)
        RGB(255, 200, 80),     // accentHover
        RGB(15, 12, 10),       // inputBg
        RGB(218, 165, 32),     // titleColor
        L"Georgia",            // fontName (elegant)
        L"Cascadia Mono",      // monoFontName
        15,                    // fontSize
        24,                    // titleSize
        false,                 // sidebarLeft
        false,                 // inputAtTop
        true,                  // compactMode (refined spacing)
        12                     // cornerRadius (more rounded, luxurious)
    }
};

int g_currentTheme = 0;
Theme* g_theme = &g_themes[0];

// Control IDs
#define ID_INPUT_EDIT       101
#define ID_OUTPUT_EDIT      102
#define ID_SEND_BUTTON      103
#define ID_MODEL_COMBO      104
#define ID_DIR_BUTTON       105
#define ID_DIR_LABEL        106
#define ID_FILE_LIST        107
#define ID_REFRESH_BUTTON   108
#define ID_CLEAR_BUTTON     110
#define ID_VERBOSE_CHECK    111
#define ID_TITLE_LABEL      112
#define ID_THEME_COMBO      113

// Global variables
HWND g_hWnd = NULL;
HWND g_hInputEdit = NULL;
HWND g_hOutputEdit = NULL;
HWND g_hModelCombo = NULL;
HWND g_hThemeCombo = NULL;
HWND g_hDirLabel = NULL;
HWND g_hFileList = NULL;
HWND g_hSendButton = NULL;
HWND g_hClearButton = NULL;
HWND g_hDirButton = NULL;
HWND g_hRefreshButton = NULL;
HWND g_hVerboseCheck = NULL;
HWND g_hTitleLabel = NULL;

HBRUSH g_hBackgroundBrush = NULL;
HBRUSH g_hSurfaceBrush = NULL;
HBRUSH g_hInputBrush = NULL;
HFONT g_hFont = NULL;
HFONT g_hFontBold = NULL;
HFONT g_hMonoFont = NULL;
HFONT g_hTitleFont = NULL;

std::unique_ptr<ollama_agent::OllamaClient> g_client;
std::unique_ptr<ollama_agent::FileManager> g_fileManager;
std::unique_ptr<ollama_agent::Agent> g_agent;

std::mutex g_outputMutex;
bool g_isProcessing = false;
bool g_verboseMode = false;

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ButtonSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
void InitializeControls(HWND hWnd);
void InitializeOllama();
void RefreshModels();
void RefreshFileList();
void BrowseForFolder();
void SendRequest();
void AppendOutput(const std::wstring& text);
void ClearOutput();
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);
void EnableDarkMode(HWND hWnd);
void ApplyTheme();
void UpdateLayout();
void CreateThemeBrushes();
void CreateThemeFonts();
void DestroyThemeResources();

// Dark mode API
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND, bool);

void EnableDarkMode(HWND hWnd) {
    HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxtheme) {
        auto SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        auto AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));

        if (SetPreferredAppMode) SetPreferredAppMode(ForceDark);
        if (AllowDarkModeForWindow) AllowDarkModeForWindow(hWnd, true);

        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hWnd, 20, &darkMode, sizeof(darkMode));
    }
}

void CreateThemeBrushes() {
    if (g_hBackgroundBrush) DeleteObject(g_hBackgroundBrush);
    if (g_hSurfaceBrush) DeleteObject(g_hSurfaceBrush);
    if (g_hInputBrush) DeleteObject(g_hInputBrush);
    
    g_hBackgroundBrush = CreateSolidBrush(g_theme->background);
    g_hSurfaceBrush = CreateSolidBrush(g_theme->surface);
    g_hInputBrush = CreateSolidBrush(g_theme->inputBg);
}

void CreateThemeFonts() {
    if (g_hFont) DeleteObject(g_hFont);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hMonoFont) DeleteObject(g_hMonoFont);
    if (g_hTitleFont) DeleteObject(g_hTitleFont);
    
    g_hFont = CreateFont(g_theme->fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_theme->fontName.c_str());
    
    g_hFontBold = CreateFont(g_theme->fontSize, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_theme->fontName.c_str());

    g_hMonoFont = CreateFont(g_theme->fontSize - 1, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, g_theme->monoFontName.c_str());

    g_hTitleFont = CreateFont(g_theme->titleSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_theme->fontName.c_str());
}

void ApplyTheme() {
    CreateThemeBrushes();
    CreateThemeFonts();
    
    // Update fonts on controls
    SendMessage(g_hModelCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hThemeCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hDirLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hOutputEdit, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    SendMessage(g_hInputEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hFileList, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);
    
    // Update window background
    SetClassLongPtr(g_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)g_hBackgroundBrush);
    
    UpdateLayout();
    InvalidateRect(g_hWnd, NULL, TRUE);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    CreateThemeBrushes();
    CreateThemeFonts();

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBackgroundBrush;
    wc.lpszClassName = L"OllamaAgentWindow";
    
    HICON hAppIcon = (HICON)LoadImage(hInstance, L"IDI_APPICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    HICON hAppIconSm = (HICON)LoadImage(hInstance, L"IDI_APPICON", IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
    if (!hAppIcon) hAppIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!hAppIconSm) hAppIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    wc.hIcon = hAppIcon;
    wc.hIconSm = hAppIconSm;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    g_hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"OllamaAgentWindow",
        L"OllamaAgent",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 750,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    EnableDarkMode(g_hWnd);
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyThemeResources();
    return (int)msg.wParam;
}

void DestroyThemeResources() {
    if (g_hBackgroundBrush) DeleteObject(g_hBackgroundBrush);
    if (g_hSurfaceBrush) DeleteObject(g_hSurfaceBrush);
    if (g_hInputBrush) DeleteObject(g_hInputBrush);
    if (g_hFont) DeleteObject(g_hFont);
    if (g_hFontBold) DeleteObject(g_hFontBold);
    if (g_hMonoFont) DeleteObject(g_hMonoFont);
    if (g_hTitleFont) DeleteObject(g_hTitleFont);
}

void DrawModernButton(HDC hdc, RECT rc, const wchar_t* text, bool hover, bool accent = false) {
    COLORREF bgColor = hover ? g_theme->surfaceLight : g_theme->surface;
    if (accent) {
        bgColor = hover ? g_theme->accentHover : g_theme->accent;
    }
    
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, accent ? bgColor : g_theme->border);
    
    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, g_theme->cornerRadius, g_theme->cornerRadius);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_theme->text);
    SelectObject(hdc, g_hFontBold);
    DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static bool isHovering = false;
    
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            wchar_t text[256];
            GetWindowText(hWnd, text, 256);
            
            bool isAccent = (GetDlgCtrlID(hWnd) == ID_SEND_BUTTON);
            bool hover = (GetCapture() == hWnd) || isHovering;
            
            FillRect(hdc, &rc, g_hBackgroundBrush);
            DrawModernButton(hdc, rc, text, hover, isAccent);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE:
            if (!isHovering) {
                isHovering = true;
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
            
        case WM_MOUSELEAVE:
            isHovering = false;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
            
        case WM_ERASEBKGND:
            return 1;
    }
    
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void UpdateLayout() {
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    int margin = g_theme->compactMode ? 15 : 20;
    int topBarHeight = 70;
    int inputHeight = g_theme->compactMode ? 80 : 100;
    int sidebarWidth = g_theme->compactMode ? 180 : 220;
    int buttonWidth = 100;
    int buttonHeight = 40;
    
    int contentHeight = height - topBarHeight - margin * 2;
    int outputHeight, inputTop;
    
    if (g_theme->inputAtTop) {
        inputTop = topBarHeight + margin;
        outputHeight = contentHeight - inputHeight - margin;
        
        // Input at top layout
        int inputWidth = width - sidebarWidth - margin * 3 - buttonWidth - 15;
        MoveWindow(g_hInputEdit, 
            g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin,
            inputTop, inputWidth, inputHeight, TRUE);
        
        int btnX = (g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin) + inputWidth + 10;
        MoveWindow(g_hSendButton, btnX, inputTop, buttonWidth, (inputHeight - 10) / 2, TRUE);
        MoveWindow(g_hClearButton, btnX, inputTop + (inputHeight + 10) / 2, buttonWidth, (inputHeight - 10) / 2, TRUE);
        
        // Output below input
        MoveWindow(g_hOutputEdit,
            g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin,
            inputTop + inputHeight + margin,
            width - sidebarWidth - margin * 3,
            outputHeight, TRUE);
    } else {
        outputHeight = contentHeight - inputHeight - margin;
        inputTop = topBarHeight + outputHeight + margin * 2;
        
        // Output at top, input at bottom
        MoveWindow(g_hOutputEdit,
            g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin,
            topBarHeight + margin,
            width - sidebarWidth - margin * 3,
            outputHeight, TRUE);
        
        int inputWidth = width - sidebarWidth - margin * 3 - buttonWidth - 15;
        MoveWindow(g_hInputEdit,
            g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin,
            inputTop, inputWidth, inputHeight, TRUE);
        
        int btnX = (g_theme->sidebarLeft ? sidebarWidth + margin * 2 : margin) + inputWidth + 10;
        MoveWindow(g_hSendButton, btnX, inputTop, buttonWidth, (inputHeight - 10) / 2, TRUE);
        MoveWindow(g_hClearButton, btnX, inputTop + (inputHeight + 10) / 2, buttonWidth, (inputHeight - 10) / 2, TRUE);
    }

    // Sidebar position
    int sidebarX = g_theme->sidebarLeft ? margin : width - sidebarWidth - margin;
    MoveWindow(g_hFileList, sidebarX, topBarHeight + margin, sidebarWidth, contentHeight - buttonHeight - margin, TRUE);
    MoveWindow(g_hRefreshButton, sidebarX, height - margin - buttonHeight, sidebarWidth, buttonHeight, TRUE);

    // Top bar
    int topY = margin + 35;
    int labelX = margin;
    
    MoveWindow(g_hTitleLabel, margin, margin - 5, 200, 30, TRUE);
    MoveWindow(g_hThemeCombo, margin + 210, margin - 2, 140, 30, TRUE);
    MoveWindow(g_hModelCombo, margin, topY, 200, 30, TRUE);
    MoveWindow(g_hDirButton, margin + 210, topY, buttonWidth, 28, TRUE);
    MoveWindow(g_hDirLabel, margin + 320, topY + 4, width - sidebarWidth - 500, 24, TRUE);
    MoveWindow(g_hVerboseCheck, width - sidebarWidth - margin - 90, topY + 4, 90, 24, TRUE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            InitializeControls(hWnd);
            InitializeOllama();
            break;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_theme->text);
            SetBkColor(hdc, g_theme->inputBg);
            SetBkMode(hdc, OPAQUE);
            return (LRESULT)g_hInputBrush;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            
            if (hCtrl == g_hOutputEdit) {
                SetTextColor(hdc, g_theme->text);
                SetBkColor(hdc, g_theme->inputBg);
                SetBkMode(hdc, OPAQUE);
                return (LRESULT)g_hInputBrush;
            }
            
            if (hCtrl == g_hTitleLabel) {
                SetTextColor(hdc, g_theme->titleColor);
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)g_hBackgroundBrush;
            }
            
            SetTextColor(hdc, g_theme->text);
            SetBkColor(hdc, g_theme->background);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)g_hBackgroundBrush;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            if (pDIS->CtlID == ID_VERBOSE_CHECK) {
                HDC hdc = pDIS->hDC;
                RECT rc = pDIS->rcItem;
                
                FillRect(hdc, &rc, g_hBackgroundBrush);
                
                RECT boxRect = {rc.left, rc.top + 3, rc.left + 16, rc.top + 19};
                HBRUSH boxBrush = CreateSolidBrush(g_theme->surface);
                HPEN boxPen = CreatePen(PS_SOLID, 1, g_theme->border);
                SelectObject(hdc, boxBrush);
                SelectObject(hdc, boxPen);
                Rectangle(hdc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
                
                if (g_verboseMode) {
                    HPEN checkPen = CreatePen(PS_SOLID, 2, g_theme->accent);
                    SelectObject(hdc, checkPen);
                    MoveToEx(hdc, boxRect.left + 3, boxRect.top + 7, NULL);
                    LineTo(hdc, boxRect.left + 6, boxRect.top + 11);
                    LineTo(hdc, boxRect.left + 12, boxRect.top + 4);
                    DeleteObject(checkPen);
                }
                
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, g_theme->text);
                SelectObject(hdc, g_hFont);
                RECT textRect = {rc.left + 22, rc.top, rc.right, rc.bottom};
                DrawText(hdc, L"Verbose", -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                
                DeleteObject(boxBrush);
                DeleteObject(boxPen);
                return TRUE;
            }
            break;
        }

        case WM_SIZE:
            UpdateLayout();
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_SEND_BUTTON:
                    if (!g_isProcessing) SendRequest();
                    break;
                case ID_DIR_BUTTON:
                    BrowseForFolder();
                    break;
                case ID_REFRESH_BUTTON:
                    RefreshFileList();
                    break;
                case ID_CLEAR_BUTTON:
                    ClearOutput();
                    break;
                case ID_VERBOSE_CHECK:
                    g_verboseMode = !g_verboseMode;
                    if (g_agent) g_agent->setVerbose(g_verboseMode);
                    InvalidateRect(g_hVerboseCheck, NULL, TRUE);
                    break;
                case ID_THEME_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int idx = (int)SendMessage(g_hThemeCombo, CB_GETCURSEL, 0, 0);
                        if (idx != CB_ERR && idx != g_currentTheme) {
                            g_currentTheme = idx;
                            g_theme = &g_themes[idx];
                            ApplyTheme();
                        }
                    }
                    break;
                case ID_MODEL_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int idx = (int)SendMessage(g_hModelCombo, CB_GETCURSEL, 0, 0);
                        if (idx != CB_ERR) {
                            wchar_t buffer[256];
                            SendMessage(g_hModelCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
                            if (g_client) g_client->setModel(WStringToString(buffer));
                        }
                    }
                    break;
            }
            break;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 900;
            mmi->ptMinTrackSize.y = 600;
            break;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, g_hBackgroundBrush);
            return 1;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_USER + 1: {
            // Handle async output message from agent callback
            std::wstring* msg = (std::wstring*)lParam;
            if (msg) {
                AppendOutput(*msg);
                delete msg;
            }
            break;
        }

        case WM_USER + 2: {
            // Handle request completion
            bool success = (wParam != 0);
            std::vector<std::string>* files = (std::vector<std::string>*)lParam;
            
            AppendOutput(L"\r\n────────────────────────────────────\r\n");
            if (files && !files->empty()) {
                AppendOutput(L"Files created/updated:\r\n");
                for (const auto& f : *files) {
                    AppendOutput(L"  ✓ " + StringToWString(f) + L"\r\n");
                }
                AppendOutput(L"\r\n[OK] Operation complete!\r\n");
            } else if (!success) {
                AppendOutput(L"[!] Operation failed - check verbose output for details\r\n");
            } else {
                AppendOutput(L"[i] No files were created/modified\r\n");
            }
            AppendOutput(L"────────────────────────────────────\r\n");
            
            if (files) delete files;
            g_isProcessing = false;
            
            // Refresh file list
            RefreshFileList();
            break;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

HWND CreateModernButton(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id, HINSTANCE hInst) {
    HWND hBtn = CreateWindow(L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, hInst, NULL);
    SetWindowSubclass(hBtn, ButtonSubclassProc, 0, 0);
    return hBtn;
}

void InitializeControls(HWND hWnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    // Title
    g_hTitleLabel = CreateWindow(L"STATIC", L"OllamaAgent",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 15, 200, 30, hWnd, (HMENU)ID_TITLE_LABEL, hInst, NULL);
    SendMessage(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);

    // Theme selector
    g_hThemeCombo = CreateWindow(WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        230, 13, 140, 200, hWnd, (HMENU)ID_THEME_COMBO, hInst, NULL);
    SendMessage(g_hThemeCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    for (int i = 0; i < 3; i++) {
        SendMessage(g_hThemeCombo, CB_ADDSTRING, 0, (LPARAM)g_themes[i].name.c_str());
    }
    SendMessage(g_hThemeCombo, CB_SETCURSEL, 0, 0);

    // Model combo
    g_hModelCombo = CreateWindow(WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        20, 50, 200, 200, hWnd, (HMENU)ID_MODEL_COMBO, hInst, NULL);
    SendMessage(g_hModelCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Directory button
    g_hDirButton = CreateModernButton(hWnd, L"Browse...", 230, 50, 100, 28, ID_DIR_BUTTON, hInst);

    // Directory label
    g_hDirLabel = CreateWindow(L"STATIC", L"No directory selected",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS,
        340, 54, 400, 24, hWnd, (HMENU)ID_DIR_LABEL, hInst, NULL);
    SendMessage(g_hDirLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Verbose checkbox
    g_hVerboseCheck = CreateWindow(L"BUTTON", L"Verbose",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 90, 24, hWnd, (HMENU)ID_VERBOSE_CHECK, hInst, NULL);

    // Output edit
    g_hOutputEdit = CreateWindowEx(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        20, 90, 600, 400, hWnd, (HMENU)ID_OUTPUT_EDIT, hInst, NULL);
    SendMessage(g_hOutputEdit, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    SendMessage(g_hOutputEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));

    // Input edit
    g_hInputEdit = CreateWindowEx(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        20, 500, 500, 100, hWnd, (HMENU)ID_INPUT_EDIT, hInst, NULL);
    SendMessage(g_hInputEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hInputEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));
    SetWindowText(g_hInputEdit, L"Type your request here...");

    // Buttons
    g_hSendButton = CreateModernButton(hWnd, L"Send", 530, 500, 100, 45, ID_SEND_BUTTON, hInst);
    g_hClearButton = CreateModernButton(hWnd, L"Clear", 530, 555, 100, 45, ID_CLEAR_BUTTON, hInst);

    // File list
    g_hFileList = CreateWindowEx(0, WC_LISTBOX, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        640, 90, 220, 450, hWnd, (HMENU)ID_FILE_LIST, hInst, NULL);
    SendMessage(g_hFileList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // Refresh button
    g_hRefreshButton = CreateModernButton(hWnd, L"Refresh Files", 640, 550, 220, 40, ID_REFRESH_BUTTON, hInst);
}

void InitializeOllama() {
    AppendOutput(L"Connecting to Ollama...\r\n");

    ollama_agent::OllamaConfig config;
    config.host = "127.0.0.1";
    config.port = 11434;
    config.timeoutSeconds = 300;

    g_client = std::make_unique<ollama_agent::OllamaClient>(config);
    
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    g_fileManager = std::make_unique<ollama_agent::FileManager>(WStringToString(currentDir));
    
    g_agent = std::make_unique<ollama_agent::Agent>(*g_client, *g_fileManager);
    
    // Set output callback for verbose mode and status messages
    g_agent->setOutputCallback([](const std::string& message) {
        std::lock_guard<std::mutex> lock(g_outputMutex);
        std::wstring wmsg = StringToWString(message);
        // Normalize line endings for Windows
        std::wstring normalized;
        for (size_t i = 0; i < wmsg.size(); i++) {
            if (wmsg[i] == L'\n' && (i == 0 || wmsg[i-1] != L'\r')) {
                normalized += L"\r\n";
            } else {
                normalized += wmsg[i];
            }
        }
        PostMessage(g_hWnd, WM_USER + 1, 0, (LPARAM)new std::wstring(normalized));
    });

    if (!g_client->isAvailable()) {
        AppendOutput(L"\r\n[ERROR] Cannot connect to Ollama at 127.0.0.1:11434\r\n\r\n");
        AppendOutput(L"Make sure Ollama is running:\r\n");
        AppendOutput(L"  1. Open a terminal\r\n");
        AppendOutput(L"  2. Run: ollama serve\r\n");
        return;
    }

    AppendOutput(L"[OK] Connected to Ollama\r\n");
    RefreshModels();
    
    SetWindowText(g_hDirLabel, StringToWString(g_fileManager->getWorkingDirectory()).c_str());
    RefreshFileList();

    AppendOutput(L"\r\n========================================\r\n");
    AppendOutput(L"        OllamaAgent Ready\r\n");
    AppendOutput(L"========================================\r\n\r\n");
    AppendOutput(L"Select a theme from the dropdown.\r\n");
    AppendOutput(L"Type your request and click Send.\r\n\r\n");
}

void RefreshModels() {
    SendMessage(g_hModelCombo, CB_RESETCONTENT, 0, 0);
    auto models = g_client->listModels();
    if (models.empty()) {
        SendMessage(g_hModelCombo, CB_ADDSTRING, 0, (LPARAM)L"(no models)");
        return;
    }
    for (const auto& model : models) {
        SendMessage(g_hModelCombo, CB_ADDSTRING, 0, (LPARAM)StringToWString(model).c_str());
    }
    SendMessage(g_hModelCombo, CB_SETCURSEL, 0, 0);
    g_client->setModel(models[0]);
    AppendOutput(L"[OK] Using model: " + StringToWString(models[0]) + L"\r\n");
}

void RefreshFileList() {
    SendMessage(g_hFileList, LB_RESETCONTENT, 0, 0);
    auto files = g_fileManager->listFiles();
    for (const auto& file : files) {
        SendMessage(g_hFileList, LB_ADDSTRING, 0, (LPARAM)StringToWString(file).c_str());
    }
    if (files.empty()) {
        SendMessage(g_hFileList, LB_ADDSTRING, 0, (LPARAM)L"(empty folder)");
    }
}

void BrowseForFolder() {
    BROWSEINFO bi = {0};
    bi.lpszTitle = L"Select Output Directory";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.hwndOwner = g_hWnd;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            g_fileManager->setWorkingDirectory(WStringToString(path));
            SetWindowText(g_hDirLabel, path);
            RefreshFileList();
            AppendOutput(L"[OK] Directory: " + std::wstring(path) + L"\r\n");
        }
        CoTaskMemFree(pidl);
    }
}

void SendRequest() {
    int len = GetWindowTextLength(g_hInputEdit);
    if (len == 0) return;

    std::vector<wchar_t> buffer(len + 1);
    GetWindowText(g_hInputEdit, buffer.data(), len + 1);
    std::wstring winput(buffer.data());
    std::string input = WStringToString(winput);

    if (input.find("Type your request") == 0) return;

    g_isProcessing = true;
    AppendOutput(L"\r\n────────────────────────────────────\r\n");
    AppendOutput(L"> " + winput + L"\r\n");
    AppendOutput(L"────────────────────────────────────\r\n");
    AppendOutput(L"[...] Processing request...\r\n");
    if (g_verboseMode) {
        AppendOutput(L"[Verbose mode ON]\r\n");
    }
    SetWindowText(g_hInputEdit, L"");

    std::thread([input]() {
        bool success = g_agent->processRequest(input);
        auto files = g_agent->getCreatedFiles();

        // Post completion message
        PostMessage(g_hWnd, WM_USER + 2, success ? 1 : 0, (LPARAM)new std::vector<std::string>(files));
    }).detach();
}

void AppendOutput(const std::wstring& text) {
    int len = GetWindowTextLength(g_hOutputEdit);
    SendMessage(g_hOutputEdit, EM_SETSEL, len, len);
    SendMessage(g_hOutputEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessage(g_hOutputEdit, EM_SCROLLCARET, 0, 0);
}

void ClearOutput() { SetWindowText(g_hOutputEdit, L""); }

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, NULL, NULL);
    return str;
}
