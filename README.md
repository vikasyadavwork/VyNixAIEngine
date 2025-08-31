# VyNix AI Engine

VyNix is a small 2D game engine written in C++20 with native OpenGL.

The repository includes **Skybound**, a two-plane shooting game. Both planes start with 100 health. It also includes a basic scene editor for creating and testing simple scenes.

## Controls

- `WASD` or arrow keys — move
- `Space` — shoot
- `P` or `Escape` — pause
- `R` — restart
- `M` — mute sound
- `F1`, `F2`, `F3` — switch between the game, scene editor, and engine screen

## Build

```powershell
cmake --preset vynix-windows
cmake --build --preset vynix-release --parallel 4
ctest --preset vynix-tests
```

Run the game from:

```text
out/build/vynix-release/bin/Release/Skybound.exe
```

The current platform target is Windows x64. Visual Studio 2022, CMake 3.25 or newer, and an OpenGL-capable driver are required.
