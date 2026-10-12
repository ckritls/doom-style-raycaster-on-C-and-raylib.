# Doom-style Raycaster (C++17 + raylib)

A pseudo-3D shooter in the spirit of Doom/Wolfenstein, written from scratch in C++17.
Rendering is a classic raycaster (DDA) with textured walls and enemy sprites.
The level is generated procedurally (random rooms and corridors), and the textures
are generated procedurally too — no assets are needed.
This is my first project, so don't beat me with sticks =P.

raylib is downloaded automatically from the official repository
<https://github.com/raysan5/raylib> (via `git clone`, and if git is not
installed — as a source archive straight from GitHub).

## Controls

| Key                        | Action                         |
| -------------------------- | ------------------------------ |
| `W` / `S` / `↑` / `↓`      | forward / backward             |
| `A` / `D` / `←` / `→`      | turn camera                    |
| Mouse (X movement)         | turn camera                    |
| `LMB` / `Space` / `Ctrl`   | shoot (SMG — hold)             |
| `1` / `2` / `3`            | pistol / shotgun / SMG         |
| Mouse wheel                | switch weapon                  |
| `R`                        | reload                         |
| `Esc`                      | quit                           |
| `Enter` / `Space`          | new area (after clearing)      |
| `R` / `Enter`              | new area (after death)         |

In the top-right corner there is a minimap (walls, enemies, pickups and the player
with a direction arrow). **Medkits** (heal) and **armor** (absorbs part of the
damage) are scattered across the map. The pistol has infinite ammo, while the
shotgun and SMG have limited ammo, with reloading. Shots are visible as tracers.
All sounds (shots, hits, damage, enemy death, reload, item pickup) are generated
procedurally — no assets required.

The level is regenerated on every launch and after each clear:
random rooms connected by corridors, a random number of enemies (4–9) and
pickups. The goal is to destroy all enemies; after that, a new area is created.

## Building (Windows, MinGW/MSYS2)

Requirements: CMake >= 3.16 and a MinGW compiler (e.g., `mingw-w64-ucrt-x86_64-gcc`).

```powershell
cmake -G "MinGW Makefiles" -S . -B build
cmake --build build
.\build\idk.exe
```

If desired, you can build a Release version:

```powershell
cmake -G "MinGW Makefiles" -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Building (Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/idk
```

## Structure

```
CMakeLists.txt        — build + automatic raylib download (FetchContent)
src/Config.h          — all game constants
src/Map.h/.cpp        — level grid, collisions, line-of-sight
src/Player.h/.cpp     — player (movement, turning, shooting)
src/Enemy.h/.cpp      — enemy (A* along walls, separation from neighbors, attack)
src/Renderer.h/.cpp   — raycaster (walls + sprites + z-buffer)
src/Textures.h/.cpp   — procedural textures
src/Game.h/.cpp       — game loop, procedural generation, HUD
src/main.cpp          — entry point
src/audoio.cpp/h      — sounds ^_^
```

## How to change the raylib version

In `CMakeLists.txt`, change the `RAYLIB_VERSION` variable.

## License
This project is distributed under the **GNU General Public License v3.0 (GPLv3)**.
See the [LICENSE](LICENSE) file for the full license text.

### Third-Party Libraries

* **[raylib](https://www.raysan5.com/)** - used for rendering and input.
    distributed under the **zlib/libpng license**
    Copyright (c) 2013-2026 Ramon Santamaria (@raysan5).