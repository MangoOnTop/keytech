#pragma once
#include <vector>
#include <windows.h>
#include <ViGEm/Client.h>

struct Mapping {
    int key;      // which keyboard key
    WORD button;  // which gamepad button
    int player;   // 0 = pad 1, 1 = pad 2
};

extern std::vector<Mapping> mappings;

bool hasDuplicates();