#include<iostream>
#include<windows.h>
using namespace std;

int main() {
    cout << "Hold W to test. Press Q to quit." << endl;

    while (true) {
        if (GetAsyncKeyState('W') & 0x8000) {
            cout << "W is pressed!" << endl;
        }
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }
        Sleep(50);
    }
}