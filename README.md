# keytech

**Play modern fighting games in local versus using only one keyboard.**

keytech creates two virtual Xbox 360 controllers from keyboard input using
[ViGEmBus](https://github.com/nefarius/ViGEmBus). Tested with KOF XV, Street Fighter V, and KOF XIII.

---

## Requirements

- Windows 10/11 (64-bit)
- [ViGEmBus driver](https://github.com/nefarius/ViGEmBus/releases/latest) installed

## Download

Grab the latest `keytech.exe` from the [Releases](../../releases) page.

> **Windows SmartScreen / Smart App Control**: On first run, Windows may warn you
> because the binary is unsigned. Click **"More info" → "Run anyway"**, or see
> [Building from source](#building-from-source) to compile it yourself.

---

## Default Key Mapping

| Action   | Player 1 | Player 2  |
|----------|----------|-----------|
| Up       | W        | ↑         |
| Down     | S        | ↓         |
| Left     | A        | ←         |
| Right    | D        | →         |
| A        | J        | Numpad 1  |
| B        | K        | Numpad 2  |
| X        | U        | Numpad 4  |
| Y        | I        | Numpad 5  |
| Start    | E        | Numpad 0  |
| Back     | R        | Numpad .  |
| LB       | O        | Numpad 6  |
| RB       | L        | Numpad 3  |

Toggle controllers on/off: **F9**  
Quit: **Q**

---

## Customizing Key Bindings

Edit `config.json` in the same folder as the exe. Keys are Windows
[virtual-key codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
(decimal). Button names: `UP DOWN LEFT RIGHT A B X Y START BACK LB RB`.

```json
{
  "mappings": [
    { "player": 0, "button": "A", "key": 74 },
    { "player": 1, "button": "A", "key": 97 }
  ]
}
```

---

## Usage

1. Install ViGEmBus (see Requirements).
2. Run `keytech.exe` **before** launching your game.
3. Launch the game — it will detect two Xbox 360 controllers.
4. Press **F9** to toggle input on/off. Press **Q** to quit.

---

## Building from Source

**Prerequisites:** [MinGW-w64 / g++](https://www.mingw-w64.org/) on your PATH.

```bat
git clone --recurse-submodules https://github.com/YOUR_USERNAME/keytech.git
cd keytech
build.bat
```

The compiled binary lands in `build\keytech.exe`.

---

## Third-party Libraries

| Library | License | Purpose |
|---------|---------|---------|
| [ViGEmClient](https://github.com/nefarius/ViGEmBus) | MIT | Virtual Xbox 360 controller API |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | JSON config file parsing |

---

## License

MIT — see [LICENSE](LICENSE).
