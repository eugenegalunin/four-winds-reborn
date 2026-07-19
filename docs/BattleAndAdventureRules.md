# Battle and Adventure Rules

This document is the implementation contract for the current battle and map
phase. The original game rules remain in `docs/Manual.txt`.

## Combat order

1. Hellblast creatures reduce every opposing creature by the configured Hell
   Blast loyalty damage when the parties enter combat. Both sides resolve this
   ability before casualties are removed.
2. The human attacker selects an opening leader; Auto Resolve uses Battle AI's
   recommendation through the same choice primitive. If the selected creature
   has First Strike, creature missile combat is skipped and the attacking party
   acts first in melee.
3. The defending territory fires after the leader is selected. Ignore Missiles
   creatures cannot be selected, and Force Shield reduces received ranged
   damage. Territory fire is not skipped by First Strike.
4. Without First Strike, every surviving attacker shooter receives a manual
   target assignment. No damage is applied until all assignments are present;
   the defending plan is built from the same untouched state and both creature
   volleys then resolve simultaneously. A shooter killed by the opposing volley
   still completes its planned shot, including deterministic overkill when no
   other legal target existed in the pre-volley state.
5. Melee continues action by action until the attacking party is defeated or
   the territory is captured. Initiative and targets are recalculated after
   every exchange.

Supporting creatures add one melee point each to the acting leader. Matching
Merge creatures add one defense point per additional matching creature. Swarm
attacks every member of the opposing party. Fire Shield retaliates after a
melee hit. Mighty Blow has a 25 percent chance to add three melee and always
deals at least one damage when it triggers. Regeneration restores full loyalty
at the start of the map phase.

## Player-controlled battle session

Human attackers enter a serializable authoritative `BattleSession`. The state
machine pauses for the opening leader, then once for every attacker missile
assignment, and again before each attacker melee action until the battle ends.
Missile assignments are saved without applying a partial volley, so save and
recovery are supported even between two shooters.

The choice screen uses stable battle-unit IDs. Yellow outlines are legal
actors, red outlines are legal targets and blue outlines show the Battle AI
recommendation. A leader click commits the target-free opening choice; missile
and melee choices select an actor and target. The current phase, latest
authoritative event and concise damage/hit/Mighty Blow preview remain visible.
`Enter` accepts the recommendation; the visible `Auto Resolve` control or
`Escape` delegates every remaining choice to Battle AI.

The selected action and Auto Resolve both call the same damage and speciality
primitives in `battle.cpp`. Invalid or stale actor/target IDs are rejected
without changing state. The pending session stores parties, town, phase, round,
profiles and emitted strikes in the authoritative save, recovery checkpoint and
replay hash, so saving while the choice is visible is supported.

The final combat playback shows phase/type, event progress and exact applied
damage. It can be paused and resumed, and speed cycles through 1x, 2x and 4x.
Playback timing uses a virtual clock and never blocks the Windows UI thread with
per-strike sleeps. Auto Resolve stops safely after 1024 consecutive choices
without damage instead of hanging forever on a pathological deterministic roll.

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
  Ignore Missiles, redirecting later shots after a predicted kill while still
  assigning every shooter against the untouched state if all legal targets are
  already projected dead;
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

## Map visibility

Invisible creatures are removed only from the copied `LocalData` sent to a
specific observer. The authoritative armies always retain them. An ordinary
observer reveals an invisible enemy only when their own Adventure Party,
Griffon, or other See Invisible creature occupies an adjacent territory. The
detector's territory owner does not matter, and another player's detector does
not share visibility.

Ziag's Monocle reveals every invisible creature globally. Players always see
their own creatures, and creatures at the Tower of Four Winds are public. The
full rationale and the resolved manual ambiguities are recorded in
`docs/RulesDecisions.md`.

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

The default AI difficulty is selected in **Settings**. A large portrait card in
the left panel and its arrows cycle through `Training`, `Easy`, `Normal`, `Hard`
and `Unfair`; the right panel keeps the language, presentation, audio and
display controls. The setting is copied into a newly started game. From that
point the selected value belongs to that game and is stored in its save, so
changing the default does not rewrite existing saves. Older saves without the
field load as `Normal`.

`Training` is a teaching policy rather than a weaker approximation of normal
play. Its AI players do not claim land, enter enemy territory or cast offensive
spells. They may reinforce friendly territory, summon creatures and heal their
own armies, so the Rune Game and economy still demonstrate their ordinary
rules without turning the map into a war.

`Easy`, `Normal` and `Hard` modify only bounded decision quality around an
unchanged `Normal` reference. They do not modify creature or town statistics,
rune or spell-point income, legal actions, random rolls or the information
available to an AI:

- `Easy` runs 8 battle samples, looks only at the immediate spell cast and
  limits coordinated attacks to one party per target. It preserves goal runes
  less strongly, keeps a smaller ordinary spell-point and defensive reserve,
  waits for clearer map threats and accepts riskier attacks. Routine offensive
  casts receive a penalty, while lethal casts remain available. On a stable
  subset of turns it
  may choose the second plan only when that plan scores at least 88 percent of
  the best one; this bounded mistake is deterministic rather than favorable or
  hostile RNG.
- `Normal` runs 16 battle samples and retains the established behavior
  profile's spell horizon, rune valuation, army coordination, threat response,
  battle thresholds and exact best-plan selection.
