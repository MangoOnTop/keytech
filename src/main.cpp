#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
#include "mapping.h"
using namespace std;

bool keyState[256] = {};   // is each key down right now?
bool active = true;        // are the pads switched on?

bool isMapped(DWORD vk) {
    for (Mapping m : mappings) {
        if (m.key == (int)vk) return true;
    }
    return false;
}

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;  // the "box" describing this key press
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (k->vkCode < 256) {
            keyState[k->vkCode] = down;
        }
        if (active || k->vkCode == VK_F9) {
            return 1;  // swallow everything while ON, and always swallow F9
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

int main() {
    if (loadConfig("config.json")) {
        cout << "Loaded config.json" << endl;
    } else {
        cout << "No valid config.json, using built-in defaults." << endl;
        saveConfig("config.json");
    }
    if (hasDuplicates()) {
        cout << "Fix the duplicate keys, then run again." << endl;
        return 1;
    }
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
    bool f9WasDown = false; 
    XUSB_REPORT reports[2] = {};
    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);

    while (true) {
        MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                }
        if (keyState['Q']) { break; }
        bool f9IsDown = keyState[VK_F9];

        if (f9IsDown && !f9WasDown) {
            active = !active;
            cout << (active ? "Controllers ON" : "Controllers OFF") << endl;
        }
        f9WasDown = f9IsDown;
        for (Mapping m : mappings) {
            if (active && keyState[m.key]) {
                reports[m.player].wButtons |= m.button;
            } else {
                reports[m.player].wButtons &= ~m.button;
            }
        }

        vigem_target_x360_update(client, pad1, reports[0]);
        vigem_target_x360_update(client, pad2, reports[1]);
        Sleep(10);
    }  

    UnhookWindowsHookEx(hook);
    vigem_target_remove(client, pad1);
    vigem_target_remove(client, pad2);
    vigem_target_free(pad1);
    vigem_target_free(pad2);
    vigem_disconnect(client);
    vigem_free(client);
}