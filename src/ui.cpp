#include "ui.h"
#include <gdiplus.h>
#include <dwmapi.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace Gdiplus;

static ULONG_PTR g_gdiplusToken = 0;

void initGDIPlus() {
    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
}

void shutdownGDIPlus() {
    if (g_gdiplusToken) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

void triggerToast(AppState& state, const std::string& msg) {
    state.toastMessage = msg;
    state.toastTimestamp = GetTickCount();
}

// Helper to draw a rounded rectangle
static void addRoundedRect(GraphicsPath& path, const RectF& rect, float radius) {
    float d = radius * 2.0f;
    if (d > rect.Width) d = rect.Width;
    if (d > rect.Height) d = rect.Height;

    path.AddArc(rect.X, rect.Y, d, d, 180, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
    path.CloseFigure();
}

static void fillRoundedRect(Graphics& g, const Brush& brush, const RectF& rect, float radius) {
    GraphicsPath path;
    addRoundedRect(path, rect, radius);
    g.FillPath(&brush, &path);
}

static void drawRoundedRect(Graphics& g, const Pen& pen, const RectF& rect, float radius) {
    GraphicsPath path;
    addRoundedRect(path, rect, radius);
    g.DrawPath(&pen, &path);
}

// Modern Dark Theme Palette
namespace Theme {
    const Color BG_DARK        = Color(255, 15, 17, 23);       // #0F1117
    const Color BG_OVERLAY     = Color(255, 18, 20, 28);       // #12141C
    const Color PANEL_BG       = Color(255, 23, 27, 38);       // #171B26
    const Color PANEL_BORDER   = Color(255, 42, 48, 66);       // #2A3042
    const Color TEXT_PRIMARY   = Color(255, 240, 243, 246);    // #F0F3F6
    const Color TEXT_SECONDARY = Color(255, 140, 148, 164);    // #8C94A4
    const Color TEXT_MUTED     = Color(255, 90, 98, 115);      // #5A6273
    
    // Player Accents
    const Color P1_ACCENT      = Color(255, 0, 229, 255);      // Neon Cyan
    const Color P1_GLOW        = Color(120, 0, 229, 255);
    const Color P1_BG          = Color(40, 0, 229, 255);

    const Color P2_ACCENT      = Color(255, 255, 61, 113);     // Neon Coral/Pink
    const Color P2_GLOW        = Color(120, 255, 61, 113);
    const Color P2_BG          = Color(40, 255, 61, 113);

    // Xbox Button Colors
    const Color BTN_A          = Color(255, 0, 200, 83);       // Green
    const Color BTN_A_GLOW     = Color(160, 0, 230, 118);
    const Color BTN_B          = Color(255, 255, 23, 68);      // Red
    const Color BTN_B_GLOW     = Color(160, 255, 82, 82);
    const Color BTN_X          = Color(255, 41, 121, 255);     // Blue
    const Color BTN_X_GLOW     = Color(160, 64, 196, 255);
    const Color BTN_Y          = Color(255, 255, 214, 0);      // Yellow
    const Color BTN_Y_GLOW     = Color(160, 255, 235, 59);

    const Color STATUS_GREEN   = Color(255, 0, 230, 118);
    const Color STATUS_YELLOW  = Color(255, 255, 179, 0);
    const Color STATUS_RED     = Color(255, 255, 51, 51);
}

// Forward declarations of sub-drawers
static void drawHeader(Graphics& g, int width, const AppState& state);
static void drawTabs(Graphics& g, int width, const AppState& state);
static void drawDashboard(Graphics& g, int width, int height, const AppState& state);
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state);
static void drawSettings(Graphics& g, int width, int height, const AppState& state);
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state);
static void drawToast(Graphics& g, int width, int height, const AppState& state);
static void drawSingleGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state);

