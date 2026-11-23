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

// Cyberpunk / Gaming-Grade Palette
namespace Theme {
    const Color VOID_BG       = Color(255, 11, 13, 20);       // #0B0D14
    const Color PANEL_BG      = Color(255, 18, 22, 32);       // #121620
    const Color CARD_BG       = Color(255, 23, 28, 42);       // #171C2A
    const Color CARD_BORDER   = Color(255, 38, 46, 68);       // #262E44
    const Color TEXT_PRIMARY  = Color(255, 245, 248, 255);    // #F5F8FF
    const Color TEXT_SECONDARY= Color(255, 140, 150, 175);    // #8C96AF
    const Color TEXT_MUTED    = Color(255, 120, 132, 160);    // #7884A0
    const Color TEXT_DIM      = Color(255, 75, 84, 108);      // #4B546C
    
    // Player Accents
    const Color P1_NEON       = Color(255, 0, 240, 255);      // Electric Cyan
    const Color P1_GLOW       = Color(100, 0, 240, 255);
    const Color P1_AURA       = Color(30, 0, 240, 255);

    const Color P2_NEON       = Color(255, 255, 46, 99);      // Neon Coral
    const Color P2_GLOW       = Color(100, 255, 46, 99);
    const Color P2_AURA       = Color(30, 255, 46, 99);

    // Xbox Authentic Button Colors
    const Color XBOX_A        = Color(255, 16, 175, 50);      // Authentic Xbox Green
    const Color XBOX_A_GLOW   = Color(180, 0, 255, 100);
    const Color XBOX_B        = Color(255, 235, 30, 50);      // Authentic Xbox Red
    const Color XBOX_B_GLOW   = Color(180, 255, 50, 70);
    const Color XBOX_X        = Color(255, 0, 130, 230);      // Authentic Xbox Blue
    const Color XBOX_X_GLOW   = Color(180, 30, 180, 255);
    const Color XBOX_Y        = Color(255, 255, 200, 0);      // Authentic Xbox Yellow
    const Color XBOX_Y_GLOW   = Color(180, 255, 230, 30);

    const Color STATUS_GREEN  = Color(255, 0, 230, 118);
    const Color STATUS_AMBER  = Color(255, 255, 179, 0);
    const Color STATUS_RED    = Color(255, 255, 51, 68);
}

// Forward declarations
static void drawHeader(Graphics& g, int width, const AppState& state);
static void drawTabs(Graphics& g, int width, const AppState& state);
static void drawDashboard(Graphics& g, int width, int height, const AppState& state);
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state);
static void drawSettings(Graphics& g, int width, int height, const AppState& state);
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state);
static void drawToast(Graphics& g, int width, int height, const AppState& state);
static void drawRealisticGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state);

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
        SolidBrush bgBrush(Color(state.overlayOpacity, 11, 14, 22));
        g.Clear(Color(0, 0, 0, 0));
        fillRoundedRect(g, bgBrush, RectF(0, 0, (float)width, (float)height), 16.0f);
        
        Pen borderPen(Color(180, 0, 240, 255), 1.5f);
        drawRoundedRect(g, borderPen, RectF(1, 1, (float)width - 2, (float)height - 2), 16.0f);

        drawOverlayHUD(g, width, height, state);
    } else {
        // Main Application Window: Cyberpunk Obsidian Aesthetic
        SolidBrush bgBrush(Theme::VOID_BG);
        g.FillRectangle(&bgBrush, 0, 0, width, height);

        // Subtle tech grid lines
        Pen gridPen(Color(20, 255, 255, 255), 1.0f);
        for (int x = 0; x < width; x += 40) {
            g.DrawLine(&gridPen, x, 96, x, height);
        }
        for (int y = 96; y < height; y += 40) {
            g.DrawLine(&gridPen, 0, y, width, y);
        }

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
    g.FillRectangle(&barBg, 0, 0, width, 58);

    Pen borderPen(Theme::CARD_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0, 58, width, 58);

    // Glowing accent trim on top
    Pen cyanTrim(Theme::P1_NEON, 2.0f);
    g.DrawLine(&cyanTrim, 0, 0, width, 0);

    Font brandFont(L"Segoe UI", 15, FontStyleBold, UnitPixel);
    Font subFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font badgeFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);

    // Brand Title with neon slash
    SolidBrush whiteBrush(Theme::TEXT_PRIMARY);
    SolidBrush cyanBrush(Theme::P1_NEON);
    SolidBrush mutedBrush(Theme::TEXT_MUTED);

    g.DrawString(L"KEYTECH", -1, &brandFont, PointF(20, 12), &cyanBrush);
    g.DrawString(L"// DUAL VIRTUAL GAMEPAD HUB", -1, &subFont, PointF(20, 34), &mutedBrush);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Active Status Badge (Clickable to toggle F9)
    RectF statusPill(235, 15, 155, 28);
    Color statusCol = state.active ? Theme::STATUS_GREEN : Theme::STATUS_AMBER;
    SolidBrush statusBg(Color(35, statusCol.GetR(), statusCol.GetG(), statusCol.GetB()));
    Pen statusPen(statusCol, 1.2f);
    fillRoundedRect(g, statusBg, statusPill, 14.0f);
    drawRoundedRect(g, statusPen, statusPill, 14.0f);

    SolidBrush statusTxt(statusCol);
    g.DrawString(state.active ? L"* PADS ACTIVE [F9]" : L"II PAUSED [F9]", -1, &badgeFont, statusPill, &sfCenter, &statusTxt);

    // ViGEmBus driver badge
    RectF vigemPill(400, 15, 145, 28);
    Color vigemCol = state.vigemConnected ? Theme::STATUS_GREEN : Theme::STATUS_RED;
    SolidBrush vigemBg(Color(30, vigemCol.GetR(), vigemCol.GetG(), vigemCol.GetB()));
    Pen vigemPen(vigemCol, 1.0f);
    fillRoundedRect(g, vigemBg, vigemPill, 14.0f);
    drawRoundedRect(g, vigemPen, vigemPill, 14.0f);

    SolidBrush vigemTxt(vigemCol);
    g.DrawString(state.vigemConnected ? L"ViGEmBus: ONLINE" : L"ViGEmBus: MISSING", -1, &badgeFont, vigemPill, &sfCenter, &vigemTxt);

    // In-Game Overlay Mode Button (F10)
    RectF overlayBtn(width - 250, 15, 140, 28);
    SolidBrush overlayBg(Color(255, 14, 28, 44));
    Pen overlayPen(Theme::P1_NEON, 1.2f);
    fillRoundedRect(g, overlayBg, overlayBtn, 6.0f);
    drawRoundedRect(g, overlayPen, overlayBtn, 6.0f);
    g.DrawString(L"In-Game Overlay [F10]", -1, &badgeFont, overlayBtn, &sfCenter, &cyanBrush);

    // Clean Exit Button
    RectF exitBtn(width - 95, 15, 75, 28);
    SolidBrush exitBg(Color(255, 45, 18, 25));
    Pen exitPen(Theme::STATUS_RED, 1.2f);
    fillRoundedRect(g, exitBg, exitBtn, 6.0f);
    drawRoundedRect(g, exitPen, exitBtn, 6.0f);
    SolidBrush redTxt(Theme::STATUS_RED);
    g.DrawString(L"Exit", -1, &badgeFont, exitBtn, &sfCenter, &redTxt);
}

