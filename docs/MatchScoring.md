# Match scoring

The final strategic score is a deterministic rules contract shared by the live
Game Summary screen and headless simulations. It is intentionally calculated
from authoritative end-of-match state rather than UI state or a simplified
balance model.

## Category scores

Each player receives five raw category scores:

| Category | Raw score |
| --- | --- |
| Territory | Sum of the point values of all owned territories |
| Summon Circle | Number of owned territories marked as summoning circles |
| Unit | Sum of the summon costs of all surviving creatures |
| Spell Point | Unspent spell points |
| Land Claim | Unspent land-claim points against all three opposing clans |

Negative resource values are clamped to zero. Stable avatar, clan and wind IDs
identify a player; display names are never used as identity.

## Ranks and total

Each category is ranked independently, highest raw score first. Ties use
competition ranking: `1, 1, 3, 4`. With four players, a category rank awards
`4, 3, 2, 1` standing points respectively (`player count - rank + 1`). Tied
players receive the same standing points and the next rank is skipped.

`Total Score` is the sum of the five category standing-point awards. The final
rank is a second competition ranking over that total, again highest first.
This keeps land, armies and both resource economies relevant without allowing
the much larger numeric unit-cost scale to dominate every match.

The Game Summary screen displays each raw category score beside its category
rank, followed by total standing points and final rank. `Simulation::MatchResult`
contains the same structured result. Any formula change must update this file,
the pure tie/rank regression and the deterministic full-match canary together.

The original 1998 executable and installed Win98 saves are useful behavioral
references, but no recoverable source-level final-map scoring contract exists in
the inherited engine. This documented Reborn contract is therefore explicit
instead of presenting an uncertain reverse-engineering guess as classic fact.
