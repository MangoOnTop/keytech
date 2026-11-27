#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <urlmon.h>
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
    STATE_FAILED = 3,
    STATE_UNINSTALLING = 4,
    STATE_UNINSTALLED = 5
};

static InstallState g_state = STATE_READY;
static bool g_isUninstallMode = false;
static bool g_optDesktopShortcut = true;
static bool g_optStartMenu = true;
static bool g_optLaunchNow = true;
static bool g_vigemAlreadyInstalled = false;
static std::string g_statusMsg = "Ready to install KEYTECH v2.1.1";
static std::string g_errorMsg = "";
static std::string g_installDir = "";

// Check if ViGEmBus driver service is registered in Windows
bool isViGEmBusInstalled() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\ViGEmBus", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// Download ViGEmBus installer from official release
bool downloadViGEmInstaller(const std::string& targetPath) {
    const char* url = "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe";
    
    // Method 1: URLDownloadToFile
    HRESULT hr = URLDownloadToFileA(NULL, url, targetPath.c_str(), 0, NULL);
    if (SUCCEEDED(hr)) {
        HANDLE hFile = CreateFileA(targetPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD sz = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
            if (sz > 300000) return true;
        }
    }

    // Method 2: Fallback to curl.exe (built-in Windows 10/11)
    std::string cmd = "curl.exe -L -s -o \"" + targetPath + "\" \"" + std::string(url) + "\"";
    int res = system(cmd.c_str());
    if (res == 0) {
        HANDLE hFile = CreateFileA(targetPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD sz = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
            if (sz > 300000) return true;
        }
    }

    // Method 3: Fallback to PowerShell
    std::string psCmd = "powershell -NoProfile -ExecutionPolicy Bypass -Command \"[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('" + std::string(url) + "', '" + targetPath + "')\"";
    res = system(psCmd.c_str());
    if (res == 0) {
        HANDLE hFile = CreateFileA(targetPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD sz = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
            if (sz > 300000) return true;
        }
    }

    return false;
}

// Auto-install ViGEmBus driver
bool installViGEmBusDriver() {
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string installerPath = std::string(tempDir) + "ViGEmBus_Setup.exe";

    // Check if offline installer was packaged alongside KeytechSetup.exe
    char localDir[MAX_PATH];
    GetModuleFileNameA(NULL, localDir, MAX_PATH);
    std::string selfDir = std::string(localDir).substr(0, std::string(localDir).find_last_of("\\/"));
    std::string offlineInstaller = selfDir + "\\ViGEmBus_Setup.exe";

    if (GetFileAttributesA(offlineInstaller.c_str()) != INVALID_FILE_ATTRIBUTES) {
        installerPath = offlineInstaller;
    } else {
        if (!downloadViGEmInstaller(installerPath)) {
            return false;
        }
    }

    // Launch ViGEmBus installer with UAC elevation & passive progress
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/passive /norestart";
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExA(&sei)) {
        WaitForSingleObject(sei.hProcess, 180000); // Wait up to 3 mins
        DWORD exitCode = 0;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        DeleteFileA(installerPath.c_str());
        return isViGEmBusInstalled();
    }
    return false;
}

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
        std::string ver = "2.1.1";
        std::string pub = "Keytech Project";
        std::string uninst = "\"" + installDir + "\\KeytechSetup.exe\" /uninstall";

        RegSetValueExA(hKey, "DisplayName", 0, REG_SZ, (const BYTE*)name.c_str(), name.length() + 1);
        RegSetValueExA(hKey, "DisplayIcon", 0, REG_SZ, (const BYTE*)icon.c_str(), icon.length() + 1);
        RegSetValueExA(hKey, "DisplayVersion", 0, REG_SZ, (const BYTE*)ver.c_str(), ver.length() + 1);
        RegSetValueExA(hKey, "Publisher", 0, REG_SZ, (const BYTE*)pub.c_str(), pub.length() + 1);
        RegSetValueExA(hKey, "InstallLocation", 0, REG_SZ, (const BYTE*)installDir.c_str(), installDir.length() + 1);
        RegSetValueExA(hKey, "UninstallString", 0, REG_SZ, (const BYTE*)uninst.c_str(), uninst.length() + 1);
        RegCloseKey(hKey);
    }
}

