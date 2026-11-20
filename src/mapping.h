#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <ViGEm/Client.h>

struct Mapping {
    int key;      // Virtual-key code (VK_*)
    WORD button;  // XUSB_GAMEPAD_* button bitmask
    int player;   // 0 = Player 1, 1 = Player 2
};

struct ButtonMeta {
    WORD button;
    const char* id;
    const char* label;
};

extern std::vector<Mapping> mappings;
extern const std::vector<ButtonMeta> g_availableButtons;

// Helper functions
std::string getKeyName(int vkCode);
bool isMapped(DWORD vk);
std::vector<int> getDuplicateKeys();
bool hasDuplicates();
int getKeyForButton(int player, WORD button);
void setMapping(int player, WORD button, int newKey);
void resetToDefaults();

bool loadConfig(const std::string& path);
bool saveConfig(const std::string& path);
