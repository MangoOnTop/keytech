#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
using namespace std;


XUSB_REPORT report1 = {};   
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
    cout << "Hold W = Pad 1 Up. Press Q to quit." << endl;

    while (true) {
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }

        if (GetAsyncKeyState('W') & 0x8000) {
            report1.wButtons |= XUSB_GAMEPAD_DPAD_UP;
        } else {
            report1.wButtons &= ~XUSB_GAMEPAD_DPAD_UP;
        }

    vigem_target_x360_update(client, pad1, report1);
    Sleep(10);
}

    vigem_target_remove(client, pad1);
    vigem_target_remove(client, pad2);
    vigem_target_free(pad1);
    vigem_target_free(pad2);
    vigem_disconnect(client);
    vigem_free(client);
}