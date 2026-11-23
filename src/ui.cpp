#include "ui.h"
#include <gdiplus.h>
#include <dwmapi.h>
#include <sstream>
#include <iomanip>
#include <cmath>
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

// -------------------------------------------------------------
// Geometric and Drawing Helpers
// -------------------------------------------------------------
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

// Clean, Modern, Minimal Tournament Palette
namespace Theme {
    const Color BG_MAIN       = Color(255, 13, 17, 23);       // #0D1117 (GitHub-Dark style obsidian)
    const Color PANEL_BG      = Color(255, 22, 27, 34);       // #161B22
    const Color CARD_BG       = Color(255, 27, 33, 44);       // #1B212C
    const Color CARD_BORDER   = Color(255, 48, 54, 66);       // #303642
    const Color CARD_BORDER_HI= Color(255, 65, 75, 95);       // #414B5F
    
    const Color TEXT_PRIMARY  = Color(255, 240, 246, 252);    // #F0F6FC
    const Color TEXT_SECONDARY= Color(255, 139, 148, 158);    // #8B949E
    const Color TEXT_MUTED    = Color(255, 110, 118, 129);    // #6E7681
    const Color TEXT_DIM      = Color(255, 75, 85, 99);       // #4B5563
    
    // Player Accents
    const Color P1_ACCENT     = Color(255, 0, 212, 255);      // Cyan #00D4FF
    const Color P1_BG         = Color(40, 0, 212, 255);
    const Color P2_ACCENT     = Color(255, 255, 77, 106);     // Coral #FF4D6A
    const Color P2_BG         = Color(40, 255, 77, 106);

    // Xbox Authentic Button Colors
    const Color XBOX_A        = Color(255, 63, 185, 80);      // Authentic Xbox Green
    const Color XBOX_B        = Color(255, 248, 81, 73);      // Authentic Xbox Red
    const Color XBOX_X        = Color(255, 88, 166, 255);     // Authentic Xbox Blue
    const Color XBOX_Y        = Color(255, 242, 204, 96);     // Authentic Xbox Yellow

    // Status Colors (Never Green for Paused!)
    const Color STATUS_GREEN  = Color(255, 63, 185, 80);      // Active / Online
    const Color STATUS_AMBER  = Color(255, 240, 136, 62);     // Paused / Warning
    const Color STATUS_RED    = Color(255, 248, 81, 73);      // Offline / Error / Exit
}

// Forward declarations
static void drawHeader(Graphics& g, int width, const AppState& state);
static void drawTabs(Graphics& g, int width, const AppState& state);
static void drawDashboard(Graphics& g, int width, int height, const AppState& state);
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state);
static void drawSettings(Graphics& g, int width, int height, const AppState& state);
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state);
static void drawToast(Graphics& g, int width, int height, const AppState& state);
static void drawCleanGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state, bool showLegend);

