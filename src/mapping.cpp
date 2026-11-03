#include "mapping.h"
#include <iostream>
using namespace std;


vector<Mapping> mappings = {
    // Player 1
    {'W', XUSB_GAMEPAD_DPAD_UP,    0},
    {'S', XUSB_GAMEPAD_DPAD_DOWN,  0},
    {'A', XUSB_GAMEPAD_DPAD_LEFT,  0},
    {'D', XUSB_GAMEPAD_DPAD_RIGHT, 0},
    {'J', XUSB_GAMEPAD_A,          0},
    {'K', XUSB_GAMEPAD_B,          0},
    {'U', XUSB_GAMEPAD_X,          0},
    {'I', XUSB_GAMEPAD_Y,          0},
    // Player 2
    {VK_UP,    XUSB_GAMEPAD_DPAD_UP,    1},
    {VK_DOWN,  XUSB_GAMEPAD_DPAD_DOWN,  1},
    {VK_LEFT,  XUSB_GAMEPAD_DPAD_LEFT,  1},
    {VK_RIGHT, XUSB_GAMEPAD_DPAD_RIGHT, 1},
    {VK_NUMPAD1, XUSB_GAMEPAD_A, 1},
    {VK_NUMPAD2, XUSB_GAMEPAD_B, 1},
    {VK_NUMPAD4, XUSB_GAMEPAD_X, 1},
    {VK_NUMPAD5, XUSB_GAMEPAD_Y, 1},
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