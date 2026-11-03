#include <iostream>
#include <windows.h>
#include <ViGEm/Client.h>
using namespace std;

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
    cin.get();

    vigem_target_remove(client, pad1);
    vigem_target_remove(client, pad2);
    vigem_target_free(pad1);
    vigem_target_free(pad2);
    vigem_disconnect(client);
    vigem_free(client);
}