void renderApp(HWND hwnd, HDC hdc, int width, int height, const AppState& state) {
    if (width <= 0 || height <= 0) return;

    // Double buffering setup
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    if (state.isOverlayMode) {
        // Overlay Mode: Draw transparent dark HUD background
        SolidBrush bgBrush(Color(state.overlayOpacity, 12, 14, 20));
        g.Clear(Color(0, 0, 0, 0));
        fillRoundedRect(g, bgBrush, RectF(0, 0, (float)width, (float)height), 12.0f);
        
        Pen borderPen(Color(160, 42, 48, 66), 1.5f);
        drawRoundedRect(g, borderPen, RectF(1, 1, (float)width - 2, (float)height - 2), 12.0f);

        drawOverlayHUD(g, width, height, state);
    } else {
        // Full Windowed Mode
        SolidBrush bgBrush(Theme::BG_DARK);
        g.FillRectangle(&bgBrush, 0, 0, width, height);

        drawHeader(g, width, state);
        drawTabs(g, width, state);

        switch (state.currentTab) {
            case TAB_DASHBOARD:
                drawDashboard(g, width, height, state);
                break;
            case TAB_KEYBINDS:
                drawKeybinds(g, width, height, state);
                break;
            case TAB_SETTINGS:
                drawSettings(g, width, height, state);
                break;
        }

        drawToast(g, width, height, state);
    }

    // Blit to screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// -------------------------------------------------------------
// UI Headers & Tabs
// -------------------------------------------------------------
static void drawHeader(Graphics& g, int width, const AppState& state) {
    // Top app bar
    SolidBrush barBg(Theme::PANEL_BG);
    g.FillRectangle(&barBg, 0, 0, width, 56);

    Pen borderPen(Theme::PANEL_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0, 56, width, 56);

    Font titleFont(L"Segoe UI", 14, FontStyleBold, UnitPixel);
    Font subFont(L"Segoe UI", 10, FontStyleRegular, UnitPixel);
    Font badgeFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);

    // App Logo & Name
    SolidBrush whiteBrush(Theme::TEXT_PRIMARY);
    SolidBrush cyanBrush(Theme::P1_ACCENT);
    SolidBrush grayBrush(Theme::TEXT_SECONDARY);

    g.DrawString(L"KEYTECH", -1, &titleFont, PointF(18, 12), &cyanBrush);
    g.DrawString(L"Dual Gamepad Hub", -1, &subFont, PointF(18, 32), &grayBrush);

    // Controllers Active / Paused Pill Badge
    RectF statusPill(170, 14, 150, 28);
    Color statusCol = state.active ? Theme::STATUS_GREEN : Theme::STATUS_YELLOW;
    SolidBrush statusBg(Color(35, statusCol.GetR(), statusCol.GetG(), statusCol.GetB()));
    Pen statusBorder(statusCol, 1.2f);
    fillRoundedRect(g, statusBg, statusPill, 14.0f);
    drawRoundedRect(g, statusBorder, statusPill, 14.0f);

    SolidBrush statusTextBrush(statusCol);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(state.active ? L"* PADS ACTIVE [F9]" : L"II PAUSED [F9]", -1, &badgeFont, statusPill, &sf, &statusTextBrush);

    // ViGEmBus status badge
    RectF vigemPill(330, 14, 145, 28);
    Color vigemCol = state.vigemConnected ? Theme::STATUS_GREEN : Theme::STATUS_RED;
    SolidBrush vigemBg(Color(30, vigemCol.GetR(), vigemCol.GetG(), vigemCol.GetB()));
    Pen vigemBorder(vigemCol, 1.0f);
    fillRoundedRect(g, vigemBg, vigemPill, 14.0f);
    drawRoundedRect(g, vigemBorder, vigemPill, 14.0f);

    SolidBrush vigemTextBrush(vigemCol);
    g.DrawString(state.vigemConnected ? L"ViGEmBus: OK" : L"ViGEmBus: Missing", -1, &badgeFont, vigemPill, &sf, &vigemTextBrush);

    // Overlay Mode Button
    RectF overlayBtn(width - 240, 14, 130, 28);
    SolidBrush overlayBtnBg(Color(255, 36, 42, 58));
    Pen overlayBtnPen(Theme::P1_ACCENT, 1.0f);
    fillRoundedRect(g, overlayBtnBg, overlayBtn, 6.0f);
    drawRoundedRect(g, overlayBtnPen, overlayBtn, 6.0f);
    g.DrawString(L"Overlay [F10]", -1, &badgeFont, overlayBtn, &sf, &cyanBrush);

    // Close / Exit Button
    RectF exitBtn(width - 95, 14, 80, 28);
    SolidBrush exitBtnBg(Color(255, 45, 25, 30));
    Pen exitBtnPen(Theme::STATUS_RED, 1.0f);
    fillRoundedRect(g, exitBtnBg, exitBtn, 6.0f);
    drawRoundedRect(g, exitBtnPen, exitBtn, 6.0f);
    SolidBrush redBrush(Theme::STATUS_RED);
    g.DrawString(L"Exit", -1, &badgeFont, exitBtn, &sf, &redBrush);
}

static void drawTabs(Graphics& g, int width, const AppState& state) {
    // Tabs container bar
    float tabY = 56.0f;
    float tabH = 38.0f;
    SolidBrush tabsBg(Color(255, 18, 21, 29));
    g.FillRectangle(&tabsBg, 0.0f, tabY, (float)width, tabH);

    Pen borderPen(Theme::PANEL_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0.0f, tabY + tabH, (float)width, tabY + tabH);

    const wchar_t* tabTitles[] = {
        L"🎮 Live Gamepad Visualizer",
        L"⌨ Key Remapping & Duplicates",
        L"⚙ Settings & Hotkeys"
    };

    Font tabFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    float curX = 18.0f;
    for (int i = 0; i < 3; ++i) {
        float tabW = 210.0f;
        RectF tabRect(curX, tabY + 4, tabW, tabH - 8);
        bool isSelected = (state.currentTab == (AppTab)i);

        if (isSelected) {
            SolidBrush activeTabBg(Theme::PANEL_BG);
            fillRoundedRect(g, activeTabBg, tabRect, 6.0f);
            
            Pen activeTabPen(Theme::P1_ACCENT, 1.5f);
            drawRoundedRect(g, activeTabPen, tabRect, 6.0f);

            SolidBrush activeText(Theme::P1_ACCENT);
            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sf, &activeText);
        } else {
            SolidBrush inactiveText(Theme::TEXT_SECONDARY);
            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sf, &inactiveText);
        }

        curX += tabW + 8.0f;
    }
}

