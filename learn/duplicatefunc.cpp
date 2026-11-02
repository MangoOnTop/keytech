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
int main(){
    if (isDuplicate(21,20)){
        cout << "warning!" << endl;
    }
    else{
        cout << "OK!" <<endl;
    }
}
