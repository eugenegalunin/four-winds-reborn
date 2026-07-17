# Balance laboratory

The balance laboratory runs the real authoritative Mahjong, Adventure and
battle phase machine without rendering. It is a development tool, not a second
simplified rules model and not a blocking thousands-of-games unit test.

## Baseline matrix

The first published roster is Nucrus, Lakkho, Ziag and Dayla. Their legal clans
are fixed to Red, Yellow, Aqua and Purple respectively. For every seed, the
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
.\scripts\run-balance-cohorts.ps1 -Clans Aqua -SeedCount 2
```

The cohort root exports `balance-cohorts.json`, `balance-cohorts.csv` and
`balance-cohorts.txt`. Deltas are candidate minus the baseline avatar in the
same clan slot; a negative mean-rank delta is an improvement. `native` is
deliberately unavailable here because it would change doctrine together with
the avatar and defeat the control. Machine-readable reports use the canonical
IDs `red`, `yellow`, `aqua` and `purple`; the `clan` display column uses the
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
| Red | Nucrus | Nucrus | 43.75% | - | 1.813 | - | 15.688 | - |
| Red | Nucrus | Orachi | 37.50% | -6.25 pp | 2.281 | +0.469 | 14.719 | -0.969 |
| Red | Nucrus | Javed | 18.75% | -25.00 pp | 2.688 | +0.875 | 13.719 | -1.969 |
| Red | Nucrus | Logun | 31.25% | -12.50 pp | 2.281 | +0.469 | 14.438 | -1.250 |
| Yellow | Lakkho | Lakkho | 34.38% | - | 2.281 | - | 14.594 | - |
| Yellow | Lakkho | Niana | 37.50% | +3.13 pp | 2.188 | -0.094 | 14.344 | -0.250 |
| Yellow | Lakkho | Javed | 28.13% | -6.25 pp | 2.313 | +0.031 | 14.094 | -0.500 |
| Yellow | Lakkho | Logun | 25.00% | -9.38 pp | 2.125 | -0.156 | 14.188 | -0.406 |
| Aqua | Ziag | Ziag | 9.38% | - | 3.188 | - | 12.094 | - |
| Aqua | Ziag | Niana | 12.50% | +3.13 pp | 2.906 | -0.281 | 13.031 | +0.938 |
| Aqua | Ziag | Kierac | 28.13% | +18.75 pp | 2.313 | -0.875 | 14.000 | +1.906 |
| Aqua | Ziag | Logun | 18.75% | +9.38 pp | 2.906 | -0.281 | 12.625 | +0.531 |
| Purple | Dayla | Dayla | 21.88% | - | 2.313 | - | 14.688 | - |
| Purple | Dayla | Orachi | 34.38% | +12.50 pp | 2.000 | -0.313 | 14.844 | +0.156 |
| Purple | Dayla | Kierac | 31.25% | +9.38 pp | 2.313 | +0.000 | 14.156 | -0.531 |
| Purple | Dayla | Logun | 40.63% | +18.75 pp | 2.219 | -0.094 | 14.438 | -0.250 |

The controls materially narrow the diagnosis. Nucrus remains strongest among
the tested Red candidates; the Yellow substitutions are close to Lakkho; and
the Purple result depends on which outcome is emphasized. The clearest
directional signal is Aqua: replacing Ziag with Kierac improves rank-one rate,
mean rank and mean total together. The Wilson intervals still overlap and this
is not yet evidence for a specific numeric edit. The paired fixture and replay
review required by this result is documented below; it confirms the direction
without authorizing an automatic balance change.

The resulting direct fixed-clan tiers and provisional clan-adjusted nine-avatar
ranking are maintained in [`BalanceTiers.md`](BalanceTiers.md). The legacy
`--balance-only` table remains a fast data/plan diagnostic, not the current tier
list.

## Paired Ziag/Kierac Aqua review

The controlled Aqua cohort was repeated on Windows Release on 2026-07-16 from
game commit `7c244ee` and engine commit `f7368e2`, after fixing action telemetry
to follow the avatar currently occupying each wind. The repeat completed all
128 isolated matches: the Ziag baseline plus Niana, Kierac and Logun in the same
Aqua slot, using eight seeds, four seat rotations, `Normal` difficulty and the
common `Balanced` doctrine. All ten retained outlier replays reproduced every
intermediate and final hash.

The telemetry fix changed attribution only. Every final state hash and event
count in all 128 fixtures matches the pre-fix run. The paired report can be
reproduced from any two compatible schema-2 reports:

```powershell
.\scripts\compare-balance-pairs.ps1 `
  -BaselineReport diagnostics\balance-cohorts-aqua\scenarios\baseline\balance-report.json `
  -CandidateReport diagnostics\balance-cohorts-aqua\scenarios\aqua-kierac\balance-report.json `
  -BaselineAvatar Ziag -CandidateAvatar Kierac `
  -OutputDirectory diagnostics\balance-cohorts-aqua\paired-ziag-kierac