// -------------------------------------------------------------
// Gamepad Drawing Logic
// -------------------------------------------------------------
static void drawSingleGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state) {
    Color accent = (player == 0) ? Theme::P1_ACCENT : Theme::P2_ACCENT;
    Color glow   = (player == 0) ? Theme::P1_GLOW   : Theme::P2_GLOW;
    const wchar_t* pName = (player == 0) ? L"PLAYER 1 (WASD + JUKILROE)" : L"PLAYER 2 (ARROWS + NUMPAD)";

    // Outer Controller Shell
    float w = 340.0f * scale;
    float h = 230.0f * scale;
    float x = cx - w * 0.5f;
    float y = cy - h * 0.5f;

    RectF bodyRect(x, y, w, h);
    SolidBrush bodyBrush(Color(255, 23, 27, 36));
    fillRoundedRect(g, bodyBrush, bodyRect, 24.0f * scale);

    Pen bodyBorder(accent, 1.5f);
    drawRoundedRect(g, bodyBorder, bodyRect, 24.0f * scale);

    // Controller Header Banner
    RectF headerRect(x + 12, y + 10, w - 24, 26 * scale);
    SolidBrush headerBg(Color(35, accent.GetR(), accent.GetG(), accent.GetB()));
    fillRoundedRect(g, headerBg, headerRect, 8.0f * scale);

    Font headerFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);
    SolidBrush accentBrush(accent);
    g.DrawString(pName, -1, &headerFont, headerRect, &sfCenter, &accentBrush);

    Font keyFont(L"Segoe UI", (int)(9 * scale), FontStyleBold, UnitPixel);
    Font letterFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);

    // Bumpers (LB & RB)
    auto drawBumper = [&](float bx, float by, float bw, float bh, WORD btnMask, const wchar_t* label, const std::string& keyStr) {
        bool pressed = (report.wButtons & btnMask) != 0;
        RectF bRect(bx, by, bw, bh);
        SolidBrush bBg(pressed ? accent : Color(255, 36, 42, 56));
        fillRoundedRect(g, bBg, bRect, 6.0f * scale);
        
        Pen bPen(pressed ? Color(255, 255, 255, 255) : Color(255, 55, 62, 80), 1.0f);
        drawRoundedRect(g, bPen, bRect, 6.0f * scale);

        std::wstring text = label;
        text += L" [";
        text += std::wstring(keyStr.begin(), keyStr.end());
        text += L"]";

        SolidBrush textBrush(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_PRIMARY);
        g.DrawString(text.c_str(), -1, &keyFont, bRect, &sfCenter, &textBrush);
    };

    int lbKey = getKeyForButton(player, XUSB_GAMEPAD_LEFT_SHOULDER);
    int rbKey = getKeyForButton(player, XUSB_GAMEPAD_RIGHT_SHOULDER);
    drawBumper(x + 20 * scale, y + 42 * scale, 100 * scale, 22 * scale, XUSB_GAMEPAD_LEFT_SHOULDER, L"LB", getKeyName(lbKey));
    drawBumper(x + w - 120 * scale, y + 42 * scale, 100 * scale, 22 * scale, XUSB_GAMEPAD_RIGHT_SHOULDER, L"RB", getKeyName(rbKey));

    // Middle Buttons: BACK and START
    auto drawSmallBtn = [&](float bx, float by, float bw, float bh, WORD btnMask, const wchar_t* label, int key) {
        bool pressed = (report.wButtons & btnMask) != 0;
        RectF r(bx, by, bw, bh);
        SolidBrush b(pressed ? accent : Color(255, 32, 38, 50));
        fillRoundedRect(g, b, r, 4.0f * scale);

        std::wstring full = label;
        full += L" [";
        std::string kn = getKeyName(key);
        full += std::wstring(kn.begin(), kn.end());
        full += L"]";

        SolidBrush txt(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_SECONDARY);
        g.DrawString(full.c_str(), -1, &keyFont, r, &sfCenter, &txt);
    };

    drawSmallBtn(cx - 65 * scale, y + 80 * scale, 55 * scale, 18 * scale, XUSB_GAMEPAD_BACK, L"BACK", getKeyForButton(player, XUSB_GAMEPAD_BACK));
    drawSmallBtn(cx + 10 * scale, y + 80 * scale, 55 * scale, 18 * scale, XUSB_GAMEPAD_START, L"START", getKeyForButton(player, XUSB_GAMEPAD_START));

    // Center Xbox Guide Orb
    float orbR = 14.0f * scale;
    SolidBrush orbBg(Color(255, 18, 22, 28));
    g.FillEllipse(&orbBg, cx - orbR, y + 46 * scale, orbR * 2, orbR * 2);
    Pen orbPen(accent, 1.5f);
    g.DrawEllipse(&orbPen, cx - orbR, y + 46 * scale, orbR * 2, orbR * 2);

    // D-Pad (Left side)
    float dpadCenterX = x + 85 * scale;
    float dpadCenterY = y + 135 * scale;
    float dpadArm = 26 * scale;
    float dpadThick = 24 * scale;

    auto drawDpadDir = [&](float bx, float by, float bw, float bh, WORD mask, const wchar_t* arrow, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(bx, by, bw, bh);
        SolidBrush b(pressed ? accent : Color(255, 32, 37, 50));
        fillRoundedRect(g, b, r, 4.0f * scale);

        Pen p(pressed ? Color(255, 255, 255, 255) : Color(255, 45, 52, 68), 1.0f);
        drawRoundedRect(g, p, r, 4.0f * scale);

        std::wstring t = arrow;
        t += L" ";
        std::string kn = getKeyName(key);
        t += std::wstring(kn.begin(), kn.end());

        SolidBrush txt(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_PRIMARY);
        g.DrawString(t.c_str(), -1, &keyFont, r, &sfCenter, &txt);
    };

    drawDpadDir(dpadCenterX - dpadThick*0.5f, dpadCenterY - dpadArm - dpadThick*0.5f, dpadThick, dpadArm, XUSB_GAMEPAD_DPAD_UP, L"^", getKeyForButton(player, XUSB_GAMEPAD_DPAD_UP));
    drawDpadDir(dpadCenterX - dpadThick*0.5f, dpadCenterY + dpadThick*0.5f, dpadThick, dpadArm, XUSB_GAMEPAD_DPAD_DOWN, L"v", getKeyForButton(player, XUSB_GAMEPAD_DPAD_DOWN));
    drawDpadDir(dpadCenterX - dpadArm - dpadThick*0.5f, dpadCenterY - dpadThick*0.5f, dpadArm, dpadThick, XUSB_GAMEPAD_DPAD_LEFT, L"<", getKeyForButton(player, XUSB_GAMEPAD_DPAD_LEFT));
    drawDpadDir(dpadCenterX + dpadThick*0.5f, dpadCenterY - dpadThick*0.5f, dpadArm, dpadThick, XUSB_GAMEPAD_DPAD_RIGHT, L">", getKeyForButton(player, XUSB_GAMEPAD_DPAD_RIGHT));

    // D-Pad center block
    SolidBrush dpadCenterBg(Color(255, 28, 33, 44));
    g.FillRectangle(&dpadCenterBg, dpadCenterX - dpadThick*0.5f, dpadCenterY - dpadThick*0.5f, dpadThick, dpadThick);

    // Action Face Buttons (ABXY diamond layout on Right side)
    float faceCenterX = x + w - 85 * scale;
    float faceCenterY = y + 135 * scale;
    float faceDist = 32.0f * scale;
    float btnRadius = 15.0f * scale;

    auto drawFaceBtn = [&](float bx, float by, WORD mask, const wchar_t* letter, Color baseCol, Color glowCol, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        float dia = btnRadius * 2.0f;
        RectF r(bx - btnRadius, by - btnRadius, dia, dia);

        if (pressed) {
            SolidBrush glowB(glowCol);
            g.FillEllipse(&glowB, bx - btnRadius - 4, by - btnRadius - 4, dia + 8, dia + 8);
            SolidBrush b(baseCol);
            g.FillEllipse(&b, r);
            Pen p(Color(255, 255, 255, 255), 1.5f);
            g.DrawEllipse(&p, r);
            SolidBrush txt(Color(255, 10, 12, 16));
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        } else {
            SolidBrush b(Color(255, 28, 32, 42));
            g.FillEllipse(&b, r);
            Pen p(baseCol, 1.5f);
            g.DrawEllipse(&p, r);
            SolidBrush txt(baseCol);
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        }

        // Key tag below/beside button
        std::string kn = getKeyName(key);
        std::wstring knt = L"[" + std::wstring(kn.begin(), kn.end()) + L"]";
        RectF tagRect(bx - 30 * scale, by + btnRadius + 1, 60 * scale, 14 * scale);
        SolidBrush tagTxt(Theme::TEXT_SECONDARY);
        g.DrawString(knt.c_str(), -1, &keyFont, tagRect, &sfCenter, &tagTxt);
    };

    // Diamond layout: Y=top, A=bottom, X=left, B=right
    drawFaceBtn(faceCenterX, faceCenterY - faceDist, XUSB_GAMEPAD_Y, L"Y", Theme::BTN_Y, Theme::BTN_Y_GLOW, getKeyForButton(player, XUSB_GAMEPAD_Y));
    drawFaceBtn(faceCenterX, faceCenterY + faceDist, XUSB_GAMEPAD_A, L"A", Theme::BTN_A, Theme::BTN_A_GLOW, getKeyForButton(player, XUSB_GAMEPAD_A));
    drawFaceBtn(faceCenterX - faceDist, faceCenterY, XUSB_GAMEPAD_X, L"X", Theme::BTN_X, Theme::BTN_X_GLOW, getKeyForButton(player, XUSB_GAMEPAD_X));
    drawFaceBtn(faceCenterX + faceDist, faceCenterY, XUSB_GAMEPAD_B, L"B", Theme::BTN_B, Theme::BTN_B_GLOW, getKeyForButton(player, XUSB_GAMEPAD_B));

    // Left & Right Analog Thumbstick Stubs
    float stickR = 18.0f * scale;
    auto drawThumbstick = [&](float sx, float sy, const wchar_t* lbl) {
        SolidBrush stickBg(Color(255, 22, 26, 34));
        g.FillEllipse(&stickBg, sx - stickR, sy - stickR, stickR*2, stickR*2);
        Pen stickPen(Color(255, 48, 56, 72), 1.5f);
        g.DrawEllipse(&stickPen, sx - stickR, sy - stickR, stickR*2, stickR*2);
        
        SolidBrush dotBg(Color(255, 38, 44, 58));
        g.FillEllipse(&dotBg, sx - 6*scale, sy - 6*scale, 12*scale, 12*scale);
    };

    drawThumbstick(cx - 38 * scale, y + 165 * scale, L"LS");
    drawThumbstick(cx + 38 * scale, y + 165 * scale, L"RS");
}