// Uninstall execution thread
DWORD WINAPI doUninstallThread(LPVOID) {
    g_state = STATE_UNINSTALLING;
    g_statusMsg = "Removing KEYTECH shortcuts and registration...";
    InvalidateRect(g_hwnd, NULL, FALSE);
    Sleep(300);

    // Remove shortcuts
    char deskPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, deskPath))) {
        std::string sc = std::string(deskPath) + "\\Keytech.lnk";
        DeleteFileA(sc.c_str());
    }

    char smPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, smPath))) {
        std::string smDir = std::string(smPath) + "\\Keytech";
        std::string sc = smDir + "\\Keytech.lnk";
        DeleteFileA(sc.c_str());
        RemoveDirectoryA(smDir.c_str());
    }

    // Delete registry
    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Keytech");

    // Optional ViGEmBus removal prompt
    if (isViGEmBusInstalled()) {
        int choice = MessageBoxA(g_hwnd, 
            "KEYTECH files and shortcuts have been removed.\n\n"
            "Would you also like to launch the Windows uninstaller for the ViGEmBus driver?",
            "KEYTECH Uninstaller - ViGEmBus Driver",
            MB_YESNO | MB_ICONQUESTION);
        
        if (choice == IDYES) {
            // Open Windows Installed Apps / Settings so user can cleanly uninstall ViGEmBus
            ShellExecuteA(NULL, "open", "ms-settings:appsfeatures", NULL, NULL, SW_SHOWNORMAL);
        }
    }

    // Delete application files on exit via cmd
    std::string selfDelete = "cmd.exe /c ping 127.0.0.1 -n 2 > nul & rd /s /q \"" + g_installDir + "\"";
    ShellExecuteA(NULL, "open", "cmd.exe", ("/c " + selfDelete).c_str(), NULL, SW_HIDE);

    g_state = STATE_UNINSTALLED;
    g_statusMsg = "KEYTECH has been completely uninstalled.";
    InvalidateRect(g_hwnd, NULL, FALSE);
    Sleep(1000);
    PostMessage(g_hwnd, WM_CLOSE, 0, 0);
    return 0;
}

