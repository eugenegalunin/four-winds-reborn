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

Reports contain:

- `balance-report.json`: versioned configuration, per-match data, stage
  snapshots and per-avatar aggregates;
- `balance-players.csv`: one row per match/avatar for external analysis;
- `balance-report.txt`: concise rank-one rate, Wilson 95% interval, mean final
  rank and mean total score.

The Windows wrapper suppresses verbose engine diagnostics by default. Pass
`-KeepEngineLog` during a focused failed/outlier reproduction to retain them in
`balance-engine.log`; expected rejected movement candidates make this file very
large. Headless matches suppress rolling gameplay recovery checkpoints and
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

The script deliberately uses the developer test executable under
`build-windows`; balance tooling is not shipped inside the player Release
package. Large process-isolated batches and retained full replays for failures
and statistical outliers remain later 15.5 work. Until that isolation exists,
use this runner for bounded local matrices only.