// -------------------------------------------------------------
// Tab 0: Dashboard
// -------------------------------------------------------------
static void drawDashboard(Graphics& g, int width, int height, const AppState& state) {
    float midY = 100.0f + (height - 150.0f) * 0.45f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;

    // Draw both gamepads
    drawSingleGamepad(g, p1CenterX, midY, 1.0f, 0, state.liveReports[0], state);
    drawSingleGamepad(g, p2CenterX, midY, 1.0f, 1, state.liveReports[1], state);

    // Bottom Quick Status & Controls Strip
    RectF bottomBar(18, height - 52.0f, width - 36.0f, 38.0f);
    SolidBrush barBg(Theme::PANEL_BG);
    fillRoundedRect(g, barBg, bottomBar, 8.0f);
    Pen barPen(Theme::PANEL_BORDER, 1.0f);
    drawRoundedRect(g, barPen, bottomBar, 8.0f);

    Font hintFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    SolidBrush hintBrush(Theme::TEXT_SECONDARY);
    g.DrawString(L"Hotkeys:  F9: Toggle Controllers ON/OFF   |   F10: Toggle In-Game Overlay Mode   |   Ultra-low latency: 1000 Hz (~1ms)", -1, &hintFont, PointF(30, height - 42.0f), &hintBrush);
}

