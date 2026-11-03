#include<iostream>
using namespace std;

struct Binding {
    int player;
    int keyCode;
};

void ChangeCopy(Binding b){
    b.keyCode = 65;
}

void changeOriginal(Binding* b){
    b->keyCode = 65;
}

int main(){
    Binding a;
    a.player = 1;
    a.keyCode = 87;
    
    ChangeCopy(a);
    cout << "after changeCopy: " << a.keyCode << endl;

    changeOriginal(&a);
    cout << "after changeOriginal: " << a.keyCode << endl;
}