void renderApp(HWND hwnd, HDC hdc, int width, int height, AppState& state) {
    if (width <= 0 || height <= 0) return;

    state.animationTick = GetTickCount();

    // Double buffering setup
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    if (state.isOverlayMode) {
        // Overlay Mode: Translucent glass HUD
        SolidBrush bgBrush(Color(state.overlayOpacity, 13, 17, 23));
        g.Clear(Color(0, 0, 0, 0));
        fillRoundedRect(g, bgBrush, RectF(0, 0, (float)width, (float)height), 12.0f);
        
        Pen borderPen(Theme::P1_ACCENT, 1.5f);
        drawRoundedRect(g, borderPen, RectF(1, 1, (float)width - 2, (float)height - 2), 12.0f);

        drawOverlayHUD(g, width, height, state);
    } else {
        // Main Application Window: Clean Modern Dark Theme (NO grid lines!)
        SolidBrush bgBrush(Theme::BG_MAIN);
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

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// -------------------------------------------------------------
// Header Bar & Status Controls
// -------------------------------------------------------------
static void drawHeader(Graphics& g, int width, const AppState& state) {
    SolidBrush barBg(Theme::PANEL_BG);
    g.FillRectangle(&barBg, 0, 0, width, 56);

    Pen borderPen(Theme::CARD_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0, 56, width, 56);

    // Accent line along top edge
    Pen topTrim(Theme::P1_ACCENT, 2.0f);
    g.DrawLine(&topTrim, 0, 0, width, 0);

    Font brandFont(L"Segoe UI", 16, FontStyleBold, UnitPixel);
    Font subFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    Font badgeFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);

    SolidBrush cyanBrush(Theme::P1_ACCENT);
    SolidBrush mutedBrush(Theme::TEXT_MUTED);

    g.DrawString(L"KEYTECH", -1, &brandFont, PointF(22, 10), &cyanBrush);
    g.DrawString(L"DUAL VIRTUAL GAMEPAD HUB", -1, &subFont, PointF(22, 32), &mutedBrush);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Active Status Badge (Clickable to toggle F9)
    RectF statusPill(235, 14, 155, 28);
    bool isActive = state.active;
    Color statusCol = isActive ? Theme::STATUS_GREEN : Theme::STATUS_AMBER;
    SolidBrush statusBg(Color(30, statusCol.GetR(), statusCol.GetG(), statusCol.GetB()));
    Pen statusPen(statusCol, 1.2f);
    fillRoundedRect(g, statusBg, statusPill, 14.0f);
    drawRoundedRect(g, statusPen, statusPill, 14.0f);

    // Status Indicator Dot
    SolidBrush dotBrush(statusCol);
    g.FillEllipse(&dotBrush, 247, 24, 8, 8);

    SolidBrush statusTxt(statusCol);
    RectF statusLabelR(255, 14, 130, 28);
    g.DrawString(isActive ? L"PADS ACTIVE [F9]" : L"PAUSED [F9]", -1, &badgeFont, statusLabelR, &sfCenter, &statusTxt);

    // ViGEmBus driver badge
    RectF vigemPill(400, 14, 175, 28);
    bool isOnline = state.vigemConnected;
    Color vigemCol = isOnline ? Theme::STATUS_GREEN : Theme::STATUS_AMBER;
    SolidBrush vigemBg(Color(30, vigemCol.GetR(), vigemCol.GetG(), vigemCol.GetB()));
    Pen vigemPen(vigemCol, 1.2f);
    fillRoundedRect(g, vigemBg, vigemPill, 14.0f);
    drawRoundedRect(g, vigemPen, vigemPill, 14.0f);

    SolidBrush vigemDotBrush(vigemCol);
    g.FillEllipse(&vigemDotBrush, 412, 24, 8, 8);

    SolidBrush vigemTxt(vigemCol);
    RectF vigemLabelR(420, 14, 150, 28);
    g.DrawString(isOnline ? L"ViGEm: ONLINE" : L"ViGEm: AUTO-INSTALL", -1, &badgeFont, vigemLabelR, &sfCenter, &vigemTxt);

    // In-Game Overlay Mode Button (F10)
    RectF overlayBtn(width - 250, 14, 145, 28);
    SolidBrush overlayBg(Color(255, 20, 28, 40));
    Pen overlayPen(Theme::P1_ACCENT, 1.0f);
    fillRoundedRect(g, overlayBg, overlayBtn, 6.0f);
    drawRoundedRect(g, overlayPen, overlayBtn, 6.0f);
    g.DrawString(L"In-Game Overlay [F10]", -1, &badgeFont, overlayBtn, &sfCenter, &cyanBrush);

    // Exit Button
    RectF exitBtn(width - 95, 14, 75, 28);
    SolidBrush exitBg(Color(255, 38, 22, 28));
    Pen exitPen(Theme::STATUS_RED, 1.0f);
    fillRoundedRect(g, exitBg, exitBtn, 6.0f);
    drawRoundedRect(g, exitPen, exitBtn, 6.0f);
    SolidBrush redTxt(Theme::STATUS_RED);
    g.DrawString(L"Exit", -1, &badgeFont, exitBtn, &sfCenter, &redTxt);
}

// -------------------------------------------------------------
// Clean ASCII Tab Bar (NO Broken Emojis)
// -------------------------------------------------------------
static void drawTabs(Graphics& g, int width, const AppState& state) {
    float tabY = 56.0f;
    float tabH = 38.0f;
    SolidBrush tabsBg(Theme::PANEL_BG);
    g.FillRectangle(&tabsBg, 0.0f, tabY, (float)width, tabH);

    Pen borderPen(Theme::CARD_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0.0f, tabY + tabH, (float)width, tabY + tabH);

    const wchar_t* tabTitles[] = {
        L"LIVE GAMEPAD HUD",
        L"ARCADE KEY MAPPER",
        L"OVERLAY & SETTINGS"
    };

    Font tabFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float curX = 22.0f;
    for (int i = 0; i < 3; ++i) {
        float tabW = 190.0f;
        RectF tabRect(curX, tabY + 4, tabW, tabH - 8);
        bool isSelected = (state.currentTab == (AppTab)i);

        if (isSelected) {
            SolidBrush activeTabBg(Theme::CARD_BG);
            fillRoundedRect(g, activeTabBg, tabRect, 6.0f);
            
            Pen activePen(Theme::CARD_BORDER_HI, 1.0f);
            drawRoundedRect(g, activePen, tabRect, 6.0f);

            // Active accent line indicator
            SolidBrush cyanBrush(Theme::P1_ACCENT);
            g.FillRectangle(&cyanBrush, curX + 20, tabY + tabH - 2, tabW - 40, 2.0f);

            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sfCenter, &cyanBrush);
        } else {
            SolidBrush inactiveTxt(Theme::TEXT_SECONDARY);
            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sfCenter, &inactiveTxt);
        }

        curX += tabW + 8.0f;
    }
}

