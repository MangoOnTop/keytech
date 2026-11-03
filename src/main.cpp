#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
using namespace std;

struct Mapping {
    int key;      
    WORD button;  
    int player;   
};

Mapping mappings[] = {
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

XUSB_REPORT reports[2] = {};

int main() {
    PVIGEM_CLIENT client = vigem_alloc();

    if (vigem_connect(client) != VIGEM_ERROR_NONE) {
        cout << "Could not connect. Is ViGEmBus installed?" << endl;
        return 1;
    }

    PVIGEM_TARGET pad1 = vigem_target_x360_alloc();
    PVIGEM_TARGET pad2 = vigem_target_x360_alloc();
    vigem_target_add(client, pad1);
    vigem_target_add(client, pad2);

    cout << "Two pads plugged in. Check joy.cpl. Press Enter to quit." << endl;
    cout << "Keytech running. Press Q to quit." << endl;
    bool active = true;      
    bool f9WasDown = false; 
    while (true) {
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }
        bool f9IsDown = GetAsyncKeyState(VK_F9) & 0x8000;

        if (f9IsDown && !f9WasDown) {
            active = !active;
            cout << (active ? "Controllers ON" : "Controllers OFF") << endl;
        }
        f9WasDown = f9IsDown;
        for (Mapping m : mappings) {
            if (active && (GetAsyncKeyState(m.key) & 0x8000)) {
                reports[m.player].wButtons |= m.button;
            } else {
                reports[m.player].wButtons &= ~m.button;
            }
        }

        vigem_target_x360_update(client, pad1, reports[0]);
        vigem_target_x360_update(client, pad2, reports[1]);
        Sleep(10);
    }     

    vigem_target_remove(client, pad1);
    vigem_target_remove(client, pad2);
    vigem_target_free(pad1);
    vigem_target_free(pad2);
    vigem_disconnect(client);
    vigem_free(client);
}