static void drawTabs(Graphics& g, int width, const AppState& state) {
    float tabY = 58.0f;
    float tabH = 38.0f;
    SolidBrush tabsBg(Color(255, 14, 17, 26));
    g.FillRectangle(&tabsBg, 0.0f, tabY, (float)width, tabH);

    Pen borderPen(Theme::CARD_BORDER, 1.0f);
    g.DrawLine(&borderPen, 0.0f, tabY + tabH, (float)width, tabY + tabH);

    const wchar_t* tabTitles[] = {
        L"🎮 LIVE GAMEPAD HUD",
        L"⌨ ARCADE KEY MAPPER",
        L"⚙ OVERLAY & SETTINGS"
    };

    Font tabFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float curX = 20.0f;
    for (int i = 0; i < 3; ++i) {
        float tabW = 200.0f;
        RectF tabRect(curX, tabY + 4, tabW, tabH - 8);
        bool isSelected = (state.currentTab == (AppTab)i);

        if (isSelected) {
            SolidBrush activeTabBg(Theme::CARD_BG);
            fillRoundedRect(g, activeTabBg, tabRect, 6.0f);
            
            Pen activePen(Theme::P1_NEON, 1.5f);
            drawRoundedRect(g, activePen, tabRect, 6.0f);

            // Glowing under-dash
            SolidBrush cyanBrush(Theme::P1_NEON);
            g.FillRectangle(&cyanBrush, curX + 25, tabY + tabH - 3, tabW - 50, 3.0f);

            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sfCenter, &cyanBrush);
        } else {
            SolidBrush inactiveTxt(Theme::TEXT_MUTED);
            g.DrawString(tabTitles[i], -1, &tabFont, tabRect, &sfCenter, &inactiveTxt);
        }

        curX += tabW + 10.0f;
    }
}

