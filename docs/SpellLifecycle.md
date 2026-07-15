# Spell Effect Lifecycle

Rune spells use three distinct lifecycles.

## Instant spells

Instant spells apply their result immediately and are not stored on the target.
Examples include Healing, Lightning Bolt, Hell Blast, Teleport and Dispel Magic.

## Player effects

Player effects use `duration` as a number of that player's completed turns.
Silence and Scry Runes expire after the affected player discards on the final
turn. Mana Fog follows the same rule, but its casting turn does not consume the
first blocked turn.

One-shot player effects use `duration: 1` and consume it when they trigger.
Random Discard is removed after forcing the target's next discard, while the
three Summon Rune effects are removed after replacing the target's next draw.

## Creature enchantments

A spell with `persistent: true` is stored on the target creature.

- Without `duration`, it remains until Dispel Magic, Mass Dispel Magic, creature
  death or removal, or the end of the game.
- With `duration: N`, it remains for `N` complete Adventure phases. Paralyze has
  `duration: 1`, so it blocks movement during the next map phase and then ends.
- Casting the same spell on the same creature replaces the existing instance;
  it does not stack. Different spell IDs may stack.
- Persistent and timed enchantments are serialized in save games.

Permanent enchantments are encoded internally with duration `-1`. Duration `0`
means expired. Saves created before this rule used `0` for permanent effects and
are migrated when loaded.