// -------------------------------------------------------------
// Tab 1: Key Remapping & Duplicates
// -------------------------------------------------------------
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state) {
    float startY = 106.0f;

    // Check for duplicates
    std::vector<int> duplicates = getDuplicateKeys();
    if (!duplicates.empty()) {
        RectF alertRect(18, startY, width - 36.0f, 34.0f);
        SolidBrush alertBg(Color(40, 255, 51, 51));
        Pen alertPen(Theme::STATUS_RED, 1.0f);
        fillRoundedRect(g, alertBg, alertRect, 6.0f);
        drawRoundedRect(g, alertPen, alertRect, 6.0f);

        std::wstring warnText = L"⚠️ Duplicate Key Conflict: ";
        for (size_t i = 0; i < duplicates.size(); ++i) {
            std::string kn = getKeyName(duplicates[i]);
            warnText += std::wstring(kn.begin(), kn.end());
            if (i + 1 < duplicates.size()) warnText += L", ";
        }
        warnText += L" is mapped to multiple actions! Please resolve duplicates.";

        Font warnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
        SolidBrush redBrush(Theme::STATUS_RED);
        g.DrawString(warnText.c_str(), -1, &warnFont, PointF(30, startY + 8), &redBrush);
        startY += 44.0f;
    }

    Font colHeaderFont(L"Segoe UI", 12, FontStyleBold, UnitPixel);
    Font rowFont(L"Segoe UI", 10, FontStyleRegular, UnitPixel);
    Font keyBoxFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float colW = (width - 54.0f) * 0.5f;

    for (int p = 0; p < 2; ++p) {
        float colX = 18.0f + p * (colW + 18.0f);
        Color accent = (p == 0) ? Theme::P1_ACCENT : Theme::P2_ACCENT;

        // Column Box
        RectF colBox(colX, startY, colW, height - startY - 60.0f);
        SolidBrush colBg(Theme::PANEL_BG);
        fillRoundedRect(g, colBg, colBox, 8.0f);
        Pen colBorder(accent, 1.0f);
        drawRoundedRect(g, colBorder, colBox, 8.0f);

        // Column Header
        RectF colHeadRect(colX, startY, colW, 30.0f);
        SolidBrush colHeadBg(Color(30, accent.GetR(), accent.GetG(), accent.GetB()));
        fillRoundedRect(g, colHeadBg, colHeadRect, 8.0f);
        SolidBrush accentBrush(accent);
        g.DrawString(p == 0 ? L"PLAYER 1 CONTROLLER" : L"PLAYER 2 CONTROLLER", -1, &colHeaderFont, colHeadRect, &sfCenter, &accentBrush);

        // Buttons List
        float itemY = startY + 36.0f;
        for (const auto& b : g_availableButtons) {
            int currentKey = getKeyForButton(p, b.button);
            bool isRebindingThis = (state.rebindingPlayer == p && state.rebindingButton == b.button);

            // Row Label
            RectF lblRect(colX + 14, itemY, colW * 0.45f, 24.0f);
            SolidBrush txtBrush(Theme::TEXT_PRIMARY);
            std::wstring lblW(b.label, b.label + strlen(b.label));
            StringFormat sfLeft;
            sfLeft.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(lblW.c_str(), -1, &rowFont, lblRect, &sfLeft, &txtBrush);

            // Key Box (clickable to rebind)
            RectF keyBox(colX + colW * 0.48f, itemY, colW * 0.48f, 24.0f);
            
            if (isRebindingThis) {
                SolidBrush rebindingBg(Color(255, 60, 40, 10));
                Pen rebindingPen(Theme::STATUS_YELLOW, 1.5f);
                fillRoundedRect(g, rebindingBg, keyBox, 4.0f);
                drawRoundedRect(g, rebindingPen, keyBox, 4.0f);

                SolidBrush rebindingTxt(Theme::STATUS_YELLOW);
                g.DrawString(L"[Press Any Key...]", -1, &keyBoxFont, keyBox, &sfCenter, &rebindingTxt);
            } else {
                SolidBrush boxBg(Color(255, 30, 35, 48));
                Pen boxPen(Color(255, 48, 56, 75), 1.0f);
                fillRoundedRect(g, boxBg, keyBox, 4.0f);
                drawRoundedRect(g, boxPen, keyBox, 4.0f);

                std::string kn = getKeyName(currentKey);
                std::wstring knw(kn.begin(), kn.end());
                SolidBrush boxTxt(Theme::TEXT_PRIMARY);
                g.DrawString(knw.c_str(), -1, &keyBoxFont, keyBox, &sfCenter, &boxTxt);
            }

            itemY += 28.0f;
        }
    }

    // Bottom Action Bar
    float bottomY = height - 48.0f;
    Font btnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);

    // Save Button
    RectF saveBtn(18, bottomY, 160, 32);
    SolidBrush saveBg(Color(255, 0, 150, 70));
    Pen savePen(Theme::STATUS_GREEN, 1.0f);
    fillRoundedRect(g, saveBg, saveBtn, 6.0f);
    drawRoundedRect(g, savePen, saveBtn, 6.0f);
    SolidBrush whiteTxt(Color(255, 255, 255, 255));
    g.DrawString(L"Save Config", -1, &btnFont, saveBtn, &sfCenter, &whiteTxt);

    // Reset Defaults Button
    RectF resetBtn(190, bottomY, 160, 32);
    SolidBrush resetBg(Color(255, 40, 46, 62));
    Pen resetPen(Color(255, 60, 68, 90), 1.0f);
    fillRoundedRect(g, resetBg, resetBtn, 6.0f);
    drawRoundedRect(g, resetPen, resetBtn, 6.0f);
    SolidBrush resetTxt(Theme::TEXT_SECONDARY);
    g.DrawString(L"Reset Defaults", -1, &btnFont, resetBtn, &sfCenter, &resetTxt);

    // Rebind instruction text
    Font tipFont(L"Segoe UI", 10, FontStyleRegular, UnitPixel);
    SolidBrush tipBrush(Theme::TEXT_MUTED);
    g.DrawString(L"Tip: Click any key box to rebind. Press ESC while rebinding to cancel.", -1, &tipFont, PointF(370, bottomY + 8), &tipBrush);
}

