# Battle and Adventure Rules

This document is the implementation contract for the current battle and map
phase. The original game rules remain in `docs/Manual.txt`.

## Combat order

1. Hellblast creatures reduce every opposing creature by the configured Hell
   Blast loyalty damage when the parties enter combat. Both sides resolve this
   ability before casualties are removed.
2. The defending territory fires first. Ignore Missiles creatures cannot be
   selected, and Force Shield reduces the received ranged damage.
3. Attacking and defending creature missiles are planned before damage is
   applied, then resolved simultaneously. A shooter killed by the opposing
   volley still completes its planned shot.
4. Battle AI selects the opening leader. If the selected creature has First
   Strike, creature missile combat is skipped and the attacking party acts
   first in melee. Otherwise the defending party or territory acts first.
5. Melee continues action by action until the attacking party is defeated or
   the territory is captured. Initiative and targets are recalculated after
   every exchange.

Supporting creatures add one melee point each to the acting leader. Matching
Merge creatures add one defense point per additional matching creature. Swarm
attacks every member of the opposing party. Fire Shield retaliates after a
melee hit. Mighty Blow has a 25 percent chance to add three melee and always
deals at least one damage when it triggers. Regeneration restores full loyalty
at the start of the map phase.

## Battle AI

Battle decisions live in `aibattle.cpp`; damage, hit chances and speciality
effects remain exclusively in `battle.cpp`. A `BattleAttackPlan` contains only
stable battle-unit IDs and never mutates either party.

The planner currently provides:

- opening and normal initiative orders from attack, defense, loyalty, ranged
  strength and tactical specialities;
- guaranteed First Strike opening priority without incorrectly preserving that
  bonus during later exchanges;
- simultaneous ranged assignments with projected loyalty, Force Shield and
  Ignore Missiles, redirecting later shots after a predicted kill;
- dynamic melee attacker and target selection, including support, Merge,
  Swarm, Fire Shield risk, lethal damage, overkill and enemy threat;
- separate attacker and defender profiles. AI players use their avatar's
  `ai_profile`; automatic resolution for human players remains `balanced`.

Aggressive AI favors immediate damage and kills. Economic AI puts durable
creatures first and avoids wasting damage. Control AI prioritizes dangerous
specialities and high-threat targets. Plans are deterministic with stable-ID
tie breaking.

Individual tactical choices are bounded heuristics. `BattleForecast` adds a
deterministic, seeded Monte Carlo layer around the real resolver: it runs combat
on copied parties and reports capture chance plus attacker and defender
survival. The injected local random source never consumes the real game's
global random stream. Ranged fire is projected exactly for each volley; melee
is replanned from actual surviving units after every simulated action.

## Land Claim

Mahjong payments award the winner Land Claim points against each paying clan.
The amount is the winning hand score multiplied by that opponent's payment
factor.

A territory can be claimed when all of these conditions hold:

- It has a valid enemy owner and is not the Tower of Four Winds.
- The player has points against that owner equal to the territory value.
- It borders a territory owned by the player; the Tower does not establish a
  border for this rule.
- No player has a creature on the target, including invisible creatures.

Legal territories have a yellow outline. Select one and left-click it again to
claim it. Territory ownership and per-clan claim balances are persisted in save
games. Older saves without these fields use the theme's initial owners and zero
claim balances.

Undo restores the current player's complete army snapshot, including movement
and dismissed creatures, then reverses all Land Claims made during that map
turn and refunds their cost. Finishing the turn commits the snapshot.

## Adventure AI

Adventure AI uses the same validated client actions as a human player; its
planner does not mutate armies or territory owners directly.

Its current strategy is intentionally deterministic and auditable:

- Build an `AdventureClaimPlan` from cost, land value, power, friendly borders
  and the new frontier opened by the deed. Claims are recalculated after every
  successful purchase.
- Build an `AdventureMovePlan` from target value, path length, town and party
  strength, power, border threat, simulated capture chance and expected
  attacker survival. A direct enemy step is legal only when its forecast meets
  the active profile's minimum win chance.
- Predict an `AdventureThreat` when an enemy complete party can reach an owned
  territory on its next map turn and its simulated capture chance crosses the
  profile's response threshold. The estimate also records expected enemy
  survival, territory value, power and response distance.
- Build one `AdventureTurnPlan` for every friendly party. The highest threats
  receive a profile-limited defensive reserve: an existing garrison holds, or
  the nearest legal party reinforces the territory.
- Coordinate the remaining attacks without overbooking the three-creature
  destination capacity. Aggressive AI may concentrate two parties on one
  target; other profiles spread their forces across distinct objectives.

Only planned claims and movements are sent through `client2Adventure`, which
revalidates ownership, points, path, movement and capacity against current game
state. Threat and whole-turn planning are pure calculations and cannot move a
unit by themselves.

