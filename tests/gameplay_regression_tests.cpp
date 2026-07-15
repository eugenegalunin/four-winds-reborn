#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <map>

#include "aiadventure.h"
#include "aibattle.h"
#include "aispell.h"
#include "battle.h"

namespace GameData
{
bool loadIndexes(const JsonObject &);
extern std::vector<CreatureInfo> creaturesInfo;
extern std::vector<SpellInfo> spellsInfo;
extern std::vector<LandInfo> landsInfo;
extern std::vector<AvatarInfo> avatarsInfo;
extern LocalPlayers gamers;
JsonObject toJsonObject(const JsonObject &);
}

namespace
{
int failures = 0;

void expect(bool condition, const char* message)
{
    if(condition) return;

    std::cerr << "FAIL: " << message << '\n';
    ++failures;
}

BattleCreature creature(int uid, int move, int loyalty = 3, int ranger = 1)
{
    CreatureStat stat;
    stat.attack = 2;
    stat.ranger = ranger;
    stat.defense = 1;
    stat.loyalty = loyalty;
    stat.move = move;
    return BattleCreature(Clan::Red, Creature::SkeletonHorde, CreatureSkill(stat), uid);
}

BattleCreature combatCreature(const Clan & clan, const Creature & id, int uid,
                              int attack, int ranger, int defense, int loyalty, int move = 1)
{
    CreatureStat stat;
    stat.attack = attack;
    stat.ranger = ranger;
    stat.defense = defense;
    stat.loyalty = loyalty;
    stat.move = move;
    return BattleCreature(clan, id, CreatureSkill(stat), uid);
}

template<class T>
std::vector<T> loadIndexed(const char* filename)
{
    JsonContentFile file(std::string(FOUR_WINDS_SOURCE_DIR) + "/themes/default/json/gamedata/" + filename);
    expect(file.isValid(), "game data JSON must parse");

    const std::list<T> values = file.toArray().toStdList<T>();
    std::vector<T> result(values.size() + 1);
    for(const auto & value : values) result[value.id.index()] = value;
    return result;
}

const SpellInfo* findSpell(const std::list<SpellInfo> & spells, const Spell & spell)
{
    auto it = std::find_if(spells.begin(), spells.end(), [&](const SpellInfo & info)
    {
        return info.id == spell;
    });
    return it != spells.end() ? &*it : nullptr;
}

const AvatarInfo* findAvatar(const std::list<AvatarInfo> & avatars, const Avatar & avatar)
{
    auto it = std::find_if(avatars.begin(), avatars.end(), [&](const AvatarInfo & info)
    {
        return info.id == avatar;
    });
    return it != avatars.end() ? &*it : nullptr;
}

bool hasSpell(const AvatarInfo & avatar, const Spell & spell)
{
    return std::find(avatar.spells.begin(), avatar.spells.end(), spell) != avatar.spells.end();
}

AI::SpellCastPlan chooseOnly(const LocalPlayer & player, Spell::spell_t spell)
{
    Spells spells;
    spells.push_back(Spell(spell));
    return AI::chooseSpellCast(player, spells);
}

Spells spellSet(std::initializer_list<Spell::spell_t> values)
{
    Spells spells;
    for(Spell::spell_t spell : values) spells.push_back(Spell(spell));
    return spells;
}

void testDifficultyRules()
{
    expect(AI::difficultyFromString("EASY") == AI::Difficulty::Easy,
           "AI difficulty parsing must be case insensitive");
    expect(AI::difficultyFromString("") == AI::Difficulty::Normal,
           "missing AI difficulty must preserve Normal compatibility for old saves");
    expect(AI::nextDifficulty(AI::Difficulty::Easy) == AI::Difficulty::Normal &&
           AI::nextDifficulty(AI::Difficulty::Normal) == AI::Difficulty::Hard &&
           AI::nextDifficulty(AI::Difficulty::Hard) == AI::Difficulty::Easy,
           "AI difficulty selector must cycle Easy, Normal, Hard");

    expect(AI::difficultyRules(AI::Difficulty::Easy).battleForecastSamples == 8 &&
           AI::difficultyRules(AI::Difficulty::Normal).battleForecastSamples == 16 &&
           AI::difficultyRules(AI::Difficulty::Hard).battleForecastSamples == 48,
           "higher AI difficulty must spend more work on battle forecasts");
    expect(AI::spellPlanningHorizon(AI::BehaviorProfile::Control, AI::Difficulty::Easy) == 1 &&
           AI::spellPlanningHorizon(AI::BehaviorProfile::Control, AI::Difficulty::Normal) == 3 &&
           AI::spellPlanningHorizon(AI::BehaviorProfile::Control, AI::Difficulty::Hard) == 4,
           "AI difficulty must control spell combination depth");
    expect(AI::maximumPartiesPerTarget(AI::BehaviorProfile::Aggressive,
                                       AI::Difficulty::Easy) == 1 &&
           AI::maximumPartiesPerTarget(AI::BehaviorProfile::Aggressive,
                                       AI::Difficulty::Hard) == 3,
           "hard AI must coordinate more attacking parties than easy AI");

    GameData::setAIDifficulty(AI::Difficulty::Hard);
    expect(GameData::toJsonObject(JsonObject()).getString("ai:difficulty") == "hard",
           "save data must persist the selected AI difficulty");
    GameData::setAIDifficulty(AI::Difficulty::Normal);
}

void testSelectedPartyMovement()
{
    BattleArmy army;
    BattleParty source(Clan::Red, Land::Corzen);
    BattleCreature first = creature(250, 2);
    BattleCreature second = creature(251, 2);
    first.setSelected(true);
    second.setSelected(true);
    expect(source.join(first) && source.join(second), "selected movement source fixture must join");
    army.push_back(source);

    const std::vector<int> moved = army.moveSelectedCreatures(Land::Corzen, Land::Zubrus);
    const BattleParty* destination = army.findPartyConst(Land::Zubrus);
    expect(moved.size() == 2 && destination && destination->findBattleUnitConst(250) &&
           destination->findBattleUnitConst(251) && !army.findPartyConst(Land::Corzen),
           "selected movement must move the whole group to a legal destination");

    BattleArmy blocked;
    BattleParty blockedSource(Clan::Red, Land::Corzen);
    BattleCreature blockedFirst = creature(252, 2);
    BattleCreature blockedSecond = creature(253, 2);
    blockedFirst.setSelected(true);
    blockedSecond.setSelected(true);
    expect(blockedSource.join(blockedFirst) && blockedSource.join(blockedSecond),
           "blocked movement source fixture must join");
    blocked.push_back(blockedSource);

    BattleParty crowded(Clan::Red, Land::Zubrus);
    expect(crowded.join(creature(254, 2)) && crowded.join(creature(255, 2)),
           "blocked movement destination fixture must join");
    blocked.push_back(crowded);

    expect(blocked.moveSelectedCreatures(Land::Corzen, Land::Zubrus).empty() &&
           blocked.findPartyConst(Land::Corzen)->findBattleUnitConst(252) &&
           blocked.findPartyConst(Land::Corzen)->findBattleUnitConst(253) &&
           blocked.findPartyConst(Land::Zubrus)->count() == 2,
           "failed group movement must leave both parties unchanged");
    expect(!blocked.canMoveCreature(*blocked.findBattleUnitConst(252), Land::Corzen, Lands()),
           "empty movement paths must fail safely");
}

void testGateMovement()
{
    BattleArmy supportGate;
    BattleParty supportSource(Clan::Red, Land::Corzen);
    BattleCreature carol = combatCreature(Clan::Red, Creature::GreatCarol, 280, 3, 0, 4, 4, 1);
    BattleCreature ally = combatCreature(Clan::Red, Creature::Durlock, 281, 2, 0, 2, 3, 2);
    carol.setSelected(false);
    ally.setSelected(true);
    expect(supportSource.join(carol) && supportSource.join(ally), "Gate support fixture must join");
    supportGate.push_back(supportSource);

    expect(supportGate.canGateParty(Land::Corzen, Land::Maithaius),
           "Carol must open a Gate to a friendly summoning circle");
    const std::vector<int> supportMoved =
        supportGate.moveSelectedCreatures(Land::Corzen, Land::Maithaius);
    const BattleParty* supportOrigin = supportGate.findPartyConst(Land::Corzen);
    const BattleCreature* gatedAlly = supportGate.findBattleUnitConst(281);
    expect(supportMoved.size() == 1 && supportMoved.front() == 281 && supportOrigin &&
           supportOrigin->findBattleUnitConst(280) &&
           supportGate.findPartyConst(Land::Maithaius)->findBattleUnitConst(281),
           "Carol must be able to Gate selected allies while staying behind");
    expect(gatedAlly && gatedAlly->freeMovePoint() == 1,
           "Gate must consume exactly one movement point from each moved creature");

    BattleArmy groupGate;
    BattleParty groupSource(Clan::Red, Land::Corzen);
    BattleCreature groupCarol = combatCreature(Clan::Red, Creature::GreatCarol, 282, 3, 0, 4, 4, 1);
    BattleCreature groupAlly = combatCreature(Clan::Red, Creature::Durlock, 283, 2, 0, 2, 3, 1);
    groupCarol.setSelected(true);
    groupAlly.setSelected(true);
    expect(groupSource.join(groupCarol) && groupSource.join(groupAlly), "Gate group fixture must join");
    groupGate.push_back(groupSource);

    const std::vector<int> groupMoved =
        groupGate.moveSelectedCreatures(Land::Corzen, Land::PyramusReach);
    const BattleParty* groupDestination = groupGate.findPartyConst(Land::PyramusReach);
    expect(groupMoved.size() == 2 && groupMoved.front() == 283 && groupMoved.back() == 282 &&
           groupDestination && groupDestination->findBattleUnitConst(282) &&
           groupDestination->findBattleUnitConst(283) && !groupGate.findPartyConst(Land::Corzen),
           "Gate must move companions before Carol so the whole selected group arrives");
    expect(groupDestination && groupDestination->findBattleUnitConst(282)->freeMovePoint() == 0 &&
           groupDestination->findBattleUnitConst(283)->freeMovePoint() == 0,
           "Gate must charge one movement point to Carol and every companion");

    BattleArmy invalidGate;
    BattleParty invalidSource(Clan::Red, Land::Corzen);
    BattleCreature invalidCarol = combatCreature(Clan::Red, Creature::GreatCarol, 284, 3, 0, 4, 4, 1);
    invalidCarol.setSelected(true);
    expect(invalidSource.join(invalidCarol), "invalid Gate fixture must join");
    invalidGate.push_back(invalidSource);
    expect(!invalidGate.canGateParty(Land::Corzen, Land::Baliphon) &&
           !invalidGate.canGateParty(Land::Corzen, Land::Inkartha),
           "Gate must reject friendly non-power lands and enemy summoning circles");
    expect(invalidGate.moveSelectedCreatures(Land::Corzen, Land::Baliphon).empty(),
           "an invalid Gate destination must not bypass normal movement distance");

    expect(invalidGate.canGateParty(Land::Corzen, Land::TowerOf4Winds) &&
           invalidGate.moveSelectedCreatures(Land::Corzen, Land::TowerOf4Winds).size() == 1,
           "Gate must allow Carol to enter the Tower of Four Winds");

    LocalPlayer gateAI;
    gateAI.avatar = Avatar::Logun;
    gateAI.clan = Clan::Red;
    gateAI.wind = Wind::East;
    BattleParty aiSource(Clan::Red, Land::Corzen);
    expect(aiSource.join(combatCreature(Clan::Red, Creature::GreatCarol, 285, 20, 0, 10, 10, 1)) &&
           aiSource.join(combatCreature(Clan::Red, Creature::Durlock, 286, 20, 2, 10, 10, 1)),
           "Gate AI fixture must join");
    gateAI.army.push_back(aiSource);
    Lands gateTargets;
    gateTargets << Land::Sunspot;
    const AI::AdventureMovePlan gatePlan = AI::chooseAdventureMove(
        gateAI, *gateAI.army.findPartyConst(Land::Corzen), AI::BehaviorProfile::Balanced,
        gateTargets);
    expect(gatePlan.isValid() && gatePlan.target == Land(Land::Sunspot) &&
           gatePlan.destination == Land(Land::PyramusReach) && !gatePlan.engagesEnemy,
           "Adventure AI must use Gate when a friendly summoning circle shortens its route");
}

void testSpellCastingAI()
{
    GameData::gamers.clear();

    LocalPlayer red;
    red.avatar = Avatar::Orachi;
    red.clan = Clan::Red;
    red.wind = Wind::East;
    red.points = 1000;

    BattleParty allies(Clan::Red, Land::Corzen);
    BattleCreature wounded = combatCreature(Clan::Red, Creature::Durlock, 270, 3, 2, 2, 4);
    wounded.applyDamage(2);
    expect(allies.join(wounded), "spell AI wounded ally fixture must join");
    expect(allies.join(combatCreature(Clan::Red, Creature::SkeletonHorde, 271, 3, 0, 1, 3)),
           "spell AI non-ranged ally fixture must join");
    expect(allies.join(combatCreature(Clan::Red, Creature::EarthElemental, 272, 1, 0, 1, 3)),
           "spell AI creature-caster fixture must join");
    red.army.push_back(allies);

    LocalPlayer yellow;
    yellow.avatar = Avatar::Lakkho;
    yellow.clan = Clan::Yellow;
    yellow.wind = Wind::South;
    yellow.points = 900;

    BattleParty enemies(Clan::Yellow, Land::Zubrus);
    expect(enemies.join(combatCreature(Clan::Yellow, Creature::Durlock, 280, 5, 1, 2, 1)),
           "spell AI lethal enemy fixture must join");
    BattleCreature buffedEnemy = combatCreature(Clan::Yellow, Creature::SkeletonHorde, 281, 2, 1, 2, 4);
    expect(buffedEnemy.applySpell(Spell(Spell::BattleFury)),
           "spell AI dispel fixture must accept a beneficial enchantment");
    expect(enemies.join(buffedEnemy), "spell AI enchanted enemy fixture must join");
    yellow.army.push_back(enemies);

    GameData::gamers.push_back(red);
    GameData::gamers.push_back(yellow);
    LocalPlayer & caster = GameData::gamers.front();
    caster.stones.add(GameStone(Stone("wind1"), false));

    const Spells available = AI::availableSpellCasts(caster);
    expect(std::find(available.begin(), available.end(), Spell(Spell::ScryRunes)) != available.end(),
           "spell AI must include spells granted by creatures in the army");
    expect(AI::behaviorProfile(caster) == AI::BehaviorProfile::Aggressive,
           "spell AI must load Orachi's aggressive profile from the theme");
    expect(AI::behaviorProfile(GameData::gamers.back()) == AI::BehaviorProfile::Economic,
           "spell AI must load Lakkho's economic profile from the theme");
    expect(AI::behaviorProfileFromString("unknown") == AI::BehaviorProfile::Balanced,
           "an unknown spell AI profile must safely fall back to balanced");
    expect(AI::creatureSummonScore(Creature(Creature::MazRa), AI::BehaviorProfile::Aggressive) >
           AI::creatureSummonScore(Creature(Creature::MazRa), AI::BehaviorProfile::Economic),
           "aggressive AI must value an expensive damage summon more than economic AI");
    expect(AI::creatureSummonScore(Creature(Creature::EarthElemental), AI::BehaviorProfile::Control) >
           AI::creatureSummonScore(Creature(Creature::EarthElemental), AI::BehaviorProfile::Balanced),
           "control AI must value creature spell abilities when ordering summons");

    const AI::SpellCastPlan healing = chooseOnly(caster, Spell::Healing);
    expect(healing.spell == Spell(Spell::Healing) && healing.unit == 270 && healing.land == Land(Land::Corzen),
           "spell AI must heal a wounded friendly creature");

    const AI::SpellCastPlan lightning = chooseOnly(caster, Spell::LightningBolt);
    expect(lightning.spell == Spell(Spell::LightningBolt) && lightning.unit == 280 &&
           lightning.land == Land(Land::Zubrus),
           "spell AI must prioritize a lethal Lightning Bolt target");
    const ClientCastSpell lightningCommand = AI::spellCastCommand(lightning);
    expect(lightningCommand.spell() == Spell(Spell::LightningBolt) &&
           lightningCommand.land() == Land(Land::Zubrus) && lightningCommand.unit() == 280,
           "spell AI plan must convert to the correct creature-target command");

    const AI::SpellCastPlan guidance = chooseOnly(caster, Spell::Guidance);
    expect(guidance.spell == Spell(Spell::Guidance) && guidance.unit == 270,
           "spell AI must cast Guidance only on a creature with ranged attack");

    const AI::SpellCastPlan forceShield = chooseOnly(caster, Spell::ForceShield);
    expect(forceShield.spell == Spell(Spell::ForceShield) && forceShield.unit == 270,
           "spell AI must find a useful Force Shield target");

    const AI::SpellCastPlan dispel = chooseOnly(caster, Spell::DispelMagic);
    expect(dispel.spell == Spell(Spell::DispelMagic) && dispel.unit == 281,
           "spell AI must remove a beneficial enchantment from an enemy");

    const AI::SpellCastPlan massDispel = chooseOnly(caster, Spell::MassDispel);
    expect(massDispel.spell == Spell(Spell::MassDispel) && massDispel.land == Land(Land::Zubrus),
           "spell AI must choose the land with a profitable Mass Dispel");

    const AI::SpellCastPlan hellBlast = chooseOnly(caster, Spell::HellBlast);
    expect(hellBlast.spell == Spell(Spell::HellBlast) && hellBlast.land == Land(Land::Zubrus) &&
           (hellBlast.unit == 280 || hellBlast.unit == 281),
           "spell AI must encode an enemy party through one of its battle units");

    const AI::SpellCastPlan teleport = chooseOnly(caster, Spell::Teleport);
    expect(teleport.spell == Spell(Spell::Teleport) && teleport.unit == 270 &&
           teleport.land != Land(Land::Corzen) &&
           (teleport.land.isTowerWinds() || GameData::landInfo(teleport.land).clan == Clan(Clan::Red)),
           "spell AI must select a legal friendly Teleport destination");

    const AI::SpellCastPlan silence = chooseOnly(caster, Spell::Silence);
    expect(silence.spell == Spell(Spell::Silence) && silence.target == Avatar(Avatar::Lakkho),
           "spell AI must select another player for Silence");
    const ClientCastSpell silenceCommand = AI::spellCastCommand(silence);
    expect(silenceCommand.target() == Avatar(Avatar::Lakkho),
           "spell AI plan must convert to the correct player-target command");

    const Spells damageOrControl = spellSet({ Spell::LightningBolt, Spell::Silence });
    const Spells noFuture;
    const AI::SpellCastPlan aggressiveChoice = AI::chooseSpellCast(
        caster, damageOrControl, AI::BehaviorProfile::Aggressive, noFuture);
    const AI::SpellCastPlan controlChoice = AI::chooseSpellCast(
        caster, damageOrControl, AI::BehaviorProfile::Control, noFuture);
    expect(aggressiveChoice.spell == Spell(Spell::LightningBolt),
           "aggressive spell AI must prefer lethal damage over player control");
    expect(controlChoice.spell == Spell(Spell::Silence),
           "control spell AI must prefer Silence over the same damage option");
    expect(AI::shouldCastBeforeSummon(aggressiveChoice) && AI::shouldCastBeforeSummon(controlChoice),
           "high-value aggressive and control plans must be allowed to override summoning");

    const AI::SpellCastPlan controlCombo = AI::chooseSpellCast(
        caster, spellSet({ Spell::DispelMagic }), AI::BehaviorProfile::Control,
        spellSet({ Spell::Paralyze, Spell::LightningBolt }));
    expect(controlCombo.spell == Spell(Spell::DispelMagic) && controlCombo.unit == 281,
           "control spell AI combo must start by dispelling the enchanted target");
    expect(controlCombo.followUps.size() == 2 &&
           controlCombo.followUps[0].spell == Spell(Spell::Paralyze) &&
           controlCombo.followUps[0].unit == 281 &&
           controlCombo.followUps[1].spell == Spell(Spell::LightningBolt) &&
           controlCombo.followUps[1].unit == 281,
           "control spell AI must project Dispel, Paralyze and Lightning on one target");
    expect(controlCombo.futureScore > 0 &&
           controlCombo.score == controlCombo.immediateScore + controlCombo.futureScore,
           "spell AI combo score must separate immediate and discounted future value");
    const BattleCreature* projectedTarget = GameData::getBattleCreature(281);
    expect(projectedTarget && projectedTarget->isAffectedSpell(Spell::BattleFury) &&
           !projectedTarget->isAffectedSpell(Spell::Paralyze),
           "spell AI projection must not mutate the current board state");

    GameData::gamers.back().avatar = Avatar::Javed;
    expect(!chooseOnly(caster, Spell::Silence).isValid(),
           "spell AI must respect Javed's Telepath immunity to Silence");
    expect(chooseOnly(caster, Spell::ScryRunes).target == Avatar(Avatar::Javed),
           "spell AI must consider Scry Runes useful even against an empty hand");
    expect(chooseOnly(caster, Spell::RandomDiscard).target == Avatar(Avatar::Javed),
           "spell AI must select another player for Random Discard");
    expect(chooseOnly(caster, Spell::ManaFog).spell == Spell(Spell::ManaFog),
           "spell AI must evaluate all-player control spells");
    expect(chooseOnly(caster, Spell::DrawNumber).spell == Spell(Spell::DrawNumber),
           "spell AI must evaluate self-target rune spells");

    for(BattleCreature* creature : caster.army.toBattleCreatures())
        expect(creature->applySpell(Spell(Spell::MysticalFountain)),
               "spell AI Mystical Fountain fixture must accept the enchantment");
    expect(!chooseOnly(caster, Spell::MysticalFountain).isValid(),
           "spell AI must recognize every stored Mystical Fountain variant");

    GameData::gamers.back().army.clear();
    BattleParty durableEnemies(Clan::Yellow, Land::Zubrus);
    expect(durableEnemies.join(combatCreature(Clan::Yellow, Creature::Durlock, 290, 1, 0, 1, 5)),
           "spell AI economic profile fixture must join");
    GameData::gamers.back().army.push_back(durableEnemies);
    const Spells damageOrRune = spellSet({ Spell::LightningBolt, Spell::DrawNumber });
    const AI::SpellCastPlan aggressiveEconomyChoice = AI::chooseSpellCast(
        caster, damageOrRune, AI::BehaviorProfile::Aggressive, noFuture);
    const AI::SpellCastPlan economicChoice = AI::chooseSpellCast(
        caster, damageOrRune, AI::BehaviorProfile::Economic, noFuture);
    expect(aggressiveEconomyChoice.spell == Spell(Spell::LightningBolt),
           "aggressive spell AI must prefer pressure over rune economy");
    expect(economicChoice.spell == Spell(Spell::DrawNumber),
           "economic spell AI must prefer rune economy over non-lethal damage");
    expect(!AI::shouldCastBeforeSummon(economicChoice),
           "an ordinary economic spell must not delay a legal summon");

    const int savedPoints = caster.points;
    caster.points = 100;
    expect(!AI::chooseSpellCast(caster, spellSet({ Spell::DrawNumber }),
                                AI::BehaviorProfile::Economic, noFuture).isValid(),
           "economic spell AI must preserve its configured mana reserve");
    caster.points = savedPoints;

    caster.affected.insert(AffectedSpell(Spell(Spell::Silence), 3));
    expect(!chooseOnly(caster, Spell::LightningBolt).isValid(),
           "Silence must block spell AI planning");
    caster.affected.clear();
    caster.affected.insert(AffectedSpell(Spell(Spell::ManaFog), 3));
    expect(!chooseOnly(caster, Spell::LightningBolt).isValid(),
           "Mana Fog must block spell AI planning");

    expect(!AI::spellCastCommand(AI::SpellCastPlan()).spell().isValid(),
           "an empty spell plan must convert safely without indexing spell data");
}

void testAdventureProfiles()
{
    GameData::gamers.clear();

    LocalPlayer red;
    red.avatar = Avatar::Orachi;
    red.clan = Clan::Red;
    red.wind = Wind::East;
    red.addLandClaimPoints(Clan::Yellow, 3000);
    red.addLandClaimPoints(Clan::Purple, 3000);
    red.addLandClaimPoints(Clan::Aqua, 3000);
    BattleParty attackers(Clan::Red, Land::Greenbaw);
    expect(attackers.join(combatCreature(Clan::Red, Creature::Durlock, 300, 12, 2, 4, 8, 3)),
           "Adventure AI attacker fixture must join");
    red.army.push_back(attackers);

    LocalPlayer yellow;
    yellow.avatar = Avatar::Lakkho;
    yellow.clan = Clan::Yellow;
    yellow.wind = Wind::South;
    BattleParty kernDefenders(Clan::Yellow, Land::Kern);
    expect(kernDefenders.join(combatCreature(Clan::Yellow, Creature::KnightTemplar,
                                               301, 5, 0, 3, 5)),
           "Adventure AI defended target fixture must join");
    yellow.army.push_back(kernDefenders);

    LocalPlayer aqua;
    aqua.avatar = Avatar::Ziag;
    aqua.clan = Clan::Aqua;
    aqua.wind = Wind::West;

    LocalPlayer purple;
    purple.avatar = Avatar::Dayla;
    purple.clan = Clan::Purple;
    purple.wind = Wind::North;

    GameData::gamers.push_back(red);
    GameData::gamers.push_back(yellow);
    GameData::gamers.push_back(aqua);
    GameData::gamers.push_back(purple);

    const LocalPlayer & player = GameData::gamers.front();
    const BattleParty & party = player.army.front();
    const AI::AdventureClaimPlan aggressiveClaim =
        AI::chooseAdventureClaim(player, AI::BehaviorProfile::Aggressive);
    const AI::AdventureClaimPlan economicClaim =
        AI::chooseAdventureClaim(player, AI::BehaviorProfile::Economic);
    const AI::AdventureClaimPlan balancedClaim =
        AI::chooseAdventureClaim(player, AI::BehaviorProfile::Balanced);
    expect(aggressiveClaim.land == Land(Land::Corimar),
           "aggressive Adventure AI must claim the highest-pressure frontier");
    expect(economicClaim.land == Land(Land::Sunspot),
           "economic Adventure AI must claim the cheapest safe expansion");
    expect(balancedClaim.land == Land(Land::Sunspot),
           "balanced Adventure AI must preserve the cheapest-claim baseline");
    expect(GameData::landInfo(Land::Corimar).clan == Clan(Clan::Yellow) &&
           GameData::landInfo(Land::Sunspot).clan == Clan(Clan::Purple),
           "Adventure AI claim planning must not mutate territory ownership");

    LocalPlayer lowBudget = player;
    lowBudget.landClaims = LandClaims();
    lowBudget.addLandClaimPoints(Clan::Purple, 300);
    expect(AI::chooseAdventureClaim(lowBudget, AI::BehaviorProfile::Aggressive).isValid(),
           "aggressive Adventure AI must spend its final claim points");
    expect(!AI::chooseAdventureClaim(lowBudget, AI::BehaviorProfile::Economic).isValid(),
           "economic Adventure AI must preserve its Land Claim reserve");

    Lands nearbyTargets;
    nearbyTargets << Land::Kern << Land::Corimar;
    const AI::AdventureMovePlan aggressiveMove =
        AI::chooseAdventureMove(player, party, AI::BehaviorProfile::Aggressive, nearbyTargets);
    const AI::AdventureMovePlan economicMove =
        AI::chooseAdventureMove(player, party, AI::BehaviorProfile::Economic, nearbyTargets);
    expect(aggressiveMove.target == Land(Land::Kern) &&
           aggressiveMove.destination == Land(Land::Kern) && aggressiveMove.engagesEnemy,
           "aggressive Adventure AI must accept a calculated attack on a defended land");
    expect(economicMove.target == Land(Land::Corimar) &&
           economicMove.destination == Land(Land::Corimar) && economicMove.engagesEnemy,
           "economic Adventure AI must prefer a winnable empty territory");

    Lands defendedTarget;
    defendedTarget << Land::Kern;
    expect(AI::chooseAdventureMove(player, party, AI::BehaviorProfile::Aggressive,
                                   defendedTarget).isValid(),
           "aggressive Adventure AI must tolerate the configured battle risk");
    const AI::AdventureMovePlan economicDefendedMove =
        AI::chooseAdventureMove(player, party, AI::BehaviorProfile::Economic, defendedTarget);
    expect(economicDefendedMove.isValid() &&
           economicDefendedMove.captureChance >=
               AI::behaviorRules(AI::BehaviorProfile::Economic).minimumBattleWinChance,
           "Battle forecast must allow an economic attack that the old strength ratio rejected");

    BattleParty unsafeRaid(Clan::Red, Land::Greenbaw);
    expect(unsafeRaid.join(combatCreature(Clan::Red, Creature::Durlock, 302, 1, 0, 0, 1)),
           "Adventure AI unsafe forecast fixture must join");
    expect(!AI::chooseAdventureMove(player, unsafeRaid, AI::BehaviorProfile::Aggressive,
                                    defendedTarget).isValid(),
           "Adventure AI must reject a genuinely unsafe simulated battle");

    Lands strategicTargets;
    strategicTargets << Land::Corimar << Land::Inkartha;
    const AI::AdventureMovePlan controlMove =
        AI::chooseAdventureMove(player, party, AI::BehaviorProfile::Control, strategicTargets);
    expect(controlMove.target == Land(Land::Inkartha),
           "control Adventure AI must prioritize a strategic power territory");
    expect(player.army.front().land() == Land(Land::Greenbaw),
           "Adventure AI movement planning must not mutate the army");
}

void testAdventureCoordination()
{
    GameData::gamers.clear();

    LocalPlayer red;
    red.avatar = Avatar::Orachi;
    red.clan = Clan::Red;
    red.wind = Wind::East;

    BattleParty borderGuard(Clan::Red, Land::Corzen);
    expect(borderGuard.join(combatCreature(Clan::Red, Creature::Durlock, 310, 1, 0, 1, 1)),
           "Adventure coordinator border guard fixture must join");
    red.army.push_back(borderGuard);

    BattleParty mainForce(Clan::Red, Land::Greenbaw);
    expect(mainForce.join(combatCreature(Clan::Red, Creature::Durlock, 311, 8, 1, 3, 5, 2)),
           "Adventure coordinator main force fixture must join");
    expect(mainForce.join(combatCreature(Clan::Red, Creature::SkeletonHorde, 312, 5, 1, 2, 4, 2)),
           "Adventure coordinator main force second unit fixture must join");
    expect(mainForce.join(combatCreature(Clan::Red, Creature::KnightTemplar, 313, 4, 0, 3, 5, 2)),
           "Adventure coordinator main force third unit fixture must join");
    red.army.push_back(mainForce);

    BattleParty flank(Clan::Red, Land::Talon);
    expect(flank.join(combatCreature(Clan::Red, Creature::Durlock, 314, 6, 1, 2, 4, 2)),
           "Adventure coordinator flank fixture must join");
    red.army.push_back(flank);

    LocalPlayer yellow;
    yellow.avatar = Avatar::Lakkho;
    yellow.clan = Clan::Yellow;
    yellow.wind = Wind::South;
    BattleParty counterAttack(Clan::Yellow, Land::Zubrus);
    expect(counterAttack.join(combatCreature(Clan::Yellow, Creature::KnightTemplar,
                                              320, 4, 1, 3, 5, 1)),
           "Adventure coordinator counter-attack fixture must join");
    yellow.army.push_back(counterAttack);

    GameData::gamers.push_back(red);
    GameData::gamers.push_back(yellow);

    const LocalPlayer & player = GameData::gamers.front();
    const std::vector<AI::AdventureThreat> economicThreats =
        AI::predictAdventureThreats(player, AI::BehaviorProfile::Economic);
    const auto corzenThreat = std::find_if(economicThreats.begin(), economicThreats.end(),
                                           [](const AI::AdventureThreat & threat)
    {
        return threat.land == Land(Land::Corzen);
    });
    expect(corzenThreat != economicThreats.end() &&
           corzenThreat->enemyOrigin == Land(Land::Zubrus) &&
           corzenThreat->captureChance >=
               AI::behaviorRules(AI::BehaviorProfile::Economic).minimumThreatCaptureChance &&
           corzenThreat->enemyStrength < corzenThreat->defenseStrength,
           "economic Adventure AI must predict the next-turn attack from Zubrus to Corzen");

    const std::vector<AI::AdventureThreat> aggressiveThreats =
        AI::predictAdventureThreats(player, AI::BehaviorProfile::Aggressive);
    expect(std::none_of(aggressiveThreats.begin(), aggressiveThreats.end(),
                        [](const AI::AdventureThreat & threat)
    {
        return threat.land == Land(Land::Corzen);
    }), "aggressive Adventure AI must ignore the same moderate border threat");

    const AI::AdventureTurnPlan economicPlan =
        AI::chooseAdventureTurn(player, AI::BehaviorProfile::Economic);
    const auto guardOrder = std::find_if(economicPlan.orders.begin(), economicPlan.orders.end(),
                                         [](const AI::AdventurePartyOrder & order)
    {
        return order.origin == Land(Land::Corzen);
    });
    expect(economicPlan.reservedParties == 1 && guardOrder != economicPlan.orders.end() &&
           guardOrder->hold && guardOrder->defend == Land(Land::Corzen),
           "economic Adventure AI must hold the threatened border party in reserve");

    const AI::AdventureTurnPlan aggressivePlan =
        AI::chooseAdventureTurn(player, AI::BehaviorProfile::Aggressive);
    expect(aggressivePlan.reservedParties == 0,
           "aggressive Adventure AI must keep all parties available when no serious threat exists");

    std::map<int, int> arrivals;
    std::map<int, int> targetAssignments;
    for(const AI::AdventurePartyOrder & order : aggressivePlan.orders)
    {
        if(!order.isMove()) continue;
        arrivals[order.move.destination()] += static_cast<int>(order.units.size());
        targetAssignments[order.move.target()]++;
    }
    for(const auto & arrival : arrivals)
    {
        const BattleParty* occupants = player.army.findPartyConst(
            Land(static_cast<Land::land_t>(arrival.first)));
        expect((occupants ? occupants->count() : 0) + arrival.second <= 3,
               "Adventure coordinator must not overbook a destination party");
    }
    for(const auto & assignment : targetAssignments)
        expect(assignment.second <= AI::behaviorRules(AI::BehaviorProfile::Aggressive).maxPartiesPerTarget,
               "Adventure coordinator must respect profile target concentration");

    expect(player.army.findPartyConst(Land(Land::Corzen))->findBattleUnitConst(310) &&
           player.army.findPartyConst(Land(Land::Greenbaw))->findBattleUnitConst(311) &&
           player.army.findPartyConst(Land(Land::Talon))->findBattleUnitConst(314),
           "Adventure whole-turn planning must not mutate unit positions");

    LocalPlayer & playerWithoutGuard = GameData::gamers.front();
    const BattleCreature removedGuard =
        *playerWithoutGuard.army.findPartyConst(Land(Land::Corzen))->findBattleUnitConst(310);
    playerWithoutGuard.army.remove(removedGuard);
    const AI::AdventureTurnPlan reinforcementPlan =
        AI::chooseAdventureTurn(playerWithoutGuard, AI::BehaviorProfile::Economic);
    const auto reinforcement = std::find_if(reinforcementPlan.orders.begin(),
                                             reinforcementPlan.orders.end(),
                                             [](const AI::AdventurePartyOrder & order)
    {
        return order.origin == Land(Land::Greenbaw) &&
               order.defend == Land(Land::Corzen);
    });
    expect(reinforcement != reinforcementPlan.orders.end() && reinforcement->isMove() &&
           reinforcement->move.destination == Land(Land::Corzen),
           "economic Adventure AI must reinforce an unguarded threatened border");
}

void testBattleAI()
{
    BattleParty forecastAttackers(Clan::Red, Land::Baliphon);
    expect(forecastAttackers.join(combatCreature(Clan::Red, Creature::Durlock,
                                                  325, 12, 0, 3, 10)),
           "Battle forecast overwhelming attacker fixture must join");
    BattleTown forecastTown(BattleUnit(BaseStat(0, 0, 0, 2)), Clan::Yellow, Land::Baliphon);
    const AI::BattleForecast winningForecast = AI::forecastBattle(
        forecastAttackers, forecastTown, nullptr, AI::BehaviorProfile::Aggressive,
        AI::BehaviorProfile::Balanced, 32);
    const AI::BattleForecast repeatedForecast = AI::forecastBattle(
        forecastAttackers, forecastTown, nullptr, AI::BehaviorProfile::Aggressive,
        AI::BehaviorProfile::Balanced, 32);
    expect(winningForecast.captureChance == 100 &&
           winningForecast.captureChance == repeatedForecast.captureChance &&
           winningForecast.attackerSurvival == repeatedForecast.attackerSurvival,
           "Battle forecast must be deterministic and recognize an overwhelming victory");
    expect(forecastAttackers.findBattleUnitConst(325)->loyalty() == 10 &&
           forecastTown.loyalty() == 2,
           "Battle forecast must run only on copied combat state");

    BattleParty doomedAttackers(Clan::Red, Land::Baliphon);
    expect(doomedAttackers.join(combatCreature(Clan::Red, Creature::Durlock,
                                                326, 1, 0, 0, 1)),
           "Battle forecast doomed attacker fixture must join");
    BattleTown fortress(BattleUnit(BaseStat(10, 0, 10, 10)), Clan::Yellow, Land::Baliphon);
    expect(AI::forecastBattle(doomedAttackers, fortress, nullptr,
                              AI::BehaviorProfile::Aggressive,
                              AI::BehaviorProfile::Balanced, 32).captureChance == 0,
           "Battle forecast must recognize a hopeless attack");

    std::srand(4242);
    const int expectedFirstRandom = std::rand();
    const int expectedSecondRandom = std::rand();
    std::srand(4242);
    const int actualFirstRandom = std::rand();
    AI::forecastBattle(forecastAttackers, forecastTown, nullptr,
                       AI::BehaviorProfile::Balanced, AI::BehaviorProfile::Balanced, 8);
    const int actualSecondRandom = std::rand();
    expect(actualFirstRandom == expectedFirstRandom && actualSecondRandom == expectedSecondRandom,
           "Battle forecast must not consume the real game's global random stream");

    BattleParty opening(Clan::Red, Land::Baliphon);
    expect(opening.join(combatCreature(Clan::Red, Creature::Durlock, 330, 12, 0, 2, 4)),
           "Battle AI opening damage fixture must join");
    expect(opening.join(combatCreature(Clan::Red, Creature::GreatCarol, 331, 1, 0, 1, 2)),
           "Battle AI opening First Strike fixture must join");
    const std::vector<int> openingOrder = AI::chooseBattleInitiative(
        opening, AI::BattleAttackMode::Melee, AI::BehaviorProfile::Balanced, true);
    const std::vector<int> normalOrder = AI::chooseBattleInitiative(
        opening, AI::BattleAttackMode::Melee, AI::BehaviorProfile::Balanced, false);
    expect(!openingOrder.empty() && openingOrder.front() == 331,
           "Battle AI must move a First Strike creature to opening initiative");
    expect(!normalOrder.empty() && normalOrder.front() == 330,
           "First Strike must not distort initiative after the opening");

    BattleParty doctrine(Clan::Red, Land::Baliphon);
    expect(doctrine.join(combatCreature(Clan::Red, Creature::Durlock, 340, 10, 0, 0, 2)),
           "Battle AI aggressive doctrine fixture must join");
    expect(doctrine.join(combatCreature(Clan::Red, Creature::AdventureParty, 341, 3, 0, 8, 10)),
           "Battle AI economic doctrine fixture must join");
    const std::vector<int> aggressiveOrder = AI::chooseBattleInitiative(
        doctrine, AI::BattleAttackMode::Melee, AI::BehaviorProfile::Aggressive);
    const std::vector<int> economicOrder = AI::chooseBattleInitiative(
        doctrine, AI::BattleAttackMode::Melee, AI::BehaviorProfile::Economic);
    expect(!aggressiveOrder.empty() && aggressiveOrder.front() == 340,
           "aggressive Battle AI must lead with the highest damage creature");
    expect(!economicOrder.empty() && economicOrder.front() == 341,
           "economic Battle AI must lead with the durable creature");

    BattleParty controlDoctrine(Clan::Red, Land::Baliphon);
    expect(controlDoctrine.join(combatCreature(Clan::Red, Creature::Durlock, 342, 7, 0, 3, 5)),
           "Battle AI control baseline fixture must join");
    expect(controlDoctrine.join(combatCreature(Clan::Red, Creature::SkeletonHorde,
                                                343, 4, 0, 1, 3)),
           "Battle AI control speciality fixture must join");
    const std::vector<int> controlOrder = AI::chooseBattleInitiative(
        controlDoctrine, AI::BattleAttackMode::Melee, AI::BehaviorProfile::Control);
    expect(!controlOrder.empty() && controlOrder.front() == 343,
           "control Battle AI must prioritize a tactical Swarm attacker");

    BattleParty shooters(Clan::Red, Land::Baliphon);
    expect(shooters.join(combatCreature(Clan::Red, Creature::Durlock, 350, 1, 2, 1, 3)),
           "Battle AI first ranged fixture must join");
    expect(shooters.join(combatCreature(Clan::Red, Creature::Durlock, 351, 1, 2, 1, 3)),
           "Battle AI second ranged fixture must join");
    BattleParty rangedTargets(Clan::Yellow, Land::Baliphon);
    expect(rangedTargets.join(combatCreature(Clan::Yellow, Creature::Durlock, 360, 1, 0, 0, 2)),
           "Battle AI lethal ranged target fixture must join");
    expect(rangedTargets.join(combatCreature(Clan::Yellow, Creature::Durlock, 361, 1, 0, 0, 6)),
           "Battle AI durable ranged target fixture must join");
    const AI::BattleRoundPlan rangedPlan = AI::chooseRangedBattlePlan(
        shooters, rangedTargets, AI::BehaviorProfile::Balanced);
    expect(rangedPlan.attacks.size() == 2 && rangedPlan.attacks[0].targets.front() == 360 &&
           rangedPlan.attacks[1].targets.front() == 361,
           "Battle AI must redirect later missiles after projecting a lethal shot");
    expect(rangedTargets.findBattleUnitConst(360)->loyalty() == 2 &&
           rangedTargets.findBattleUnitConst(361)->loyalty() == 6,
           "Battle AI ranged projection must not apply its predicted damage");

    BattleParty missileTargets(Clan::Yellow, Land::Baliphon);
    expect(missileTargets.join(combatCreature(Clan::Yellow, Creature::KiLin, 362, 1, 0, 0, 1)),
           "Battle AI Ignore Missiles fixture must join");
    expect(missileTargets.join(combatCreature(Clan::Yellow, Creature::Durlock, 363, 1, 0, 0, 3)),
           "Battle AI legal missile fixture must join");
    const AI::BattleRoundPlan missilePlan = AI::chooseRangedBattlePlan(
        shooters, missileTargets, AI::BehaviorProfile::Balanced);
    expect(std::all_of(missilePlan.attacks.begin(), missilePlan.attacks.end(),
                       [](const AI::BattleAttackPlan & attack)
    {
        return !attack.targets.empty() && attack.targets.front() != 362;
    }), "Battle AI must never assign missiles to an immune creature");

    BattleParty meleeActor(Clan::Red, Land::Baliphon);
    expect(meleeActor.join(combatCreature(Clan::Red, Creature::Durlock, 370, 5, 0, 1, 5)),
           "Battle AI melee attacker fixture must join");
    BattleParty meleeTargets(Clan::Yellow, Land::Baliphon);
    expect(meleeTargets.join(combatCreature(Clan::Yellow, Creature::Durlock, 371, 0, 0, 0, 1)),
           "Battle AI easy melee target fixture must join");
    expect(meleeTargets.join(combatCreature(Clan::Yellow, Creature::GreatCarol, 372, 8, 3, 1, 8)),
           "Battle AI dangerous melee target fixture must join");
    const AI::BattleAttackPlan aggressiveAttack = AI::chooseMeleeBattlePlan(
        meleeActor, meleeTargets, AI::BehaviorProfile::Aggressive);
    const AI::BattleAttackPlan controlAttack = AI::chooseMeleeBattlePlan(
        meleeActor, meleeTargets, AI::BehaviorProfile::Control);
    expect(aggressiveAttack.isValid() && aggressiveAttack.targets.front() == 371,
           "aggressive Battle AI must take the immediate kill");
    expect(controlAttack.isValid() && controlAttack.targets.front() == 372,
           "control Battle AI must prioritize the more dangerous tactical target");
    expect(meleeTargets.findBattleUnitConst(371)->loyalty() == 1 &&
           meleeTargets.findBattleUnitConst(372)->loyalty() == 8,
           "Battle AI melee planning must not mutate its targets");
}
}