// -------------------------------------------------------------
// Tab 2: Settings & Info
// -------------------------------------------------------------
static void drawSettings(Graphics& g, int width, int height, const AppState& state) {
    float startY = 110.0f;
    Font titleFont(L"Segoe UI", 13, FontStyleBold, UnitPixel);
    Font bodyFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    Font btnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Section 1: In-Game Overlay Configuration
    RectF s1(18, startY, width - 36.0f, 120.0f);
    SolidBrush s1Bg(Theme::PANEL_BG);
    fillRoundedRect(g, s1Bg, s1, 8.0f);
    Pen s1Pen(Theme::PANEL_BORDER, 1.0f);
    drawRoundedRect(g, s1Pen, s1, 8.0f);

    SolidBrush cyanBrush(Theme::P1_ACCENT);
    g.DrawString(L"In-Game Overlay Settings", -1, &titleFont, PointF(30, startY + 12), &cyanBrush);

    SolidBrush whiteTxt(Theme::TEXT_PRIMARY);
    SolidBrush grayTxt(Theme::TEXT_SECONDARY);
    g.DrawString(L"Opacity Preset:", -1, &bodyFont, PointF(30, startY + 45), &whiteTxt);

    int opacities[] = { 150, 190, 220, 255 };
    const wchar_t* opLabels[] = { L"60%", L"75%", L"88%", L"100%" };
    for (int i = 0; i < 4; ++i) {
        RectF opBtn(150 + i * 85, startY + 40, 75, 28);
        bool isSel = (state.overlayOpacity == opacities[i]);
        SolidBrush obBg(isSel ? Color(255, 0, 150, 170) : Color(255, 34, 40, 54));
        Pen obPen(isSel ? Theme::P1_ACCENT : Color(255, 50, 58, 78), 1.0f);
        fillRoundedRect(g, obBg, opBtn, 4.0f);
        drawRoundedRect(g, obPen, opBtn, 4.0f);
        SolidBrush obTxt(isSel ? Color(255, 255, 255, 255) : Theme::TEXT_SECONDARY);
        g.DrawString(opLabels[i], -1, &btnFont, opBtn, &sfCenter, &obTxt);
    }

    g.DrawString(L"Press F10 at any time to toggle In-Game Overlay mode over borderless windowed games.", -1, &bodyFont, PointF(30, startY + 85), &grayTxt);

    // Section 2: Fighting Game Guide
    startY += 135.0f;
    RectF s2(18, startY, width - 36.0f, 150.0f);
    fillRoundedRect(g, s1Bg, s2, 8.0f);
    drawRoundedRect(g, s1Pen, s2, 8.0f);

    g.DrawString(L"Fighting Game Setup Guide (KOF XV, SFV, Tekken)", -1, &titleFont, PointF(30, startY + 12), &cyanBrush);
    g.DrawString(L"1. Ensure ViGEmBus driver is installed on your PC.", -1, &bodyFont, PointF(30, startY + 42), &whiteTxt);
    g.DrawString(L"2. Launch Keytech BEFORE starting your game so the game enumerates both virtual Xbox controllers.", -1, &bodyFont, PointF(30, startY + 66), &whiteTxt);
    g.DrawString(L"3. Inside the game, navigate to Versus / Local Battle mode.", -1, &bodyFont, PointF(30, startY + 90), &whiteTxt);
    g.DrawString(L"4. Player 1 uses WASD + JUKILROE. Player 2 uses Arrow Keys + Numpad.", -1, &bodyFont, PointF(30, startY + 114), &whiteTxt);

    // Section 3: Clean Exit
    startY += 165.0f;
    RectF exitBox(18, startY, 220, 36);
    SolidBrush exitBg(Color(255, 180, 30, 45));
    Pen exitPen(Theme::STATUS_RED, 1.0f);
    fillRoundedRect(g, exitBg, exitBox, 6.0f);
    drawRoundedRect(g, exitPen, exitBox, 6.0f);
    g.DrawString(L"Quit Keytech Application", -1, &btnFont, exitBox, &sfCenter, &whiteTxt);
}