Counter-move prediction is deliberately bounded to one enemy map response. It
evaluates each reachable enemy party independently rather than building a full
multi-player minimax tree. Every turn rebuilds both forecasts and orders from
the resulting board instead.

### Difficulty and player forecast

AI difficulty is selected when starting a new game, on the wizard and clan
screen. Clicking the `AI Level` panel cycles through `Easy`, `Normal` and
`Hard`. The selected value belongs to that game and is stored in the save;
older saves without the field load as `Normal`.

Difficulty does not modify creature statistics, town statistics, rune income
or random rolls. It changes the AI planning budget:

- `Easy` runs 8 battle samples, looks only at the immediate spell cast and
  limits coordinated attacks to one party per target.
- `Normal` runs 16 battle samples and uses each behavior profile's normal spell
  horizon and army coordination.
- `Hard` runs 48 battle samples, extends spell projection by one step (up to
  four casts) and lets a profile coordinate one additional party per target.

Behavior profiles and difficulty remain independent. An aggressive wizard on
`Easy` still behaves aggressively, but plans less deeply; on `Hard` it keeps
the same doctrine and evaluates it more thoroughly.

During the player's movement phase, select movable creatures on one of your
territories and hover an enemy territory. The right panel shows simulated
capture chance and expected attacker survival for exactly the selected party.
The preview uses the same battle resolver and the current difficulty's sample
budget, runs on copies and never consumes the live game RNG. It also uses the
player's filtered `LocalData`, so invisible enemies are not disclosed by the
preview. The same forecast remains visible while dragging the party flag.

## Spellcasting AI

Mahjong spell decisions live in a separate, testable layer. The planner builds
a `SpellCastPlan`; only the existing validated `client2Mahjong` path is allowed
to apply it, so AI players follow the same costs, rune requirements, target
rules, immunities and cast limits as human players.

The candidate set combines avatar spells with `cast_*` abilities supplied by
creatures in the current army. The planner evaluates:

- healing, lethal damage and the expected value after Magic Resistance;
- friendly buffs, enemy curses and targets that cannot receive a spell;
- single-target Dispel and territory-wide Mass Dispel by enchantment value;
- party damage and control, player effects and Telepath immunity to Silence;
- legal Teleport destinations using border pressure, territory value and party
  capacity.

### Behavior profiles

Each avatar selects a reusable `ai_profile` in `avatars.json`:

- `balanced` uses neutral weights and a two-turn planning horizon.
- `aggressive` favors damage, mobility and offensive summons, spends power more
  freely, attacks from a 35 percent forecast, reacts only to threats at 75
  percent and may concentrate two armies.
- `economic` favors rune generation, durable cost-effective summons and larger
  power and map-defense reserves. It attacks from 75 percent but starts
  defending at 30 percent; ordinary spells rarely delay a legal summon.
- `control` favors curses, dispels, player denial and creatures with spell-cast
  abilities, strategic territories and early border defense. It attacks from
  60 percent, responds to threats from 25 percent and projects spell
  combinations up to three turns.

Profile weights are centralized in `aiprofile.cpp`, so spell selection, summon
ordering, Land Claim, map targeting, battle risk and Battle AI tactics share
one behavior model instead of maintaining unrelated special cases. Economic AI
also preserves a Land Claim point reserve; aggressive AI is willing to spend
it completely.

### Multi-turn projection

The selected plan separates immediate score from discounted future score and
records its projected follow-up casts. Recognized chains include control into
damage, Dispel into disable, Mass Panic into party damage, Scry/Silence into
Random Discard, Blind Ambition into Healing, and buff/Teleport deployment.

Projection never mutates the board. It may consider a known spell whose rune is
not currently available because future draws are unknown, but it reserves the
power cost of every projected step. Only the currently legal first action is
sent through `client2Mahjong`; the next turn rebuilds and validates the plan
from current state. This avoids blindly committing to a combo after a target
dies, moves, resists a spell or becomes otherwise invalid.

Summoning remains the normal economic priority. A high-value tactical cast can
override it; otherwise a useful spell is the fallback when no legal summon is
available. Equal choices use stable IDs, making decisions deterministic and
regression-testable. This is a bounded heuristic rather than an exhaustive game
tree: exact board simulation and opponent counter-play remain future work.

## Regression checks

`tests/gameplay_regression_tests.cpp` covers spell lifecycle, AI profiles and
AI difficulty parsing, planning budgets and save persistence,
multi-turn combo projection and target selection, Adventure threat prediction,
defensive reserve and coordinated capacity, Battle AI initiative, ranged
allocation, profile targets, forecast determinism, copied-state purity and RNG
isolation, Land Claim balances and legality, full Adventure Undo, simultaneous
ranged combat, First Strike, Merge, Swarm, Regeneration, movement and Teleport
invariants.