// -------------------------------------------------------------
// Clean, Proportionate Flat Xbox Controller Silhouette
// -------------------------------------------------------------
static void drawCleanGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state, bool showLegend) {
    Color accent = (player == 0) ? Theme::P1_ACCENT : Theme::P2_ACCENT;
    Color accentBg = (player == 0) ? Theme::P1_BG : Theme::P2_BG;
    const wchar_t* pTitle = (player == 0) ? L"PLAYER 1  [WASD + ACTION KEYS]" : L"PLAYER 2  [ARROWS + NUMPAD]";

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // ---------------------------------------------------------
    // Top Player Banner
    // ---------------------------------------------------------
    RectF pBadge(cx - 130 * scale, cy - 105 * scale, 260 * scale, 24 * scale);
    SolidBrush badgeBg(accentBg);
    fillRoundedRect(g, badgeBg, pBadge, 12.0f * scale);
    Pen badgeBorder(accent, 1.2f);
    drawRoundedRect(g, badgeBorder, pBadge, 12.0f * scale);

    Font badgeFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
    SolidBrush accentBrush(accent);
    g.DrawString(pTitle, -1, &badgeFont, pBadge, &sfCenter, &accentBrush);

    // ---------------------------------------------------------
    // 1. Shoulder Bumpers (LB & RB) sitting flush atop chassis
    // ---------------------------------------------------------
    auto drawBumper = [&](float bx, float by, float bw, float bh, WORD mask, const wchar_t* lbl) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(bx, by, bw, bh);
        GraphicsPath bp;
        addRoundedRect(bp, r, 4.0f * scale);

        if (pressed) {
            SolidBrush b(accent);
            g.FillPath(&b, &bp);
            Pen p(Theme::TEXT_PRIMARY, 1.2f);
            g.DrawPath(&p, &bp);
            SolidBrush txt(Theme::BG_MAIN);
            Font bFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(lbl, -1, &bFont, r, &sfCenter, &txt);
        } else {
            SolidBrush b(Color(255, 32, 38, 50));
            g.FillPath(&b, &bp);
            Pen p(Theme::CARD_BORDER_HI, 1.0f);
            g.DrawPath(&p, &bp);
            SolidBrush txt(Theme::TEXT_SECONDARY);
            Font bFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(lbl, -1, &bFont, r, &sfCenter, &txt);
        }
    };

    drawBumper(cx - 105 * scale, cy - 74 * scale, 64 * scale, 16 * scale, XUSB_GAMEPAD_LEFT_SHOULDER, L"LB");
    drawBumper(cx + 41 * scale, cy - 74 * scale, 64 * scale, 16 * scale, XUSB_GAMEPAD_RIGHT_SHOULDER, L"RB");

    // ---------------------------------------------------------
    // 2. Symmetrical, Proportionate Xbox Controller Silhouette
    // ---------------------------------------------------------
    GraphicsPath bodyPath;

    // Top bridge between bumpers
    bodyPath.AddBezier(
        cx - 48*scale, cy - 64*scale,
        cx - 20*scale, cy - 67*scale,
        cx + 20*scale, cy - 67*scale,
        cx + 48*scale, cy - 64*scale
    );
    // Right shoulder
    bodyPath.AddBezier(
        cx + 48*scale, cy - 64*scale,
        cx + 100*scale, cy - 66*scale,
        cx + 124*scale, cy - 40*scale,
        cx + 130*scale, cy - 8*scale
    );
    // Right outer grip (ergonomic, smooth slope)
    bodyPath.AddBezier(
        cx + 130*scale, cy - 8*scale,
        cx + 136*scale, cy + 34*scale,
        cx + 124*scale, cy + 76*scale,
        cx + 102*scale, cy + 88*scale
    );
    // Right grip bottom rounded tip
    bodyPath.AddBezier(
        cx + 102*scale, cy + 88*scale,
        cx + 86*scale, cy + 96*scale,
        cx + 74*scale, cy + 82*scale,
        cx + 70*scale, cy + 62*scale
    );
    // Right inner grip to center arch
    bodyPath.AddBezier(
        cx + 70*scale, cy + 62*scale,
        cx + 62*scale, cy + 32*scale,
        cx + 42*scale, cy + 18*scale,
        cx + 28*scale, cy + 18*scale
    );
    // Center bottom arch
    bodyPath.AddBezier(
        cx + 28*scale, cy + 18*scale,
        cx, cy + 24*scale,
        cx, cy + 24*scale,
        cx - 28*scale, cy + 18*scale
    );
    // Left inner grip
    bodyPath.AddBezier(
        cx - 28*scale, cy + 18*scale,
        cx - 42*scale, cy + 18*scale,
        cx - 62*scale, cy + 32*scale,
        cx - 70*scale, cy + 62*scale
    );
    // Left grip bottom rounded tip
    bodyPath.AddBezier(
        cx - 70*scale, cy + 62*scale,
        cx - 74*scale, cy + 82*scale,
        cx - 86*scale, cy + 96*scale,
        cx - 102*scale, cy + 88*scale
    );
    // Left outer grip
    bodyPath.AddBezier(
        cx - 102*scale, cy + 88*scale,
        cx - 124*scale, cy + 76*scale,
        cx - 136*scale, cy + 34*scale,
        cx - 130*scale, cy - 8*scale
    );
    // Left shoulder back to crown
    bodyPath.AddBezier(
        cx - 130*scale, cy - 8*scale,
        cx - 124*scale, cy - 40*scale,
        cx - 100*scale, cy - 66*scale,
        cx - 48*scale, cy - 64*scale
    );
    bodyPath.CloseFigure();

    // Subtle drop shadow
    SolidBrush shadowBrush(Color(45, 0, 0, 0));
    Matrix transMat;
    transMat.Translate(0, 6 * scale);
    GraphicsPath* shadowPath = bodyPath.Clone();
    shadowPath->Transform(&transMat);
    g.FillPath(&shadowBrush, shadowPath);
    delete shadowPath;

    // Chassis Fill: Minimal Dark Slate
    SolidBrush chassisBrush(Color(255, 22, 27, 36));
    g.FillPath(&chassisBrush, &bodyPath);

    // Subtle Crisp 1.2px Border
    Pen chassisPen(Theme::CARD_BORDER_HI, 1.2f * scale);
    g.DrawPath(&chassisPen, &bodyPath);

    // Subtle ergonomic grip accent line (no heavy glow!)
    Pen gripAccent(Color(50, accent.GetR(), accent.GetG(), accent.GetB()), 1.0f * scale);
    g.DrawPath(&gripAccent, &bodyPath);

    // ---------------------------------------------------------
    // 3. Center Guide Emblem (Sleek Circle, NO weird X!)
    // ---------------------------------------------------------
    float orbX = cx;
    float orbY = cy - 25 * scale;
    float orbR = 13.0f * scale;

    SolidBrush ringBrush(Color(255, 35, 42, 56));
    g.FillEllipse(&ringBrush, orbX - orbR, orbY - orbR, orbR * 2, orbR * 2);

    float innerR = orbR - 2.5f * scale;
    SolidBrush centerBrush(Color(255, 14, 18, 25));
    g.FillEllipse(&centerBrush, orbX - innerR, orbY - innerR, innerR * 2, innerR * 2);

    // Player quadrant ring indicator
    Pen ledPen(accent, 2.0f * scale);
    float startAngle = (player == 0) ? 190.0f : 280.0f; // P1 top-left, P2 top-right
    g.DrawArc(&ledPen, orbX - innerR + 1, orbY - innerR + 1, (innerR - 1) * 2, (innerR - 1) * 2, startAngle, 70.0f);

    // ---------------------------------------------------------
    // 4. Back (<) & Start (>) Menu Buttons
    // ---------------------------------------------------------
    auto drawMenuBtn = [&](float mx, float my, WORD mask, const wchar_t* symbol) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(mx - 15 * scale, my - 8 * scale, 30 * scale, 16 * scale);
        GraphicsPath mp;
        addRoundedRect(mp, r, 3.0f * scale);

        if (pressed) {
            SolidBrush b(accent);
            g.FillPath(&b, &mp);
            SolidBrush txt(Theme::BG_MAIN);
            Font mFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(symbol, -1, &mFont, r, &sfCenter, &txt);
        } else {
            SolidBrush b(Color(255, 30, 36, 48));
            g.FillPath(&b, &mp);
            Pen p(Theme::CARD_BORDER_HI, 1.0f);
            g.DrawPath(&p, &mp);
            SolidBrush txt(Theme::TEXT_SECONDARY);
            Font mFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(symbol, -1, &mFont, r, &sfCenter, &txt);
        }
    };

    drawMenuBtn(cx - 36 * scale, cy - 25 * scale, XUSB_GAMEPAD_BACK, L"<");
    drawMenuBtn(cx + 36 * scale, cy - 25 * scale, XUSB_GAMEPAD_START, L">");

    // ---------------------------------------------------------
    // 5. Left Analog Thumbstick (LS) - Upper Left
    // ---------------------------------------------------------
    auto drawStick = [&](float sx, float sy, const wchar_t* lbl) {
        float stickWellR = 20.0f * scale;
        float stickCapR  = 14.0f * scale;

        // Recessed well
        SolidBrush wellB(Color(255, 14, 18, 25));
        g.FillEllipse(&wellB, sx - stickWellR, sy - stickWellR, stickWellR * 2, stickWellR * 2);
        Pen wellP(Theme::CARD_BORDER, 1.0f);
        g.DrawEllipse(&wellP, sx - stickWellR, sy - stickWellR, stickWellR * 2, stickWellR * 2);

        // Raised Cap
        SolidBrush capB(Color(255, 28, 34, 46));
        g.FillEllipse(&capB, sx - stickCapR, sy - stickCapR, stickCapR * 2, stickCapR * 2);
        Pen capP(Theme::CARD_BORDER_HI, 1.2f);
        g.DrawEllipse(&capP, sx - stickCapR, sy - stickCapR, stickCapR * 2, stickCapR * 2);

        // 4 subtle tactile grip dots
        SolidBrush dotB(accent);
        float dOff = 8.0f * scale;
        g.FillEllipse(&dotB, sx - 1, sy - dOff - 1, 2.0f, 2.0f);
        g.FillEllipse(&dotB, sx - 1, sy + dOff - 1, 2.0f, 2.0f);
        g.FillEllipse(&dotB, sx - dOff - 1, sy - 1, 2.0f, 2.0f);
        g.FillEllipse(&dotB, sx + dOff - 1, sy - 1, 2.0f, 2.0f);

        Font sFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
        RectF r(sx - 12, sy - 8, 24, 16);
        SolidBrush sTxt(Theme::TEXT_SECONDARY);
        g.DrawString(lbl, -1, &sFont, r, &sfCenter, &sTxt);
    };

    drawStick(cx - 66 * scale, cy - 18 * scale, L"LS");
    drawStick(cx + 40 * scale, cy + 22 * scale, L"RS");

    // ---------------------------------------------------------
    // 6. Cross D-Pad (Lower Left)
    // ---------------------------------------------------------
    float dpadX = cx - 40 * scale;
    float dpadY = cy + 22 * scale;
    float dishR = 24.0f * scale;

    // Recessed circular dish
    SolidBrush dishB(Color(255, 14, 18, 25));
    g.FillEllipse(&dishB, dpadX - dishR, dpadY - dishR, dishR * 2, dishR * 2);
    Pen dishP(Theme::CARD_BORDER, 1.0f);
    g.DrawEllipse(&dishP, dpadX - dishR, dpadY - dishR, dishR * 2, dishR * 2);

    float armLen = 20.0f * scale;
    float armW   = 16.0f * scale;

    auto drawDpadArm = [&](float ax, float ay, float aw, float ah, WORD mask, const wchar_t* arr) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(ax, ay, aw, ah);

        if (pressed) {
            SolidBrush b(accent);
            g.FillRectangle(&b, r);
            SolidBrush txt(Theme::BG_MAIN);
            Font arrFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(arr, -1, &arrFont, r, &sfCenter, &txt);
        } else {
            SolidBrush b(Color(255, 30, 36, 48));
            g.FillRectangle(&b, r);
            Pen p(Theme::CARD_BORDER_HI, 1.0f);
            g.DrawRectangle(&p, ax, ay, aw, ah);
            SolidBrush txt(Theme::TEXT_PRIMARY);
            Font arrFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(arr, -1, &arrFont, r, &sfCenter, &txt);
        }
    };

    drawDpadArm(dpadX - armW*0.5f, dpadY - armLen, armW, armLen - armW*0.5f, XUSB_GAMEPAD_DPAD_UP, L"^");
    drawDpadArm(dpadX - armW*0.5f, dpadY + armW*0.5f, armW, armLen - armW*0.5f, XUSB_GAMEPAD_DPAD_DOWN, L"v");
    drawDpadArm(dpadX - armLen, dpadY - armW*0.5f, armLen - armW*0.5f, armW, XUSB_GAMEPAD_DPAD_LEFT, L"<");
    drawDpadArm(dpadX + armW*0.5f, dpadY - armW*0.5f, armLen - armW*0.5f, armW, XUSB_GAMEPAD_DPAD_RIGHT, L">");

    // Center D-Pad Hub
    SolidBrush dpadHub(Color(255, 24, 29, 39));
    g.FillRectangle(&dpadHub, dpadX - armW*0.5f, dpadY - armW*0.5f, armW, armW);

    // ---------------------------------------------------------
    // 7. ABXY Diamond Buttons (Upper Right)
    // ---------------------------------------------------------
    float faceX = cx + 66 * scale;
    float faceY = cy - 18 * scale;
    float faceSpread = 22.0f * scale;
    float btnR = 11.0f * scale;

    auto drawABXY = [&](float bx, float by, WORD mask, const wchar_t* letter, Color baseCol) {
        bool pressed = (report.wButtons & mask) != 0;
        float dia = btnR * 2.0f;
        RectF r(bx - btnR, by - btnR, dia, dia);

        if (pressed) {
            SolidBrush b(accent);
            g.FillEllipse(&b, r);
            Pen p(Theme::TEXT_PRIMARY, 1.5f);
            g.DrawEllipse(&p, r);
            SolidBrush txt(Theme::BG_MAIN);
            Font letterFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        } else {
            SolidBrush b(Color(255, 24, 29, 40));
            g.FillEllipse(&b, r);
            Pen p(baseCol, 1.5f * scale);
            g.DrawEllipse(&p, r);
            SolidBrush txt(baseCol);
            Font letterFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        }
    };

    drawABXY(faceX, faceY - faceSpread, XUSB_GAMEPAD_Y, L"Y", Theme::XBOX_Y);
    drawABXY(faceX, faceY + faceSpread, XUSB_GAMEPAD_A, L"A", Theme::XBOX_A);
    drawABXY(faceX - faceSpread, faceY, XUSB_GAMEPAD_X, L"X", Theme::XBOX_X);
    drawABXY(faceX + faceSpread, faceY, XUSB_GAMEPAD_B, L"B", Theme::XBOX_B);

    // ---------------------------------------------------------
    // 8. Keymap Legend Card (Below Controller - Zero Overlap!)
    // ---------------------------------------------------------
    if (showLegend) {
        float legY = cy + 104.0f * scale;
        float legW = 280.0f * scale;
        float legH = 112.0f * scale;
        RectF legCard(cx - legW * 0.5f, legY, legW, legH);

        SolidBrush legBg(Theme::PANEL_BG);
        fillRoundedRect(g, legBg, legCard, 8.0f * scale);
        Pen legBorder(Theme::CARD_BORDER, 1.0f);
        drawRoundedRect(g, legBorder, legCard, 8.0f * scale);

        // Header
        Font legHeadFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
        Font itemFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
        SolidBrush headTxt(Theme::TEXT_SECONDARY);
        StringFormat sfLeft;
        sfLeft.SetLineAlignment(StringAlignmentCenter);

        struct LegendItem {
            const wchar_t* btnLbl;
            WORD mask;
        };

        const LegendItem colLeft[6] = {
            { L"UP",    XUSB_GAMEPAD_DPAD_UP },
            { L"DOWN",  XUSB_GAMEPAD_DPAD_DOWN },
            { L"LEFT",  XUSB_GAMEPAD_DPAD_LEFT },
            { L"RIGHT", XUSB_GAMEPAD_DPAD_RIGHT },
            { L"LB",    XUSB_GAMEPAD_LEFT_SHOULDER },
            { L"RB",    XUSB_GAMEPAD_RIGHT_SHOULDER }
        };

        const LegendItem colRight[6] = {
            { L"A",     XUSB_GAMEPAD_A },
            { L"B",     XUSB_GAMEPAD_B },
            { L"X",     XUSB_GAMEPAD_X },
            { L"Y",     XUSB_GAMEPAD_Y },
            { L"BACK",  XUSB_GAMEPAD_BACK },
            { L"START", XUSB_GAMEPAD_START }
        };

        float halfW = legW * 0.5f;

        auto drawColumn = [&](const LegendItem items[6], float startX) {
            float rowY = legY + 6.0f * scale;
            for (int i = 0; i < 6; ++i) {
                bool pressed = (report.wButtons & items[i].mask) != 0;
                int key = getKeyForButton(player, items[i].mask);
                std::string kn = getKeyName(key);
                std::wstring knw(kn.begin(), kn.end());

                // Button badge
                RectF btnBadge(startX + 8 * scale, rowY, 34 * scale, 15 * scale);
                SolidBrush bBg(pressed ? accent : Color(255, 32, 38, 50));
                fillRoundedRect(g, bBg, btnBadge, 3.0f * scale);
                SolidBrush bTxt(pressed ? Theme::BG_MAIN : Theme::TEXT_SECONDARY);
                g.DrawString(items[i].btnLbl, -1, &itemFont, btnBadge, &sfCenter, &bTxt);

                // Keychip
                RectF keyChip(startX + 46 * scale, rowY, halfW - 54 * scale, 15 * scale);
                SolidBrush kBg(pressed ? accentBg : Color(255, 24, 29, 39));
                fillRoundedRect(g, kBg, keyChip, 3.0f * scale);
                Pen kPen(pressed ? accent : Theme::CARD_BORDER, 1.0f);
                drawRoundedRect(g, kPen, keyChip, 3.0f * scale);

                SolidBrush kTxt(pressed ? accent : Theme::TEXT_PRIMARY);
                g.DrawString(knw.c_str(), -1, &itemFont, keyChip, &sfCenter, &kTxt);

                rowY += 17.0f * scale;
            }
        };

        drawColumn(colLeft, cx - legW * 0.5f);
        drawColumn(colRight, cx);
    }
}