// -------------------------------------------------------------
// In-Game Overlay HUD (F10 mode)
// -------------------------------------------------------------
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state) {
    // Top drag bar
    Font titleFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font hintFont(L"Segoe UI", 9, FontStyleRegular, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Title / Drag handle
    SolidBrush cyanTxt(Theme::P1_ACCENT);
    g.DrawString(L"KEYTECH OVERLAY (Drag to reposition) [F10: Restore Window]", -1, &titleFont, PointF(16, 8), &cyanTxt);

    // Status Indicator
    Color statusCol = state.active ? Theme::STATUS_GREEN : Theme::STATUS_YELLOW;
    SolidBrush statusDot(statusCol);
    g.FillEllipse(&statusDot, width - 170, 11, 8, 8);
    SolidBrush statusTxt(statusCol);
    g.DrawString(state.active ? L"ACTIVE [F9]" : L"PAUSED [F9]", -1, &hintFont, PointF(width - 156, 8), &statusTxt);

    // Restore button [🗖]
    RectF restoreBtn(width - 70, 6, 26, 20);
    SolidBrush rBg(Color(255, 34, 40, 56));
    fillRoundedRect(g, rBg, restoreBtn, 4.0f);
    SolidBrush wTxt(Color(255, 255, 255, 255));
    g.DrawString(L"[]", -1, &hintFont, restoreBtn, &sfCenter, &wTxt);

    // Close button [X]
    RectF closeBtn(width - 38, 6, 26, 20);
    SolidBrush cBg(Color(255, 60, 20, 25));
    fillRoundedRect(g, cBg, closeBtn, 4.0f);
    SolidBrush redTxt(Theme::STATUS_RED);
    g.DrawString(L"X", -1, &hintFont, closeBtn, &sfCenter, &redTxt);

    // Draw both compact gamepads inside overlay
    float midY = (height + 25.0f) * 0.5f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;
    float scale = 0.85f;

    drawSingleGamepad(g, p1CenterX, midY, scale, 0, state.liveReports[0], state);
    drawSingleGamepad(g, p2CenterX, midY, scale, 1, state.liveReports[1], state);
}

// -------------------------------------------------------------
// Toast notification
// -------------------------------------------------------------
static void drawToast(Graphics& g, int width, int height, const AppState& state) {
    if (state.toastMessage.empty()) return;

    DWORD now = GetTickCount();
    if (now - state.toastTimestamp > 3000) return; // 3 seconds expiry

    float toastW = 320.0f;
    float toastH = 34.0f;
    float toastX = (width - toastW) * 0.5f;
    float toastY = 65.0f;

    RectF toastRect(toastX, toastY, toastW, toastH);
    SolidBrush bgBrush(Color(240, 0, 160, 80));
    fillRoundedRect(g, bgBrush, toastRect, 6.0f);
    Pen borderPen(Theme::STATUS_GREEN, 1.2f);
    drawRoundedRect(g, borderPen, toastRect, 6.0f);

    Font toastFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    std::wstring msgW(state.toastMessage.begin(), state.toastMessage.end());
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    g.DrawString(msgW.c_str(), -1, &toastFont, toastRect, &sf, &whiteBrush);
}

// -------------------------------------------------------------
// Mouse Input Handling
// -------------------------------------------------------------
void handleUIMouseDown(HWND hwnd, int x, int y, AppState& state) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    if (state.isOverlayMode) {
        // Close button in overlay
        if (x >= width - 38 && x <= width - 12 && y >= 6 && y <= 26) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return;
        }
        // Restore window button in overlay
        if (x >= width - 70 && x <= width - 44 && y >= 6 && y <= 26) {
            PostMessage(hwnd, WM_COMMAND, 1002, 0); // Toggle Overlay
            return;
        }
        // Draggable anywhere in top bar of overlay
        if (y < 35) {
            state.isDragging = true;
            GetCursorPos(&state.dragOffset);
            RECT wrc;
            GetWindowRect(hwnd, &wrc);
            state.dragOffset.x -= wrc.left;
            state.dragOffset.y -= wrc.top;
            SetCapture(hwnd);
            return;
        }
        return;
    }

    // Windowed Mode: Top Bar buttons
    // Exit button
    if (x >= width - 95 && x <= width - 15 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    // Overlay button
    if (x >= width - 240 && x <= width - 110 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_COMMAND, 1002, 0); // Toggle Overlay
        return;
    }

    // Toggle active status pill
    if (x >= 170 && x <= 320 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_COMMAND, 1001, 0); // Toggle F9
        return;
    }

    // Tab buttons
    if (y >= 60 && y <= 90) {
        float curX = 18.0f;
        for (int i = 0; i < 3; ++i) {
            float tabW = 210.0f;
            if (x >= curX && x <= curX + tabW) {
                state.currentTab = (AppTab)i;
                state.rebindingPlayer = -1;
                state.rebindingButton = 0;
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
            curX += tabW + 8.0f;
        }
    }

    // Tab 1: Key Remapping Actions
    if (state.currentTab == TAB_KEYBINDS) {
        float startY = 106.0f;
        if (hasDuplicates()) startY += 44.0f;

        float colW = (width - 54.0f) * 0.5f;

        // Check if user clicked any key box to rebind
        for (int p = 0; p < 2; ++p) {
            float colX = 18.0f + p * (colW + 18.0f);
            float itemY = startY + 36.0f;

            for (const auto& b : g_availableButtons) {
                float kx = colX + colW * 0.48f;
                float ky = itemY;
                float kw = colW * 0.48f;
                float kh = 24.0f;

                if (x >= kx && x <= kx + kw && y >= ky && y <= ky + kh) {
                    state.rebindingPlayer = p;
                    state.rebindingButton = b.button;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return;
                }
                itemY += 28.0f;
            }
        }

        // Save Button
        float bottomY = height - 48.0f;
        if (x >= 18 && x <= 178 && y >= bottomY && y <= bottomY + 32) {
            if (saveConfig("config.json")) {
                triggerToast(state, "Config saved to config.json!");
            } else {
                triggerToast(state, "Failed to save config.json");
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }

        // Reset Defaults Button
        if (x >= 190 && x <= 350 && y >= bottomY && y <= bottomY + 32) {
            resetToDefaults();
            state.rebindingPlayer = -1;
            state.rebindingButton = 0;
            triggerToast(state, "Reset to default fighting layout!");
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }

    // Tab 2: Settings Actions
    if (state.currentTab == TAB_SETTINGS) {
        float startY = 110.0f;
        // Opacity buttons
        int opacities[] = { 150, 190, 220, 255 };
        for (int i = 0; i < 4; ++i) {
            float bx = 150 + i * 85;
            float by = startY + 40;
            if (x >= bx && x <= bx + 75 && y >= by && y <= by + 28) {
                state.overlayOpacity = opacities[i];
                triggerToast(state, "Overlay opacity updated");
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
        }

        // Quit Button in Settings
        float quitY = startY + 300.0f;
        if (x >= 18 && x <= 238 && y >= quitY && y <= quitY + 36) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return;
        }
    }
}

void handleUIMouseMove(HWND hwnd, int x, int y, AppState& state) {
    if (state.isDragging) {
        POINT cur;
        GetCursorPos(&cur);
        SetWindowPos(hwnd, NULL, cur.x - state.dragOffset.x, cur.y - state.dragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void handleUIMouseUp(HWND hwnd, int x, int y, AppState& state) {
    if (state.isDragging) {
        state.isDragging = false;
        ReleaseCapture();
    }
}
