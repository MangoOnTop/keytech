#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "mapping.h"

enum AppTab {
    TAB_DASHBOARD = 0,
    TAB_KEYBINDS  = 1,
    TAB_SETTINGS  = 2
};

struct AppState {
    bool active = true;               // Controllers active or paused (F9)
    bool vigemConnected = false;      // ViGEmBus driver connection state
    bool isOverlayMode = false;       // Regular Window vs In-Game Transparent Overlay (F10)
    AppTab currentTab = TAB_DASHBOARD;
    
    // Live input reports
    XUSB_REPORT liveReports[2] = {};

    // Interactive Key Rebinding state
    int rebindingPlayer = -1;         // -1 = none, 0 = P1, 1 = P2
    WORD rebindingButton = 0;         // XUSB_GAMEPAD_*
    
    // Status feedback
    std::string toastMessage = "";
    DWORD toastTimestamp = 0;

    // Overlay settings
    int overlayOpacity = 220;         // 100-255 (alpha)
    bool overlayPinned = true;        // Always on top

    // Dragging
    bool isDragging = false;
    POINT dragOffset = {0, 0};
};

void initGDIPlus();
void shutdownGDIPlus();

void renderApp(HWND hwnd, HDC hdc, int width, int height, const AppState& state);
void handleUIMouseDown(HWND hwnd, int x, int y, AppState& state);
void handleUIMouseMove(HWND hwnd, int x, int y, AppState& state);
void handleUIMouseUp(HWND hwnd, int x, int y, AppState& state);
void triggerToast(AppState& state, const std::string& msg);