// -------------------------------------------------------------
// Realistic Xbox 360 Controller Renderer
// -------------------------------------------------------------
static void drawRealisticGamepad(Graphics& g, float cx, float cy, float scale, int player, const XUSB_REPORT& report, const AppState& state) {
    Color accent = (player == 0) ? Theme::P1_NEON : Theme::P2_NEON;
    Color glow   = (player == 0) ? Theme::P1_GLOW : Theme::P2_GLOW;
    const wchar_t* pTitle = (player == 0) ? L"PLAYER 1 [WASD + JUKILROE]" : L"PLAYER 2 [ARROWS + NUMPAD]";

    // Top Player Banner
    RectF pBadge(cx - 140 * scale, cy - 138 * scale, 280 * scale, 24 * scale);
    SolidBrush badgeBg(Color(35, accent.GetR(), accent.GetG(), accent.GetB()));
    fillRoundedRect(g, badgeBg, pBadge, 12.0f * scale);
    Pen badgeBorder(accent, 1.2f);
    drawRoundedRect(g, badgeBorder, pBadge, 12.0f * scale);

    Font badgeFont(L"Segoe UI", (int)(10 * scale), FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);
    SolidBrush accentBrush(accent);
    g.DrawString(pTitle, -1, &badgeFont, pBadge, &sfCenter, &accentBrush);

    // ---------------------------------------------------------
    // 1. Shoulder Bumpers & Triggers (Protrude from top crown)
    // ---------------------------------------------------------
    auto drawBumper = [&](float bx, float by, float bw, float bh, WORD mask, const wchar_t* lbl, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(bx, by, bw, bh);
        
        GraphicsPath bp;
        addRoundedRect(bp, r, 6.0f * scale);

        if (pressed) {
            SolidBrush glowB(glow);
            g.FillPath(&glowB, &bp);
            SolidBrush b(accent);
            g.FillPath(&b, &bp);
            Pen p(Color(255, 255, 255, 255), 1.5f);
            g.DrawPath(&p, &bp);
        } else {
            SolidBrush b(Color(255, 30, 36, 50));
            g.FillPath(&b, &bp);
            Pen p(Color(255, 55, 65, 88), 1.0f);
            g.DrawPath(&p, &bp);
        }

        std::wstring text = lbl;
        text += L" [";
        std::string kn = getKeyName(key);
        text += std::wstring(kn.begin(), kn.end());
        text += L"]";

        Font bFont(L"Segoe UI", (int)(9 * scale), FontStyleBold, UnitPixel);
        SolidBrush txt(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_PRIMARY);
        g.DrawString(text.c_str(), -1, &bFont, r, &sfCenter, &txt);
    };

    drawBumper(cx - 138 * scale, cy - 105 * scale, 95 * scale, 22 * scale, XUSB_GAMEPAD_LEFT_SHOULDER, L"LB", getKeyForButton(player, XUSB_GAMEPAD_LEFT_SHOULDER));
    drawBumper(cx + 43 * scale, cy - 105 * scale, 95 * scale, 22 * scale, XUSB_GAMEPAD_RIGHT_SHOULDER, L"RB", getKeyForButton(player, XUSB_GAMEPAD_RIGHT_SHOULDER));

    // ---------------------------------------------------------
    // 2. Realistic Ergonomic Controller Chassis Path
    // ---------------------------------------------------------
    GraphicsPath bodyPath;

    // Top crown bridge
    bodyPath.AddBezier(
        cx - 60*scale, cy - 88*scale,
        cx - 20*scale, cy - 92*scale,
        cx + 20*scale, cy - 92*scale,
        cx + 60*scale, cy - 88*scale
    );
    // Right shoulder
    bodyPath.AddBezier(
        cx + 60*scale, cy - 88*scale,
        cx + 120*scale, cy - 96*scale,
        cx + 150*scale, cy - 60*scale,
        cx + 158*scale, cy - 10*scale
    );
    // Right outer grip handle
    bodyPath.AddBezier(
        cx + 158*scale, cy - 10*scale,
        cx + 165*scale, cy + 50*scale,
        cx + 148*scale, cy + 105*scale,
        cx + 120*scale, cy + 122*scale
    );
    // Right grip bottom rounded tip
    bodyPath.AddBezier(
        cx + 120*scale, cy + 122*scale,
        cx + 100*scale, cy + 130*scale,
        cx + 86*scale, cy + 115*scale,
        cx + 82*scale, cy + 88*scale
    );
    // Right inner grip to crotch arch
    bodyPath.AddBezier(
        cx + 82*scale, cy + 88*scale,
        cx + 72*scale, cy + 45*scale,
        cx + 50*scale, cy + 28*scale,
        cx + 35*scale, cy + 28*scale
    );
    // Bottom center headset arch
    bodyPath.AddBezier(
        cx + 35*scale, cy + 28*scale,
        cx, cy + 36*scale,
        cx, cy + 36*scale,
        cx - 35*scale, cy + 28*scale
    );
    // Left inner grip
    bodyPath.AddBezier(
        cx - 35*scale, cy + 28*scale,
        cx - 50*scale, cy + 28*scale,
        cx - 72*scale, cy + 45*scale,
        cx - 82*scale, cy + 88*scale
    );
    // Left grip bottom rounded tip
    bodyPath.AddBezier(
        cx - 82*scale, cy + 88*scale,
        cx - 86*scale, cy + 115*scale,
        cx - 100*scale, cy + 130*scale,
        cx - 120*scale, cy + 122*scale
    );
    // Left outer grip handle
    bodyPath.AddBezier(
        cx - 120*scale, cy + 122*scale,
        cx - 148*scale, cy + 105*scale,
        cx - 165*scale, cy + 50*scale,
        cx - 158*scale, cy - 10*scale
    );
    // Left shoulder back to crown
    bodyPath.AddBezier(
        cx - 158*scale, cy - 10*scale,
        cx - 150*scale, cy - 60*scale,
        cx - 120*scale, cy - 96*scale,
        cx - 60*scale, cy - 88*scale
    );
    bodyPath.CloseFigure();

    // Drop Shadow
    SolidBrush shadowBrush(Color(70, 0, 0, 0));
    Matrix transMat;
    transMat.Translate(0, 10 * scale);
    GraphicsPath* shadowPath = bodyPath.Clone();
    shadowPath->Transform(&transMat);
    g.FillPath(&shadowBrush, shadowPath);
    delete shadowPath;

    // Ambient Neon Aura
    Pen auraPen(glow, 6.0f * scale);
    g.DrawPath(&auraPen, &bodyPath);

    // Chassis 3D Gradient Fill
    RectF bodyBounds;
    bodyPath.GetBounds(&bodyBounds);
    LinearGradientBrush chassisBrush(
        bodyBounds,
        Color(255, 32, 38, 54),
        Color(255, 14, 17, 26),
        LinearGradientModeVertical
    );
    g.FillPath(&chassisBrush, &bodyPath);

    // Ergonomic Grip Side Panels (Dual-tone texture wings)
    auto drawGripPanel = [&](bool isRight) {
        float dir = isRight ? 1.0f : -1.0f;
        GraphicsPath gp;
        gp.AddBezier(
            cx + dir*145*scale, cy - 5*scale,
            cx + dir*156*scale, cy + 45*scale,
            cx + dir*140*scale, cy + 95*scale,
            cx + dir*115*scale, cy + 110*scale
        );
        gp.AddBezier(
            cx + dir*115*scale, cy + 110*scale,
            cx + dir*100*scale, cy + 85*scale,
            cx + dir*115*scale, cy + 30*scale,
            cx + dir*130*scale, cy - 5*scale
        );
        gp.CloseFigure();
        SolidBrush gripB(Color(255, 20, 24, 34));
        g.FillPath(&gripB, &gp);
        Pen gripP(Color(255, 45, 54, 76), 1.0f);
        g.DrawPath(&gripP, &gp);
    };
    drawGripPanel(false);
    drawGripPanel(true);

    // Outer Chassis Rim
    Pen rimPen(Color(180, accent.GetR(), accent.GetG(), accent.GetB()), 1.8f * scale);
    g.DrawPath(&rimPen, &bodyPath);

    // ---------------------------------------------------------
    // 3. Central Xbox Guide Jewel & Quadrant LEDs
    // ---------------------------------------------------------
    float orbX = cx;
    float orbY = cy - 35 * scale;
    float orbR = 18.0f * scale;

    // Metallic outer ring
    SolidBrush chromeRing(Color(255, 45, 54, 75));
    g.FillEllipse(&chromeRing, orbX - orbR, orbY - orbR, orbR * 2, orbR * 2);

    // Dark jewel center
    float innerR = orbR - 3.0f * scale;
    SolidBrush jewelCenter(Color(255, 12, 14, 20));
    g.FillEllipse(&jewelCenter, orbX - innerR, orbY - innerR, innerR * 2, innerR * 2);

    // 4-Quadrant Ring (Xbox 360 Style)
    // Quadrant 0: Top-Left (P1), Quadrant 1: Top-Right (P2)
    auto drawQuadrantLED = [&](int quad, bool lit) {
        float startAngle = 185.0f;
        if (quad == 1) startAngle = 275.0f;
        if (quad == 2) startAngle = 5.0f;
        if (quad == 3) startAngle = 95.0f;

        Pen ledPen(lit ? accent : Color(255, 30, 36, 48), 2.5f * scale);
        g.DrawArc(&ledPen, orbX - innerR + 1, orbY - innerR + 1, (innerR - 1) * 2, (innerR - 1) * 2, startAngle, 75.0f);
    };
    drawQuadrantLED(0, player == 0); // P1 = Top-Left
    drawQuadrantLED(1, player == 1); // P2 = Top-Right
    drawQuadrantLED(2, false);
    drawQuadrantLED(3, false);

    // Xbox Cross Insignia inside Guide Jewel
    Pen xPen(Color(255, 200, 215, 235), 1.8f * scale);
    g.DrawLine(&xPen, orbX - 6*scale, orbY - 6*scale, orbX + 6*scale, orbY + 6*scale);
    g.DrawLine(&xPen, orbX + 6*scale, orbY - 6*scale, orbX - 6*scale, orbY + 6*scale);

    // ---------------------------------------------------------
    // 4. Back (<) & Start (>) Buttons
    // ---------------------------------------------------------
    auto drawMenuBtn = [&](float mx, float my, WORD mask, const wchar_t* symbol, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(mx - 15*scale, my - 9*scale, 30*scale, 18*scale);
        
        SolidBrush b(pressed ? accent : Color(255, 25, 30, 42));
        fillRoundedRect(g, b, r, 5.0f * scale);
        Pen p(pressed ? Color(255, 255, 255, 255) : Color(255, 48, 58, 80), 1.0f);
        drawRoundedRect(g, p, r, 5.0f * scale);

        Font mFont(L"Segoe UI", (int)(8 * scale), FontStyleBold, UnitPixel);
        SolidBrush txt(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_SECONDARY);
        g.DrawString(symbol, -1, &mFont, r, &sfCenter, &txt);

        // Key tag below
        std::string kn = getKeyName(key);
        std::wstring knt = L"[" + std::wstring(kn.begin(), kn.end()) + L"]";
        RectF tagR(mx - 25*scale, my + 10*scale, 50*scale, 12*scale);
        SolidBrush tagTxt(Theme::TEXT_MUTED);
        g.DrawString(knt.c_str(), -1, &mFont, tagR, &sfCenter, &tagTxt);
    };

    drawMenuBtn(cx - 42 * scale, cy - 35 * scale, XUSB_GAMEPAD_BACK, L"<", getKeyForButton(player, XUSB_GAMEPAD_BACK));
    drawMenuBtn(cx + 42 * scale, cy - 35 * scale, XUSB_GAMEPAD_START, L">", getKeyForButton(player, XUSB_GAMEPAD_START));

    // ---------------------------------------------------------
    // 5. Authentic Cross D-Pad (Lower Left)
    // ---------------------------------------------------------
    float dpadX = cx - 48 * scale;
    float dpadY = cy + 24 * scale;
    float dpadDishR = 36.0f * scale;

    // Recessed circular dish
    SolidBrush dishBrush(Color(255, 16, 20, 28));
    g.FillEllipse(&dishBrush, dpadX - dpadDishR, dpadY - dpadDishR, dpadDishR*2, dpadDishR*2);
    Pen dishPen(Color(255, 35, 42, 58), 1.5f);
    g.DrawEllipse(&dishPen, dpadX - dpadDishR, dpadY - dpadDishR, dpadDishR*2, dpadDishR*2);

    float armLen = 28.0f * scale;
    float armW   = 22.0f * scale;

    auto drawDpadArm = [&](float ax, float ay, float aw, float ah, WORD mask, const wchar_t* arr, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(ax, ay, aw, ah);

        if (pressed) {
            SolidBrush glowB(glow);
            g.FillRectangle(&glowB, ax - 2, ay - 2, aw + 4, ah + 4);
            SolidBrush b(accent);
            g.FillRectangle(&b, r);
            Pen p(Color(255, 255, 255, 255), 1.2f);
            g.DrawRectangle(&p, ax, ay, aw, ah);
        } else {
            SolidBrush b(Color(255, 32, 38, 52));
            g.FillRectangle(&b, r);
            Pen p(Color(255, 48, 58, 78), 1.0f);
            g.DrawRectangle(&p, ax, ay, aw, ah);
        }

        Font arrFont(L"Segoe UI", (int)(9 * scale), FontStyleBold, UnitPixel);
        SolidBrush txt(pressed ? Color(255, 10, 12, 16) : Theme::TEXT_PRIMARY);
        g.DrawString(arr, -1, &arrFont, r, &sfCenter, &txt);
    };

    drawDpadArm(dpadX - armW*0.5f, dpadY - armLen, armW, armLen - armW*0.5f, XUSB_GAMEPAD_DPAD_UP, L"^", getKeyForButton(player, XUSB_GAMEPAD_DPAD_UP));
    drawDpadArm(dpadX - armW*0.5f, dpadY + armW*0.5f, armW, armLen - armW*0.5f, XUSB_GAMEPAD_DPAD_DOWN, L"v", getKeyForButton(player, XUSB_GAMEPAD_DPAD_DOWN));
    drawDpadArm(dpadX - armLen, dpadY - armW*0.5f, armLen - armW*0.5f, armW, XUSB_GAMEPAD_DPAD_LEFT, L"<", getKeyForButton(player, XUSB_GAMEPAD_DPAD_LEFT));
    drawDpadArm(dpadX + armW*0.5f, dpadY - armW*0.5f, armLen - armW*0.5f, armW, XUSB_GAMEPAD_DPAD_RIGHT, L">", getKeyForButton(player, XUSB_GAMEPAD_DPAD_RIGHT));

    // Center D-Pad hub
    SolidBrush dpadHub(Color(255, 26, 32, 44));
    g.FillRectangle(&dpadHub, dpadX - armW*0.5f, dpadY - armW*0.5f, armW, armW);

    // ---------------------------------------------------------
    // 6. Authentic 3D Glass ABXY Buttons (Upper Right)
    // ---------------------------------------------------------
    float faceX = cx + 76 * scale;
    float faceY = cy - 22 * scale;
    float faceSpread = 28.0f * scale;
    float btnR = 14.0f * scale;

    auto drawABXY = [&](float bx, float by, WORD mask, const wchar_t* letter, Color baseCol, Color glowCol, int key) {
        bool pressed = (report.wButtons & mask) != 0;
        float dia = btnR * 2.0f;
        RectF r(bx - btnR, by - btnR, dia, dia);

        if (pressed) {
            // Radiant bloom glow
            SolidBrush glowB(glowCol);
            g.FillEllipse(&glowB, bx - btnR - 4, by - btnR - 4, dia + 8, dia + 8);
            SolidBrush b(baseCol);
            g.FillEllipse(&b, r);
            Pen p(Color(255, 255, 255, 255), 1.5f);
            g.DrawEllipse(&p, r);
            SolidBrush txt(Color(255, 10, 12, 16));
            Font letterFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        } else {
            // Glass 3D dome
            SolidBrush b(Color(255, 24, 28, 40));
            g.FillEllipse(&b, r);
            
            // Outer colored ring
            Pen p(baseCol, 1.8f * scale);
            g.DrawEllipse(&p, r);

            // Specular crescent highlight on top edge
            Pen specPen(Color(160, 255, 255, 255), 1.0f);
            g.DrawArc(&specPen, bx - btnR + 2, by - btnR + 2, dia - 4, dia - 4, 200, 140);

            SolidBrush txt(baseCol);
            Font letterFont(L"Segoe UI", (int)(11 * scale), FontStyleBold, UnitPixel);
            g.DrawString(letter, -1, &letterFont, r, &sfCenter, &txt);
        }

        // Cyberpunk Key tag
        std::string kn = getKeyName(key);
        std::wstring knt = L"[" + std::wstring(kn.begin(), kn.end()) + L"]";
        RectF tagR(bx - 26*scale, by + btnR + 1, 52*scale, 12*scale);
        Font tagFont(L"Segoe UI", (int)(8 * scale), FontStyleBold, UnitPixel);
        SolidBrush tagTxt(Theme::TEXT_MUTED);
        g.DrawString(knt.c_str(), -1, &tagFont, tagR, &sfCenter, &tagTxt);
    };

    // Diamond layout: Y (Top), A (Bottom), X (Left), B (Right)
    drawABXY(faceX, faceY - faceSpread, XUSB_GAMEPAD_Y, L"Y", Theme::XBOX_Y, Theme::XBOX_Y_GLOW, getKeyForButton(player, XUSB_GAMEPAD_Y));
    drawABXY(faceX, faceY + faceSpread, XUSB_GAMEPAD_A, L"A", Theme::XBOX_A, Theme::XBOX_A_GLOW, getKeyForButton(player, XUSB_GAMEPAD_A));
    drawABXY(faceX - faceSpread, faceY, XUSB_GAMEPAD_X, L"X", Theme::XBOX_X, Theme::XBOX_X_GLOW, getKeyForButton(player, XUSB_GAMEPAD_X));
    drawABXY(faceX + faceSpread, faceY, XUSB_GAMEPAD_B, L"B", Theme::XBOX_B, Theme::XBOX_B_GLOW, getKeyForButton(player, XUSB_GAMEPAD_B));

    // ---------------------------------------------------------
    // 7. Left & Right Analog Thumbsticks (Classic Xbox Layout)
    // ---------------------------------------------------------
    // Left stick at Upper-Left, Right stick at Lower-Right
    auto drawStick = [&](float sx, float sy, const wchar_t* lbl) {
        float stickWellR = 24.0f * scale;
        float stickCapR  = 17.0f * scale;

        // Recessed well
        SolidBrush wellB(Color(255, 14, 17, 24));
        g.FillEllipse(&wellB, sx - stickWellR, sy - stickWellR, stickWellR*2, stickWellR*2);
        Pen wellP(Color(255, 32, 40, 56), 1.2f);
        g.DrawEllipse(&wellP, sx - stickWellR, sy - stickWellR, stickWellR*2, stickWellR*2);

        // Raised 3D Stick Cap
        SolidBrush capB(Color(255, 26, 32, 46));
        g.FillEllipse(&capB, sx - stickCapR, sy - stickCapR, stickCapR*2, stickCapR*2);
        Pen capP(Color(255, 50, 62, 85), 1.5f);
        g.DrawEllipse(&capP, sx - stickCapR, sy - stickCapR, stickCapR*2, stickCapR*2);

        // Inner thumb concave dish
        float innerConcave = 10.0f * scale;
        SolidBrush inB(Color(255, 18, 22, 32));
        g.FillEllipse(&inB, sx - innerConcave, sy - innerConcave, innerConcave*2, innerConcave*2);

        // 4 tactile grip dots at 12, 3, 6, 9
        SolidBrush dotB(accent);
        float dotD = 5.5f * scale;
        g.FillEllipse(&dotB, sx - 1.5f, sy - innerConcave - 1, 3.0f, 3.0f);
        g.FillEllipse(&dotB, sx - 1.5f, sy + innerConcave - 2, 3.0f, 3.0f);
        g.FillEllipse(&dotB, sx - innerConcave - 1, sy - 1.5f, 3.0f, 3.0f);
        g.FillEllipse(&dotB, sx + innerConcave - 2, sy - 1.5f, 3.0f, 3.0f);

        Font sFont(L"Segoe UI", (int)(8 * scale), FontStyleBold, UnitPixel);
        RectF r(sx - 10, sy - 6, 20, 12);
        SolidBrush sTxt(Theme::TEXT_MUTED);
        g.DrawString(lbl, -1, &sFont, r, &sfCenter, &sTxt);
    };

    drawStick(cx - 78 * scale, cy - 22 * scale, L"LS");
    drawStick(cx + 46 * scale, cy + 24 * scale, L"RS");
}