```

The script requires identical seeds, difficulty, profile and fixture keys. It
also verifies that the target roster slot, target clan and all three opponents
stay fixed, derives starting wind from the scheduled seat rotation, then exports
per-pair CSV plus JSON/text summaries by wind, seed, stage, score category,
telemetry and opponent impact.

### Paired outcome

| Outcome | Ziag | Kierac | Paired result |
| --- | ---: | ---: | --- |
| Rank-one finishes | 1/32 | 15/32 | +43.75 percentage points |
| Mean final rank | 3.375 | 1.906 | -1.469; better/same/worse in 25/3/4 fixtures |
| Mean total | 12.000 | 14.531 | +2.531; better/same/worse in 23/2/7 fixtures |

Every starting wind favors Kierac on mean rank and total. Mean total deltas are
`+2.500` East, `+2.375` North, `+2.625` South and `+2.625` West, so the result is
not a single-seat artifact. The advantage is already visible early, peaks in
the middle game and persists at the end:

| Stage | Mean rank delta | Mean total delta | Rank better/same/worse |
| --- | ---: | ---: | ---: |
| Early | -0.938 | +2.125 | 21/5/6 |
| Middle | -1.438 | +3.531 | 24/5/3 |
| Late | -1.469 | +2.531 | 25/3/4 |

Final category deltas identify units and territory as the main standing gains:

| Category | Raw score delta | Standing-point delta | Category-rank delta |
| --- | ---: | ---: | ---: |
| Territory | +470.313 | +0.781 | -0.781 |
| Summon circles | +0.188 | +0.125 | -0.125 |
| Units | +309.375 | +1.125 | -1.125 |
| Spell points | +15.938 | +0.125 | -0.125 |
| Land claims | +89.688 | +0.375 | -0.375 |

Corrected action telemetry makes the mechanism clearer. Kierac summons `11.406`
creatures per fixture against Ziag's `3.438`, reaches `4.594` peak units against
`1.188`, and finishes with `3.406` against `0.250`. Ziag casts far more spells
(`56.000` against `26.938`), so Kierac's advantage is not spell volume. Kierac
absorbs more casualties (`8.000` against `3.188`) while still retaining a large
army. His most common summons are Skeleton Horde (99 total), Knight Templar
(93), Sand Wraith (105) and Shadow (28); Ziag summons Sand Wraith 87 times but
has no equivalent cheap Skull/Sword breadth in his four-creature roster.

The candidate also suppresses every fixed opponent: their mean totals fall by
`1.344` (Nucrus), `1.500` (Lakkho) and `1.781` (Dayla), with each mean rank
worsening by `0.375`.

Representative retained replays bound the interpretation:

- schedule 8 is the common positive pattern: Kierac finishes rank 2 with four
  surviving units after six summons and only two losses; Ziag finishes rank 4
  with no units;
- schedule 12 is a Ziag counterexample: Ziag wins with total 19 after 85 casts,
  while Kierac still retains four units but finishes second on 16;
- schedule 22 shows that Kierac is not intrinsically invulnerable: he leads the
  early stage, then suffers eight casualties, loses territory and falls to
  rank 4 with total 8.

The review therefore localizes the current Aqua gap to reliable summon access and
post-casualty army persistence, not wind assignment or excessive Kierac spell
casting. It does not by itself authorize a balance edit. Any rebalance experiment
should change exactly one creature-roster or summoning-access family, rerun these
same 32 pairs and retain the current reports as the comparison baseline.
