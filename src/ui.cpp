#include "ui.h"
#include <gdiplus.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

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
// Design tokens
// -------------------------------------------------------------
namespace C {
    const Color Window      (255,  15,  17,  20);  // app canvas
    const Color Sidebar     (255,  12,  14,  17);
    const Color Surface     (255,  23,  26,  31);  // cards
    const Color SurfaceAlt  (255,  29,  33,  39);  // raised controls
    const Color Hover       (255,  35,  39,  46);
    const Color Border      (255,  41,  46,  54);
    const Color BorderStrong(255,  55,  61,  71);
    const Color Keycap      (255,  26,  30,  36);
    const Color Well        (255,  13,  15,  18);

    const Color Text        (255, 232, 235, 239);
    const Color TextSecond  (255, 154, 161, 170);
    const Color TextMuted   (255, 108, 115, 124);
    const Color TextFaint   (255,  78,  84,  93);

    const Color Accent      (255,  80, 135, 245);
    const Color P1          (255,  96, 150, 255);
    const Color P2          (255, 238, 166,  96);
    const Color Green       (255,  74, 186, 122);
    const Color Amber       (255, 222, 168,  76);
    const Color Red         (255, 232, 108, 108);

    const Color XboxA       (255, 110, 191,  96);
    const Color XboxB       (255, 224, 112, 100);
    const Color XboxX       (255,  96, 158, 232);
    const Color XboxY       (255, 226, 190,  92);
}

static const float SIDEBAR_W = 208.0f;
static const float PAD       = 26.0f;
static const float ROW_H     = 30.0f;
static const float BODY_TOP  = 96.0f;
static const float CARD_RAD  = 12.0f;

// -------------------------------------------------------------
// Color / geometry / text helpers
// -------------------------------------------------------------
static Color alpha(const Color& c, int a) {
    if (a < 0) a = 0; if (a > 255) a = 255;
    return Color((BYTE)a, c.GetR(), c.GetG(), c.GetB());
}

static Color mix(const Color& a, const Color& b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    return Color(255,
        (BYTE)(a.GetR() + (b.GetR() - a.GetR()) * t),
        (BYTE)(a.GetG() + (b.GetG() - a.GetG()) * t),
        (BYTE)(a.GetB() + (b.GetB() - a.GetB()) * t));
}

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

static void fillRound(Graphics& g, const Color& color, const RectF& rect, float radius) {
    GraphicsPath path;
    addRoundedRect(path, rect, radius);
    SolidBrush brush(color);
    g.FillPath(&brush, &path);
}

static void strokeRound(Graphics& g, const Color& color, const RectF& rect, float radius, float width = 1.0f) {
    GraphicsPath path;
    addRoundedRect(path, rect, radius);
    Pen pen(color, width);
    g.DrawPath(&pen, &path);
}

