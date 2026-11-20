#include "mapping.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;
using json = nlohmann::json;

const std::vector<ButtonMeta> g_availableButtons = {
    {XUSB_GAMEPAD_DPAD_UP,        "UP",    "D-Pad Up"},
    {XUSB_GAMEPAD_DPAD_DOWN,      "DOWN",  "D-Pad Down"},
    {XUSB_GAMEPAD_DPAD_LEFT,      "LEFT",  "D-Pad Left"},
    {XUSB_GAMEPAD_DPAD_RIGHT,     "RIGHT", "D-Pad Right"},
    {XUSB_GAMEPAD_A,              "A",     "Button A"},
    {XUSB_GAMEPAD_B,              "B",     "Button B"},
    {XUSB_GAMEPAD_X,              "X",     "Button X"},
    {XUSB_GAMEPAD_Y,              "Y",     "Button Y"},
    {XUSB_GAMEPAD_LEFT_SHOULDER,  "LB",    "Bumper LB"},
    {XUSB_GAMEPAD_RIGHT_SHOULDER, "RB",    "Bumper RB"},
    {XUSB_GAMEPAD_BACK,           "BACK",  "Back / Select"},
    {XUSB_GAMEPAD_START,          "START", "Start / Menu"},
};

static const std::vector<Mapping> g_defaultMappings = {
    // Player 1 (WASD movement + JUKILROE action buttons)
    {'W', XUSB_GAMEPAD_DPAD_UP,        0},
    {'S', XUSB_GAMEPAD_DPAD_DOWN,      0},
    {'A', XUSB_GAMEPAD_DPAD_LEFT,      0},
    {'D', XUSB_GAMEPAD_DPAD_RIGHT,     0},
    {'J', XUSB_GAMEPAD_A,              0},
    {'K', XUSB_GAMEPAD_B,              0},
    {'U', XUSB_GAMEPAD_X,              0},
    {'I', XUSB_GAMEPAD_Y,              0},
    {'O', XUSB_GAMEPAD_LEFT_SHOULDER,  0},
    {'L', XUSB_GAMEPAD_RIGHT_SHOULDER, 0},
    {'R', XUSB_GAMEPAD_BACK,           0},
    {'E', XUSB_GAMEPAD_START,          0},

    // Player 2 (Arrow keys movement + Numpad action buttons)
    {VK_UP,      XUSB_GAMEPAD_DPAD_UP,            1},
    {VK_DOWN,    XUSB_GAMEPAD_DPAD_DOWN,          1},
    {VK_LEFT,    XUSB_GAMEPAD_DPAD_LEFT,          1},
    {VK_RIGHT,   XUSB_GAMEPAD_DPAD_RIGHT,         1},
    {VK_NUMPAD1, XUSB_GAMEPAD_A,                  1},
    {VK_NUMPAD2, XUSB_GAMEPAD_B,                  1},
    {VK_NUMPAD4, XUSB_GAMEPAD_X,                  1},
    {VK_NUMPAD5, XUSB_GAMEPAD_Y,                  1},
    {VK_NUMPAD6, XUSB_GAMEPAD_LEFT_SHOULDER,     1},
    {VK_NUMPAD3, XUSB_GAMEPAD_RIGHT_SHOULDER,    1},
    {VK_DECIMAL, XUSB_GAMEPAD_BACK,              1},
    {VK_NUMPAD0, XUSB_GAMEPAD_START,             1},
};

std::vector<Mapping> mappings = g_defaultMappings;