// Installation execution thread
DWORD WINAPI doInstallThread(LPVOID) {
    g_state = STATE_INSTALLING;

    // Step 1: Check and auto-install ViGEmBus driver
    if (!g_vigemAlreadyInstalled) {
        g_statusMsg = "Checking ViGEmBus virtual gamepad driver...";
        InvalidateRect(g_hwnd, NULL, FALSE);
        Sleep(300);

        if (!isViGEmBusInstalled()) {
            g_statusMsg = "Auto-downloading official ViGEmBus driver...";
            InvalidateRect(g_hwnd, NULL, FALSE);

            g_statusMsg = "Installing ViGEmBus driver (Please approve UAC prompt)...";
            InvalidateRect(g_hwnd, NULL, FALSE);

            installViGEmBusDriver();
        }
    }

    // Step 2: Create directory structure
    g_statusMsg = "Creating application directories...";
    InvalidateRect(g_hwnd, NULL, FALSE);
    CreateDirectoryA(g_installDir.c_str(), NULL);

    // Step 3: Extract files
    g_statusMsg = "Extracting KEYTECH engine and assets...";
    InvalidateRect(g_hwnd, NULL, FALSE);

    std::string exeDest = g_installDir + "\\keytech.exe";
    std::string cfgDest = g_installDir + "\\config.json";
    std::string icoDest = g_installDir + "\\icon.ico";
    std::string installerDest = g_installDir + "\\KeytechSetup.exe";

    bool okExe = extractResource(IDR_EXE, exeDest);
    bool okCfg = extractResource(IDR_CONFIG, cfgDest);
    bool okIco = extractResource(IDR_ICON, icoDest);

    // Copy setup itself into program folder for clean uninstallation
    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);
    CopyFileA(selfPath, installerDest.c_str(), FALSE);

    if (!okExe) {
        std::string selfDir = std::string(selfPath).substr(0, std::string(selfPath).find_last_of("\\/"));
        CopyFileA((selfDir + "\\keytech.exe").c_str(), exeDest.c_str(), FALSE);
        CopyFileA((selfDir + "\\config.json").c_str(), cfgDest.c_str(), FALSE);
        CopyFileA((selfDir + "\\icon.ico").c_str(), icoDest.c_str(), FALSE);
    }

    Sleep(300);
    g_statusMsg = "Creating desktop and start menu shortcuts...";
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

    // Windows Add/Remove Programs registration
    registerUninstaller(g_installDir);

    Sleep(400);
    g_state = STATE_DONE;
    g_statusMsg = "Installation Complete! Gamepads ready.";
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
    Font badgeFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font btnFont(L"Segoe UI", 12, FontStyleBold, UnitPixel);

    SolidBrush cyanBrush(Color(255, 0, 240, 255));
    SolidBrush whiteBrush(Color(255, 245, 248, 255));
    SolidBrush greenBrush(Color(255, 0, 230, 118));
    SolidBrush amberBrush(Color(255, 255, 185, 0));
    SolidBrush mutedBrush(Color(255, 125, 136, 162));

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    if (g_isUninstallMode) {
        g.DrawString(L"KEYTECH UNINSTALL WIZARD", -1, &titleFont, PointF(24, 16), &cyanBrush);
        g.DrawString(L"// REMOVE DUAL VIRTUAL GAMEPAD ENGINE", -1, &subFont, PointF(24, 42), &mutedBrush);

        RectF card(24, 95, width - 48.0f, 150.0f);
        SolidBrush cardBg(Color(255, 20, 25, 38));
        fillRoundedRect(g, cardBg, card, 8.0f);
        Pen cardPen(Color(255, 40, 48, 70), 1.0f);
        drawRoundedRect(g, cardPen, card, 8.0f);

        if (g_state == STATE_READY) {
            g.DrawString(L"Are you sure you want to completely remove KEYTECH?", -1, &titleFont, PointF(40, 115), &whiteBrush);
            g.DrawString(L"This will delete shortcuts, binaries, and configurations from your PC.", -1, &bodyFont, PointF(40, 145), &mutedBrush);
            g.DrawString(L"You can also choose to remove the ViGEmBus driver during this process.", -1, &bodyFont, PointF(40, 170), &mutedBrush);

            RectF uninstBtn(width - 200, height - 56.0f, 175, 38.0f);
            SolidBrush uBg(Color(255, 210, 30, 45));
            fillRoundedRect(g, uBg, uninstBtn, 6.0f);
            g.DrawString(L"UNINSTALL NOW", -1, &btnFont, uninstBtn, &sfCenter, &whiteBrush);

            RectF cancelBtn(24, height - 56.0f, 110, 38.0f);
            SolidBrush canBg(Color(255, 35, 40, 56));
            fillRoundedRect(g, canBg, cancelBtn, 6.0f);
            g.DrawString(L"Cancel", -1, &btnFont, cancelBtn, &sfCenter, &mutedBrush);
        } else {
            std::wstring stW(g_statusMsg.begin(), g_statusMsg.end());
            g.DrawString(stW.c_str(), -1, &bodyFont, PointF(40, 140), &whiteBrush);
        }

    } else {
        // Installation Mode
        g.DrawString(L"KEYTECH SETUP WIZARD", -1, &titleFont, PointF(24, 16), &cyanBrush);
        g.DrawString(L"// DUAL VIRTUAL XBOX 360 GAMEPAD ENGINE", -1, &subFont, PointF(24, 42), &mutedBrush);

        if (g_state == STATE_READY) {
            RectF card(24, 90, width - 48.0f, 195.0f);
            SolidBrush cardBg(Color(255, 20, 25, 38));
            fillRoundedRect(g, cardBg, card, 8.0f);
            Pen cardPen(Color(255, 40, 48, 70), 1.0f);
            drawRoundedRect(g, cardPen, card, 8.0f);

            // ViGEmBus auto-status badge
            RectF vigemCard(40, 104, width - 80.0f, 28.0f);
            if (g_vigemAlreadyInstalled) {
                SolidBrush vBg(Color(35, 0, 230, 118));
                fillRoundedRect(g, vBg, vigemCard, 4.0f);
                Pen vPen(Color(255, 0, 230, 118), 1.0f);
                drawRoundedRect(g, vPen, vigemCard, 4.0f);
                g.DrawString(L"✓ ViGEmBus Driver: Already Installed (No download needed)", -1, &badgeFont, PointF(52, 110), &greenBrush);
            } else {
                SolidBrush vBg(Color(35, 255, 185, 0));
                fillRoundedRect(g, vBg, vigemCard, 4.0f);
                Pen vPen(Color(255, 255, 185, 0), 1.0f);
                drawRoundedRect(g, vPen, vigemCard, 4.0f);
                g.DrawString(L"⚡ ViGEmBus Driver: Not detected (Will auto-download & install)", -1, &badgeFont, PointF(52, 110), &amberBrush);
            }

            g.DrawString(L"Install Directory (No admin rights required):", -1, &bodyFont, PointF(40, 140), &whiteBrush);

            RectF pathBox(40, 160, width - 80.0f, 26.0f);
            SolidBrush pathBg(Color(255, 14, 17, 26));
            fillRoundedRect(g, pathBg, pathBox, 4.0f);
            Pen pathPen(Color(255, 0, 240, 255), 1.0f);
            drawRoundedRect(g, pathPen, pathBox, 4.0f);

            std::wstring wPath(g_installDir.begin(), g_installDir.end());
            StringFormat sfLeft;
            sfLeft.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(wPath.c_str(), -1, &bodyFont, RectF(48, 160, width - 96.0f, 26.0f), &sfLeft, &cyanBrush);

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

            drawCheck(196, g_optDesktopShortcut, L"Create a Desktop Shortcut (Keytech.lnk)");
            drawCheck(220, g_optStartMenu, L"Add to Windows Start Menu Programs");
            drawCheck(244, g_optLaunchNow, L"Launch KEYTECH immediately after install");

            // Install Button
            RectF instBtn(width - 240, height - 56.0f, 215, 38.0f);
            SolidBrush instBg(Color(255, 0, 170, 90));
            fillRoundedRect(g, instBg, instBtn, 6.0f);
            Pen instPen(Color(255, 0, 230, 118), 1.2f);
            drawRoundedRect(g, instPen, instBtn, 6.0f);
            g.DrawString(L"INSTALL KEYTECH", -1, &btnFont, instBtn, &sfCenter, &whiteBrush);

            // Cancel Button
            RectF cancelBtn(24, height - 56.0f, 110, 38.0f);
            SolidBrush canBg(Color(255, 35, 40, 56));
            fillRoundedRect(g, canBg, cancelBtn, 6.0f);
            g.DrawString(L"Cancel", -1, &btnFont, cancelBtn, &sfCenter, &mutedBrush);

        } else if (g_state == STATE_INSTALLING) {
            RectF card(24, 110, width - 48.0f, 130.0f);
            SolidBrush cardBg(Color(255, 20, 25, 38));
            fillRoundedRect(g, cardBg, card, 8.0f);

            g.DrawString(L"Installing KEYTECH & Drivers...", -1, &titleFont, PointF(40, 130), &cyanBrush);

            std::wstring statusW(g_statusMsg.begin(), g_statusMsg.end());
            g.DrawString(statusW.c_str(), -1, &bodyFont, PointF(40, 162), &whiteBrush);

            RectF bar(40, 192, width - 80.0f, 12.0f);
            SolidBrush barBg(Color(255, 14, 18, 28));
            fillRoundedRect(g, barBg, bar, 6.0f);

            SolidBrush progBg(Color(255, 0, 240, 255));
            float pW = (float)((GetTickCount() / 15) % (width - 80));
            fillRoundedRect(g, progBg, RectF(40, 192, pW, 12.0f), 6.0f);

        } else if (g_state == STATE_DONE) {
            RectF card(24, 110, width - 48.0f, 130.0f);
            SolidBrush cardBg(Color(255, 20, 25, 38));
            fillRoundedRect(g, cardBg, card, 8.0f);

            SolidBrush greenB(Color(255, 0, 230, 118));
            g.DrawString(L"Installation Complete!", -1, &titleFont, PointF(40, 130), &greenB);
            g.DrawString(L"KEYTECH & ViGEmBus ready. Both controllers active.", -1, &bodyFont, PointF(40, 165), &whiteBrush);

            RectF launchBtn(width - 180, height - 56.0f, 155, 38.0f);
            SolidBrush lBg(Color(255, 0, 170, 90));
            fillRoundedRect(g, lBg, launchBtn, 6.0f);
            g.DrawString(L"Finish", -1, &btnFont, launchBtn, &sfCenter, &whiteBrush);
        }
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
            RECT rc;
            GetClientRect(hwnd, &rc);

            if (g_isUninstallMode) {
                if (g_state == STATE_READY) {
                    if (x >= rc.right - 200 && x <= rc.right - 25 && y >= rc.bottom - 56 && y <= rc.bottom - 18) {
                        CreateThread(NULL, 0, doUninstallThread, NULL, 0, NULL);
                    }
                    if (x >= 24 && x <= 134 && y >= rc.bottom - 56 && y <= rc.bottom - 18) {
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                    }
                }
                return 0;
            }

            if (g_state == STATE_READY) {
                // Checkboxes
                if (x >= 40 && x <= 340 && y >= 194 && y <= 214) {
                    g_optDesktopShortcut = !g_optDesktopShortcut;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                if (x >= 40 && x <= 340 && y >= 218 && y <= 238) {
                    g_optStartMenu = !g_optStartMenu;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                if (x >= 40 && x <= 340 && y >= 242 && y <= 262) {
                    g_optLaunchNow = !g_optLaunchNow;
                    InvalidateRect(hwnd, NULL, FALSE);
                }

                // Install Button
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
    if (strstr(lpCmdLine, "/uninstall") != NULL) {
        g_isUninstallMode = true;
    }

    g_installDir = getDefaultInstallPath();
    g_vigemAlreadyInstalled = isViGEmBusInstalled();

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

    int w = 580;
    int h = 410;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        g_isUninstallMode ? "KEYTECH Uninstaller" : "KEYTECH Installer",
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