- `Hard` runs 48 battle samples, extends spell projection by one step (up to
  four casts), retains wider strategic goal and action beams and lets a profile
  coordinate one additional party per target. It values future goal runes more,
  keeps a larger ordinary spell-point reserve, responds to weaker visible map
  threats, keeps a larger defensive reserve, requires a safer forecast before
  attacking and penalizes wasted battle damage more heavily. It also hides the
  player's map battle forecast.
- `Unfair` is deliberately asymmetric. It runs 128 battle samples, keeps wider
  strategic and spell beams, values offensive magic more strongly and accepts
  dangerous attacks when pressure is useful. Every AI begins with 500 bonus
  spell points and receives 125 spell points plus 100 Land Claim points against
  every rival clan after each Rune Game part. The mode still uses the ordinary
  legal actions and combat resolver; it receives no favorable rolls and cannot
  inspect hidden runes or invisible enemy units.

Behavior profiles and difficulty remain independent. An aggressive wizard on
`Easy` still prefers pressure, but does not spend an ordinary attack spell every
time it can; on `Hard` it keeps the same doctrine and evaluates it more
thoroughly. `Unfair` preserves the doctrine too, then adds its documented
economy handicap.

On `Training`, `Easy` and `Normal`, select movable creatures on one of your
territories and hover an enemy territory during the movement phase. The right
panel shows simulated capture chance and expected attacker survival for exactly
the selected party. The preview uses the same battle resolver and the current
difficulty's sample budget, runs on copies and never consumes the live game RNG.
It also uses the player's filtered `LocalData`, so invisible enemies are not
disclosed by the preview. The same forecast remains visible while dragging the
party flag. On `Hard` and `Unfair` the panel receives no outcome percentages;
AI players still use their own decision budget when planning actions.

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

## Strategic AI observation and rune goals

Strategic Mahjong and summon planning begins from an immutable copy of the
observer-filtered `LocalData` view. Hidden hands and undetected invisible armies
therefore cannot change an AI score. The planner ranks a difficulty-bounded set
of summon and spell rune goals, recording rune progress, visible remaining
copies, a bounded draw-completion estimate, mana gap and component scores in a
deterministic trace. Discard selection and Nucrus Luck both reuse these rune
values instead of optimizing Mahjong combinations in isolation.

Optional Chao, Pung and Kong calls use the same strategic intent. The AI scores
the completed exposed set, its point reward and progress toward a Mahjong hand
against the value of uncast runes that would be removed from future summon and
spell goals. `Game` is always accepted; other calls may prefer `Pass`. When
several Chao variants are legal, the selected variant preserves the strongest
strategic runes. The authoritative Chao mutation moves exactly one copy of each
sequence rune out of the concealed hand.

Summon planning ranks `(creature, destination)` candidates as resulting parties,
not as independent creature and land lists. Static profile value is combined
with party role coverage, movement coherence, matching Merge defense, visible
frontier pressure and land value. A globally unique creature hidden from the
observer is never consulted by the planner; authoritative validation may reject
that candidate, after which the AI tries the next observer-legal candidate.

The same observation now feeds a bounded `TurnPlan`. It keeps one best
destination per summonable creature and one observer-safe plan per currently
castable spell, combines the immediate action score with the ranked rune intent,
and adds a small Adventure follow-up score. That follow-up is a strategic target
hint rather than an executable move order: it uses public land ownership and
paths, the AI's own parties and only defenders present in filtered `LocalData`.
Difficulty limits the retained action beam to 3, 6 or 10 branches for Easy,
Normal and Hard. A rejected hidden global unique therefore falls through to the
next retained branch without revealing why it failed. The deterministic trace
records every retained branch, its immediate, intent and follow-up components,
and the visible defense used for its Adventure target.

## Regression checks

`tests/gameplay_regression_tests.cpp` covers spell lifecycle, AI profiles and
AI difficulty parsing, planning budgets and save persistence,
multi-turn combo projection and target selection, Adventure threat prediction,
defensive reserve and coordinated capacity, Battle AI initiative, ranged
allocation, profile targets, forecast determinism, copied-state purity and RNG
isolation, Land Claim balances and legality, full Adventure Undo, simultaneous
ranged combat, First Strike, Merge, Swarm, Regeneration, movement and Teleport
invariants. BattleSession coverage includes legal and forged stable IDs,
target-free leader selection, save/load at opening/melee and midway through a
missile volley, explicit town targets, repeated manual rounds, simultaneous
overkill, Auto Resolve equivalence, authoritative event history and the complete
Adventure action flow. Deterministic BattleSession scenarios cover every
battle-resolver speciality: creature Hellblast, First Strike, Ignore Missiles,
Merge, Mighty Blow, Fire Shield and Swarm. Non-combat specialities (movement,
visibility, spell resistance, Devotion, Regeneration and creature-cast Mahjong
effects) remain covered in their owning phase rather than being simulated by
BattleSession. Focused contracts additionally lock the exact persistent stat
effects of Smoke, Demonic Compulsion, Mass Panic, Reduction, Battle Fury, Dust
Cloud, Heroism, Brilliant Lights and Magical Aura; Dispel cleanup; all three
directed rune draws; the inclusive 25%/90% Magic Resistance chances; Devotion;
and every creature-to-spell speciality mapping. Mahjong call tests cover
strategic Pass, Kong/Pung selection, Chao variant choice and duplicate-safe
concealed-hand removal.
