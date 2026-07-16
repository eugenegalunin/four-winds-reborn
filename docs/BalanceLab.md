# Balance laboratory

The balance laboratory runs the real authoritative Mahjong, Adventure and
battle phase machine without rendering. It is a development tool, not a second
simplified rules model and not a blocking thousands-of-games unit test.

## Baseline matrix

The first published roster is Nucrus, Lakkho, Ziag and Dayla. Their legal clans
are fixed to red, yellow, aqua and purple respectively. For every seed, the
schedule rotates the four avatars through East, South, West and North. Persons
are stored in that same canonical wind order, so this also rotates any engine
iteration-order seat effect.

Difficulty and doctrine are independent axes. `native` uses each avatar's
theme-defined behavior profile. A forced `balanced`, `aggressive`, `economic`
or `control` cell applies that one doctrine to all four AI players while
retaining the same avatars, clans, winds and seed. The override exists only for
the lifetime of that match and is restored before the child exits. Forced cells
therefore measure how the same roster and fixtures respond to one doctrine;
they are not mixed-profile head-to-head games.

For more flexible rosters, every legal one-avatar-per-clan bijection is combined
with the four seat rotations. Unsupported avatar/clan combinations are never
invented. The schedule rejects duplicate avatars, duplicate seeds and rosters
that cannot cover all four clans.

The version-1 published seed list is stable:

```text
363134977  363134978  363134979  363134980
1331122689 1331122690 1331122691 1331122692
```

The first seed is the cross-platform deterministic canary. Adding seeds is
allowed; changing or reordering these values requires an explicit baseline
version change because comparisons use prefix subsets.

## Telemetry and reports

Every match records status, watchdog counters, phase counts, emitted actions,
RNG draws, final authoritative hash and canonical MatchScore. Raw authoritative
score snapshots are also captured after Adventure phases 5 and 10 and at the
final phase, providing early/middle/late views of territory value, summoning
circles, surviving unit value, spell points and land-claim points.

Event telemetry comes from the authoritative results of validated actions, not
AI intentions or log text. Accepted `MahjongSummon` and `MahjongCast` messages
produce a per-match event timeline and per-avatar creature/spell histograms.
Casualties are exact disappeared `battleUnit` IDs between authoritative ticks;
explicit Adventure dismiss actions are reported separately as dismissals.
The JSON also retains peak/final unit counts and final parties grouped by land.
CSV exposes the compact per-player counts; JSON is the detailed source for
timing, spell/creature names and party composition.

Reports contain:

- `balance-report.json`: versioned configuration, per-match data, stage
  snapshots and per-avatar aggregates;
- `balance-players.csv`: one row per match/avatar for external analysis;
- `balance-report.txt`: concise rank-one rate, Wilson 95% interval, mean final
  rank, mean total score and event-count means;
- `replays/match-NNNNNN.json`: complete deterministic match replay, emitted only
  for a failed match or a statistical outlier selected by the current report.

The default Windows runner is process-isolated: every scheduled match starts in
a fresh child process and writes a validated temporary record. A separate merge
process refuses missing, reordered, truncated, non-contiguous or wrong-seed
records before producing aggregate reports. This confines a native crash or
leaked process state to one match and preserves its child diagnostics. Full
replays contain the initial authoritative state, every Mahjong/Adventure driver
tick and every explicit phase transition, with the expected state hash and
success/exception outcome checked after each step. A forced doctrine is stored
in replay schema 3 and restored only for replay execution, so an outlier remains
reproducible outside its original matrix process.

Replay retention is deterministic. All non-complete statuses are selected.
With at least eight completed samples, match length and each avatar's total
score use Tukey's 1.5 IQR fence; a match beyond a fence is selected. Each match
in `balance-report.json` lists `replay_retention_reasons`, which is the manifest
for the current run even if an older output directory still contains unrelated
replay files.

The Windows wrapper suppresses verbose engine diagnostics by default. Pass
`-KeepEngineLog` during a focused reproduction to retain them in a run-specific
`balance-engine-*.log`; expected rejected movement candidates make this file
very large. A child process or merge failure always writes this diagnostic and
keeps the temporary isolated records. Pass `-KeepMatchRecords` to preserve all
successful child records too; normally only selected failure/outlier replays
survive the merge. Headless matches suppress rolling gameplay recovery checkpoints and
restore normal recovery behavior on return, so the laboratory never touches the
player's normal recovery slots.

A tied final rank of one counts as a rank-one finish for every tied avatar. The
interval therefore describes the probability of finishing at rank one, not an
exclusive zero-sum win share. Small samples intentionally produce wide
intervals.

## Running on Windows

Build the normal test target once, then run one published seed (four matches):

```powershell
.\scripts\build-windows.ps1
.\scripts\run-balance-lab.ps1
```

Run all eight published seeds (32 matches) into a chosen directory:

```powershell
.\scripts\run-balance-lab.ps1 -SeedCount 8 -OutputDirectory diagnostics\balance-baseline
```

Select a single explicit scenario when diagnosing one axis value:

```powershell
.\scripts\run-balance-lab.ps1 -SeedCount 2 -Difficulty Hard -BehaviorProfile Control
```

Run the complete factorial comparison. With the default one seed this is 15
cells and 60 isolated matches; all eight published seeds produce 480 matches:

```powershell
.\scripts\run-balance-matrix.ps1
.\scripts\run-balance-matrix.ps1 -SeedCount 8 -OutputDirectory diagnostics\balance-matrix-full
```

A focused matrix can select subsets without changing its report contract:

```powershell
.\scripts\run-balance-matrix.ps1 -Profiles Native,Balanced -Difficulties Normal
```

Every cell retains its normal `balance-report.json`, `balance-players.csv` and
`balance-report.txt` under `scenarios/<difficulty>-<profile>/`. The matrix root
adds:

- `balance-matrix.json`: versioned axes, scenario links, avatar aggregates and
  both comparison deltas;
- `balance-matrix.csv`: flat scenario/avatar rows for external analysis;
- `balance-matrix.txt`: concise human comparison table.

Profile deltas subtract `native` at the same difficulty. Difficulty deltas
subtract `normal` for the same profile. If a focused subset omits the applicable
reference cell, that delta is null/blank instead of silently changing the
baseline.

The script deliberately uses the developer test executable under
`build-windows`; balance tooling is not shipped inside the player Release
package. One seed means four child processes; all eight published seeds mean 32.
The fixed CTest canary remains small and in-process, while the manual runner is
the supported path for the larger isolated matrix.