static std::wstring widen(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static void text(Graphics& g, const std::wstring& s, Font& font, const RectF& r, const Color& color,
                 StringAlignment h = StringAlignmentNear, StringAlignment v = StringAlignmentCenter) {
    StringFormat fmt;
    fmt.SetAlignment(h);
    fmt.SetLineAlignment(v);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    SolidBrush brush(color);
    g.DrawString(s.c_str(), (INT)s.size(), &font, r, &fmt, &brush);
}

static void text(Graphics& g, const wchar_t* s, Font& font, const RectF& r, const Color& color,
                 StringAlignment h = StringAlignmentNear, StringAlignment v = StringAlignmentCenter) {
    text(g, std::wstring(s), font, r, color, h, v);
}

static bool over(const AppState& state, const RectF& r) {
    return r.Contains((REAL)state.mouseX, (REAL)state.mouseY) != 0;
}

// -------------------------------------------------------------
// Vector icons (no font glyphs, no ASCII)
// -------------------------------------------------------------
static void iconChevron(Graphics& g, float cx, float cy, float size, int dir, const Color& color, float w) {
    float h = size * 0.5f;
    PointF pts[3];
    switch (dir) {
        case 0: pts[0] = PointF(cx - h, cy + h * 0.5f); pts[1] = PointF(cx, cy - h * 0.5f); pts[2] = PointF(cx + h, cy + h * 0.5f); break;
        case 1: pts[0] = PointF(cx - h * 0.5f, cy - h); pts[1] = PointF(cx + h * 0.5f, cy); pts[2] = PointF(cx - h * 0.5f, cy + h); break;
        case 2: pts[0] = PointF(cx - h, cy - h * 0.5f); pts[1] = PointF(cx, cy + h * 0.5f); pts[2] = PointF(cx + h, cy - h * 0.5f); break;
        default: pts[0] = PointF(cx + h * 0.5f, cy - h); pts[1] = PointF(cx - h * 0.5f, cy); pts[2] = PointF(cx + h * 0.5f, cy + h); break;
    }
    Pen pen(color, w);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    g.DrawLines(&pen, pts, 3);
}

static void iconClose(Graphics& g, float cx, float cy, float s, const Color& color, float w) {
    Pen pen(color, w);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    g.DrawLine(&pen, cx - s, cy - s, cx + s, cy + s);
    g.DrawLine(&pen, cx + s, cy - s, cx - s, cy + s);
}

static void iconGamepad(Graphics& g, float x, float y, const Color& c) {
    Pen pen(c, 1.4f);
    GraphicsPath path;
    addRoundedRect(path, RectF(x + 0.8f, y + 4.2f, 14.4f, 8.4f), 4.2f);
    g.DrawPath(&pen, &path);

    Pen dp(c, 1.3f);
    dp.SetStartCap(LineCapRound);
    dp.SetEndCap(LineCapRound);
    g.DrawLine(&dp, x + 4.0f, y + 8.4f, x + 6.8f, y + 8.4f);
    g.DrawLine(&dp, x + 5.4f, y + 7.0f, x + 5.4f, y + 9.8f);

    SolidBrush b(c);
    g.FillEllipse(&b, x + 10.4f, y + 7.0f, 2.0f, 2.0f);
    g.FillEllipse(&b, x + 12.8f, y + 9.2f, 2.0f, 2.0f);
}

static void iconKeyboard(Graphics& g, float x, float y, const Color& c) {
    Pen pen(c, 1.3f);
    GraphicsPath path;
    addRoundedRect(path, RectF(x + 0.7f, y + 3.6f, 14.6f, 8.8f), 2.6f);
    g.DrawPath(&pen, &path);

    SolidBrush b(c);
    for (int i = 0; i < 4; ++i) g.FillRectangle(&b, x + 3.2f + i * 2.9f, y + 6.2f, 1.7f, 1.7f);
    g.FillRectangle(&b, x + 3.2f, y + 9.3f, 7.4f, 1.7f);
    g.FillRectangle(&b, x + 12.0f, y + 9.3f, 1.7f, 1.7f);
}

static void iconSliders(Graphics& g, float x, float y, const Color& c) {
    Pen pen(c, 1.3f);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    g.DrawLine(&pen, x + 2.0f, y + 5.0f, x + 14.0f, y + 5.0f);
    g.DrawLine(&pen, x + 2.0f, y + 11.0f, x + 14.0f, y + 11.0f);

    Pen ring(c, 1.3f);
    SolidBrush hole(C::Sidebar);
    g.FillEllipse(&hole, x + 8.8f, y + 2.4f, 5.2f, 5.2f);
    g.DrawEllipse(&ring, x + 8.8f, y + 2.4f, 5.2f, 5.2f);
    g.FillEllipse(&hole, x + 2.8f, y + 8.4f, 5.2f, 5.2f);
    g.DrawEllipse(&ring, x + 2.8f, y + 8.4f, 5.2f, 5.2f);
}

static void iconCrossMark(Graphics& g, float cx, float cy, float len, float thick, const Color& c) {
    fillRound(g, c, RectF(cx - len * 0.5f, cy - thick * 0.5f, len, thick), thick * 0.5f);
    fillRound(g, c, RectF(cx - thick * 0.5f, cy - len * 0.5f, thick, len), thick * 0.5f);
}

static void iconCheck(Graphics& g, float cx, float cy, float s, const Color& c, float w) {
    Pen pen(c, w);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    PointF pts[3] = { PointF(cx - s, cy), PointF(cx - s * 0.25f, cy + s * 0.7f), PointF(cx + s, cy - s * 0.6f) };
    g.DrawLines(&pen, pts, 3);
}

// -------------------------------------------------------------
// Small UI primitives
// -------------------------------------------------------------
static void drawButton(Graphics& g, const RectF& r, const std::wstring& label,
                       bool primary, const Color& accent, bool hovered) {
    Font font(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);
    if (primary) {
        fillRound(g, hovered ? mix(accent, Color(255, 255, 255, 255), 0.12f) : accent, r, 8.0f);
        text(g, label, font, r, Color(255, 11, 15, 22), StringAlignmentCenter);
    } else {
        fillRound(g, hovered ? C::SurfaceAlt : C::Surface, r, 8.0f);
        strokeRound(g, hovered ? C::BorderStrong : C::Border, r, 8.0f);
        text(g, label, font, r, hovered ? C::Text : C::TextSecond, StringAlignmentCenter);
    }
}

static void drawCard(Graphics& g, const RectF& r) {
    fillRound(g, C::Surface, r, CARD_RAD);
    strokeRound(g, C::Border, r, CARD_RAD);
}

static void drawKeycap(Graphics& g, const RectF& r, const std::wstring& label, const Color& fg) {
    fillRound(g, C::Keycap, r, 6.0f);
    strokeRound(g, C::Border, r, 6.0f);
    Font font(L"Segoe UI", 10.5f, FontStyleBold, UnitPixel);
    text(g, label, font, r, fg, StringAlignmentCenter);
}

static void drawLiveChip(Graphics& g, const RectF& r, const std::wstring& label, const Color& dot, const Color& fg) {
    fillRound(g, C::SurfaceAlt, r, 6.0f);
    strokeRound(g, C::Border, r, 6.0f);
    SolidBrush b(dot);
    g.FillEllipse(&b, r.X + 9.0f, r.Y + r.Height * 0.5f - 2.5f, 5.0f, 5.0f);
    Font font(L"Segoe UI", 10.0f, FontStyleRegular, UnitPixel);
    text(g, label, font, RectF(r.X + 20.0f, r.Y, r.Width - 26.0f, r.Height), fg);
}

static void drawSegmented(Graphics& g, const RectF& box, const RectF seg[4], const wchar_t* labels[4],
                          int selected, const AppState& state) {
    fillRound(g, C::Well, box, 9.0f);
    strokeRound(g, C::Border, box, 9.0f);

    Font font(L"Segoe UI", 10.5f, FontStyleBold, UnitPixel);
    for (int i = 0; i < 4; ++i) {
        bool sel = (i == selected);
        bool hov = over(state, seg[i]) && !sel;
        if (sel) {
            fillRound(g, C::SurfaceAlt, seg[i], 7.0f);
            strokeRound(g, C::BorderStrong, seg[i], 7.0f);
        } else if (hov) {
            fillRound(g, alpha(Color(255, 255, 255, 255), 10), seg[i], 7.0f);
        }
        text(g, labels[i], font, seg[i], sel ? C::Text : (hov ? C::Text : C::TextMuted), StringAlignmentCenter);
    }
}

// -------------------------------------------------------------
// Layout (single source of truth for drawing and hit-testing)
// -------------------------------------------------------------
struct AppLayout {
    RectF nav[3];
    RectF overlayBtn;
    RectF ctrlRow;
    RectF driverRow;

    RectF banner;
    bool  hasBanner = false;

    float cx = 0.0f;   // content left
    float cw = 0.0f;   // content width
    float cardTop = BODY_TOP;
    float cardW = 0.0f;
    float cardH = 0.0f;

    RectF saveBtn;
    RectF resetBtn;
    RectF dupBanner;
    bool  hasDup = false;
    std::vector<RectF> keyChip[2];

    RectF segBox;
    RectF seg[4];
    RectF quitBtn;

    RectF overlayRestore;
    RectF overlayClose;
};

static AppLayout computeLayout(int width, int height, const AppState& state) {
    AppLayout L;
    L.cx = SIDEBAR_W + PAD;
    L.cw = (float)width - L.cx - PAD;

    float navY = 88.0f;
    for (int i = 0; i < 3; ++i) {
        L.nav[i] = RectF(14.0f, navY + i * 40.0f, SIDEBAR_W - 28.0f, 34.0f);
    }
    L.ctrlRow = RectF(14.0f, height - 96.0f, SIDEBAR_W - 28.0f, 30.0f);
    L.driverRow = RectF(14.0f, height - 62.0f, SIDEBAR_W - 28.0f, 30.0f);

    L.overlayBtn = RectF((float)width - PAD - 150.0f, 28.0f, 150.0f, 34.0f);

    if (!state.vigemConnected) {
        L.hasBanner = true;
        L.banner = RectF(L.cx, BODY_TOP, L.cw, 48.0f);
    }
    L.cardTop = BODY_TOP + (L.hasBanner ? 60.0f : 0.0f);

    if (state.currentTab == TAB_KEYBINDS) {
        L.hasDup = hasDuplicates();
        if (L.hasDup) {
            L.dupBanner = RectF(L.cx, BODY_TOP, L.cw, 42.0f);
        }
        float top = BODY_TOP + (L.hasDup ? 54.0f : 0.0f);
        L.cardTop = top;
        L.cardW = (L.cw - 16.0f) * 0.5f;
        L.cardH = (float)height - top - 22.0f;

        for (int p = 0; p < 2; ++p) {
            float x = L.cx + p * (L.cardW + 16.0f);
            float rowY = top + 54.0f;
            L.keyChip[p].resize(g_availableButtons.size());
            for (size_t i = 0; i < g_availableButtons.size(); ++i) {
                L.keyChip[p][i] = RectF(x + L.cardW * 0.53f, rowY + 3.0f, L.cardW * 0.43f, ROW_H - 6.0f);
                rowY += ROW_H;
            }
        }

        L.saveBtn = RectF((float)width - PAD - 138.0f, 28.0f, 138.0f, 34.0f);
        L.resetBtn = RectF(L.saveBtn.X - 10.0f - 96.0f, 28.0f, 96.0f, 34.0f);
    } else if (state.currentTab == TAB_SETTINGS) {
        float y = BODY_TOP;
        L.segBox = RectF(L.cx + 132.0f, y + 60.0f, 4.0f * 74.0f + 6.0f, 36.0f);
        for (int i = 0; i < 4; ++i) {
            L.seg[i] = RectF(L.segBox.X + 3.0f + i * 74.0f, L.segBox.Y + 3.0f, 74.0f, 30.0f);
        }
        float card3Y = y + 148.0f + 16.0f + 176.0f + 16.0f;
        L.quitBtn = RectF((float)width - PAD - 20.0f - 92.0f, card3Y + 24.0f, 92.0f, 32.0f);
    }

    L.overlayClose   = RectF((float)width - 40.0f, 11.0f, 28.0f, 28.0f);
    L.overlayRestore = RectF((float)width - 72.0f, 11.0f, 28.0f, 28.0f);
    return L;
}

// -------------------------------------------------------------
// Controller illustration
// -------------------------------------------------------------
static void drawController(Graphics& g, float cx, float cy, float s, int player, const XUSB_REPORT& report) {
    Color accent = (player == 0) ? C::P1 : C::P2;
    auto penW = [&](float w) { return std::max(1.0f, w * s); };

    // ---------------------------------------------------------
    // Triggers and bumpers, floating above the shell
    // ---------------------------------------------------------
    auto triggerCap = [&](float ox) {
        RectF r(cx + ox, cy - 106 * s, 20 * s, 30 * s);
        fillRound(g, Color(255, 26, 30, 36), r, 10 * s);
        strokeRound(g, Color(255, 54, 60, 70), r, 10 * s, penW(1.0f));
    };
    triggerCap(-80 * s);
    triggerCap(60 * s);

    auto bumper = [&](float ox, WORD mask, const wchar_t* label) {
        bool pressed = (report.wButtons & mask) != 0;
        RectF r(cx + ox, cy - 74 * s, 28 * s, 10 * s);
        fillRound(g, pressed ? alpha(accent, 235) : Color(255, 32, 37, 44), r, 5 * s);
        strokeRound(g, pressed ? accent : Color(255, 52, 58, 68), r, 5 * s, penW(1.0f));
        Font f(L"Segoe UI", std::max(6.0f, 7.5f * s), FontStyleBold, UnitPixel);
        text(g, label, f, r, pressed ? Color(255, 12, 15, 20) : C::TextMuted, StringAlignmentCenter);
    };
    bumper(-84 * s, XUSB_GAMEPAD_LEFT_SHOULDER, L"LB");
    bumper(56 * s, XUSB_GAMEPAD_RIGHT_SHOULDER, L"RB");

    // ---------------------------------------------------------
    // Shell
    // ---------------------------------------------------------
    GraphicsPath body;
    body.AddBezier(cx-52*s, cy-64*s, cx-22*s, cy-71*s, cx+22*s, cy-71*s, cx+52*s, cy-64*s);
    body.AddBezier(cx+52*s, cy-64*s, cx+102*s, cy-64*s, cx+122*s, cy-38*s, cx+125*s, cy-8*s);
    body.AddBezier(cx+125*s, cy-8*s, cx+130*s, cy+26*s, cx+122*s, cy+64*s, cx+102*s, cy+80*s);
    body.AddBezier(cx+102*s, cy+80*s, cx+88*s, cy+90*s, cx+72*s, cy+80*s, cx+68*s, cy+58*s);
    body.AddBezier(cx+68*s, cy+58*s, cx+60*s, cy+30*s, cx+40*s, cy+16*s, cx+24*s, cy+16*s);
    body.AddBezier(cx+24*s, cy+16*s, cx, cy+22*s, cx, cy+22*s, cx-24*s, cy+16*s);
    body.AddBezier(cx-24*s, cy+16*s, cx-40*s, cy+16*s, cx-60*s, cy+30*s, cx-68*s, cy+58*s);
    body.AddBezier(cx-68*s, cy+58*s, cx-72*s, cy+80*s, cx-88*s, cy+90*s, cx-102*s, cy+80*s);
    body.AddBezier(cx-102*s, cy+80*s, cx-122*s, cy+64*s, cx-130*s, cy+26*s, cx-125*s, cy-8*s);
    body.AddBezier(cx-125*s, cy-8*s, cx-122*s, cy-38*s, cx-102*s, cy-64*s, cx-52*s, cy-64*s);
    body.CloseFigure();

    GraphicsPath* shadow = body.Clone();
    Matrix offset;
    offset.Translate(0, 6 * s);
    shadow->Transform(&offset);
    SolidBrush shadowBrush(Color(64, 0, 0, 0));
    g.FillPath(&shadowBrush, shadow);
    delete shadow;

    RectF bounds(cx - 140*s, cy - 72*s, 280*s, 170*s);
    LinearGradientBrush chassis(bounds, Color(255, 38, 43, 52), Color(255, 23, 26, 32), LinearGradientModeVertical);
    g.FillPath(&chassis, &body);
    Pen rim(Color(255, 72, 80, 93), penW(1.2f));
    g.DrawPath(&rim, &body);

    // ---------------------------------------------------------
    // Analog sticks (open rings, like the reference)
    // ---------------------------------------------------------
    auto stick = [&](float sx, float sy) {
        float R = 24 * s;

        SolidBrush well(Color(255, 15, 18, 23));
        g.FillEllipse(&well, sx - R, sy - R, R * 2, R * 2);
        Pen rimPen(Color(255, 56, 63, 74), penW(1.2f));
        g.DrawEllipse(&rimPen, sx - R, sy - R, R * 2, R * 2);

        float ir = R * 0.62f;
        SolidBrush cap(Color(255, 34, 39, 47));
        g.FillEllipse(&cap, sx - ir, sy - ir, ir * 2, ir * 2);
        Pen capRim(Color(255, 78, 86, 99), penW(1.0f));
        g.DrawEllipse(&capRim, sx - ir, sy - ir, ir * 2, ir * 2);

        Pen sheen(Color(40, 255, 255, 255), penW(1.1f));
        g.DrawArc(&sheen, sx - ir + 2*s, sy - ir + 2*s, (ir - 2*s) * 2, (ir - 2*s) * 2, 200.0f, 120.0f);
    };
    stick(cx - 64 * s, cy - 24 * s);
    stick(cx + 64 * s, cy + 28 * s);

    // ---------------------------------------------------------
    // D-pad and face buttons as four-lobe clusters
    // ---------------------------------------------------------
    auto rosette = [&](float rx, float ry, float R, WORD up, WORD down, WORD left, WORD right, bool colored) {
        SolidBrush disc(Color(255, 20, 23, 28));
        g.FillEllipse(&disc, rx - R, ry - R, R * 2, R * 2);
        Pen ring(Color(255, 56, 63, 74), penW(1.2f));
        g.DrawEllipse(&ring, rx - R, ry - R, R * 2, R * 2);

        auto arm = [&](float ax, float ay, float aw, float ah, WORD mask, const Color& col) {
            bool pressed = (report.wButtons & mask) != 0;
            RectF r(ax, ay, aw, ah);
            Color fillC = pressed ? col : (colored ? alpha(col, 42) : Color(255, 33, 38, 45));
            Color lineC = pressed ? col : (colored ? alpha(col, 170) : Color(255, 66, 73, 85));
            fillRound(g, fillC, r, 6 * s);
            strokeRound(g, lineC, r, 6 * s, penW(1.2f));
        };

        float armW = R * 0.72f;
        float armL = R * 0.58f;
        float off  = R * 0.26f;

        arm(rx - armW * 0.5f, ry - off - armL, armW, armL, up,    C::XboxY);
        arm(rx - armW * 0.5f, ry + off,        armW, armL, down,  C::XboxA);
        arm(rx - off - armL, ry - armW * 0.5f, armL, armW, left,  C::XboxX);
        arm(rx + off,        ry - armW * 0.5f, armL, armW, right, C::XboxB);
    };

    rosette(cx - 38 * s, cy + 26 * s, 24 * s, XUSB_GAMEPAD_DPAD_UP, XUSB_GAMEPAD_DPAD_DOWN,
            XUSB_GAMEPAD_DPAD_LEFT, XUSB_GAMEPAD_DPAD_RIGHT, false);

    rosette(cx + 40 * s, cy - 22 * s, 24 * s, XUSB_GAMEPAD_Y, XUSB_GAMEPAD_A,
            XUSB_GAMEPAD_X, XUSB_GAMEPAD_B, true);

    // ---------------------------------------------------------
    // View / Menu dots between the clusters
    // ---------------------------------------------------------
    auto dotButton = [&](float dx, WORD mask) {
        bool pressed = (report.wButtons & mask) != 0;
        float r = 4.2f * s;
        SolidBrush fill(pressed ? accent : Color(255, 34, 39, 47));
        g.FillEllipse(&fill, cx + dx - r, cy - 16 * s - r, r * 2, r * 2);
        Pen rimPen(pressed ? accent : Color(255, 66, 73, 85), penW(1.0f));
        g.DrawEllipse(&rimPen, cx + dx - r, cy - 16 * s - r, r * 2, r * 2);
    };
    dotButton(-8 * s, XUSB_GAMEPAD_BACK);
    dotButton(6 * s, XUSB_GAMEPAD_START);
}

// -------------------------------------------------------------
// Sidebar
// -------------------------------------------------------------
static void drawSidebar(Graphics& g, int width, int height, const AppState& state, const AppLayout& L) {
    (void)width;
    SolidBrush sidebar(C::Sidebar);
    g.FillRectangle(&sidebar, 0.0f, 0.0f, SIDEBAR_W, (REAL)height);
    Pen edge(C::Border);
    g.DrawLine(&edge, SIDEBAR_W - 0.5f, 0.0f, SIDEBAR_W - 0.5f, (REAL)height);

    // Brand mark
    RectF mark(16, 18, 32, 32);
    fillRound(g, alpha(C::P1, 30), mark, 9.0f);
    strokeRound(g, alpha(C::P1, 110), mark, 9.0f);
    iconCrossMark(g, 32, 34, 13.0f, 3.0f, C::P1);

    Font brandFont(L"Segoe UI", 14.0f, FontStyleBold, UnitPixel);
    Font captionFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
    text(g, L"Keytech", brandFont, RectF(58, 19, 130, 18), C::Text);
    text(g, L"Gamepad hub", captionFont, RectF(58, 37, 130, 14), C::TextMuted);

    // Navigation
    const wchar_t* labels[3] = { L"Live input", L"Key mapping", L"Settings" };
    Font navFont(L"Segoe UI", 11.5f, FontStyleRegular, UnitPixel);
    Font navFontSel(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);

    for (int i = 0; i < 3; ++i) {
        bool sel = (state.currentTab == (AppTab)i);
        bool hov = over(state, L.nav[i]);
        if (sel) {
            fillRound(g, C::Surface, L.nav[i], 8.0f);
            strokeRound(g, C::Border, L.nav[i], 8.0f);
        } else if (hov) {
            fillRound(g, alpha(Color(255, 255, 255, 255), 12), L.nav[i], 8.0f);
        }

        Color iconColor = sel ? C::P1 : (hov ? C::TextSecond : C::TextMuted);
        float ix = L.nav[i].X + 13.0f;
        float iy = L.nav[i].Y + 9.0f;
        if (i == 0) iconGamepad(g, ix, iy, iconColor);
        else if (i == 1) iconKeyboard(g, ix, iy, iconColor);
        else iconSliders(g, ix, iy, iconColor);

        text(g, labels[i], sel ? navFontSel : navFont,
             RectF(L.nav[i].X + 40.0f, L.nav[i].Y, L.nav[i].Width - 48.0f, L.nav[i].Height),
             sel ? C::Text : (hov ? C::Text : C::TextSecond));
    }

    // Shortcuts block
    float sepY = 222.0f;
    Pen sep(C::Border);
    g.DrawLine(&sep, 16.0f, sepY, SIDEBAR_W - 16.0f, sepY);

    Font sectionFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
    text(g, L"Shortcuts", sectionFont, RectF(18, sepY + 14, 150, 16), C::TextMuted);

    Font hintFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
    auto shortcut = [&](float y, const wchar_t* key, const wchar_t* what) {
        drawKeycap(g, RectF(18, y, 30, 20), key, C::TextSecond);
        text(g, what, hintFont, RectF(58, y, 140, 20), C::TextSecond);
    };
    shortcut(sepY + 36, L"F9", L"Pause input");
    shortcut(sepY + 62, L"F10", L"Toggle overlay");

    // Status rows
    float statusSep = (float)height - 120.0f;
    g.DrawLine(&sep, 16.0f, statusSep, SIDEBAR_W - 16.0f, statusSep);

    Font statusLabel(L"Segoe UI", 11.0f, FontStyleRegular, UnitPixel);
    Font statusValue(L"Segoe UI", 11.0f, FontStyleBold, UnitPixel);

    // Controllers
    bool ctrlHover = over(state, L.ctrlRow);
    if (ctrlHover) fillRound(g, alpha(Color(255, 255, 255, 255), 10), L.ctrlRow, 7.0f);
    Color ctrlCol = state.active ? C::Green : C::Amber;
    SolidBrush dot(ctrlCol);
    g.FillEllipse(&dot, L.ctrlRow.X + 4.0f, L.ctrlRow.Y + 13.0f, 6.0f, 6.0f);
    text(g, L"Controllers", statusLabel, RectF(L.ctrlRow.X + 18.0f, L.ctrlRow.Y, 90, 30), C::TextSecond);
    text(g, state.active ? L"Active" : L"Paused", statusValue,
         RectF(L.ctrlRow.X, L.ctrlRow.Y, L.ctrlRow.Width - 6.0f, 30), ctrlCol, StringAlignmentFar);

    // Driver
    Color drvCol = state.vigemConnected ? C::Green : C::Amber;
    SolidBrush dot2(drvCol);
    g.FillEllipse(&dot2, L.driverRow.X + 4.0f, L.driverRow.Y + 13.0f, 6.0f, 6.0f);
    text(g, L"ViGEmBus", statusLabel, RectF(L.driverRow.X + 18.0f, L.driverRow.Y, 90, 30), C::TextSecond);
    text(g, state.vigemConnected ? L"Online" : L"Install",
         statusValue, RectF(L.driverRow.X, L.driverRow.Y, L.driverRow.Width - 6.0f, 30),
         state.vigemConnected ? drvCol : C::P1, StringAlignmentFar);
}

// -------------------------------------------------------------
// Page header
// -------------------------------------------------------------
static void drawPageHeader(Graphics& g, int width, const AppState& state, const AppLayout& L) {
    const wchar_t* title = L"Live input";
    const wchar_t* subtitle = L"Buttons light up as you play. Your game still receives every keystroke.";
    if (state.currentTab == TAB_KEYBINDS) {
        title = L"Key mapping";
        subtitle = L"Click any binding to change it. Press Esc to cancel.";
    } else if (state.currentTab == TAB_SETTINGS) {
        title = L"Settings";
        subtitle = L"Overlay appearance and application options.";
    }

    Font titleFont(L"Segoe UI", 19.0f, FontStyleBold, UnitPixel);
    Font subFont(L"Segoe UI", 11.5f, FontStyleRegular, UnitPixel);
    text(g, title, titleFont, RectF(L.cx, 26, L.cw - 180, 26), C::Text);
    text(g, subtitle, subFont, RectF(L.cx, 52, L.cw - 180, 18), C::TextMuted);

    if (state.currentTab == TAB_KEYBINDS) {
        drawButton(g, L.resetBtn, L"Reset", false, C::Accent, over(state, L.resetBtn));
        drawButton(g, L.saveBtn, L"Save changes", true, C::Accent, over(state, L.saveBtn));
        return;
    }

    RectF btn = L.overlayBtn;
    bool hov = over(state, btn);
    fillRound(g, hov ? C::SurfaceAlt : C::Surface, btn, 8.0f);
    strokeRound(g, hov ? C::BorderStrong : C::Border, btn, 8.0f);
    iconGamepad(g, btn.X + 13.0f, btn.Y + 9.0f, hov ? C::Text : C::TextSecond);
    Font btnFont(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);
    text(g, L"Overlay", btnFont, RectF(btn.X + 40.0f, btn.Y, 62.0f, btn.Height), hov ? C::Text : C::TextSecond);
    drawKeycap(g, RectF(btn.X + btn.Width - 44.0f, btn.Y + 8.0f, 32.0f, 18.0f), L"F10", C::TextMuted);
}

// -------------------------------------------------------------
// Overview
// -------------------------------------------------------------
static void drawOverview(Graphics& g, int height, const AppState& state, const AppLayout& L) {
    float top = L.cardTop;

    if (L.hasBanner) {
        RectF b = L.banner;
        fillRound(g, alpha(C::Amber, 18), b, 10.0f);
        strokeRound(g, alpha(C::Amber, 90), b, 10.0f);

        Font f(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);
        Font fs(L"Segoe UI", 11.0f, FontStyleRegular, UnitPixel);
        text(g, L"ViGEmBus is not installed", f, RectF(b.X + 18, b.Y + 7, 320, 16), C::Amber);
        text(g, L"Virtual controllers need the driver before they can appear in games.", fs,
             RectF(b.X + 18, b.Y + 25, 420, 16), C::TextSecond);

        RectF btn(b.X + b.Width - 118.0f, b.Y + 11.0f, 100.0f, 26.0f);
        drawButton(g, btn, L"Install", true, C::Amber, over(state, b));
    }

    float cardH = (float)height - top - 52.0f;
    if (cardH < 200.0f) cardH = 200.0f;
    float cardW = (L.cw - 16.0f) * 0.5f;

    for (int p = 0; p < 2; ++p) {
        float x = L.cx + p * (cardW + 16.0f);
        RectF card(x, top, cardW, cardH);
        drawCard(g, card);

        Color accent = (p == 0) ? C::P1 : C::P2;
        SolidBrush dot(accent);
        g.FillEllipse(&dot, x + 20.0f, top + 23.0f, 7.0f, 7.0f);

        Font headFont(L"Segoe UI", 13.0f, FontStyleBold, UnitPixel);
        text(g, p == 0 ? L"Player 1" : L"Player 2", headFont,
             RectF(x + 34.0f, top + 16.0f, cardW - 60.0f, 20.0f), C::Text);

        int pressed = 0;
        for (const auto& b : g_availableButtons) {
            if (state.liveReports[p].wButtons & b.button) ++pressed;
        }
        std::wstring chipText = pressed > 0
            ? std::to_wstring(pressed) + L" pressed"
            : L"Idle";
        RectF chip(x + cardW - 84.0f, top + 15.0f, 66.0f, 22.0f);
        drawLiveChip(g, chip, chipText, pressed ? accent : C::TextFaint,
                     pressed ? C::Text : C::TextMuted);

        // Hairline under the card header
        Pen line(Color(255, 31, 35, 42));
        g.DrawLine(&line, x + 16.0f, top + 48.0f, x + cardW - 16.0f, top + 48.0f);

        // Keyboard hint
        Font hintFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
        text(g, p == 0 ? L"WASD and action keys" : L"Arrow keys and numpad",
             hintFont, RectF(x + 20.0f, top + cardH - 30.0f, cardW - 40.0f, 16.0f), C::TextMuted);

        float scale = std::min((cardW - 34.0f) / 282.0f, (cardH - 120.0f) / 208.0f);
        scale = std::max(0.55f, std::min(scale, 1.25f));
        drawController(g, x + cardW * 0.5f, top + 52.0f + (cardH - 80.0f) * 0.53f, scale, p, state.liveReports[p]);
    }

    Font footFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
    text(g, L"1,000 Hz polling  ·  sub-millisecond latency", footFont,
         RectF(L.cx, (float)height - 34.0f, L.cw * 0.6f, 16.0f), C::TextFaint);
    text(g, L"F9 pauses input  ·  F10 shows the overlay", footFont,
         RectF(L.cx, (float)height - 34.0f, L.cw, 16.0f), C::TextFaint, StringAlignmentFar);
}

// -------------------------------------------------------------
// Key mapping
// -------------------------------------------------------------
static void drawKeybinds(Graphics& g, int height, const AppState& state, const AppLayout& L) {
    (void)height;

    if (L.hasDup) {
        RectF b = L.dupBanner;
        fillRound(g, alpha(C::Amber, 16), b, 10.0f);
        strokeRound(g, alpha(C::Amber, 80), b, 10.0f);

        std::vector<int> dups = getDuplicateKeys();
        std::wstring list;
        for (size_t i = 0; i < dups.size(); ++i) {
            list += widen(getKeyName(dups[i]));
            if (i + 1 < dups.size()) list += L", ";
        }

        std::wstring detail = (dups.size() == 1)
            ? list + L" is bound more than once"
            : L"Bound more than once: " + list;

        Font f(L"Segoe UI", 11.0f, FontStyleBold, UnitPixel);
        Font fs(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
        text(g, L"Duplicate key", f, RectF(b.X + 18, b.Y + 6, 200, 16), C::Amber);
        text(g, detail, fs, RectF(b.X + 18, b.Y + 23, b.Width - 36, 15), C::TextSecond);
    }

    for (int p = 0; p < 2; ++p) {
        float x = L.cx + p * (L.cardW + 16.0f);
        RectF card(x, L.cardTop, L.cardW, L.cardH);
        drawCard(g, card);

        Color accent = (p == 0) ? C::P1 : C::P2;
        SolidBrush dot(accent);
        g.FillEllipse(&dot, x + 20.0f, L.cardTop + 23.0f, 7.0f, 7.0f);

        Font headFont(L"Segoe UI", 13.0f, FontStyleBold, UnitPixel);
        text(g, p == 0 ? L"Player 1" : L"Player 2", headFont,
             RectF(x + 34.0f, L.cardTop + 16.0f, L.cardW - 50.0f, 20.0f), C::Text);

        Font bodyFont(L"Segoe UI", 11.5f, FontStyleRegular, UnitPixel);
        Font chipFont(L"Segoe UI", 10.5f, FontStyleBold, UnitPixel);

        float rowY = L.cardTop + 54.0f;
        for (size_t i = 0; i < g_availableButtons.size(); ++i) {
            const auto& b = g_availableButtons[i];
            bool pressed = (state.liveReports[p].wButtons & b.button) != 0;
            bool rebinding = (state.rebindingPlayer == p && state.rebindingButton == b.button);

            if (pressed) {
                fillRound(g, alpha(accent, 16), RectF(x + 12.0f, rowY, L.cardW - 24.0f, ROW_H - 4.0f), 6.0f);
            }

            SolidBrush rowDot(pressed ? accent : Color(255, 45, 50, 58));
            g.FillEllipse(&rowDot, x + 22.0f, rowY + ROW_H * 0.5f - 2.0f, 4.0f, 4.0f);

            text(g, widen(b.label), bodyFont, RectF(x + 34.0f, rowY, L.cardW * 0.45f, ROW_H - 4.0f),
                 pressed ? C::Text : C::TextSecond);

            RectF chip = L.keyChip[p][i];
            if (rebinding) {
                fillRound(g, alpha(C::Amber, 26), chip, 6.0f);
                strokeRound(g, alpha(C::Amber, 200), chip, 6.0f);
                text(g, L"Press a key", chipFont, chip, C::Amber, StringAlignmentCenter);
            } else {
                int key = getKeyForButton(p, b.button);
                bool bound = key != 0;
                drawKeycap(g, chip, widen(getKeyName(key)), bound ? C::Text : C::TextFaint);
            }

            if (i + 1 < g_availableButtons.size()) {
                Pen sep(Color(255, 30, 34, 41), 1.0f);
                g.DrawLine(&sep, x + 22.0f, rowY + ROW_H - 2.0f, x + L.cardW - 22.0f, rowY + ROW_H - 2.0f);
            }

            rowY += ROW_H;
        }
    }
}

// -------------------------------------------------------------
// Settings
// -------------------------------------------------------------
static void drawSettings(Graphics& g, const AppState& state, const AppLayout& L) {
    float y = BODY_TOP;
    Font titleFont(L"Segoe UI", 13.0f, FontStyleBold, UnitPixel);
    Font bodyFont(L"Segoe UI", 11.5f, FontStyleRegular, UnitPixel);
    Font smallFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);

    // Overlay opacity
    RectF c1(L.cx, y, L.cw, 148.0f);
    drawCard(g, c1);
    text(g, L"In-game overlay", titleFont, RectF(c1.X + 20, c1.Y + 16, c1.Width - 40, 18), C::Text);
    text(g, L"How solid the overlay looks when it floats over your game.", bodyFont,
         RectF(c1.X + 20, c1.Y + 36, c1.Width - 40, 16), C::TextMuted);
    text(g, L"Opacity", bodyFont, RectF(c1.X + 20, c1.Y + 60, 100, 36), C::TextSecond);

    const wchar_t* labels[4] = { L"60%", L"75%", L"88%", L"100%" };
    int opacities[4] = { 150, 190, 220, 255 };
    int selected = 2;
    for (int i = 0; i < 4; ++i) if (state.overlayOpacity == opacities[i]) selected = i;
    drawSegmented(g, L.segBox, L.seg, labels, selected, state);

    text(g, L"Press F10 anywhere to show or hide the overlay.", smallFont,
         RectF(c1.X + 20, c1.Y + 108, c1.Width - 40, 16), C::TextFaint);

    // Getting started
    float y2 = y + 148.0f + 16.0f;
    RectF c2(L.cx, y2, L.cw, 176.0f);
    drawCard(g, c2);
    text(g, L"Getting started", titleFont, RectF(c2.X + 20, c2.Y + 16, c2.Width - 40, 18), C::Text);

    const wchar_t* steps[4] = {
        L"Make sure ViGEmBus shows Online in the sidebar.",
        L"Open Keytech before your game so both pads are detected.",
        L"Use borderless windowed mode for a clean overlay.",
        L"Press F9 to release input when you need to type.",
    };
    for (int i = 0; i < 4; ++i) {
        float sy = c2.Y + 46.0f + i * 30.0f;
        RectF num(c2.X + 20.0f, sy, 18.0f, 18.0f);
        SolidBrush nb(alpha(C::P1, 28));
        g.FillEllipse(&nb, num);
        Font numFont(L"Segoe UI", 9.5f, FontStyleBold, UnitPixel);
        text(g, std::to_wstring(i + 1), numFont, num, C::P1, StringAlignmentCenter);
        text(g, steps[i], bodyFont, RectF(c2.X + 48.0f, sy - 3.0f, c2.Width - 70.0f, 24.0f), C::TextSecond);
    }

    // Application
    float y3 = y2 + 176.0f + 16.0f;
    RectF c3(L.cx, y3, L.cw, 80.0f);
    drawCard(g, c3);
    text(g, L"Quit Keytech", titleFont, RectF(c3.X + 20, c3.Y + 18, 200, 18), C::Text);
    text(g, L"Controllers disconnect when the app closes.", bodyFont,
         RectF(c3.X + 20, c3.Y + 38, 320, 16), C::TextMuted);

    bool hov = over(state, L.quitBtn);
    fillRound(g, hov ? alpha(C::Red, 34) : C::SurfaceAlt, L.quitBtn, 8.0f);
    strokeRound(g, hov ? alpha(C::Red, 200) : C::BorderStrong, L.quitBtn, 8.0f);
    Font btnFont(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);
    text(g, L"Quit", btnFont, L.quitBtn, hov ? C::Red : C::TextSecond, StringAlignmentCenter);
}

// -------------------------------------------------------------
// Toast
// -------------------------------------------------------------
static void drawToast(Graphics& g, int width, int height, const AppState& state) {
    if (state.toastMessage.empty()) return;
    if (GetTickCount() - state.toastTimestamp > 3000) return;

    Font font(L"Segoe UI", 11.0f, FontStyleRegular, UnitPixel);
    std::wstring msg = widen(state.toastMessage);

    RectF r(0, 0, 300, 40);
    float x = (width - r.Width) * 0.5f;
    float y = (float)height - 92.0f;

    RectF shadow(x, y + 3.0f, r.Width, r.Height);
    fillRound(g, Color(70, 0, 0, 0), shadow, 10.0f);

    RectF box(x, y, r.Width, r.Height);
    fillRound(g, C::SurfaceAlt, box, 10.0f);
    strokeRound(g, C::BorderStrong, box, 10.0f);

    SolidBrush circle(alpha(C::Green, 34));
    g.FillEllipse(&circle, box.X + 14.0f, box.Y + 12.0f, 16.0f, 16.0f);
    iconCheck(g, box.X + 22.0f, box.Y + 20.0f, 4.0f, C::Green, 1.6f);

    text(g, msg, font, RectF(box.X + 40.0f, box.Y, box.Width - 54.0f, box.Height), C::Text);
}

// -------------------------------------------------------------
// In-game overlay
// -------------------------------------------------------------
static void drawOverlay(Graphics& g, int width, int height, const AppState& state, const AppLayout& L) {
    // Opacity comes from the layered window itself, so the shell is painted fully
    // opaque here and edge to edge (no dark blobs in the rounded corners).
    const Color shellColor(255, 17, 20, 25);
    g.Clear(shellColor);

    RectF shell(0, 0, (float)width, (float)height);
    fillRound(g, shellColor, shell, 12.0f);
    strokeRound(g, Color(46, 255, 255, 255), shell, 12.0f, 1.0f);

    // Drag strip
    RectF brand(16, 12, 20, 20);
    fillRound(g, alpha(C::P1, 34), brand, 6.0f);
    strokeRound(g, alpha(C::P1, 120), brand, 6.0f);
    iconCrossMark(g, brand.X + 10.0f, brand.Y + 10.0f, 9.0f, 2.0f, C::P1);

    Font titleFont(L"Segoe UI", 11.5f, FontStyleBold, UnitPixel);
    text(g, L"Keytech", titleFont, RectF(44, 12, 80, 20), C::Text);

    Color statusCol = state.active ? C::Green : C::Amber;
    SolidBrush sd(statusCol);
    g.FillEllipse(&sd, 126.0f, 20.0f, 6.0f, 6.0f);
    Font statusFont(L"Segoe UI", 10.5f, FontStyleRegular, UnitPixel);
    text(g, state.active ? L"Active" : L"Paused", statusFont, RectF(138, 12, 80, 20), statusCol);

    // Window controls
    bool closeHover = over(state, L.overlayClose);
    bool restoreHover = over(state, L.overlayRestore);
    if (closeHover) fillRound(g, alpha(C::Red, 40), L.overlayClose, 6.0f);
    if (restoreHover) fillRound(g, alpha(Color(255, 255, 255, 255), 16), L.overlayRestore, 6.0f);
    iconChevron(g, L.overlayRestore.X + 14.0f, L.overlayRestore.Y + 14.0f, 9.0f, 2,
                restoreHover ? C::Text : C::TextSecond, 1.5f);
    iconClose(g, L.overlayClose.X + 14.0f, L.overlayClose.Y + 14.0f, 5.0f,
              closeHover ? C::Red : C::TextSecond, 1.5f);

    Pen sep(Color(255, 38, 43, 51));
    g.DrawLine(&sep, 0.0f, 44.0f, (float)width, 44.0f);

    // Controllers
    float colW = ((float)width - 48.0f) * 0.5f;
    Font labelFont(L"Segoe UI", 11.0f, FontStyleBold, UnitPixel);

    for (int p = 0; p < 2; ++p) {
        float centerX = 16.0f + p * (colW + 16.0f) + colW * 0.5f;
        Color accent = (p == 0) ? C::P1 : C::P2;

        SolidBrush pd(accent);
        g.FillEllipse(&pd, centerX - 34.0f, 54.0f, 6.0f, 6.0f);
        text(g, p == 0 ? L"Player 1" : L"Player 2", labelFont,
             RectF(centerX - 24.0f, 47.0f, 110.0f, 20.0f), accent);

        drawController(g, centerX, 166.0f, 0.9f, p, state.liveReports[p]);
    }
}

// -------------------------------------------------------------
// Render entry point
// -------------------------------------------------------------
void renderApp(HWND hwnd, HDC hdc, int width, int height, AppState& state) {
    if (width <= 0 || height <= 0) return;

    state.animationTick = GetTickCount();

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    AppLayout L = computeLayout(width, height, state);

    if (state.isOverlayMode) {
        drawOverlay(g, width, height, state, L);
    } else {
        SolidBrush bg(C::Window);
        g.FillRectangle(&bg, 0.0f, 0.0f, (REAL)width, (REAL)height);

        drawSidebar(g, width, height, state, L);
        drawPageHeader(g, width, state, L);

        switch (state.currentTab) {
            case TAB_DASHBOARD: drawOverview(g, height, state, L); break;
            case TAB_KEYBINDS:  drawKeybinds(g, height, state, L); break;
            case TAB_SETTINGS:  drawSettings(g, state, L); break;
        }

        drawToast(g, width, height, state);
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

// -------------------------------------------------------------
// Input handling
// -------------------------------------------------------------
void handleUIMouseDown(HWND hwnd, int x, int y, AppState& state) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    AppLayout L = computeLayout(width, height, state);

    if (state.isOverlayMode) {
        if (L.overlayClose.Contains((REAL)x, (REAL)y)) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return;
        }
        if (L.overlayRestore.Contains((REAL)x, (REAL)y)) {
            PostMessage(hwnd, WM_COMMAND, 1002, 0);
            return;
        }
        if (y < 44) {
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

    // Sidebar navigation
    for (int i = 0; i < 3; ++i) {
        if (L.nav[i].Contains((REAL)x, (REAL)y)) {
            state.currentTab = (AppTab)i;
            state.rebindingPlayer = -1;
            state.rebindingButton = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }

    if (L.overlayBtn.Contains((REAL)x, (REAL)y)) {
        PostMessage(hwnd, WM_COMMAND, 1002, 0);
        return;
    }

    if (L.ctrlRow.Contains((REAL)x, (REAL)y)) {
        PostMessage(hwnd, WM_COMMAND, 1001, 0);
        return;
    }

    if (L.driverRow.Contains((REAL)x, (REAL)y) && !state.vigemConnected) {
        PostMessage(hwnd, WM_COMMAND, 1003, 0);
        return;
    }

    if (L.hasBanner && !state.vigemConnected && L.banner.Contains((REAL)x, (REAL)y)) {
        PostMessage(hwnd, WM_COMMAND, 1003, 0);
        return;
    }

    if (state.currentTab == TAB_KEYBINDS) {
        for (int p = 0; p < 2; ++p) {
            for (size_t i = 0; i < L.keyChip[p].size(); ++i) {
                if (L.keyChip[p][i].Contains((REAL)x, (REAL)y)) {
                    state.rebindingPlayer = p;
                    state.rebindingButton = g_availableButtons[i].button;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return;
                }
            }
        }

        if (L.saveBtn.Contains((REAL)x, (REAL)y)) {
            if (saveConfig("config.json")) {
                triggerToast(state, "Changes saved to config.json");
            } else {
                triggerToast(state, "Could not save config.json");
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }

        if (L.resetBtn.Contains((REAL)x, (REAL)y)) {
            resetToDefaults();
            state.rebindingPlayer = -1;
            state.rebindingButton = 0;
            triggerToast(state, "Default bindings restored");
            InvalidateRect(hwnd, NULL, FALSE);
            return;
        }
    }

    if (state.currentTab == TAB_SETTINGS) {
        int opacities[4] = { 150, 190, 220, 255 };
        for (int i = 0; i < 4; ++i) {
            if (L.seg[i].Contains((REAL)x, (REAL)y)) {
                state.overlayOpacity = opacities[i];
                triggerToast(state, "Overlay opacity updated");
                InvalidateRect(hwnd, NULL, FALSE);
                return;
            }
        }

        if (L.quitBtn.Contains((REAL)x, (REAL)y)) {
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
        return;
    }

    // Pointer feedback over interactive surfaces
    RECT rc;
    GetClientRect(hwnd, &rc);
    AppLayout L = computeLayout(rc.right - rc.left, rc.bottom - rc.top, state);
    bool clickable = false;

    if (state.isOverlayMode) {
        clickable = L.overlayClose.Contains((REAL)x, (REAL)y) || L.overlayRestore.Contains((REAL)x, (REAL)y);
        if (y < 44) clickable = true;
    } else {
        for (int i = 0; i < 3; ++i) if (L.nav[i].Contains((REAL)x, (REAL)y)) clickable = true;
        clickable = clickable || L.overlayBtn.Contains((REAL)x, (REAL)y)
                             || L.ctrlRow.Contains((REAL)x, (REAL)y);
        if (!state.vigemConnected) {
            clickable = clickable || L.driverRow.Contains((REAL)x, (REAL)y)
                                 || (L.hasBanner && L.banner.Contains((REAL)x, (REAL)y));
        }
        if (state.currentTab == TAB_KEYBINDS) {
            clickable = clickable || L.saveBtn.Contains((REAL)x, (REAL)y)
                                 || L.resetBtn.Contains((REAL)x, (REAL)y);
            for (int p = 0; p < 2 && !clickable; ++p)
                for (size_t i = 0; i < L.keyChip[p].size(); ++i)
                    if (L.keyChip[p][i].Contains((REAL)x, (REAL)y)) { clickable = true; break; }
        }
        if (state.currentTab == TAB_SETTINGS) {
            clickable = clickable || L.quitBtn.Contains((REAL)x, (REAL)y);
            for (int i = 0; i < 4; ++i) if (L.seg[i].Contains((REAL)x, (REAL)y)) clickable = true;
        }
    }

    SetCursor(LoadCursor(NULL, clickable ? IDC_HAND : IDC_ARROW));
    InvalidateRect(hwnd, NULL, FALSE);
}

void handleUIMouseUp(HWND hwnd, int x, int y, AppState& state) {
    (void)x; (void)y;
    if (state.isDragging) {
        state.isDragging = false;
        ReleaseCapture();
    }
}