// -------------------------------------------------------------
// Tab 0: Live Gamepad HUD
// -------------------------------------------------------------
static void drawDashboard(Graphics& g, int width, int height, const AppState& state) {
    float midY = 115.0f + (height - 180.0f) * 0.46f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;

    drawRealisticGamepad(g, p1CenterX, midY, 1.05f, 0, state.liveReports[0], state);
    drawRealisticGamepad(g, p2CenterX, midY, 1.05f, 1, state.liveReports[1], state);

    // Bottom Pro Status & Hotkeys Strip
    RectF bottomBar(20, height - 52.0f, width - 40.0f, 36.0f);
    SolidBrush barBg(Theme::PANEL_BG);
    fillRoundedRect(g, barBg, bottomBar, 8.0f);
    Pen barPen(Theme::CARD_BORDER, 1.0f);
    drawRoundedRect(g, barPen, bottomBar, 8.0f);

    Font hintFont(L"Segoe UI", 10, FontStyleRegular, UnitPixel);
    SolidBrush hintBrush(Theme::TEXT_MUTED);
    g.DrawString(L"Hotkeys:  F9: Toggle Controllers ON/OFF   |   F10: Toggle In-Game Overlay   |   Engine Rate: 1000 Hz (<1ms latency)", -1, &hintFont, PointF(35, height - 42.0f), &hintBrush);
}