// -------------------------------------------------------------
// Tab 0: Live Gamepad HUD
// -------------------------------------------------------------
static void drawDashboard(Graphics& g, int width, int height, const AppState& state) {
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Missing ViGEm driver auto-install banner
    if (!state.vigemConnected) {
        RectF vBanner(22, 100, width - 44.0f, 32.0f);
        SolidBrush vBg(Color(35, 240, 136, 62));
        fillRoundedRect(g, vBg, vBanner, 6.0f);
        Pen vPen(Theme::STATUS_AMBER, 1.0f);
        drawRoundedRect(g, vPen, vBanner, 6.0f);

        Font warnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
        SolidBrush amberBrush(Theme::STATUS_AMBER);
        g.DrawString(L"ViGEmBus Driver Not Installed: Virtual gamepads require ViGEmBus. [ CLICK HERE TO AUTO-INSTALL DRIVER ]", -1, &warnFont, vBanner, &sfCenter, &amberBrush);
    }

    float midY = 195.0f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;

    drawCleanGamepad(g, p1CenterX, midY, 1.05f, 0, state.liveReports[0], state, true);
    drawCleanGamepad(g, p2CenterX, midY, 1.05f, 1, state.liveReports[1], state, true);

    // Bottom Status & Hotkeys Strip
    RectF bottomBar(22, height - 48.0f, width - 44.0f, 34.0f);
    SolidBrush barBg(Theme::PANEL_BG);
    fillRoundedRect(g, barBg, bottomBar, 6.0f);
    Pen barPen(Theme::CARD_BORDER, 1.0f);
    drawRoundedRect(g, barPen, bottomBar, 6.0f);

    Font hintFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    SolidBrush hintBrush(Theme::TEXT_MUTED);
    g.DrawString(L"Hotkeys:  F9: Toggle Controllers (Active / Paused)   |   F10: Toggle In-Game Overlay   |   Engine Rate: 1000 Hz (<1ms latency)", -1, &hintFont, PointF(35, height - 39.0f), &hintBrush);
}

