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

Clone with the engine submodule:
```bash
git clone --recurse-submodules https://github.com/eugenegalunin/four-winds-reborn.git
cd four-winds-reborn
```

macOS or Linux builds the Release binary and runs all tests with one command:
```bash
./scripts/build.sh
./four-winds-reborn
```

On a fresh macOS or Debian/Ubuntu machine, let the script install dependencies:
```bash
./scripts/build.sh --install-deps
```

Windows uses MSYS2 UCRT64/MinGW because the SDL engine is not an MSVC project:
```powershell
.\scripts\build-windows.ps1 -InstallDeps
.\dist\windows\four-winds-reborn.exe
```

Install [MSYS2](https://www.msys2.org/) in `C:\msys64` first, or pass
`-MsysRoot D:\path\to\msys64`. Later builds only need
`.\scripts\build-windows.ps1`. Unix scripts accept `--clean`, `--debug` and
`--no-tests`; PowerShell accepts `-Clean`, `-DebugBuild` and `-NoTests`.

Manual requirements are CMake 3.14+, a C++17 compiler, pkg-config, zlib, SDL2,
SDL2_image, SDL2_mixer and SDL2_ttf. Boost stacktrace is optional because the
game has a native crash-report fallback.

Useful launch flags:
```bash
./four-winds-reborn -f              # fullscreen
./four-winds-reborn -g 1280x720     # window size
./four-winds-reborn -t default      # theme
./four-winds-reborn -s /path/game.sav
```

Manual gameplay regression tests:
```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

GitHub Actions runs Debug and Release builds plus CTest on Linux, macOS and
Windows UCRT64. The suite includes a fixed-seed replay hash canary so an
authoritative simulation drift between platforms is visible immediately. Tags
matching `v*` publish tested Linux, macOS and self-contained Windows archives,
along with SHA-256 checksums, to GitHub Releases.

Print the deterministic avatar and Hell Blast balance report:
```bash
./build-tests/gameplay_regression_tests --balance-only
```

The baseline compares summon rosters, spell plans and avatar passives on common fixtures.
Catastrophic is represented by Hell Blast in the spell score; Bard, Monacle, Luck and
Telepath are reported in a separate passive column.

## Crash Diagnostics

The game keeps a persistent engine log and a crash report with recent Mahjong
actions. On macOS they are stored in:

```text
~/Library/Application Support/four-winds-reborn/four-winds-reborn.log
~/Library/Application Support/four-winds-reborn/crash-report.log
```

Linux and Windows use the SDL per-user application data directory. All platforms
record uncaught C++/SWE exceptions and recent actions; macOS/Linux also append a
native stack for fatal signals. Reports rotate at 4 MiB; a hard crash deliberately
has no `clean shutdown` marker.

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