// -------------------------------------------------------------
// Tab 1: Arcade Key Mapper
// -------------------------------------------------------------
static void drawKeybinds(Graphics& g, int width, int height, const AppState& state) {
    float startY = 104.0f;

    // Duplicate Key Conflict Warning Banner
    std::vector<int> duplicates = getDuplicateKeys();
    if (!duplicates.empty()) {
        RectF alertRect(20, startY, width - 40.0f, 34.0f);
        SolidBrush alertBg(Color(45, 255, 46, 68));
        Pen alertPen(Theme::STATUS_RED, 1.2f);
        fillRoundedRect(g, alertBg, alertRect, 6.0f);
        drawRoundedRect(g, alertPen, alertRect, 6.0f);

        std::wstring warnText = L"⚠️ DUPLICATE KEY CONFLICT: ";
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
    Font rowFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font keyBoxFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    float colW = (width - 60.0f) * 0.5f;

    for (int p = 0; p < 2; ++p) {
        float colX = 20.0f + p * (colW + 20.0f);
        Color accent = (p == 0) ? Theme::P1_NEON : Theme::P2_NEON;

        // Metallic Command Box
        RectF colBox(colX, startY, colW, height - startY - 60.0f);
        SolidBrush colBg(Theme::PANEL_BG);
        fillRoundedRect(g, colBg, colBox, 8.0f);
        Pen colBorder(accent, 1.2f);
        drawRoundedRect(g, colBorder, colBox, 8.0f);

        // Header
        RectF headRect(colX, startY, colW, 30.0f);
        SolidBrush headBg(Color(35, accent.GetR(), accent.GetG(), accent.GetB()));
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
                Pen boxPen(Color(255, 48, 58, 80), 1.0f);
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
    RectF saveBtn(20, bottomY, 160, 32);
    SolidBrush saveBg(Color(255, 0, 160, 80));
    Pen savePen(Theme::STATUS_GREEN, 1.2f);
    fillRoundedRect(g, saveBg, saveBtn, 6.0f);
    drawRoundedRect(g, savePen, saveBtn, 6.0f);
    SolidBrush whiteTxt(Color(255, 255, 255, 255));
    g.DrawString(L"Save Config", -1, &btnFont, saveBtn, &sfCenter, &whiteTxt);

    // Reset Defaults Button
    RectF resetBtn(195, bottomY, 180, 32);
    SolidBrush resetBg(Color(255, 34, 40, 56));
    Pen resetPen(Color(255, 55, 65, 88), 1.0f);
    fillRoundedRect(g, resetBg, resetBtn, 6.0f);
    drawRoundedRect(g, resetPen, resetBtn, 6.0f);
    SolidBrush resetTxt(Theme::TEXT_MUTED);
    g.DrawString(L"Reset Fightstick Defaults", -1, &btnFont, resetBtn, &sfCenter, &resetTxt);

    Font tipFont(L"Segoe UI", 10, FontStyleRegular, UnitPixel);
    SolidBrush tipBrush(Theme::TEXT_DIM);
    g.DrawString(L"Click any key box to rebind. Press ESC while rebinding to cancel.", -1, &tipFont, PointF(390, bottomY + 8), &tipBrush);
}

// -------------------------------------------------------------
// Tab 2: Settings & Overlay Options
// -------------------------------------------------------------
static void drawSettings(Graphics& g, int width, int height, const AppState& state) {
    float startY = 108.0f;
    Font titleFont(L"Segoe UI", 13, FontStyleBold, UnitPixel);
    Font bodyFont(L"Segoe UI", 11, FontStyleRegular, UnitPixel);
    Font btnFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Section 1: In-Game Overlay Presets
    RectF s1(20, startY, width - 40.0f, 115.0f);
    SolidBrush s1Bg(Theme::PANEL_BG);
    fillRoundedRect(g, s1Bg, s1, 8.0f);
    Pen s1Pen(Theme::CARD_BORDER, 1.0f);
    drawRoundedRect(g, s1Pen, s1, 8.0f);

    SolidBrush cyanBrush(Theme::P1_NEON);
    g.DrawString(L"In-Game Overlay Configuration (F10 Mode)", -1, &titleFont, PointF(32, startY + 12), &cyanBrush);

    SolidBrush whiteTxt(Theme::TEXT_PRIMARY);
    SolidBrush mutedTxt(Theme::TEXT_MUTED);
    g.DrawString(L"HUD Opacity Preset:", -1, &bodyFont, PointF(32, startY + 44), &whiteTxt);

    int opacities[] = { 150, 190, 220, 255 };
    const wchar_t* opLabels[] = { L"60%", L"75%", L"88%", L"100%" };
    for (int i = 0; i < 4; ++i) {
        RectF opBtn(165 + i * 85, startY + 40, 75, 26);
        bool isSel = (state.overlayOpacity == opacities[i]);
        SolidBrush obBg(isSel ? Color(255, 0, 160, 180) : Color(255, 30, 36, 50));
        Pen obPen(isSel ? Theme::P1_NEON : Color(255, 48, 58, 80), 1.0f);
        fillRoundedRect(g, obBg, opBtn, 4.0f);
        drawRoundedRect(g, obPen, opBtn, 4.0f);
        SolidBrush obTxt(isSel ? Color(255, 255, 255, 255) : Theme::TEXT_MUTED);
        g.DrawString(opLabels[i], -1, &btnFont, opBtn, &sfCenter, &obTxt);
    }

    g.DrawString(L"When In-Game Overlay is toggled (F10), KEYTECH floats seamlessly over your fighting game window.", -1, &bodyFont, PointF(32, startY + 80), &mutedTxt);

    // Section 2: Fighting Game Setup Guide
    startY += 130.0f;
    RectF s2(20, startY, width - 40.0f, 140.0f);
    fillRoundedRect(g, s1Bg, s2, 8.0f);
    drawRoundedRect(g, s1Pen, s2, 8.0f);

    g.DrawString(L"Pro Tournament Guide (KOF XV, SF6, SFV, Tekken 8, Guilty Gear)", -1, &titleFont, PointF(32, startY + 12), &cyanBrush);
    g.DrawString(L"1. Ensure ViGEmBus driver is installed (Status badge at top must say ONLINE).", -1, &bodyFont, PointF(32, startY + 40), &whiteTxt);
    g.DrawString(L"2. Launch KEYTECH first, then launch your game so it detects Controller 1 & 2 at startup.", -1, &bodyFont, PointF(32, startY + 64), &whiteTxt);
    g.DrawString(L"3. In game settings, set display mode to 'Borderless Windowed' for optimal in-game overlay display.", -1, &bodyFont, PointF(32, startY + 88), &whiteTxt);
    g.DrawString(L"4. Press F9 at any time to temporarily release keyboard focus for typing in chat.", -1, &bodyFont, PointF(32, startY + 112), &whiteTxt);

    // Section 3: Clean Exit
    startY += 155.0f;
    RectF exitBox(20, startY, 210, 34);
    SolidBrush exitBg(Color(255, 160, 24, 36));
    Pen exitPen(Theme::STATUS_RED, 1.2f);
    fillRoundedRect(g, exitBg, exitBox, 6.0f);
    drawRoundedRect(g, exitPen, exitBox, 6.0f);
    g.DrawString(L"Quit KEYTECH Application", -1, &btnFont, exitBox, &sfCenter, &whiteTxt);
}

// -------------------------------------------------------------
// In-Game Floating Transparent HUD (F10 Mode)
// -------------------------------------------------------------
static void drawOverlayHUD(Graphics& g, int width, int height, const AppState& state) {
    Font titleFont(L"Segoe UI", 10, FontStyleBold, UnitPixel);
    Font hintFont(L"Segoe UI", 9, FontStyleRegular, UnitPixel);
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    // Title / Drag handle
    SolidBrush cyanTxt(Theme::P1_NEON);
    g.DrawString(L"KEYTECH OVERLAY // [DRAG ANYWHERE TO MOVE] // [F10: RESTORE WINDOW]", -1, &titleFont, PointF(18, 8), &cyanTxt);

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
    SolidBrush wTxt(Color(255, 255, 255, 255));
    g.DrawString(L"[]", -1, &hintFont, restoreBtn, &sfCenter, &wTxt);

    // Close button [X]
    RectF closeBtn(width - 38, 6, 26, 20);
    SolidBrush cBg(Color(255, 55, 18, 24));
    fillRoundedRect(g, cBg, closeBtn, 4.0f);
    SolidBrush redTxt(Theme::STATUS_RED);
    g.DrawString(L"X", -1, &hintFont, closeBtn, &sfCenter, &redTxt);

    // Render realistic gamepads inside overlay
    float midY = (height + 26.0f) * 0.5f;
    float p1CenterX = width * 0.27f;
    float p2CenterX = width * 0.73f;
    float scale = 0.88f;

    drawRealisticGamepad(g, p1CenterX, midY, scale, 0, state.liveReports[0], state);
    drawRealisticGamepad(g, p2CenterX, midY, scale, 1, state.liveReports[1], state);
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
    SolidBrush bgBrush(Color(245, 0, 160, 80));
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
    if (x >= width - 95 && x <= width - 20 && y >= 15 && y <= 43) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (x >= width - 250 && x <= width - 110 && y >= 15 && y <= 43) {
        PostMessage(hwnd, WM_COMMAND, 1002, 0);
        return;
    }

    if (x >= 235 && x <= 390 && y >= 15 && y <= 43) {
        PostMessage(hwnd, WM_COMMAND, 1001, 0);
        return;
    }

    // Tabs
    if (y >= 60 && y <= 94) {
        float curX = 20.0f;
        for (int i = 0; i < 3; ++i) {
            float tabW = 200.0f;
            if (x >= curX && x <= curX + tabW) {
                state.currentTab = (AppTab)i;
                state.rebindingPlayer = -1;
                state.rebindingButton = 0;
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
            curX += tabW + 10.0f;
        }
    }

    // Key Mapping Tab Actions
    if (state.currentTab == TAB_KEYBINDS) {
        float startY = 104.0f;
        if (hasDuplicates()) startY += 44.0f;

        float colW = (width - 60.0f) * 0.5f;

        for (int p = 0; p < 2; ++p) {
            float colX = 20.0f + p * (colW + 20.0f);
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
        if (x >= 20 && x <= 180 && y >= bottomY && y <= bottomY + 32) {
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
        if (x >= 20 && x <= 230 && y >= quitY && y <= quitY + 34) {
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
    if (state.isDragging) {
        state.isDragging = false;
        ReleaseCapture();
    }
}