// -------------------------------------------------------------
// Tab 1: Arcade Key Mapper
// -------------------------------------------------------------
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state) {
    float startY = 104.0f;

    // Duplicate Key Conflict Warning Banner
    std::vector<int> duplicates = getDuplicateKeys();
    if (!duplicates.empty()) {
        RectF alertRect(22, startY, width - 44.0f, 34.0f);
        SolidBrush alertBg(Color(35, 248, 81, 73));
        Pen alertPen(Theme::STATUS_RED, 1.2f);
        fillRoundedRect(g, alertBg, alertRect, 6.0f);
        drawRoundedRect(g, alertPen, alertRect, 6.0f);

        std::wstring warnText = L"DUPLICATE KEY CONFLICT: ";
        for (size_t i = 0; i < duplicates.size(); ++i) {
            std::string kn = getKeyName(duplicates[i]);
            warnText += std::wstring(kn.begin(), kn.end());
            if (i + 1 < duplicates.size()) warnText += L", ";
        }
        warnText += L" is mapped to multiple actions! Please resolve duplicates.";

        Font warnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
        SolidBrush redBrush(Theme::STATUS_RED);
        g.DrawString(warnText.c_str(), -1, &warnFont, PointF(35, startY + 8), &redBrush);
        startY += 44.0f;
    }

    Font colHeaderFont(L"Segoe UI", 12, FontStyleBold, UnitPixel);
    Font rowFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    Font keyBoxFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float colW = (width - 64.0f) * 0.5f;

    for (int p = 0; p < 2; ++p) {
        float colX = 22.0f + p * (colW + 20.0f);
        Color accent = (p == 0) ? Theme::P1_ACCENT : Theme::P2_ACCENT;

        // Command Box
        RectF colBox(colX, startY, colW, height - startY - 58.0f);
        SolidBrush colBg(Theme::PANEL_BG);
        fillRoundedRect(g, colBg, colBox, 8.0f);
        Pen colBorder(accent, 1.0f);
        drawRoundedRect(g, colBorder, colBox, 8.0f);

        // Header
        RectF headRect(colX, startY, colW, 30.0f);
        SolidBrush headBg(Color(30, accent.GetR(), accent.GetG(), accent.GetB()));
        fillRoundedRect(g, headBg, headRect, 8.0f);
        SolidBrush accentBrush(accent);
        g.DrawString(p == 0 ? L"PLAYER 1 ARCADE MAPPINGS" : L"PLAYER 2 ARCADE MAPPINGS", -1, &colHeaderFont, headRect, &sfCenter, &accentBrush);

        // Rows
        float itemY = startY + 36.0f;
        for (const auto& b : g_availableButtons) {
            int currentKey = getKeyForButton(p, b.button);
            bool isRebindingThis = (state.rebindingPlayer == p && state.rebindingButton == b.button);

            // Row Label
            RectF lblRect(colX + 16, itemY, colW * 0.44f, 22.0f);
            SolidBrush txtBrush(Theme::TEXT_PRIMARY);
            std::wstring lblW(b.label, b.label + strlen(b.label));
            StringFormat sfLeft;
            sfLeft.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(lblW.c_str(), -1, &rowFont, lblRect, &sfLeft, &txtBrush);

            // Key Box (Click to rebind)
            RectF keyBox(colX + colW * 0.48f, itemY, colW * 0.48f, 22.0f);
            
            if (isRebindingThis) {
                SolidBrush rebindingBg(Color(255, 60, 42, 10));
                Pen rebindingPen(Theme::STATUS_AMBER, 1.5f);
                fillRoundedRect(g, rebindingBg, keyBox, 4.0f);
                drawRoundedRect(g, rebindingPen, keyBox, 4.0f);

                SolidBrush rebindingTxt(Theme::STATUS_AMBER);
                g.DrawString(L"[PRESS ANY KEY...]", -1, &keyBoxFont, keyBox, &sfCenter, &rebindingTxt);
            } else {
                SolidBrush boxBg(Color(255, 26, 32, 46));
                Pen boxPen(Theme::CARD_BORDER_HI, 1.0f);
                fillRoundedRect(g, boxBg, keyBox, 4.0f);
                drawRoundedRect(g, boxPen, keyBox, 4.0f);

                std::string kn = getKeyName(currentKey);
                std::wstring knw(kn.begin(), kn.end());
                SolidBrush boxTxt(Theme::TEXT_PRIMARY);
                g.DrawString(knw.c_str(), -1, &keyBoxFont, keyBox, &sfCenter, &boxTxt);
            }

            itemY += 26.0f;
        }
    }

    // Action Controls
    float bottomY = height - 48.0f;
    Font btnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);

    // Save Button
    RectF saveBtn(22, bottomY, 160, 32);
    SolidBrush saveBg(Color(255, 35, 134, 54));
    Pen savePen(Theme::STATUS_GREEN, 1.2f);
    fillRoundedRect(g, saveBg, saveBtn, 6.0f);
    drawRoundedRect(g, savePen, saveBtn, 6.0f);
    SolidBrush whiteTxt(Theme::TEXT_PRIMARY);
    g.DrawString(L"Save Config", -1, &btnFont, saveBtn, &sfCenter, &whiteTxt);

    // Reset Defaults Button
    RectF resetBtn(195, bottomY, 180, 32);
    SolidBrush resetBg(Color(255, 34, 40, 56));
    Pen resetPen(Theme::CARD_BORDER_HI, 1.0f);
    fillRoundedRect(g, resetBg, resetBtn, 6.0f);
    drawRoundedRect(g, resetPen, resetBtn, 6.0f);
    SolidBrush resetTxt(Theme::TEXT_SECONDARY);
    g.DrawString(L"Reset Fightstick Defaults", -1, &btnFont, resetBtn, &sfCenter, &resetTxt);

    Font tipFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    SolidBrush tipBrush(Theme::TEXT_MUTED);
    g.DrawString(L"Click any key box to rebind. Press ESC while rebinding to cancel.", -1, &tipFont, PointF(390, bottomY + 8), &tipBrush);
}

