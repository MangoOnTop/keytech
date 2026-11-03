#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
#include "mapping.h"
using namespace std;


int main() {
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
    bool active = true;      
    bool f9WasDown = false; 
    XUSB_REPORT reports[2] = {};
    
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