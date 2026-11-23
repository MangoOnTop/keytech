# KEYTECH

KEYTECH maps keyboard input to two virtual Xbox 360 controllers via ViGEmBus, allowing two players to play local versus fighting games on a single keyboard.

## Installation

### Option A: Installer (Easiest)
Download and run **[KeytechSetup.exe](https://github.com/MangoOnTop/keytech/releases/latest/download/KeytechSetup.exe)** from Releases. It automatically installs the ViGEmBus driver if missing.
- Windows SmartScreen may show a warning: click **More info** then **Run anyway**.
- If Windows Smart App Control (SAC) is enabled, it blocks unsigned installers with no override. Use Option B instead.

### Option B: Build from Source (Works with Smart App Control)
1. Install MSYS2 from https://www.msys2.org, install MinGW-w64 (`pacman -S mingw-w64-x86_64-gcc`), and add `C:\msys64\mingw64\bin` to your system PATH.
2. Click **Code > Download ZIP** on GitHub and extract the folder.
3. Double-click `run.bat`. The first run builds the app; subsequent runs launch it directly. If ViGEmBus is missing, KEYTECH will offer to install it.

*Why Option B works:* Locally built executables are not marked with a web download identifier, so Windows Smart App Control permits them to run.

## Controls & Hotkeys

- **F9**: Pause / Resume controller input
- **F10**: Toggle in-game transparent overlay HUD

### Default Key Bindings
| Action | Player 1 | Player 2 |
|---|:---:|:---:|
| Up / Down / Left / Right | W / S / A / D | Up / Down / Left / Right Arrow |
| A / B / X / Y | J / K / U / I | Numpad 1 / 2 / 4 / 5 |
| LB / RB | O / L | Numpad 6 / 3 |
| Back / Start | R / E | Numpad . / Numpad 0 |

All keys can be rebound inside the app under the Key Mapper tab.

## Uninstallation
- **Option A**: Windows Settings > Apps > Installed apps > KEYTECH > Uninstall.
- **Option B**: Delete the extracted folder.
- The ViGEmBus driver can be uninstalled separately via Windows Settings > Apps.

## License
MIT License. See [LICENSE](LICENSE) for details.
