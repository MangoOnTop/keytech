# KEYTECH — Dual Virtual Gamepad Hub & In-Game Overlay

**Play modern fighting games in local offline versus using only one keyboard.**

KEYTECH creates two virtual Xbox 360 controllers from keyboard input using [ViGEmBus](https://github.com/nefarius/ViGEmBus). It features a modern hardware-accelerated GUI dashboard, real-time input visualization, interactive key rebinding with conflict detection, and a floating in-game transparent overlay.

---

## ✨ Features

- **Dual Virtual Xbox 360 Gamepads**: Fully detected by modern fighting games (KOF XV, Street Fighter 6, SFV, Tekken 8, Guilty Gear, KOF XIII, etc.).
- **Live Controller Visualizer**: Side-by-side graphical Xbox 360 gamepads for Player 1 (Neon Cyan) and Player 2 (Neon Coral). Buttons, D-pad directions, and bumpers illuminate in real time when keys are pressed.
- **In-Game Overlay Mode (`F10`)**: Transform the window into a sleek, semi-transparent, frameless, Always-On-Top HUD that floats cleanly over borderless windowed games.
- **Interactive Key Remapping**: Click any gamepad button in the app to rebind it ("Press any key..."). Includes automatic **duplicate key conflict detection & alerts**.
- **Ultra-low Latency (1000 Hz)**: Dedicated background worker thread polls keyboard input and updates virtual controllers at 1ms intervals.
- **Professional Windows UI**: Smooth GDI+ double-buffered rendering, dark theme, zero command prompt window (`-mwindows`), and clean in-app Exit button.

---

## 🎮 Hotkeys & Controls

| Shortcut | Action |
|:---:|---|
| **F9** | **Toggle Controllers ON / OFF** (Switch between keyboard typing and gamepad mode) |
| **F10** | **Toggle In-Game Overlay Mode** (Switch between Full Window and Floating Transparent HUD) |
| **ESC** | Cancel key rebinding |
| **Mouse Drag** | Drag the In-Game Overlay anywhere across your screen |

> **Note:** The previous `Q` to quit shortcut has been removed. You can now use the `Q` key in mappings freely! To exit, click the **Exit** button in the app header or the standard window close button `[X]`.

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

## 🚀 Quick Start

1. Install the official [ViGEmBus driver](https://github.com/nefarius/ViGEmBus/releases/latest) (one-time requirement).
2. Download or compile `keytech.exe`.
3. Launch `keytech.exe` **before** launching your fighting game.
4. Launch your game and enter Local Versus mode — both Player 1 and Player 2 controllers will be detected automatically!
5. Press **F10** while in-game to see the floating controller overlay.

---

## 🛠️ Building from Source

**Prerequisites:** [MSYS2 / MinGW-w64 (g++)](https://www.mingw-w64.org/) installed and available on your PATH.

```bat
git clone --recurse-submodules https://github.com/YOUR_USERNAME/keytech.git
cd keytech
build.bat
```

The resulting `build\keytech.exe` is completely self-contained with zero external runtime DLL dependencies.

---

## 📄 License

MIT License — see [LICENSE](LICENSE).
