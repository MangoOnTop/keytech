#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <iostream>

using namespace Gdiplus;

#define IDR_EXE    101
#define IDR_CONFIG 102
#define IDR_ICON   103

static ULONG_PTR g_gdiToken = 0;
static HWND g_hwnd = NULL;

enum InstallState {
    STATE_READY = 0,
    STATE_INSTALLING = 1,
    STATE_DONE = 2,
    STATE_FAILED = 3
};

static InstallState g_state = STATE_READY;
static bool g_optDesktopShortcut = true;
static bool g_optStartMenu = true;
static bool g_optLaunchNow = true;
static std::string g_statusMsg = "Ready to install KEYTECH v2.0";
static std::string g_errorMsg = "";
static std::string g_installDir = "";

// Helper to get local appdata programs path
std::string getDefaultInstallPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::string(path) + "\\Programs\\Keytech";
    }
    return "C:\\Keytech";
}

// Helper to create shell shortcut (.lnk)
bool createShortcut(const std::string& targetPath, const std::string& shortcutPath, const std::string& desc, const std::string& iconPath) {
    CoInitialize(NULL);
    IShellLinkA* psl = NULL;
    bool success = false;

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void**)&psl))) {
        psl->SetPath(targetPath.c_str());
        psl->SetDescription(desc.c_str());
        
        // Working directory = target directory
        std::string workDir = targetPath.substr(0, targetPath.find_last_of("\\/"));
        psl->SetWorkingDirectory(workDir.c_str());

        if (!iconPath.empty()) {
            psl->SetIconLocation(iconPath.c_str(), 0);
        }

        IPersistFile* ppf = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
            WCHAR wsz[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, shortcutPath.c_str(), -1, wsz, MAX_PATH);
            if (SUCCEEDED(ppf->Save(wsz, TRUE))) {
                success = true;
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return success;
}

// Extract resource to disk
bool extractResource(int resId, const std::string& destFile) {
    HMODULE hMod = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return false;

    DWORD size = SizeofResource(hMod, hRes);
    void* pBytes = LockResource(hData);
    if (!pBytes || size == 0) return false;

    HANDLE hFile = CreateFileA(destFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    WriteFile(hFile, pBytes, size, &written, NULL);
    CloseHandle(hFile);
    return (written == size);
}

// Register in Windows Add/Remove Programs
void registerUninstaller(const std::string& installDir) {
    HKEY hKey;
    const char* subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Keytech";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, subKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        std::string name = "KEYTECH - Dual Virtual Gamepad Hub";
        std::string exe = installDir + "\\keytech.exe";
        std::string icon = installDir + "\\icon.ico";
        std::string ver = "2.0.0";
        std::string pub = "Keytech Project";
        std::string uninst = "cmd.exe /c rd /s /q \"" + installDir + "\" & del \"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Keytech.lnk\" & del \"%USERPROFILE%\\Desktop\\Keytech.lnk\" & reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Keytech\" /f";

        RegSetValueExA(hKey, "DisplayName", 0, REG_SZ, (const BYTE*)name.c_str(), name.length() + 1);
        RegSetValueExA(hKey, "DisplayIcon", 0, REG_SZ, (const BYTE*)icon.c_str(), icon.length() + 1);
        RegSetValueExA(hKey, "DisplayVersion", 0, REG_SZ, (const BYTE*)ver.c_str(), ver.length() + 1);
        RegSetValueExA(hKey, "Publisher", 0, REG_SZ, (const BYTE*)pub.c_str(), pub.length() + 1);
        RegSetValueExA(hKey, "InstallLocation", 0, REG_SZ, (const BYTE*)installDir.c_str(), installDir.length() + 1);
        RegSetValueExA(hKey, "UninstallString", 0, REG_SZ, (const BYTE*)uninst.c_str(), uninst.length() + 1);
        RegCloseKey(hKey);
    }
}

// Run the installation
DWORD WINAPI doInstallThread(LPVOID) {
    g_state = STATE_INSTALLING;
    g_statusMsg = "Creating directory structure...";
    InvalidateRect(g_hwnd, NULL, FALSE);
    Sleep(300);

    // Create directories
    CreateDirectoryA(g_installDir.c_str(), NULL);

    // Extract files
    g_statusMsg = "Extracting KEYTECH engine & binaries...";
    InvalidateRect(g_hwnd, NULL, FALSE);

    std::string exeDest = g_installDir + "\\keytech.exe";
    std::string cfgDest = g_installDir + "\\config.json";
    std::string icoDest = g_installDir + "\\icon.ico";

    bool okExe = extractResource(IDR_EXE, exeDest);
    bool okCfg = extractResource(IDR_CONFIG, cfgDest);
    bool okIco = extractResource(IDR_ICON, icoDest);

    if (!okExe) {
        // Fallback: check if keytech.exe exists next to installer
        char localDir[MAX_PATH];
        GetModuleFileNameA(NULL, localDir, MAX_PATH);
        std::string selfDir = std::string(localDir).substr(0, std::string(localDir).find_last_of("\\/"));
        CopyFileA((selfDir + "\\keytech.exe").c_str(), exeDest.c_str(), FALSE);
        CopyFileA((selfDir + "\\config.json").c_str(), cfgDest.c_str(), FALSE);
        CopyFileA((selfDir + "\\icon.ico").c_str(), icoDest.c_str(), FALSE);
    }

    Sleep(300);
    g_statusMsg = "Configuring system shortcuts and registration...";
    InvalidateRect(g_hwnd, NULL, FALSE);

    // Desktop Shortcut
    if (g_optDesktopShortcut) {
        char deskPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, deskPath))) {
            std::string sc = std::string(deskPath) + "\\Keytech.lnk";
            createShortcut(exeDest, sc, "KEYTECH Dual Virtual Gamepad Hub", icoDest);
        }
    }

    // Start Menu Shortcut
    if (g_optStartMenu) {
        char smPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, smPath))) {
            std::string smDir = std::string(smPath) + "\\Keytech";
            CreateDirectoryA(smDir.c_str(), NULL);
            std::string sc = smDir + "\\Keytech.lnk";
            createShortcut(exeDest, sc, "KEYTECH Dual Virtual Gamepad Hub", icoDest);
        }
    }

    // Uninstaller Registration
    registerUninstaller(g_installDir);

    Sleep(400);
    g_state = STATE_DONE;
    g_statusMsg = "Installation Complete! Ready to play.";
    InvalidateRect(g_hwnd, NULL, FALSE);

    if (g_optLaunchNow) {
        Sleep(500);
        ShellExecuteA(NULL, "open", exeDest.c_str(), NULL, g_installDir.c_str(), SW_SHOWNORMAL);
        Sleep(200);
        PostMessage(g_hwnd, WM_CLOSE, 0, 0);
    }

    return 0;
}

