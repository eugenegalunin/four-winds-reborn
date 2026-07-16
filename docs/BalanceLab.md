# Balance laboratory

The balance laboratory runs the real authoritative Mahjong, Adventure and
battle phase machine without rendering. It is a development tool, not a second
simplified rules model and not a blocking thousands-of-games unit test.

## Baseline matrix

The first published roster is Nucrus, Lakkho, Ziag and Dayla. Their legal clans
are fixed to Maitha, Kartha, Iz and Marz respectively. For every seed, the
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

Select an explicit legal four-avatar roster when isolating roster and clan
effects. The supervisor asks the C++ scheduler for its real match count, so a
flexible roster with several legal clan bijections is not truncated to four
seat rotations:

```powershell
.\scripts\run-balance-lab.ps1 -Roster Orachi,Lakkho,Ziag,Dayla
```

Run controlled clan cohorts before attributing a fixed-roster result to an
avatar. The baseline is run once; each selected clan slot then substitutes three
other avatars that legally support that clan while the other three avatars,
forced doctrine, difficulty, seeds and seat rotations remain fixed:

```powershell
.\scripts\run-balance-cohorts.ps1
.\scripts\run-balance-cohorts.ps1 -SeedCount 8 -OutputDirectory diagnostics\balance-cohorts-full
.\scripts\run-balance-cohorts.ps1 -Clans Iz -SeedCount 2
```

The cohort root exports `balance-cohorts.json`, `balance-cohorts.csv` and
`balance-cohorts.txt`. Deltas are candidate minus the baseline avatar in the
same clan slot; a negative mean-rank delta is an improvement. `native` is
deliberately unavailable here because it would change doctrine together with
the avatar and defeat the control. Machine-readable reports use the canonical
IDs `maitha`, `kartha`, `iz` and `marz`; the `clan` display column uses the
corresponding capitalized names.

Verify one retained replay, one scenario, or a complete matrix after the run:

```powershell
.\scripts\verify-balance-replays.ps1 diagnostics\balance-matrix-full
```

The verifier restores the replay's initial authoritative state, reapplies every
recorded validated action and system transition, checks every intermediate state
hash, and requires the final hash to match the retained match record. This is a
development integrity check; a future player-facing replay viewer is a separate
feature.

## Published baseline 1

The first complete matrix was run on Windows Release from game commit `67e440f`
and engine commit `f7368e2` on 2026-07-16. It used all eight published seeds,
five behavior-profile cells and three difficulty cells: 480 isolated matches in
about 29 minutes. All 480 completed, producing 88,871 authoritative events and
34 deterministically retained outlier replays. The retained replays were then
verified independently through the command above.

Native-profile results pool Easy, Normal and Hard, so each avatar has 96 fixed
fixtures. A tied first place counts for every tied avatar.

| Avatar | Rank-one finishes | Rank-one rate | Wilson 95% | Mean rank | Mean total |
| --- | ---: | ---: | ---: | ---: | ---: |
| Lakkho | 53/96 | 55.21% | 45.25-64.76% | 1.531 | 16.583 |
| Nucrus | 40/96 | 41.67% | 32.31-51.66% | 1.708 | 15.573 |
| Dayla | 7/96 | 7.29% | 3.58-14.29% | 2.938 | 12.104 |
| Ziag | 0/96 | 0.00% | 0.00-3.85% | 3.573 | 9.781 |

Forcing one common doctrine across the same fixtures confirms that the profile
axis materially changes play. Economic averaged 8.17 summons and 31.81 casts per
player; Control averaged 4.76 summons and 40.87 casts. Aggressive finished with
only 0.24 surviving units per player on average, while Economic finished with
2.33. Difficulty also acts as a computation budget: mean emitted actions rose
from 3,699.81 on Easy to 3,824.90 on Normal and 3,948.96 on Hard, while mean total
scores remained close (13.775, 13.866 and 13.756).

Seat rotation is working as a control. Across the native fixtures the four wind
rank-one rates ranged from 21.88% to 31.25%, much smaller than the observed
avatar spread, and every avatar occupied every wind equally.