int main()
{
    const std::string unicodeSample = "Four Winds: Привет, мир! 🌬️";
    expect(UnicodeString(unicodeSample).toString() == unicodeSample,
           "engine UTF-8/UTF-16 conversion must preserve Cyrillic and supplementary characters");

    JsonContentFile indexFile(std::string(FOUR_WINDS_SOURCE_DIR) + "/themes/default/index.json");
    expect(indexFile.isValid(), "theme index JSON must parse");
    expect(GameData::loadIndexes(indexFile.toObject()), "theme indexes must load");
    GameData::creaturesInfo = loadIndexed<CreatureInfo>("creatures.json");
    GameData::spellsInfo = loadIndexed<SpellInfo>("spells.json");
    GameData::landsInfo = loadIndexed<LandInfo>("lands.json");
    GameData::avatarsInfo = loadIndexed<AvatarInfo>("avatars.json");

    testDifficultyRules();
    testSelectedPartyMovement();
    testGateMovement();
    testSpellCastingAI();
    testAdventureProfiles();
    testAdventureCoordination();
    testBattleAI();

    expect(Speciality(Speciality::CastSilence).toSpell()() == Spell::Silence,
           "CastSilence must grant Silence");
    expect(Speciality(Speciality::CastScryRunes).toSpell()() == Spell::ScryRunes,
           "CastScryRunes must grant ScryRunes");

    AffectedSpells fog;
    fog.insert(AffectedSpell(Spell(Spell::ManaFog), 3));
    expect(fog.isAffected(Spell::ManaFog), "Mana Fog must start active");
    fog.spellAffected(Spell::ManaFog);
    expect(fog.isAffected(Spell::ManaFog), "Mana Fog must survive the first turn");
    fog.spellAffected(Spell::ManaFog);
    expect(fog.isAffected(Spell::ManaFog), "Mana Fog must survive the second turn");
    fog.spellAffected(Spell::ManaFog);
    expect(!fog.isAffected(Spell::ManaFog), "Mana Fog must expire after the third turn");

    AffectedSpells scry;
    const Avatar scryCaster(Avatar::Niana);
    scry.insert(AffectedSpell(Spell(Spell::ScryRunes), 5, scryCaster));
    expect(scry.isAffected(Spell::ScryRunes, scryCaster),
           "Scry Runes must reveal the hand to its caster");
    expect(!scry.isAffected(Spell::ScryRunes, Avatar(Avatar::Javed)),
           "Scry Runes must not reveal the hand to another player");

    AffectedSpells restoredScry = AffectedSpells::fromJsonArray(scry.toJsonArray());
    expect(restoredScry.isAffected(Spell::ScryRunes, scryCaster),
           "Scry Runes caster must survive save/load");

    AffectedSpells enchantments;
    enchantments.insert(AffectedSpell(Spell(Spell::ForceShield), AffectedSpell::Permanent));
    enchantments.insert(AffectedSpell(Spell(Spell::ForceShield), AffectedSpell::Permanent));
    expect(enchantments.size() == 1, "the same persistent enchantment must not stack");
    enchantments.advanceAdventureRound();
    enchantments.advanceAdventureRound();
    expect(enchantments.isAffected(Spell::ForceShield),
           "persistent enchantments must survive Adventure rounds");

    AffectedSpells restoredEnchantments = AffectedSpells::fromJsonArray(enchantments.toJsonArray());
    expect(restoredEnchantments.isAffected(Spell::ForceShield),
           "persistent enchantments must survive save/load");

    AffectedSpells timedEnchantments;
    timedEnchantments.insert(AffectedSpell(Spell(Spell::Paralyze), 1));
    timedEnchantments.advanceAdventureRound();
    expect(!timedEnchantments.isAffected(Spell::Paralyze),
           "one-round enchantments must expire after one Adventure phase");

    AffectedSpells expiredEnchantments;
    expiredEnchantments.insert(AffectedSpell(Spell(Spell::Paralyze), 0));
    expect(expiredEnchantments.empty(), "expired enchantments must not enter active state");

    expect(Battle::rangedDamage(3, 1) == 2,
           "Force Shield must reduce ranged damage by one");
    expect(Battle::rangedDamage(0, 1) == 0,
           "Force Shield must not turn ranged damage negative");
    expect(Battle::meleeHitChance(5, 4) == 100, "melee above defense must always hit");
    expect(Battle::meleeHitChance(4, 4) == 50, "equal melee and defense must have 50% hit chance");
    expect(Battle::meleeHitChance(3, 4) == 25, "one defense advantage must have 25% hit chance");
    expect(Battle::meleeHitChance(2, 4) == 12, "two defense advantage must have 12% hit chance");
    expect(Battle::meleeHitChance(1, 4) == 6, "three defense advantage must have 6% hit chance");

    LandClaims claims;
    claims.add(Clan::Yellow, 500);
    expect(claims.points(Clan::Yellow) == 500, "land claim points must be credited per opponent clan");
    expect(!claims.spend(Clan::Yellow, 501), "land claim points must reject overdraft");
    expect(claims.spend(Clan::Yellow, 200), "land claim points must pay a valid deed cost");
    expect(claims.points(Clan::Yellow) == 300, "land claim payment must deduct its cost");
    const LandClaims restoredClaims = LandClaims::fromJsonObject(claims.toJsonObject());
    expect(restoredClaims.points(Clan::Yellow) == 300, "land claim points must survive save/load");

    JsonContentFile spellFile(std::string(FOUR_WINDS_SOURCE_DIR) +
                              "/themes/default/json/gamedata/spells.json");
    expect(spellFile.isValid(), "spell theme JSON must parse");
    const std::list<SpellInfo> spellInfos = spellFile.toArray().toStdList<SpellInfo>();

    const SpellInfo* blindAmbition = findSpell(spellInfos, Spell(Spell::BlindAmbition));
    expect(blindAmbition != nullptr, "Blind Ambition must exist in the theme");
    if(blindAmbition)
    {
        expect(blindAmbition->effect.attack == 1 && blindAmbition->effect.ranger == 1 &&
               blindAmbition->effect.defense == 0 && blindAmbition->effect.loyalty == -2,
               "Blind Ambition must grant melee and ranged attack at the cost of loyalty");
    }

    const SpellInfo* hellBlast = findSpell(spellInfos, Spell(Spell::HellBlast));
    expect(hellBlast != nullptr, "Hell Blast must exist in the theme");
    if(hellBlast)
    {
        expect(hellBlast->target() == (SpellTarget::Enemy | SpellTarget::Party),
               "Hell Blast must target one enemy party");
        expect(hellBlast->effect.loyalty == -2,
               "Hell Blast must remove two loyalty");
    }

    const SpellInfo* forceShield = findSpell(spellInfos, Spell(Spell::ForceShield));
    expect(forceShield != nullptr, "Force Shield must exist in the theme");
    if(forceShield)
        expect(forceShield->value == 1, "Force Shield must absorb one ranged damage");

    const SpellInfo* silence = findSpell(spellInfos, Spell(Spell::Silence));
    expect(silence && silence->duration() == 3,
           "Silence must last three turns");

    const SpellInfo* scryRunes = findSpell(spellInfos, Spell(Spell::ScryRunes));
    expect(scryRunes && scryRunes->duration() == 5,
           "Scry Runes must last five turns");

    const SpellInfo* randomDiscard = findSpell(spellInfos, Spell(Spell::RandomDiscard));
    expect(randomDiscard && randomDiscard->duration() == 1,
           "Random Discard must remain active until the next discard");
    if(randomDiscard)
    {
        AffectedSpells discardEffects;
        discardEffects.insert(AffectedSpell(randomDiscard->id, randomDiscard->duration()));
        expect(discardEffects.isAffected(Spell::RandomDiscard),
               "Random Discard must enter active state");
        discardEffects.spellAffected(Spell::RandomDiscard);
        expect(!discardEffects.isAffected(Spell::RandomDiscard),
               "Random Discard must expire after one discard");
    }

    const SpellInfo* massDispel = findSpell(spellInfos, Spell(Spell::MassDispel));
    expect(massDispel && massDispel->target() == SpellTarget::Land,
           "Mass Dispel must target a territory");
    expect(massDispel && massDispel->description.find("every creature") != std::string::npos,
           "Mass Dispel description must match its territory-wide effect");

    const SpellInfo* dispel = findSpell(spellInfos, Spell(Spell::DispelMagic));
    expect(dispel && dispel->target() == SpellTarget::Any,
           "Dispel Magic must target one creature");
    expect(dispel && dispel->description.find("target creature") != std::string::npos,
           "Dispel Magic description must remain single-target");

    expect(creature(107, 3).canReceiveSpell(Spell(Spell::Guidance)),
           "Guidance must accept creatures with a ranged attack");
    expect(!creature(108, 3, 3, 0).canReceiveSpell(Spell(Spell::Guidance)),
           "Guidance must reject creatures without a ranged attack");

    JsonContentFile avatarFile(std::string(FOUR_WINDS_SOURCE_DIR) +
                               "/themes/default/json/gamedata/avatars.json");
    expect(avatarFile.isValid(), "avatar theme JSON must parse");
    const std::list<AvatarInfo> avatarInfos = avatarFile.toArray().toStdList<AvatarInfo>();

    const AvatarInfo* ziag = findAvatar(avatarInfos, Avatar(Avatar::Ziag));
    expect(ziag && hasSpell(*ziag, Spell(Spell::MassDispel)),
           "Ziag must know Mass Dispel");

    const AvatarInfo* javed = findAvatar(avatarInfos, Avatar(Avatar::Javed));
    expect(javed && hasSpell(*javed, Spell(Spell::MassDispel)),
           "Javed must know Mass Dispel");

    expect(WinResults::scoreMultiplier(0) == 1, "zero doubles must use x1");
    expect(WinResults::scoreMultiplier(1) == 2, "one double must use x2");
    expect(WinResults::scoreMultiplier(4) == 16, "four doubles must use x16");
    expect(WinResults::scoreMultiplier(5) == 32, "five doubles must reach the limit tier");

    BattleParty party(Clan::Red, Land::Maithaius);
    expect(party.join(creature(101, 4)), "first creature must join party");
    expect(party.movePoint() == 4, "empty party slots must not reduce movement");
    expect(party.join(creature(102, 2)), "second creature must join party");
    expect(party.movePoint() == 2, "party movement must use the slowest creature");

    BattleParty deadParty(Clan::Red, Land::Maithaius);
    expect(deadParty.join(creature(103, 3, 0)), "dead fixture must join party");
    deadParty.removeUnloyalty();
    expect(deadParty.isEmpty(), "zero-loyalty creatures must be removed");

    BattleArmy army;
    BattleParty source(Clan::Red, Land::Maithaius);
    expect(source.join(creature(104, 5)), "teleport fixture must join source party");
    army.push_back(source);

    const BattleCreature* before = army.findBattleUnitConst(104);
    expect(before && army.teleportCreature(*before, Land::Baliphon),
           "teleport must move a valid creature");

    const BattleCreature* after = army.findBattleUnitConst(104);
    expect(after != nullptr, "teleport must preserve battle unit id");
    expect(after && after->freeMovePoint() == 5,
           "teleport must not consume movement points");
    expect(army.findPartyConst(Land::Maithaius) == nullptr,
           "teleport must remove an empty source party");
    expect(army.findPartyConst(Land::Baliphon) != nullptr,
           "teleport must create the destination party");

    BattleArmy duplicateArmy;
    BattleParty duplicateSource(Clan::Red, Land::Maithaius);
    expect(duplicateSource.join(creature(105, 4)), "first duplicate fixture must join");
    expect(duplicateSource.join(creature(106, 3)), "second duplicate fixture must join");
    duplicateArmy.push_back(duplicateSource);

    const BattleCreature* duplicate = duplicateArmy.findBattleUnitConst(106);
    expect(duplicate && duplicateArmy.teleportCreature(*duplicate, Land::Baliphon),
           "teleport must move the selected duplicate");
    const BattleParty* remaining = duplicateArmy.findPartyConst(Land::Maithaius);
    expect(remaining != nullptr, "teleport must keep a non-empty source party");
    expect(remaining && remaining->findBattleUnitConst(105) != nullptr,
           "teleport must leave the other creature in place");
    expect(remaining && remaining->findBattleUnitConst(106) == nullptr,
           "teleport must remove the selected battle unit from its source");

    BattleParty merged(Clan::Red, Land::Maithaius);
    expect(merged.join(combatCreature(Clan::Red, Creature::Shadow, 201, 5, 0, 2, 3)),
           "first merge creature must join");
    expect(merged.join(combatCreature(Clan::Red, Creature::Shadow, 202, 5, 0, 2, 3)),
           "second merge creature must join");
    expect(Battle::mergeDefenseBonus(merged, *merged.findBattleUnitConst(201)) == 1,
           "two matching Merge creatures must grant one defense");

    BattleParty rangedAttackers(Clan::Red, Land::Baliphon);
    BattleParty rangedDefenders(Clan::Yellow, Land::Baliphon);
    expect(rangedAttackers.join(combatCreature(Clan::Red, Creature::Durlock, 210, 0, 2, 0, 1)),
           "ranged attacker fixture must join");
    expect(rangedDefenders.join(combatCreature(Clan::Yellow, Creature::AdventureParty, 211, 0, 2, 0, 1)),
           "ranged defender fixture must join");
    BattleTown quietTown(BattleUnit(BaseStat(0, 0, 100, 100)), Clan::Yellow, Land::Baliphon);
    const BattleStrikes rangedStrikes = Battle::doAttackParty(rangedAttackers, quietTown, &rangedDefenders);
    expect(rangedAttackers.isEmpty() && rangedDefenders.isEmpty(),
           "simultaneous ranged combat must let both lethal shooters fire");
    expect(std::count_if(rangedStrikes.begin(), rangedStrikes.end(), [](const BattleStrike & strike)
           { return strike.type == BattleStrike::Ranger; }) == 2,
           "simultaneous ranged combat must record both shots");

    BattleParty firstStrikeAttackers(Clan::Red, Land::Baliphon);
    BattleParty firstStrikeDefenders(Clan::Yellow, Land::Baliphon);
    expect(firstStrikeAttackers.join(combatCreature(Clan::Red, Creature::Durlock, 221, 1, 20, 0, 2)),
           "First Strike ranged support fixture must join");
    expect(firstStrikeAttackers.join(combatCreature(Clan::Red, Creature::GreatCarol, 220, 10, 0, 0, 1)),
           "First Strike leader fixture must join after its support");
    expect(firstStrikeDefenders.join(combatCreature(Clan::Yellow, Creature::AdventureParty, 222, 10, 20, 0, 1)),
           "First Strike defender fixture must join");
    BattleTown weakTown(BattleUnit(BaseStat(0, 0, 0, 1)), Clan::Yellow, Land::Baliphon);
    const BattleStrikes firstStrikes = Battle::doAttackParty(firstStrikeAttackers, weakTown, &firstStrikeDefenders);
    expect(!weakTown.isAlive() && !firstStrikeAttackers.isEmpty(),
           "First Strike leader must let the attacking party strike first and capture the town");
    expect(std::none_of(firstStrikes.begin(), firstStrikes.end(), [](const BattleStrike & strike)
           { return strike.type == BattleStrike::Ranger; }),
           "First Strike must skip all creature ranged attacks");

    BattleParty swarmAttackers(Clan::Red, Land::Baliphon);
    BattleParty swarmDefenders(Clan::Yellow, Land::Baliphon);
    expect(swarmAttackers.join(combatCreature(Clan::Red, Creature::SkeletonHorde, 230, 10, 0, 0, 10)),
           "Swarm attacker fixture must join");
    expect(swarmDefenders.join(combatCreature(Clan::Yellow, Creature::Durlock, 231, 0, 0, 0, 1)),
           "first Swarm target fixture must join");
    expect(swarmDefenders.join(combatCreature(Clan::Yellow, Creature::Durlock, 232, 0, 0, 0, 1)),
           "second Swarm target fixture must join");
    expect(swarmDefenders.join(combatCreature(Clan::Yellow, Creature::Durlock, 233, 0, 0, 0, 1)),
           "third Swarm target fixture must join");
    BattleTown swarmTown(BattleUnit(BaseStat(0, 0, 0, 1)), Clan::Yellow, Land::Baliphon);
    Battle::doAttackParty(swarmAttackers, swarmTown, &swarmDefenders);
    expect(swarmDefenders.isEmpty(), "Swarm must attack every creature in the defending party");

    BattleCreature regenerating = combatCreature(Clan::Red, Creature::Wraith, 240, 7, 0, 4, 5);
    regenerating.applyDamage(4);
    regenerating.initAdventurePart(Ability::None);
    expect(regenerating.loyalty() == regenerating.baseLoyalty(),
           "Regeneration must restore full loyalty at map phase start");

    GameData::gamers.clear();
    LocalPlayer red;
    red.avatar = Avatar::Orachi;
    red.clan = Clan::Red;
    red.wind = Wind::East;
    red.addLandClaimPoints(Clan::Yellow, 500);
    BattleParty undoParty(Clan::Red, Land::Corzen);
    expect(undoParty.join(combatCreature(Clan::Red, Creature::SkeletonHorde, 260, 5, 0, 1, 3)),
           "Adventure undo fixture must join");
    red.army.push_back(undoParty);
    LocalPlayer yellow;
    yellow.avatar = Avatar::Lakkho;
    yellow.clan = Clan::Yellow;
    yellow.wind = Wind::South;
    GameData::gamers.push_back(red);
    GameData::gamers.push_back(yellow);

    expect(GameData::canClaimLand(GameData::gamers.front(), Land::Zubrus),
           "adjacent empty enemy territory with enough points must be claimable");
    BattleParty hiddenGuard(Clan::Yellow, Land::Zubrus);
    expect(hiddenGuard.join(combatCreature(Clan::Yellow, Creature::Shadow, 250, 5, 0, 2, 3)),
           "hidden claim guard fixture must join");
    GameData::gamers.back().army.push_back(hiddenGuard);
    expect(!GameData::canClaimLand(GameData::gamers.front(), Land::Zubrus),
           "an invisible creature must block Land Claim");
    expect(!GameData::canClaimLand(GameData::gamers.front(), Land::TowerOf4Winds),
           "Tower of Four Winds must never be claimable");

    GameData::gamers.back().army.clear();
    ActionList adventureActions;
    expect(GameData::initAdventure(), "Adventure phase must initialize");
    expect(GameData::adventure2Client(GameData::gamers.front().avatar, adventureActions),
           "Adventure turn must begin and create an undo snapshot");
    expect(GameData::client2Adventure(GameData::gamers.front().avatar,
                                      ClientLandClaim(Land::Zubrus), adventureActions),
           "Land Claim action must be accepted during the current turn");
    expect(GameData::client2Adventure(GameData::gamers.front().avatar,
                                      ClientUnitMoved(260, Land::Zubrus), adventureActions),
           "movement after a Land Claim must be accepted");
    expect(GameData::landInfo(Land::Zubrus).clan == Clan(Clan::Red),
           "Land Claim must change territory ownership");
    expect(GameData::gamers.front().army.findPartyConst(Land::Zubrus) != nullptr,
           "movement must change the creature position before undo");
    expect(GameData::client2Adventure(GameData::gamers.front().avatar,
                                      ClientAdventureUndo(), adventureActions),
           "Adventure Undo action must be accepted");
    expect(GameData::landInfo(Land::Zubrus).clan == Clan(Clan::Yellow),
           "Adventure Undo must restore territory ownership");
    expect(GameData::gamers.front().landClaimPoints(Clan::Yellow) == 500,
           "Adventure Undo must refund Land Claim points");
    const BattleParty* restoredParty = GameData::gamers.front().army.findPartyConst(Land::Corzen);
    expect(restoredParty && restoredParty->findBattleUnitConst(260),
           "Adventure Undo must restore the original creature position");
    expect(GameData::gamers.front().army.findPartyConst(Land::Zubrus) == nullptr,
           "Adventure Undo must remove the moved destination party");

    if(failures)
        return 1;

    std::cout << "gameplay regressions: ok\n";
    return 0;
}
