#pragma once
#include <vector>
#include <windows.h>
#include <ViGEm/Client.h>

struct Mapping {
    int key;      // virtual-key code (VK_*)
    WORD button;  // XUSB_GAMEPAD_* button bitmask
    int player;   // 0 = pad 1, 1 = pad 2
};

extern std::vector<Mapping> mappings;

#include <string>
bool isMapped(DWORD vk);
bool hasDuplicates();
bool loadConfig(const std::string& path);
bool saveConfig(const std::string& path);
