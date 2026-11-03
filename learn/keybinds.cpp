#include<iostream>
using namespace std;

bool isDuplicate(int keyA, int keyB){
    if (keyA == keyB){
        return true;
    }
    else{
        return false;
    }
}
struct Binding{
    int Player;
    int keyCode;
};

int main){
    Binding a, b;
    a.Player = 1;
    a.keyCode = 87;
    b.Player = 2;
    b.keyCode = 47;
    if(isDuplicate(a.keyCode, b.keyCode)){
        cout << "waring!" <<endl
    }
    else{
        cout << "Player -> " << a.Player << "|" << "Keycode-> " <<a.keyCode << endl;
        cout << "Player -> " << b.Player << "|" << "Keycode-> " <<b.keyCode << endl;
    }
}

