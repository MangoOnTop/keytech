# KEYTECH // Dual Virtual Gamepad Hub & In-Game Overlay

**Play modern fighting games in local offline versus using only one keyboard.**

KEYTECH creates two virtual Xbox 360 controllers from keyboard input using [ViGEmBus](https://github.com/nefarius/ViGEmBus). It features authentic realistic Xbox controller graphics, a cyberpunk tournament UI, real-time input visualization, interactive key rebinding, and a floating transparent in-game HUD.

---

## 🚀 One-Click Download & Install (No Terminal Needed!)

| Edition | Description | Download Link |
|---|---|:---:|
| **One-Click Installer** | Self-contained installer that places a shortcut on your Desktop and Start Menu, registers uninstaller, and launches automatically. | **[Download KeytechSetup.exe](https://github.com/MangoOnTop/keytech/releases/latest/download/KeytechSetup.exe)** |
| **Portable Zip** | Standalone zip package: extract and run `keytech.exe` directly without installation. | **[Download keytech-portable.zip](https://github.com/MangoOnTop/keytech/releases/latest/download/keytech-portable.zip)** |

### 🛠️ One-Click Setup (Zero Manual Driver Work):
1. Download and double-click **`KeytechSetup.exe`**.
2. Click **"INSTALL KEYTECH"**:
   - The installer automatically detects if the ViGEmBus driver is present.
   - If missing, it **auto-downloads and installs ViGEmBus** for you!
   - If already installed, it automatically skips the download.
   - Automatically places a glowing gamepad icon on your Desktop and Start Menu.
3. Launch your fighting game (KOF XV, Street Fighter 6, Tekken 8, Guilty Gear, etc.) — both controllers will be recognized automatically!
4. When uninstalling, KEYTECH cleanly removes itself and offers to remove the ViGEmBus driver too.

---

## ✨ Features

- **Authentic Xbox 360 Controller Graphics**:
  - Realistic ergonomic wing handles with dual-tone grip panels.
  - Authentic cross D-Pad in recessed circular dish.
  - 3D glass ABXY jewel buttons with specular crescent highlights (Green, Red, Blue, Yellow).
  - Central Xbox Guide jewel with active 4-quadrant player LED indicators (P1 Top-Left, P2 Top-Right).
  - Left & Right analog thumbsticks with tactile grip dots.
  - Protruding shoulder bumper bars (`LB` / `RB`).
- **Floating In-Game Transparent Overlay (`F10`)**:
  - Press **`F10`** to transform the window into a frameless, semi-transparent, Always-On-Top HUD that floats seamlessly over borderless windowed fighting games.
  - Click and drag anywhere across the top bar to reposition it on your screen.
  - Opacity presets (60%, 75%, 88%, 100%) in settings.
- **Cyberpunk Tournament Interface**:
  - Obsidian dark aesthetic with neon cyan and electric coral trims.
  - Segmented arcade cabinet tabs: `[ 🎮 LIVE GAMEPAD HUD ]`, `[ ⌨ ARCADE KEY MAPPER ]`, `[ ⚙ OVERLAY & SETTINGS ]`.
  - Active controller status badges with pulsing indicators.
- **Arcade Key Remapper & Duplicate Conflict Alerts**:
  - Click any button box to rebind: prompts `[PRESS ANY KEY...]` (press `ESC` to cancel).
  - Real-time duplicate key detection warning banner if any keys conflict.
  - One-click `Save Config` and `Reset Fightstick Defaults`.
- **Ultra-low Latency (1000 Hz)**:
  - Dedicated background thread polls keyboard state and feeds virtual Xbox gamepads every 1ms (<1ms input latency).
- **Embedded Application Icon & Manifest**:
  - Statically linked with embedded 256x256 glowing gamepad `.ico`, Windows VersionInfo, and High-DPI awareness manifest.

---

## 🎮 Hotkeys & Controls

| Shortcut | Function |
|:---:|---|
| **`F9`** | **Toggle Controllers Active / Paused** (switch between keyboard typing and gaming) |
| **`F10`** | **Toggle In-Game Overlay Mode** (switch between Full Window and Floating HUD) |
| **`ESC`** | Cancel active key rebinding |
| **Mouse Drag** | Drag the In-Game Overlay anywhere across your screen |

---

## ⌨️ Default Key Mappings

| Action | Player 1 (WASD) | Player 2 (Numpad) |
|---|:---:|:---:|
| **D-Pad Up** | `W` | `↑` (Up Arrow) |
| **D-Pad Down** | `S` | `↓` (Down Arrow) |
| **D-Pad Left** | `A` | `←` (Left Arrow) |
| **D-Pad Right** | `D` | `→` (Right Arrow) |
| **Button A** (Bottom) | `J` | `Numpad 1` |
| **Button B** (Right) | `K` | `Numpad 2` |
| **Button X** (Left) | `U` | `Numpad 4` |
| **Button Y** (Top) | `I` | `Numpad 5` |
| **Bumper LB** | `O` | `Numpad 6` |
| **Bumper RB** | `L` | `Numpad 3` |
| **Back / Select** | `R` | `Numpad .` |
| **Start / Menu** | `E` | `Numpad 0` |

---

## 🛠️ Building from Source

**Prerequisites:** [MSYS2 / MinGW-w64 (g++)](https://www.mingw-w64.org/) available on your PATH.

```bat
git clone --recurse-submodules https://github.com/MangoOnTop/keytech.git
cd keytech
build.bat
```

The pipeline automatically compiles:
1. `build\KeytechSetup.exe` (Standalone self-extracting one-click installer)
2. `build\keytech.exe` (Portable executable with embedded icon and manifest)

---

## 📄 License

MIT License — see [LICENSE](LICENSE).
