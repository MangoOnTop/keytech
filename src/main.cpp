#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <urlmon.h>
#include <ViGEm/Client.h>
#include "mapping.h"
#include "ui.h"

// Global Application State
static AppState g_state;
static HWND g_hwnd = NULL;
static std::atomic<bool> g_running(true);
static bool g_keyState[256] = {};

// ViGEm references
static PVIGEM_CLIENT g_vigemClient = NULL;
static PVIGEM_TARGET g_pad1 = NULL;
static PVIGEM_TARGET g_pad2 = NULL;

static HHOOK g_keyboardHook = NULL;
static std::thread g_controllerThread;

// Window modes
static RECT g_prevWindowRect = {100, 100, 960, 680};

void toggleOverlayMode() {
    g_state.isOverlayMode = !g_state.isOverlayMode;

    if (g_state.isOverlayMode) {
        // Save current windowed position
        GetWindowRect(g_hwnd, &g_prevWindowRect);

        // Switch to Frameless Always-On-Top Layered Window
        SetWindowLongPtr(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED);
        SetLayeredWindowAttributes(g_hwnd, 0, (BYTE)g_state.overlayOpacity, LWA_ALPHA);

        // Position overlay near the top-center of screen
        int overlayW = 760;
        int overlayH = 264;

        // Spawn in the lower middle of the usable screen area
        RECT work = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
        int overlayX = work.left + (work.right - work.left - overlayW) / 2;
        int overlayY = work.bottom - overlayH - 40;

        SetWindowPos(g_hwnd, HWND_TOPMOST, overlayX, overlayY, overlayW, overlayH, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    } else {
        // Restore standard windowed mode
        SetWindowLongPtr(g_hwnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
        SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
        
        // Remove transparency
        SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

        int w = g_prevWindowRect.right - g_prevWindowRect.left;
        int h = g_prevWindowRect.bottom - g_prevWindowRect.top;
        if (w < 600 || h < 400) {
            w = 980;
            h = 640;
        }

        SetWindowPos(g_hwnd, HWND_NOTOPMOST, g_prevWindowRect.left, g_prevWindowRect.top, w, h, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }

    InvalidateRect(g_hwnd, NULL, FALSE);
}

// Low-level Keyboard Hook
LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (k->vkCode < 256) {
            g_keyState[k->vkCode] = down;
        }

        // 1. Rebinding Mode Interception
        if (g_state.rebindingPlayer != -1 && g_state.rebindingButton != 0) {
            if (down) {
                if (k->vkCode == VK_ESCAPE) {
                    // Cancel rebinding
                    g_state.rebindingPlayer = -1;
                    g_state.rebindingButton = 0;
                    triggerToast(g_state, "Rebinding cancelled");
                } else if (k->vkCode != VK_F9 && k->vkCode != VK_F10) {
                    setMapping(g_state.rebindingPlayer, g_state.rebindingButton, k->vkCode);
                    std::string kn = getKeyName(k->vkCode);
                    triggerToast(g_state, "Mapped to " + kn + " — remember to save");
                    g_state.rebindingPlayer = -1;
                    g_state.rebindingButton = 0;
                }
                if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
            }
            return 1; // Swallow all keystrokes during rebind dialog
        }

        // 2. F9: Toggle Controllers ON / OFF
        if (k->vkCode == VK_F9) {
            if (down) {
                g_state.active = !g_state.active;
                triggerToast(g_state, g_state.active ? "Controllers active" : "Controllers paused");
                if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
            }
            return 1; // Always swallow F9
        }

        // 3. F10: Toggle In-Game Overlay Mode
        if (k->vkCode == VK_F10) {
            if (down) {
                PostMessage(g_hwnd, WM_COMMAND, 1002, 0);
            }
            return 1; // Always swallow F10
        }

        // 4. In-Game controller input swallowing
        // When controllers are ACTIVE and the key is mapped, swallow the key so the game
        // doesn't receive conflicting keyboard input.
        if (g_state.active && isMapped(k->vkCode)) {
            return 1;
        }
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

// Dedicated 1000 Hz Low-Latency Background Controller Loop
void controllerWorker() {
    while (g_running) {
        XUSB_REPORT reports[2] = {};

        if (g_state.active) {
            for (const Mapping& m : mappings) {
                if (g_keyState[m.key]) {
                    reports[m.player].wButtons |= m.button;
                }
            }
        }

        // Copy reports for real-time UI visualization
        g_state.liveReports[0] = reports[0];
        g_state.liveReports[1] = reports[1];

        // Send to virtual gamepads
        if (g_vigemClient && g_pad1 && g_pad2) {
            vigem_target_x360_update(g_vigemClient, g_pad1, reports[0]);
            vigem_target_x360_update(g_vigemClient, g_pad2, reports[1]);
        }

        Sleep(1); // 1 ms = 1000 Hz input polling
    }

    // Flush zeroed reports before termination
    if (g_vigemClient && g_pad1 && g_pad2) {
        XUSB_REPORT zero = {};
        vigem_target_x360_update(g_vigemClient, g_pad1, zero);
        vigem_target_x360_update(g_vigemClient, g_pad2, zero);
    }
}

bool connectViGEm() {
    if (g_vigemClient && g_pad1 && g_pad2) {
        g_state.vigemConnected = true;
        return true;
    }
    if (!g_vigemClient) g_vigemClient = vigem_alloc();
    if (g_vigemClient && vigem_connect(g_vigemClient) == VIGEM_ERROR_NONE) {
        if (!g_pad1) g_pad1 = vigem_target_x360_alloc();
        if (!g_pad2) g_pad2 = vigem_target_x360_alloc();
        if (vigem_target_add(g_vigemClient, g_pad1) == VIGEM_ERROR_NONE &&
            vigem_target_add(g_vigemClient, g_pad2) == VIGEM_ERROR_NONE) {
            g_state.vigemConnected = true;
            return true;
        }
    }
    g_state.vigemConnected = false;
    return false;
}

DWORD WINAPI autoInstallViGEmWorker(LPVOID) {
    triggerToast(g_state, "Downloading the ViGEmBus installer…");
    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);

    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string installerPath = std::string(tempDir) + "ViGEmBus_Setup.exe";

    const char* url = "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe";
    bool downloaded = false;

    if (SUCCEEDED(URLDownloadToFileA(NULL, url, installerPath.c_str(), 0, NULL))) {
        downloaded = true;
    } else {
        std::string cmd = "curl.exe -L -s -o \"" + installerPath + "\" \"" + std::string(url) + "\"";
        if (system(cmd.c_str()) == 0) downloaded = true;
    }

    if (downloaded) {
        triggerToast(g_state, "Installing the driver — approve the Windows prompt");
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);

        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = "runas";
        sei.lpFile = installerPath.c_str();
        sei.lpParameters = "/passive /norestart";
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExA(&sei)) {
            WaitForSingleObject(sei.hProcess, 180000);
            CloseHandle(sei.hProcess);
            DeleteFileA(installerPath.c_str());
        }
    }

    if (connectViGEm()) {
        triggerToast(g_state, "ViGEmBus connected — both gamepads are active");
    } else {
        triggerToast(g_state, "Could not connect to the ViGEmBus driver");
    }

    if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    return 0;
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            SetTimer(hwnd, 1, 16, NULL); // ~60 FPS repaint timer

            // Match the native frame to the app theme (Windows 10/11)
            BOOL darkFrame = TRUE;
            DwmSetWindowAttribute(hwnd, 20, &darkFrame, sizeof(darkFrame)); // DWMWA_USE_IMMERSIVE_DARK_MODE
            DwmSetWindowAttribute(hwnd, 19, &darkFrame, sizeof(darkFrame)); // older Win10 builds

            COLORREF caption = RGB(12, 14, 17);
            DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption));   // DWMWA_CAPTION_COLOR
            COLORREF frameText = RGB(232, 235, 239);
            DwmSetWindowAttribute(hwnd, 36, &frameText, sizeof(frameText)); // DWMWA_TEXT_COLOR

            DWORD corners = 2; // DWMWCP_ROUND
            DwmSetWindowAttribute(hwnd, 33, &corners, sizeof(corners));
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001) {
                // Toggle F9
                g_state.active = !g_state.active;
                triggerToast(g_state, g_state.active ? "Controllers active" : "Controllers paused");
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 1002) {
                // Toggle F10 Overlay
                toggleOverlayMode();
            } else if (LOWORD(wParam) == 1003) {
                // Auto-install ViGEmBus driver
                CreateThread(NULL, 0, autoInstallViGEmWorker, NULL, 0, NULL);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            handleUIMouseDown(hwnd, x, y, g_state);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);

            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);

            handleUIMouseMove(hwnd, x, y, g_state);
            return 0;
        }

        case WM_MOUSELEAVE: {
            g_state.mouseX = -1;
            g_state.mouseY = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            handleUIMouseUp(hwnd, x, y, g_state);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            renderApp(hwnd, hdc, rc.right - rc.left, rc.bottom - rc.top, g_state);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Prevent flicker; rendering is double-buffered

        case WM_CLOSE: {
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// Startup driver install prompt — runs synchronously before the window is created
bool tryInstallViGEmAtStartup() {
    int choice = MessageBoxA(
        NULL,
        "KEYTECH needs the ViGEmBus driver to create virtual controllers.\n\n"
        "Download and install it now?",
        "KEYTECH \xE2\x80\x94 ViGEmBus Driver Required",
        MB_YESNO | MB_ICONQUESTION
    );

    if (choice != IDYES) return false; // user declined — caller should exit

    // Download installer to temp folder
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    std::string installerPath = std::string(tempDir) + "ViGEmBus_Setup.exe";

    const char* url = "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe";
    bool downloaded = false;

    if (SUCCEEDED(URLDownloadToFileA(NULL, url, installerPath.c_str(), 0, NULL))) {
        downloaded = true;
    } else {
        std::string cmd = "curl.exe -L -s -o \"" + installerPath + "\" \"" + std::string(url) + "\"";
        if (system(cmd.c_str()) == 0) downloaded = true;
    }

    if (!downloaded) {
        MessageBoxA(
            NULL,
            "Could not download the ViGEmBus driver.\n\n"
            "Please download it manually from:\n"
            "https://github.com/nefarius/ViGEmBus/releases/latest",
            "KEYTECH — Download Failed",
            MB_OK | MB_ICONERROR
        );
        return false; // caller should exit
    }

    // Run installer elevated
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = installerPath.c_str();
    sei.lpParameters = "/passive /norestart";
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExA(&sei)) {
        WaitForSingleObject(sei.hProcess, 180000);
        CloseHandle(sei.hProcess);
    }

    DeleteFileA(installerPath.c_str());

    // Retry connection
    if (connectViGEm()) {
        return true; // success — continue launching
    }

    MessageBoxA(
        NULL,
        "ViGEmBus driver installed but could not connect.\n\n"
        "Please restart your PC and reopen KEYTECH.",
        "KEYTECH — Restart Required",
        MB_OK | MB_ICONWARNING
    );
    return false; // caller should exit
}

// Application Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // 1. Load Configurations
    if (!loadConfig("config.json")) {
        saveConfig("config.json");
    }

    // 2. Initialize GDI+
    initGDIPlus();

    // 3. Connect to ViGEmBus (checks if driver is installed)
    if (!connectViGEm()) {
        if (!tryInstallViGEmAtStartup()) {
            shutdownGDIPlus();
            return 0;
        }
    }

    // 4. Register Win32 Window Class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "KeytechWindowClass";
    RegisterClassEx(&wc);

    // 5. Create Main Application Window
    int initW = 980;
    int initH = 640;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - initW) / 2;
    int posY = (screenH - initH) / 2;

    g_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        "Keytech",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, initW, initH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hwnd) {
        return 1;
    }

    SendMessage(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
    SendMessage(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // 6. Install Low-Level Keyboard Hook
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);

    // 7. Start Dedicated 1000 Hz Input Polling Thread
    g_controllerThread = std::thread(controllerWorker);

    // 8. Main Message Pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 9. Graceful Shutdown & Cleanup
    g_running = false;
    if (g_controllerThread.joinable()) {
        g_controllerThread.join();
    }

    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }

    if (g_vigemClient) {
        if (g_pad1) {
            vigem_target_remove(g_vigemClient, g_pad1);
            vigem_target_free(g_pad1);
        }
        if (g_pad2) {
            vigem_target_remove(g_vigemClient, g_pad2);
            vigem_target_free(g_pad2);
        }
        vigem_disconnect(g_vigemClient);
        vigem_free(g_vigemClient);
    }

    shutdownGDIPlus();
    return 0;
}

// Fallback for standard console main link
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