// -------------------------------------------------------------
// Tab 2: Settings & Overlay Options
// -------------------------------------------------------------
static void drawSettings(Graphics& g, int width, int height, const AppState& state) {
    float startY = 108.0f;
    Font titleFont(L"Segoe UI", 13, FontStyleBold, UnitPixel);
    Font bodyFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    Font btnFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Section 1: In-Game Overlay Presets
    RectF s1(22, startY, width - 44.0f, 115.0f);
    SolidBrush s1Bg(Theme::PANEL_BG);
    fillRoundedRect(g, s1Bg, s1, 8.0f);
    Pen s1Pen(Theme::CARD_BORDER, 1.0f);
    drawRoundedRect(g, s1Pen, s1, 8.0f);

    SolidBrush cyanBrush(Theme::P1_ACCENT);
    g.DrawString(L"In-Game Overlay Configuration (F10 Mode)", -1, &titleFont, PointF(34, startY + 12), &cyanBrush);

    SolidBrush whiteTxt(Theme::TEXT_PRIMARY);
    SolidBrush mutedTxt(Theme::TEXT_MUTED);
    g.DrawString(L"HUD Opacity Preset:", -1, &bodyFont, PointF(34, startY + 44), &whiteTxt);

    int opacities[] = { 150, 190, 220, 255 };
    const wchar_t* opLabels[] = { L"60%", L"75%", L"88%", L"100%" };
    for (int i = 0; i < 4; ++i) {
        RectF opBtn(165 + i * 85, startY + 40, 75, 26);
        bool isSel = (state.overlayOpacity == opacities[i]);
        SolidBrush obBg(isSel ? Color(255, 0, 140, 170) : Color(255, 30, 36, 50));
        Pen obPen(isSel ? Theme::P1_ACCENT : Theme::CARD_BORDER_HI, 1.0f);
        fillRoundedRect(g, obBg, opBtn, 4.0f);
        drawRoundedRect(g, obPen, opBtn, 4.0f);
        SolidBrush obTxt(isSel ? Theme::TEXT_PRIMARY : Theme::TEXT_SECONDARY);
        g.DrawString(opLabels[i], -1, &btnFont, opBtn, &sfCenter, &obTxt);
    }

    g.DrawString(L"When In-Game Overlay is toggled (F10), KEYTECH floats seamlessly over your fighting game window.", -1, &bodyFont, PointF(34, startY + 80), &mutedTxt);

    // Section 2: Fighting Game Setup Guide
    startY += 130.0f;
    RectF s2(22, startY, width - 44.0f, 140.0f);
    fillRoundedRect(g, s1Bg, s2, 8.0f);
    drawRoundedRect(g, s1Pen, s2, 8.0f);

    g.DrawString(L"Pro Tournament Guide (KOF XV, SF6, SFV, Tekken 8, Guilty Gear)", -1, &titleFont, PointF(34, startY + 12), &cyanBrush);
    g.DrawString(L"1. Ensure ViGEmBus driver is installed (Status badge at top must say ONLINE).", -1, &bodyFont, PointF(34, startY + 40), &whiteTxt);
    g.DrawString(L"2. Launch KEYTECH first, then launch your game so it detects Controller 1 & 2 at startup.", -1, &bodyFont, PointF(34, startY + 64), &whiteTxt);
    g.DrawString(L"3. In game settings, set display mode to 'Borderless Windowed' for optimal in-game overlay display.", -1, &bodyFont, PointF(34, startY + 88), &whiteTxt);
    g.DrawString(L"4. Press F9 at any time to temporarily release keyboard focus for typing in chat.", -1, &bodyFont, PointF(34, startY + 112), &whiteTxt);

    // Section 3: Clean Exit
    startY += 155.0f;
    RectF exitBox(22, startY, 210, 34);
    SolidBrush exitBg(Color(255, 140, 24, 36));
    Pen exitPen(Theme::STATUS_RED, 1.2f);
    fillRoundedRect(g, exitBg, exitBox, 6.0f);
    drawRoundedRect(g, exitPen, exitBox, 6.0f);
    g.DrawString(L"Quit KEYTECH Application", -1, &btnFont, exitBox, &sfCenter, &whiteTxt);
}

