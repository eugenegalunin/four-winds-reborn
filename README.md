# Four Winds Reborn

Community continuation of the retro strategy game *The Isle of Four Winds:
Rune War*, focused on completing battle mode, tactical AI, spells, and modern
desktop support.

This repository is a modified continuation of
[RuneWars: New Age](https://github.com/AndreyBarmaley/runewars.newage). The
original copyright notices and GPL-3.0 license are preserved. Project changes
from 2026 onward are maintained as Four Winds Reborn.

The bundled engine is maintained separately as
[Four Winds Engine](https://github.com/eugenegalunin/four-winds-engine), with
its original LGPL-3.0-or-later license and upstream attribution preserved.

## Build and Run

Requirements:
- CMake 3.14+
- C++17 compiler (`clang++` or `g++`)

macOS (Homebrew) dependencies:
```bash
brew install pkg-config sdl2 sdl2_image sdl2_mixer sdl2_ttf sdl2_net sdl2_gfx boost
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig:${PKG_CONFIG_PATH}"
```

Commands:
```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./four-winds-reborn
```

Useful launch flags:
```bash
./four-winds-reborn -f              # fullscreen
./four-winds-reborn -g 1280x720     # window size
./four-winds-reborn -t default      # theme
./four-winds-reborn -s /path/game.sav
```

Optional gameplay regression tests:
```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

Implemented battle and map-phase rules are documented in
[`docs/BattleAndAdventureRules.md`](docs/BattleAndAdventureRules.md). Spell
effect lifetime rules are documented in
[`docs/SpellLifecycle.md`](docs/SpellLifecycle.md).

**select peson part**

![select person](https://user-images.githubusercontent.com/8620726/96436537-8458bd80-11f5-11eb-8e5c-ffbd513dda68.png)

**mahjong part**

![majong part](https://user-images.githubusercontent.com/8620726/96436506-828efa00-11f5-11eb-9462-7e2f086b4a07.png)

**adventure map**

![adventure map](https://user-images.githubusercontent.com/8620726/96436556-8589ea80-11f5-11eb-8a53-5a9758de950a.png)
