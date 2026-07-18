# Four Winds Reborn

Community continuation of the retro strategy game *The Isle of Four Winds:
Rune War*, focused on completing battle mode, tactical AI, spells, and modern
desktop support.

This repository is a modified continuation of
[RuneWars: New Age](https://github.com/AndreyBarmaley/runewars.newage). The
original copyright notices and GPL-3.0 license are preserved. Project changes
from 2026 onward are maintained as Four Winds Reborn.

The bundled engine is maintained separately as
[Four Winds Engine](https://github.com/jaskes/four-winds-engine), with
its original LGPL-3.0-or-later license and upstream attribution preserved.

## Screenshots

### Main menu

![Four Winds Reborn main menu](docs/images/main-menu.jpg)

### Rune table

![Four Winds Reborn Mahjong rune table](docs/images/mahjong-table.jpg)

### Restored story intro

![Four Winds Reborn narrated story intro](docs/images/story-intro.jpg)

## Build and Run

Clone with the engine submodule:
```bash
git clone --recurse-submodules https://github.com/jaskes/four-winds-reborn.git
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
`--no-tests`; PowerShell accepts `-Clean`, `-DebugBuild` and `-NoTests`. Add
`-ReleaseGui` only when checking a player-facing Windows package without the
development console; the tagged release workflow enables it automatically.

Close a packaged game launched from `dist/windows` before rebuilding it. The
PowerShell wrapper detects a live packaged executable and stops before touching
the distribution; packaging is assembled in `dist/windows-staging` first so a
failed copy cannot leave a half-populated release directory.

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

## Saves and crash recovery

`Continue` loads the current autosave. It is refreshed at the end of each
completed Mahjong/adventure round and when returning to the main menu. During
Mahjong or Adventure, open the in-game menu to create a named save; using an
existing name asks before overwriting it.

`Load Game` lists the autosave and all named saves. A valid row can be loaded by
double-clicking it or using the Load button. Named saves may be deleted there
after confirmation; the autosave and emergency recovery checkpoints cannot be
deleted from this screen. Rotating recovery checkpoints remain in a separate
Recovery view for crash recovery and are not ordinary save slots.

## Music

Music is enabled by default. The restored soundtrack follows the original game
state mapping: the intro theme covers the main menu and player selection,
Mahjong uses the local player's clan theme, the Adventure map uses the map
theme, and score screens use the victory theme. Visible battle dialogs remain
deliberately unscored and resume the map theme when combat ends. The modern
build uses the original wavetable (`WT`) MIDI variants; sound effects remain
independent.

Startup also restores the original seventeen-frame narrated story intro before
the main menu, using the authentic English or Russian track selected in
Settings. It is shown once per launch and can be skipped with Esc, Enter, Space
or a left click.

## Settings and Russian localization

The main-menu Settings screen persists language, independent 0–100% music,
effects and voice levels, classic Guardian calls, presentation speed and
Windowed/Fullscreen display mode in the per-user `settings.json`. Volume bars
accept a mouse position or Left/Right keyboard adjustment. Display and audio
changes take effect when Apply is pressed without restarting; the game continues
to render its original 1024x768 canvas in both display modes. Existing settings
files with the older boolean music/sound keys remain compatible.

English uses the built-in source strings; Russian can be selected without
restarting the game and uses the restored classic terminology. The original
character, clan, creature and Mahjong call resources use the voice level;
Guardian animation calls also retain their own toggle. Proven localized avatar,
creature, clan and round announcements follow the selected English/Russian
language, while calls identical in both original editions remain shared.

Presentation speed has three profiles. `Classic` is the default and restores a
more deliberate hand deal, rune draw/discard and combat presentation. `Normal`
keeps the New Age timings, while `Fast` is intended for repeat play. The combat
dialog's 1x/2x/4x control remains available as a temporary per-battle override.

Manual gameplay regression tests:
```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-tests -j
ctest --test-dir build-tests --output-on-failure
```

GitHub Actions runs Debug and Release builds plus CTest for pushes and pull
requests targeting `develop` or `main` on Linux, macOS and Windows UCRT64. The
suite includes a fixed-seed replay hash canary so an authoritative simulation
drift between platforms is visible immediately. SemVer tags matching the CMake
project version (for example `v0.1.0`) publish tested Linux, macOS and
self-contained Windows archives, along with SHA-256 checksums, to GitHub
Releases only when the tagged commit is already on `main`.
The branch, release and hotfix procedure is documented in
[`docs/ReleaseProcess.md`](docs/ReleaseProcess.md).
Localization provenance and current coverage are recorded in
[`docs/Localization.md`](docs/Localization.md).

Print the deterministic avatar and Hell Blast balance report:
```bash
./build-tests/gameplay_regression_tests --balance-only
```

The baseline compares summon rosters, spell plans and avatar passives on common fixtures.
Catastrophic is represented by Hell Blast in the spell score; Bard, Monocle, Luck and
Telepath are reported in a separate passive column.

Run the real-rules full-match balance laboratory on Windows (one published seed,
four seat rotations by default):
```powershell
.\scripts\run-balance-lab.ps1
```

Run the independent doctrine/difficulty comparison matrix (60 isolated matches
for one seed, 480 for all eight):
```powershell
.\scripts\run-balance-matrix.ps1
.\scripts\run-balance-matrix.ps1 -SeedCount 8
```

Run controlled avatar substitutions in fixed clan slots (52 isolated matches
for one seed, 416 for all eight):
```powershell
.\scripts\run-balance-cohorts.ps1
.\scripts\run-balance-cohorts.ps1 -SeedCount 8
```

Each match runs in a fresh child process; it exports detailed event telemetry,
versioned JSON, per-player CSV, a concise text report and full replays for
failures/statistical outliers. The fixed seed list, legal clan/seat rotation
contract and larger-run guidance are in
[`docs/BalanceLab.md`](docs/BalanceLab.md). The current controlled strength view,
including direct fixed-clan and provisional clan-adjusted tiers, is in
[`docs/BalanceTiers.md`](docs/BalanceTiers.md).

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
