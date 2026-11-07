@echo off
if not exist build mkdir build
g++ -std=c++17 -Wall -Wextra -I third_party/ViGEmClient/include -I third_party/json src/main.cpp src/mapping.cpp third_party/ViGEmClient/src/ViGEmClient.cpp -o build/keytech.exe -lsetupapi