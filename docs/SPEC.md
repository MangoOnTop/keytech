# Spec of keytech

## Goal
Play modern fighting games offline versus with two virtual Xbox 360 gamepads using only one keyboard.

## Features Implemented

### Core Emulation
1. **Two virtual Xbox 360 gamepads** via ViGEmBus.
2. **Dedicated 1000 Hz input polling loop** (~1ms latency) on background thread.
3. **Toggle option** to switch between regular keyboard and gamepads using **F9**.
4. **Selective keystroke suppression**: Mapped keys are swallowed while controllers are active to prevent game keyboard conflicts.
5. **No more 'Q' kill switch**: App closes cleanly via UI Exit button or window [X]; 'Q' can now be mapped freely.

### Professional UI & Visualizer
1. **Hardware-accelerated dark theme UI** rendered via GDI+ with double-buffering (0% flicker).
2. **Live dual controller monitor**: Player 1 (Neon Cyan) and Player 2 (Neon Coral) controllers render side-by-side with live buttons and D-Pad directions lighting up on press.
3. **Interactive Key Rebinding (Tab 2)**: Click any button to rebind ("Press any key..."), with live duplicate key conflict detection & warning banners.
4. **Config persistence**: Save to / Load from `config.json` with reset-to-defaults option.

### In-Game Overlay Mode (Version 2)
1. **Floating transparent HUD** toggled via **F10** or UI button.
2. **Always-On-Top & Frameless**: Floats over borderless windowed fighting games.
3. **Draggable**: Click and drag to reposition anywhere on the screen.
4. **Adjustable opacity**: Presets for 60%, 75%, 88%, and 100%.

## Target Games
- The King of Fighters XV
- Street Fighter 6 / Street Fighter V
- The King of Fighters XIII / XIV
- Tekken 8 / 7
- Guilty Gear -Strive-