// -------------------------------------------------------------
// UI Rendering
// -------------------------------------------------------------
static void fillRoundedRect(Graphics& g, const Brush& brush, const RectF& rect, float radius) {
    GraphicsPath path;
    float d = radius * 2.0f;
    path.AddArc(rect.X, rect.Y, d, d, 180, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
    path.CloseFigure();
    g.FillPath(&brush, &path);
}

static void drawRoundedRect(Graphics& g, const Pen& pen, const RectF& rect, float radius) {
    GraphicsPath path;
    float d = radius * 2.0f;
    path.AddArc(rect.X, rect.Y, d, d, 180, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
    path.CloseFigure();
    g.DrawPath(&pen, &path);
}

void renderInstaller(HDC hdc, int width, int height) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Dark Cyberpunk Background
    SolidBrush bg(Color(255, 12, 14, 22));
    g.FillRectangle(&bg, 0, 0, width, height);

    // Glowing Cyan Top Trim
    Pen trimPen(Color(255, 0, 240, 255), 3.0f);
    g.DrawLine(&trimPen, 0, 0, width, 0);

    // Top Header Banner
    SolidBrush headBg(Color(255, 18, 22, 34));
    g.FillRectangle(&headBg, 0, 0, width, 75);
    Pen headBorder(Color(255, 35, 42, 62), 1.0f);
    g.DrawLine(&headBorder, 0, 75, width, 75);

    Font titleFont(L"Segoe UI", 16, FontStyleBold, UnitPixel);
    Font subFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font bodyFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    Font btnFont(L"Segoe UI", 12, FontStyleBold, UnitPixel);

    SolidBrush cyanBrush(Color(255, 0, 240, 255));
    SolidBrush whiteBrush(Color(255, 245, 248, 255));
    SolidBrush mutedBrush(Color(255, 125, 136, 162));

    g.DrawString(L"KEYTECH SETUP WIZARD", -1, &titleFont, PointF(24, 16), &cyanBrush);
    g.DrawString(L"// DUAL VIRTUAL XBOX 360 GAMEPAD ENGINE", -1, &subFont, PointF(24, 42), &mutedBrush);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    if (g_state == STATE_READY) {
        // Welcome and destination card
        RectF card(24, 95, width - 48.0f, 160.0f);
        SolidBrush cardBg(Color(255, 20, 25, 38));
        fillRoundedRect(g, cardBg, card, 8.0f);
        Pen cardPen(Color(255, 40, 48, 70), 1.0f);
        drawRoundedRect(g, cardPen, card, 8.0f);

        g.DrawString(L"This will install KEYTECH onto your computer:", -1, &bodyFont, PointF(40, 108), &whiteBrush);

        // Path box
        RectF pathBox(40, 134, width - 80.0f, 28.0f);
        SolidBrush pathBg(Color(255, 14, 17, 26));
        fillRoundedRect(g, pathBg, pathBox, 4.0f);
        Pen pathPen(Color(255, 0, 240, 255), 1.0f);
        drawRoundedRect(g, pathPen, pathBox, 4.0f);

        std::wstring wPath(g_installDir.begin(), g_installDir.end());
        StringFormat sfLeft;
        sfLeft.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(wPath.c_str(), -1, &bodyFont, RectF(48, 134, width - 96.0f, 28.0f), &sfLeft, &cyanBrush);

        // Options Checkboxes
        auto drawCheck = [&](float y, bool checked, const wchar_t* label) {
            RectF cb(40, y, 16, 16);
            SolidBrush cbBg(checked ? Color(255, 0, 240, 255) : Color(255, 30, 36, 52));
            fillRoundedRect(g, cbBg, cb, 3.0f);
            Pen cbPen(Color(255, 0, 240, 255), 1.0f);
            drawRoundedRect(g, cbPen, cb, 3.0f);

            if (checked) {
                SolidBrush markB(Color(255, 10, 14, 20));
                g.DrawString(L"✓", -1, &bodyFont, PointF(42, y - 2), &markB);
            }
            g.DrawString(label, -1, &bodyFont, PointF(66, y), &whiteBrush);
        };

        drawCheck(175, g_optDesktopShortcut, L"Create a Desktop Shortcut (Keytech.lnk)");
        drawCheck(202, g_optStartMenu, L"Add to Windows Start Menu Programs");
        drawCheck(229, g_optLaunchNow, L"Launch KEYTECH immediately after install");

        // Action Buttons at bottom
        RectF instBtn(width - 240, height - 56.0f, 215, 38.0f);
        SolidBrush instBg(Color(255, 0, 170, 90));
        fillRoundedRect(g, instBg, instBtn, 6.0f);
        Pen instPen(Color(255, 0, 230, 118), 1.2f);
        drawRoundedRect(g, instPen, instBtn, 6.0f);
        g.DrawString(L"INSTALL KEYTECH", -1, &btnFont, instBtn, &sfCenter, &whiteBrush);

        RectF cancelBtn(24, height - 56.0f, 110, 38.0f);
        SolidBrush canBg(Color(255, 35, 40, 56));
        fillRoundedRect(g, canBg, cancelBtn, 6.0f);
        g.DrawString(L"Cancel", -1, &btnFont, cancelBtn, &sfCenter, &mutedBrush);

    } else if (g_state == STATE_INSTALLING) {
        // Progress Card
        RectF card(24, 110, width - 48.0f, 130.0f);
        SolidBrush cardBg(Color(255, 20, 25, 38));
        fillRoundedRect(g, cardBg, card, 8.0f);

        g.DrawString(L"Installing KEYTECH...", -1, &titleFont, PointF(40, 130), &cyanBrush);

        std::wstring statusW(g_statusMsg.begin(), g_statusMsg.end());
        g.DrawString(statusW.c_str(), -1, &bodyFont, PointF(40, 162), &whiteBrush);

        // Animated neon progress bar
        RectF bar(40, 192, width - 80.0f, 12.0f);
        SolidBrush barBg(Color(255, 14, 18, 28));
        fillRoundedRect(g, barBg, bar, 6.0f);

        SolidBrush progBg(Color(255, 0, 240, 255));
        float pW = (float)((GetTickCount() / 15) % (width - 80));
        fillRoundedRect(g, progBg, RectF(40, 192, pW, 12.0f), 6.0f);

    } else if (g_state == STATE_DONE) {
        // Completion Card
        RectF card(24, 110, width - 48.0f, 130.0f);
        SolidBrush cardBg(Color(255, 20, 25, 38));
        fillRoundedRect(g, cardBg, card, 8.0f);

        SolidBrush greenB(Color(255, 0, 230, 118));
        g.DrawString(L"Installation Complete!", -1, &titleFont, PointF(40, 130), &greenB);
        g.DrawString(L"KEYTECH has been installed successfully. Launching application...", -1, &bodyFont, PointF(40, 165), &whiteBrush);

        RectF launchBtn(width - 180, height - 56.0f, 155, 38.0f);
        SolidBrush lBg(Color(255, 0, 170, 90));
        fillRoundedRect(g, lBg, launchBtn, 6.0f);
        g.DrawString(L"Finish", -1, &btnFont, launchBtn, &sfCenter, &whiteBrush);
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 30, NULL);
            return 0;

        case WM_TIMER:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);

            if (g_state == STATE_READY) {
                // Checkboxes
                if (x >= 40 && x <= 320 && y >= 173 && y <= 193) {
                    g_optDesktopShortcut = !g_optDesktopShortcut;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                if (x >= 40 && x <= 320 && y >= 200 && y <= 220) {
                    g_optStartMenu = !g_optStartMenu;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                if (x >= 40 && x <= 320 && y >= 227 && y <= 247) {
                    g_optLaunchNow = !g_optLaunchNow;
                    InvalidateRect(hwnd, NULL, FALSE);
                }

                // Install Button
                RECT rc;
                GetClientRect(hwnd, &rc);
                if (x >= rc.right - 240 && x <= rc.right - 25 && y >= rc.bottom - 56 && y <= rc.bottom - 18) {
                    CreateThread(NULL, 0, doInstallThread, NULL, 0, NULL);
                }
                // Cancel Button
                if (x >= 24 && x <= 134 && y >= rc.bottom - 56 && y <= rc.bottom - 18) {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
            } else if (g_state == STATE_DONE) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            renderInstaller(hdc, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_installDir = getDefaultInstallPath();

    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiToken, &gsi, NULL);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "KeytechInstallerClass";
    RegisterClassEx(&wc);

    int w = 560;
    int h = 380;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        "KEYTECH Installer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_gdiToken) GdiplusShutdown(g_gdiToken);
    return 0;
}