// -------------------------------------------------------------
// In-Game Floating Transparent HUD (F10 Mode)
// -------------------------------------------------------------
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state) {
    Font titleFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    Font hintFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Title / Drag handle
    SolidBrush cyanTxt(Theme::P1_ACCENT);
    g.DrawString(L"KEYTECH OVERLAY // [DRAG TOP BAR TO MOVE] // [F10: RESTORE WINDOW]", -1, &titleFont, PointF(18, 8), &cyanTxt);

    // Status Indicator
    Color statusCol = state.active ? Theme::STATUS_GREEN : Theme::STATUS_AMBER;
    SolidBrush statusDot(statusCol);
    g.FillEllipse(&statusDot, width - 170, 11, 8, 8);
    SolidBrush statusTxt(statusCol);
    g.DrawString(state.active ? L"ACTIVE [F9]" : L"PAUSED [F9]", -1, &hintFont, PointF(width - 156, 8), &statusTxt);

    // Restore button [[]]
    RectF restoreBtn(width - 70, 6, 26, 20);
    SolidBrush rBg(Color(255, 28, 36, 52));
    fillRoundedRect(g, rBg, restoreBtn, 4.0f);
    SolidBrush wTxt(Theme::TEXT_PRIMARY);
    g.DrawString(L"[]", -1, &hintFont, restoreBtn, &sfCenter, &wTxt);

    // Close button [X]
    RectF closeBtn(width - 38, 6, 26, 20);
    SolidBrush cBg(Color(255, 55, 18, 24));
    fillRoundedRect(g, cBg, closeBtn, 4.0f);
    SolidBrush redTxt(Theme::STATUS_RED);
    g.DrawString(L"X", -1, &hintFont, closeBtn, &sfCenter, &redTxt);

    // Render clean gamepads inside overlay (no legend needed in compact HUD)
    float midY = (height + 26.0f) * 0.5f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;
    float scale = 0.88f;

    drawCleanGamepad(g, p1CenterX, midY, scale, 0, state.liveReports[0], state, false);
    drawCleanGamepad(g, p2CenterX, midY, scale, 1, state.liveReports[1], state, false);
}