This baseline is evidence of a real imbalance, not yet evidence for one numeric
fix. The four baseline avatars are tied to four different clans. Ziag remains
last under every forced doctrine (including 11/96 rank-one finishes under common
Balanced behavior), so native-profile matchup alone does not explain the gap;
avatar roster, passive and fixed clan/map position remain confounded.

## Controlled avatar/clan cohort 1

The first controlled cohort was run on Windows Release on 2026-07-16 from the
cohort implementation based on game commit `767bbe1` and engine commit
`f7368e2`. It used all eight published seeds, `Normal` difficulty and one common
`Balanced` doctrine. The baseline ran once, then three legal candidates replaced
one avatar at a time in each fixed clan slot: 13 scenarios, 416 isolated matches
and 76,376 authoritative events in about 21 minutes. All 416 completed. All 16
retained outlier replays then reproduced independently with every intermediate
and final hash intact.

Each row has 32 fixtures. Rank-one delta is in percentage points; rank and total
deltas are candidate minus the named baseline in the same clan. A negative rank
delta is better.

| Clan | Baseline | Candidate | Rank one | Delta | Mean rank | Delta | Mean total | Delta |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Maitha | Nucrus | Nucrus | 43.75% | - | 1.813 | - | 15.688 | - |
| Maitha | Nucrus | Orachi | 37.50% | -6.25 pp | 2.281 | +0.469 | 14.719 | -0.969 |
| Maitha | Nucrus | Javed | 18.75% | -25.00 pp | 2.688 | +0.875 | 13.719 | -1.969 |
| Maitha | Nucrus | Logun | 31.25% | -12.50 pp | 2.281 | +0.469 | 14.438 | -1.250 |
| Kartha | Lakkho | Lakkho | 34.38% | - | 2.281 | - | 14.594 | - |
| Kartha | Lakkho | Niana | 37.50% | +3.13 pp | 2.188 | -0.094 | 14.344 | -0.250 |
| Kartha | Lakkho | Javed | 28.13% | -6.25 pp | 2.313 | +0.031 | 14.094 | -0.500 |
| Kartha | Lakkho | Logun | 25.00% | -9.38 pp | 2.125 | -0.156 | 14.188 | -0.406 |
| Iz | Ziag | Ziag | 9.38% | - | 3.188 | - | 12.094 | - |
| Iz | Ziag | Niana | 12.50% | +3.13 pp | 2.906 | -0.281 | 13.031 | +0.938 |
| Iz | Ziag | Kierac | 28.13% | +18.75 pp | 2.313 | -0.875 | 14.000 | +1.906 |
| Iz | Ziag | Logun | 18.75% | +9.38 pp | 2.906 | -0.281 | 12.625 | +0.531 |
| Marz | Dayla | Dayla | 21.88% | - | 2.313 | - | 14.688 | - |
| Marz | Dayla | Orachi | 34.38% | +12.50 pp | 2.000 | -0.313 | 14.844 | +0.156 |
| Marz | Dayla | Kierac | 31.25% | +9.38 pp | 2.313 | +0.000 | 14.156 | -0.531 |
| Marz | Dayla | Logun | 40.63% | +18.75 pp | 2.219 | -0.094 | 14.438 | -0.250 |

The controls materially narrow the diagnosis. Nucrus remains strongest among
the tested Maitha candidates; the Kartha substitutions are close to Lakkho; and
the Marz result depends on which outcome is emphasized. The clearest
directional signal is Iz: replacing Ziag with Kierac improves rank-one rate,
mean rank and mean total together. The Wilson intervals still overlap and this
is not yet evidence for a specific numeric edit. Inspect paired fixture deltas
and representative Ziag/Kierac replays before changing one passive, creature,
spell or scoring family at a time.

The resulting direct fixed-clan tiers and provisional clan-adjusted nine-avatar
ranking are maintained in [`BalanceTiers.md`](BalanceTiers.md). The legacy
`--balance-only` table remains a fast data/plan diagnostic, not the current tier
list.