std::string getKeyName(int vk) {
    if (vk == 0) return "Unbound";

    switch (vk) {
        case VK_UP: return "Up Arrow";
        case VK_DOWN: return "Down Arrow";
        case VK_LEFT: return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
        case VK_RETURN: return "Enter";
        case VK_SPACE: return "Space";
        case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab";
        case VK_ESCAPE: return "Esc";
        case VK_LSHIFT: return "L-Shift";
        case VK_RSHIFT: return "R-Shift";
        case VK_SHIFT: return "Shift";
        case VK_LCONTROL: return "L-Ctrl";
        case VK_RCONTROL: return "R-Ctrl";
        case VK_CONTROL: return "Ctrl";
        case VK_LMENU: return "L-Alt";
        case VK_RMENU: return "R-Alt";
        case VK_MENU: return "Alt";
        case VK_CAPITAL: return "Caps Lock";
        case VK_NUMPAD0: return "Num 0";
        case VK_NUMPAD1: return "Num 1";
        case VK_NUMPAD2: return "Num 2";
        case VK_NUMPAD3: return "Num 3";
        case VK_NUMPAD4: return "Num 4";
        case VK_NUMPAD5: return "Num 5";
        case VK_NUMPAD6: return "Num 6";
        case VK_NUMPAD7: return "Num 7";
        case VK_NUMPAD8: return "Num 8";
        case VK_NUMPAD9: return "Num 9";
        case VK_DECIMAL: return "Num .";
        case VK_ADD: return "Num +";
        case VK_SUBTRACT: return "Num -";
        case VK_MULTIPLY: return "Num *";
        case VK_DIVIDE: return "Num /";
        case VK_OEM_PLUS: return "=";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_1: return ";";
        case VK_OEM_2: return "/";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_5: return "\\";
        case VK_OEM_6: return "]";
        case VK_OEM_7: return "'";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9 (Reserved)";
        case VK_F10: return "F10 (Reserved)";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default: break;
    }

    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        return std::string(1, (char)vk);
    }

    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    if (scanCode != 0) {
        char buf[64];
        if (GetKeyNameTextA((scanCode << 16), buf, sizeof(buf)) > 0) {
            return std::string(buf);
        }
    }

    return "Key " + std::to_string(vk);
}

bool isMapped(DWORD vk) {
    for (const Mapping& m : mappings) {
        if (m.key == (int)vk) return true;
    }
    return false;
}

std::vector<int> getDuplicateKeys() {
    std::vector<int> duplicates;
    for (size_t i = 0; i < mappings.size(); i++) {
        if (mappings[i].key == 0) continue;
        for (size_t j = i + 1; j < mappings.size(); j++) {
            if (mappings[i].key == mappings[j].key) {
                if (std::find(duplicates.begin(), duplicates.end(), mappings[i].key) == duplicates.end()) {
                    duplicates.push_back(mappings[i].key);
                }
            }
        }
    }
    return duplicates;
}

bool hasDuplicates() {
    return !getDuplicateKeys().empty();
}

int getKeyForButton(int player, WORD button) {
    for (const Mapping& m : mappings) {
        if (m.player == player && m.button == button) {
            return m.key;
        }
    }
    return 0;
}

void setMapping(int player, WORD button, int newKey) {
    for (Mapping& m : mappings) {
        if (m.player == player && m.button == button) {
            m.key = newKey;
            return;
        }
    }
    // If not found, add it
    mappings.push_back({newKey, button, player});
}

void resetToDefaults() {
    mappings = g_defaultMappings;
}

bool loadConfig(const string& path) {
    ifstream file(path);
    if (!file) return false;

    json data = json::parse(file, nullptr, false);
    if (data.is_discarded() || !data.contains("mappings")) return false;

    vector<Mapping> loaded;
    for (auto& item : data["mappings"]) {
        string name = item.value("button", "");
        int player = item.value("player", 0);
        int key = item.value("key", 0);

        for (const auto& b : g_availableButtons) {
            if (name == b.id) {
                loaded.push_back({key, b.button, player});
                break;
            }
        }
    }

    if (loaded.empty()) return false;
    mappings = loaded;
    return true;
}

bool saveConfig(const string& path) {
    json data;
    data["mappings"] = json::array();

    for (const Mapping& m : mappings) {
        for (const auto& b : g_availableButtons) {
            if (b.button == m.button) {
                data["mappings"].push_back({
                    {"player", m.player},
                    {"button", b.id},
                    {"key", m.key}
                });
                break;
            }
        }
    }

    ofstream file(path);
    if (!file) return false;
    file << data.dump(2);
    return true;
}