// -------------------------------------------------------------
// Toast Notifications
// -------------------------------------------------------------
static void drawToast(Graphics& g, int width, int height, const AppState& state) {
    if (state.toastMessage.empty()) return;

    DWORD now = GetTickCount();
    if (now - state.toastTimestamp > 3000) return;

    float toastW = 340.0f;
    float toastH = 34.0f;
    float toastX = (width - toastW) * 0.5f;
    float toastY = 66.0f;

    RectF toastRect(toastX, toastY, toastW, toastH);
    SolidBrush bgBrush(Color(240, 22, 27, 34));
    fillRoundedRect(g, bgBrush, toastRect, 6.0f);
    Pen borderPen(Theme::P1_ACCENT, 1.2f);
    drawRoundedRect(g, borderPen, toastRect, 6.0f);

    Font toastFont(L"Segoe UI", 11, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    std::wstring msgW(state.toastMessage.begin(), state.toastMessage.end());
    SolidBrush whiteBrush(Theme::TEXT_PRIMARY);
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
        if (x >= width - 38 && x <= width - 12 && y >= 6 && y <= 26) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return;
        }
        if (x >= width - 70 && x <= width - 44 && y >= 6 && y <= 26) {
            PostMessage(hwnd, WM_COMMAND, 1002, 0);
            return;
        }
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

    // Windowed Header Controls
    if (x >= width - 95 && x <= width - 20 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (x >= width - 250 && x <= width - 105 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_COMMAND, 1002, 0);
        return;
    }

    if (x >= 235 && x <= 390 && y >= 14 && y <= 42) {
        PostMessage(hwnd, WM_COMMAND, 1001, 0);
        return;
    }

    // ViGEmBus status badge (Click to auto-install if missing)
    if (x >= 400 && x <= 585 && y >= 14 && y <= 42) {
        if (!state.vigemConnected) {
            PostMessage(hwnd, WM_COMMAND, 1003, 0);
            return;
        }
    }

    // Missing ViGEm driver banner click on Dashboard
    if (!state.vigemConnected && state.currentTab == TAB_DASHBOARD && x >= 22 && x <= width - 22 && y >= 100 && y <= 132) {
        PostMessage(hwnd, WM_COMMAND, 1003, 0);
        return;
    }

    // Tabs
    if (y >= 56 && y <= 94) {
        float curX = 22.0f;
        for (int i = 0; i < 3; ++i) {
            float tabW = 190.0f;
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

    // Key Mapping Tab Actions
    if (state.currentTab == TAB_KEYBINDS) {
        float startY = 104.0f;
        if (hasDuplicates()) startY += 44.0f;

        float colW = (width - 64.0f) * 0.5f;

        for (int p = 0; p < 2; ++p) {
            float colX = 22.0f + p * (colW + 20.0f);
            float itemY = startY + 36.0f;

            for (const auto& b : g_availableButtons) {
                float kx = colX + colW * 0.48f;
                float ky = itemY;
                float kw = colW * 0.48f;
                float kh = 22.0f;

                if (x >= kx && x <= kx + kw && y >= ky && y <= ky + kh) {
                    state.rebindingPlayer = p;
                    state.rebindingButton = b.button;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return;
                }
                itemY += 26.0f;
            }
        }

        float bottomY = height - 48.0f;
        if (x >= 22 && x <= 182 && y >= bottomY && y <= bottomY + 32) {
            if (saveConfig("config.json")) {
                triggerToast(state, "Configurations saved to config.json!");
            } else {
                triggerToast(state, "Failed to save config.json");
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }

        if (x >= 195 && x <= 375 && y >= bottomY && y <= bottomY + 32) {
            resetToDefaults();
            state.rebindingPlayer = -1;
            state.rebindingButton = 0;
            triggerToast(state, "Reset to fighting layout defaults!");
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }

    // Settings Tab Actions
    if (state.currentTab == TAB_SETTINGS) {
        float startY = 108.0f;
        int opacities[] = { 150, 190, 220, 255 };
        for (int i = 0; i < 4; ++i) {
            float bx = 165 + i * 85;
            float by = startY + 40;
            if (x >= bx && x <= bx + 75 && y >= by && y <= by + 26) {
                state.overlayOpacity = opacities[i];
                triggerToast(state, "Overlay opacity preset applied");
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
        }

        float quitY = startY + 285.0f;
        if (x >= 22 && x <= 232 && y >= quitY && y <= quitY + 34) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return;
        }
    }
}

void handleUIMouseMove(HWND hwnd, int x, int y, AppState& state) {
    state.mouseX = x;
    state.mouseY = y;

    if (state.isDragging) {
        POINT cur;
        GetCursorPos(&cur);
        SetWindowPos(hwnd, NULL, cur.x - state.dragOffset.x, cur.y - state.dragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void handleUIMouseUp(HWND hwnd, int x, int y, AppState& state) {
    (void)x; (void)y;
    if (state.isDragging) {
        state.isDragging = false;
        ReleaseCapture();
    }
}
