#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
#include "mapping.h"
using namespace std;

bool keyState[256] = {};   // is each key down right now?
bool active = true;        // are the pads switched on?

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (k->vkCode < 256) {
            keyState[k->vkCode] = down;
        }

        // Always swallow F9 so the toggle works regardless of active state.
        // When active, only swallow keys that are actually mapped — this lets
        // the game still receive un-mapped keys (menu navigation, etc.).
        if (k->vkCode == VK_F9) {
            return 1;
        }
        if (active && isMapped(k->vkCode)) {
            return 1;
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
        vigem_free(client);
        return 1;
    }

    PVIGEM_TARGET pad1 = vigem_target_x360_alloc();
    PVIGEM_TARGET pad2 = vigem_target_x360_alloc();

    if (vigem_target_add(client, pad1) != VIGEM_ERROR_NONE) {
        cout << "Failed to plug in pad 1." << endl;
        return 1;
    }
    if (vigem_target_add(client, pad2) != VIGEM_ERROR_NONE) {
        cout << "Failed to plug in pad 2." << endl;
        return 1;
    }

    cout << "Two virtual Xbox 360 pads plugged in." << endl;
    cout << "F9 = toggle controllers ON/OFF  |  Q = quit" << endl;
    cout << "Controllers are ON. Launch your game now." << endl;

    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);
    if (!hook) {
        cout << "Failed to install keyboard hook." << endl;
        return 1;
    }

    bool f9WasDown = false;

    while (true) {
        // Drain the Windows message queue — required for low-level hooks to fire.
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (keyState['Q']) break;

        bool f9IsDown = keyState[VK_F9];
        if (f9IsDown && !f9WasDown) {
            active = !active;
            cout << (active ? "Controllers ON" : "Controllers OFF") << endl;
        }
        f9WasDown = f9IsDown;

        // Rebuild both reports from scratch every frame.
        // This guarantees released keys are always cleared — no stale bits.
        XUSB_REPORT reports[2] = {};

        if (active) {
            for (const Mapping& m : mappings) {
                if (keyState[m.key]) {
                    reports[m.player].wButtons |= m.button;
                }
            }
        }

        vigem_target_x360_update(client, pad1, reports[0]);
        vigem_target_x360_update(client, pad2, reports[1]);
        Sleep(1);   // 1 ms polling — snappy response without busy-spinning
    }

    UnhookWindowsHookEx(hook);

    // Send a zeroed report before unplugging to clear any stuck buttons.
    XUSB_REPORT zero = {};
    vigem_target_x360_update(client, pad1, zero);
    vigem_target_x360_update(client, pad2, zero);

    vigem_target_remove(client, pad1);
    vigem_target_remove(client, pad2);
    vigem_target_free(pad1);
    vigem_target_free(pad2);
    vigem_disconnect(client);
    vigem_free(client);
    return 0;
}
