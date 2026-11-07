#include "mapping.h"
#include <fstream>
#include "json.hpp"
#include <iostream>
using namespace std;
using json = nlohmann::json;


vector<Mapping> mappings = {
    // Player 1
    {'W', XUSB_GAMEPAD_DPAD_UP,        0},
    {'S', XUSB_GAMEPAD_DPAD_DOWN,      0},
    {'A', XUSB_GAMEPAD_DPAD_LEFT,      0},
    {'D', XUSB_GAMEPAD_DPAD_RIGHT,     0},
    {'J', XUSB_GAMEPAD_A,              0},
    {'K', XUSB_GAMEPAD_B,              0},
    {'U', XUSB_GAMEPAD_X,              0},
    {'I', XUSB_GAMEPAD_Y,              0},
    {'E', XUSB_GAMEPAD_START,          0},
    {'R', XUSB_GAMEPAD_BACK,           0},
    {'O', XUSB_GAMEPAD_LEFT_SHOULDER,  0},
    {'L', XUSB_GAMEPAD_RIGHT_SHOULDER, 0},
    // Player 2
    {VK_UP,    XUSB_GAMEPAD_DPAD_UP,            1},
    {VK_DOWN,  XUSB_GAMEPAD_DPAD_DOWN,          1},
    {VK_LEFT,  XUSB_GAMEPAD_DPAD_LEFT,          1},
    {VK_RIGHT, XUSB_GAMEPAD_DPAD_RIGHT,         1},
    {VK_NUMPAD1, XUSB_GAMEPAD_A,                1},
    {VK_NUMPAD2, XUSB_GAMEPAD_B,                1},
    {VK_NUMPAD4, XUSB_GAMEPAD_X,                1},
    {VK_NUMPAD5, XUSB_GAMEPAD_Y,                1},
    {VK_NUMPAD0,  XUSB_GAMEPAD_START,           1},
    {VK_DECIMAL,  XUSB_GAMEPAD_BACK,            1},
    {VK_NUMPAD6,  XUSB_GAMEPAD_LEFT_SHOULDER,   1},
    {VK_NUMPAD3,  XUSB_GAMEPAD_RIGHT_SHOULDER,  1},
};

struct ButtonName { const char* name; WORD value; };

static const ButtonName buttonNames[] = {
    {"UP",    XUSB_GAMEPAD_DPAD_UP},
    {"DOWN",  XUSB_GAMEPAD_DPAD_DOWN},
    {"LEFT",  XUSB_GAMEPAD_DPAD_LEFT},
    {"RIGHT", XUSB_GAMEPAD_DPAD_RIGHT},
    {"A",     XUSB_GAMEPAD_A},
    {"B",     XUSB_GAMEPAD_B},
    {"X",     XUSB_GAMEPAD_X},
    {"Y",     XUSB_GAMEPAD_Y},
    {"START", XUSB_GAMEPAD_START},
    {"BACK",  XUSB_GAMEPAD_BACK},
    {"LB",    XUSB_GAMEPAD_LEFT_SHOULDER},
    {"RB",    XUSB_GAMEPAD_RIGHT_SHOULDER},
};

bool hasDuplicates() {
    bool found = false;

    for (size_t i = 0; i < mappings.size(); i++) {
        for (size_t j = i + 1; j < mappings.size(); j++) {
            if (mappings[i].key == mappings[j].key) {
                cout << "Warning: key " << mappings[i].key
                     << " is used twice!" << endl;
                found = true;
            }
        }
    }
    return found;
}