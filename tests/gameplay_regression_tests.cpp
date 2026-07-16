#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <initializer_list>
#include <map>
#include <vector>

#include "aiadventure.h"
#include "aibattle.h"
#include "aispell.h"
#include "aiturn.h"
#include "avatar_balance_tests.h"
#include "battle.h"
#include "crashreport.h"
#include "gameplayrng.h"
#include "recovery.h"
#include "replay.h"
#include "settings.h"

#if defined(_WIN32)
#ifdef ERROR
#undef ERROR
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#ifdef DELETE
#undef DELETE
#endif
#endif

namespace GameData
{
bool loadIndexes(const JsonObject &);
extern std::vector<CreatureInfo> creaturesInfo;
extern std::vector<SpellInfo> spellsInfo;
extern std::vector<LandInfo> landsInfo;
extern std::vector<AvatarInfo> avatarsInfo;
extern LocalPlayers gamers;
extern Wind currentWind;
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

void testActionReplay()
{
    std::vector<std::unique_ptr<ClientMessage>> messages;
    messages.emplace_back(std::make_unique<ClientReady>());
    messages.emplace_back(std::make_unique<ClientButtonGame>());
    messages.emplace_back(std::make_unique<ClientButtonPass>());
    messages.emplace_back(std::make_unique<ClientChaoVariant>(2));
    messages.emplace_back(std::make_unique<ClientButtonPung>());
    messages.emplace_back(std::make_unique<ClientButtonKong1>());
    messages.emplace_back(std::make_unique<ClientButtonKong2>());
    messages.emplace_back(std::make_unique<ClientDropIndex>(4));
    messages.emplace_back(std::make_unique<ClientSayGame>());
    messages.emplace_back(std::make_unique<ClientSayChao>());
    messages.emplace_back(std::make_unique<ClientSayPung>());
    messages.emplace_back(std::make_unique<ClientSayKong>(1));
    messages.emplace_back(std::make_unique<ClientSummonCreature>(Creature::Durlock, Land::Corzen, true));
    messages.emplace_back(std::make_unique<ClientCastSpell>(Spell::Healing));
    messages.emplace_back(std::make_unique<ClientCastSpell>(Spell::ScryRunes, Avatar::Logun));
    messages.emplace_back(std::make_unique<ClientCastSpell>(Spell::Teleport, Land::Corzen, 601, true));
    messages.emplace_back(std::make_unique<ClientUnitMoved>(601, Land::None));
    messages.emplace_back(std::make_unique<ClientLandClaim>(Land::Corzen));
    messages.emplace_back(std::make_unique<ClientAdventureUndo>());
    messages.emplace_back(std::make_unique<ClientBattleReady>());
    messages.emplace_back(std::make_unique<ClientLuckChoice>(1));

    for(const auto & message : messages)
    {
        const std::unique_ptr<ClientMessage> rebuilt = Replay::clientMessageFromJson(*message);
        expect(rebuilt && rebuilt->toString() == message->toString(),
               "replay client-message factory must preserve every action payload");
    }

    JsonObject unknown;
    unknown.addInteger("type", Action::Last + 100);
    expect(!Replay::clientMessageFromJson(unknown),
           "replay client-message factory must reject unknown action types");

    GameData::initPersons(Person(Avatar::Nucrus, Clan::Red, Wind::East));
    LocalPlayer* player = GameData::gamers.playerOfWind(Wind::East);
    expect(player != nullptr, "replay fixture player must exist");
    if(!player) return;
    const Avatar actor = player->avatar;
    BattleParty party(Clan::Red, Land::Corzen);
    expect(party.join(combatCreature(Clan::Red, Creature::Durlock,
                                     601, 2, 0, 2, 3, 1)),
           "replay Adventure fixture must join");
    player->army.push_back(party);
    expect(GameData::initAdventure(), "replay Adventure fixture must initialize");

    const JsonObject initialState = GameData::authoritativeState();
    const ClientUnitMoved dismiss(601, Land::None);
    ActionList emitted;
    expect(GameData::client2Adventure(actor, dismiss, emitted),
           "replay reference action must be accepted");
    const std::string expectedHash = Replay::authoritativeStateHash();

    const std::vector<Replay::Step> replaySteps = {
        Replay::Step(actor, dismiss, expectedHash)
    };
    std::string replayError;
    expect(Replay::run(initialState, replaySteps, &replayError) &&
           GameData::findBattleCreature(601) == nullptr,
           "replay must restore, apply a validated Adventure action and match its state hash");

    const std::vector<Replay::Step> tamperedSteps = {
        Replay::Step(actor, dismiss, "0000000000000000")
    };
    replayError.clear();
    expect(!Replay::run(initialState, tamperedSteps, &replayError) &&
           replayError.find("state hash mismatch at step 0") != std::string::npos,
           "replay must stop at the first divergent authoritative state hash");

    GameplayRng::seed(UINT64_C(0x123456789abcdef));
    GameData::initPersons(Person(Avatar::Nucrus, Clan::Red, Wind::East));
    expect(GameData::initMahjong(), "RNG replay fixture must initialize Mahjong");
    LocalPlayer* randomPlayer = GameData::gamers.playerOfWind(Wind::East);
    expect(randomPlayer != nullptr, "RNG replay fixture East player must exist");
    if(!randomPlayer) return;

    randomPlayer->affected.insert(AffectedSpell(Spell(Spell::RandomDiscard), 1));
    const Avatar randomActor = randomPlayer->avatar;
    const JsonObject randomInitialState = GameData::authoritativeState();
    ActionList rejectedActions;
    const ClientMessage unknownAction(Action::Last + 1);
    expect(!GameData::client2Mahjong(randomActor, unknownAction, rejectedActions) &&
           Replay::actionJournalSize() == 0,
           "replay journal must exclude rejected and unknown client actions");
    const ClientDropIndex randomDiscard(0);
    ActionList randomActions;
    expect(GameData::client2Mahjong(randomActor, randomDiscard, randomActions),
           "RNG replay reference Random Discard must be accepted");
    const JsonObject randomFinalState = GameData::authoritativeState();
    const std::string randomExpectedHash = Replay::authoritativeStateHash();
    const JsonObject journal = Replay::actionJournal(randomFinalState);
    expect(journal.getInteger("actionCount") == 1 &&
           journal.getBoolean("contiguousToCheckpoint"),
           "accepted RNG action must enter a contiguous persisted replay journal");

    replayError.clear();
    expect(Replay::run(journal, &replayError) &&
           Replay::authoritativeStateHash() == randomExpectedHash,
           "replay journal must restore RNG state and reproduce Random Discard");

    JsonObject tamperedRandomState = randomInitialState;
    JsonObject tamperedRng = *randomInitialState.getObject("gameplayRng");
    tamperedRng.addString("state", "1234");
    tamperedRandomState.addObject("gameplayRng", tamperedRng);
    const std::vector<Replay::Step> randomSteps = {
        Replay::Step(randomActor, randomDiscard, randomExpectedHash)
    };
    replayError.clear();
    expect(!Replay::run(tamperedRandomState, randomSteps, &replayError) &&
           replayError.find("state hash mismatch at step 0") != std::string::npos,
           "replay must detect a divergent gameplay RNG state");
}

void testGameplayRngState()
{
    GameplayRng::seed(UINT64_C(0xfedcba987654321));
    const int first = GameplayRng::uniform(-10, 10);
    const JsonObject checkpoint = GameplayRng::toJsonObject();
    const std::vector<int> expected = {
        GameplayRng::uniform(0, 1000000),
        GameplayRng::uniform(0, 1000000),
        GameplayRng::uniform(0, 1000000)
    };

    expect(-10 <= first && first <= 10,
           "gameplay RNG uniform result must stay inside inclusive bounds");
    expect(GameplayRng::fromJsonObject(checkpoint),
           "gameplay RNG checkpoint must restore");
    const std::vector<int> actual = {
        GameplayRng::uniform(0, 1000000),
        GameplayRng::uniform(0, 1000000),
        GameplayRng::uniform(0, 1000000)
    };
    expect(actual == expected && GameplayRng::draws() == 4,
           "restored gameplay RNG must reproduce values and draw count");

    std::vector<int> firstShuffle = { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::vector<int> secondShuffle = firstShuffle;
    GameplayRng::seed(777);
    GameplayRng::shuffle(firstShuffle.begin(), firstShuffle.end());
    GameplayRng::seed(777);
    GameplayRng::shuffle(secondShuffle.begin(), secondShuffle.end());
    expect(firstShuffle == secondShuffle,
           "gameplay RNG shuffle must be deterministic for a fixed seed");
}

int runFixedSeedReplaySelfTest()
{
    constexpr uint64_t seed = UINT64_C(0x123456789abcdef);
    constexpr const char* expectedHash = "8020dff30d3f1571";

    GameplayRng::seed(seed);
    GameData::initPersons(Person(Avatar::Nucrus, Clan::Red, Wind::East));
    if(!GameData::initMahjong())
    {
        std::cerr << "FAIL: fixed-seed replay could not initialize Mahjong\n";
        return 1;
    }

    LocalPlayer* player = GameData::gamers.playerOfWind(Wind::East);
    if(!player)
    {
        std::cerr << "FAIL: fixed-seed replay East player is missing\n";
        return 1;
    }

    player->affected.insert(AffectedSpell(Spell(Spell::RandomDiscard), 1));
    const Avatar actor = player->avatar;
    const ClientDropIndex randomDiscard(0);
    ActionList actions;
    if(!GameData::client2Mahjong(actor, randomDiscard, actions))
    {
        std::cerr << "FAIL: fixed-seed Random Discard action was rejected\n";
        return 1;
    }

    const JsonObject finalState = GameData::authoritativeState();
    const std::string actualHash = Replay::authoritativeStateHash();
    const JsonObject journal = Replay::actionJournal(finalState);
    std::string replayError;
    if(!Replay::run(journal, &replayError) || Replay::authoritativeStateHash() != actualHash)
    {
        std::cerr << "FAIL: fixed-seed journal replay diverged: " << replayError << '\n';
        return 1;
    }

    std::cout << "fixed-seed replay hash: " << actualHash << '\n';
    if(actualHash != expectedHash)
    {
        std::cerr << "FAIL: expected fixed-seed replay hash " << expectedHash
                  << ", got " << actualHash << '\n';
        return 1;
    }

    return 0;
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

void testLuckAbility()
{
    CroupierSet wall;
    wall.bank.clear();
    wall.trash.clear();
    wall.luckDraw.clear();
    wall.bank.push_back(Stone(Stone::Number3));
    wall.bank.push_back(Stone(Stone::Skull1));
    wall.bank.push_back(Stone(Stone::Sword4));

    expect(wall.beginLuckDraw(), "Luck must reveal two runes when the wall has enough runes");
    expect(wall.luckChoices().size() == 2 &&
           wall.luckChoices()[0] == Stone(Stone::Sword4) &&
           wall.luckChoices()[1] == Stone(Stone::Skull1),
           "Luck must reveal runes in their original draw order");
    expect(!wall.resolveLuckDraw(-1).isValid() && wall.hasLuckDraw(),
           "an invalid Luck choice must not consume either rune");

    const Stone chosen = wall.resolveLuckDraw(1);
    expect(chosen == Stone(Stone::Skull1) && !wall.hasLuckDraw() && wall.bank.size() == 2,
           "Luck must keep exactly the selected rune");
    expect(wall.bank.front() == Stone(Stone::Sword4) &&
           wall.bank.back() == Stone(Stone::Number3),
           "Luck must return the rejected rune to the bottom of the wall");
    const VecStones resolvedBank = wall.bank;
    expect(!wall.resolveLuckDraw(0).isValid() && wall.bank == resolvedBank,
           "a repeated Luck response must not consume another rune");

    expect(wall.beginLuckDraw(), "Luck reset fixture must start a pending choice");
    wall.reset();
    expect(!wall.hasLuckDraw() && wall.bank.size() == 136 && wall.trash.empty(),
           "a new round must clear pending Luck state and rebuild the wall");

    LocalData roundStart;
    roundStart.partWind = Wind(Wind::East);
    roundStart.stoneLastCount = GAME_STONE_MAX;
    expect(roundStart.newRound(),
           "the client must recognize the server rune count as a new round");

    CroupierSet saved;
    saved.bank.clear();
    saved.trash.clear();
    saved.luckDraw.clear();
    saved.bank.push_back(Stone(Stone::Dragon1));
    saved.bank.push_back(Stone(Stone::Number7));
    saved.bank.push_back(Stone(Stone::Sword8));
    expect(saved.beginLuckDraw(), "Luck save fixture must start a pending choice");
    const CroupierSet restored = CroupierSet::fromJsonObject(saved.toJsonObject());
    expect(restored.bank == saved.bank && restored.luckChoices() == saved.luckChoices(),
           "a pending Luck choice and wall order must survive save/load");

    CroupierSet malformed;
    malformed.bank.clear();
    malformed.trash.clear();
    malformed.luckDraw.clear();
    malformed.bank.push_back(Stone(Stone::Wind1));
    malformed.luckDraw.push_back(Stone(Stone::Dragon2));
    const CroupierSet repaired = CroupierSet::fromJsonObject(malformed.toJsonObject());
    expect(!repaired.hasLuckDraw() && repaired.bank.size() == 2 &&
           repaired.bank.front() == Stone(Stone::Dragon2),
           "a malformed pending Luck choice must restore its rune to the wall");

    LocalPlayer nucrus;
    nucrus.avatar = Avatar::Nucrus;
    nucrus.clan = Clan::Red;
    nucrus.wind = Wind::East;
    CroupierSet nucrusWall;
    nucrusWall.bank.clear();
    nucrusWall.trash.clear();
    nucrusWall.luckDraw.clear();
    nucrusWall.bank.push_back(Stone(Stone::Wind2));
    nucrusWall.bank.push_back(Stone(Stone::Number5));
    nucrusWall.bank.push_back(Stone(Stone::Sword6));
    expect(nucrus.newTurnEvent(nucrusWall, false) && nucrusWall.hasLuckDraw() &&
           !nucrus.newStone.isValid(),
           "Nucrus must choose between two runes on an unforced turn draw");

    LocalPlayer directedNucrus;
    directedNucrus.avatar = Avatar::Nucrus;
    directedNucrus.clan = Clan::Red;
    directedNucrus.wind = Wind::East;
    expect(directedNucrus.mahjongApplySpell(Spell(Spell::DrawNumber)),
           "directed draw fixture must apply Summon Number Rune");
    CroupierSet directedWall;
    directedWall.bank.clear();
    directedWall.trash.clear();
    directedWall.luckDraw.clear();
    directedWall.bank.push_back(Stone(Stone::Number8));
    directedWall.bank.push_back(Stone(Stone::Skull4));
    expect(!directedNucrus.newTurnEvent(directedWall, false) &&
           directedNucrus.newStone.isNumber() && !directedWall.hasLuckDraw(),
           "directed draw spells must take precedence over Luck");

    LocalPlayers openingDeal;
    const Avatar::avatar_t avatars[] = {
        Avatar::Nucrus, Avatar::Orachi, Avatar::Lakkho, Avatar::Dayla
    };
    for(int index = 0; index < 4; ++index)
    {
	LocalPlayer player;
	player.avatar = avatars[index];
	player.clan = Clan(static_cast<Clan::clan_t>(Clan::Red + index));
	player.wind = Wind(static_cast<Wind::wind_t>(Wind::East + index));
	openingDeal.push_back(player);
    }
    CroupierSet openingWall;
    openingDeal.distributeStones(openingWall);
    expect(openingDeal.front().stones.size() == GAME_SET_COUNT && !openingWall.hasLuckDraw(),
           "Luck must not trigger during the initial thirteen-rune deal");

    GameStones hand;
    hand.add(GameStone(Stone(Stone::Sword2), false));
    hand.add(GameStone(Stone(Stone::Sword3), false));
    VecStones choices;
    choices.push_back(Stone(Stone::Sword4));
    choices.push_back(Stone(Stone::Dragon1));
    expect(AI::mahjongLuckChoice(hand, choices, VecStones(), WinRules()) == 0,
           "Luck AI must prefer a rune that completes a sequence");

    const MahjongLuckChoice serverChoice(Wind(Wind::East), choices);
    const ClientLuckChoice clientChoice(1);
    expect(serverChoice.type() == Action::MahjongLuckChoice &&
           serverChoice.currentWind() == Wind(Wind::East) &&
           serverChoice.choices() == choices &&
           clientChoice.type() == Action::ClientLuckChoice && clientChoice.index() == 1,
           "Luck client/server messages must preserve their choice payloads");
}

void testAvatarPassives()
{
    LocalPlayer logun;
    logun.avatar = Avatar::Logun;
    logun.clan = Clan::Red;
    logun.wind = Wind::East;

    BattleParty band(Clan::Red, Land::Corzen);
    BattleCreature badlyWounded(Clan::Red, Creature::Durlock, 380);
    BattleCreature lightlyWounded(Clan::Red, Creature::Durlock, 381);
    badlyWounded.applyDamage(3);
    lightlyWounded.applyDamage(1);
    expect(band.join(badlyWounded) && band.join(lightlyWounded),
           "Bard healing fixture must join");
    logun.army.push_back(band);
    logun.initAdventurePart();

    const BattleCreature* healedThree = logun.army.findBattleUnitConst(380);
    const BattleCreature* healedToFull = logun.army.findBattleUnitConst(381);
    expect(healedThree && healedThree->loyalty() == 3,
           "Bard must restore exactly two loyalty at map phase start");
    expect(healedToFull && healedToFull->loyalty() == healedToFull->baseLoyalty(),
           "Bard healing must not exceed base loyalty");

    BattleCreature ordinary(Clan::Red, Creature::Durlock, 382);
    ordinary.applyDamage(3);
    ordinary.initAdventurePart(Ability::None);
    expect(ordinary.loyalty() == 1,
           "creatures without Bard or a healing speciality must not heal automatically");

    const LocalPlayers savedPlayers = GameData::gamers;
    GameData::gamers.clear();

    LocalPlayer orachi;
    orachi.avatar = Avatar::Orachi;
    orachi.clan = Clan::Red;
    orachi.wind = Wind::East;
    GameData::gamers.push_back(orachi);

    LocalPlayer ziag;
    ziag.avatar = Avatar::Ziag;
    ziag.clan = Clan::Aqua;
    ziag.wind = Wind::South;
    GameData::gamers.push_back(ziag);

    LocalPlayer lakkho;
    lakkho.avatar = Avatar::Lakkho;
    lakkho.clan = Clan::Yellow;
    lakkho.wind = Wind::West;
    BattleParty hiddenParty(Clan::Yellow, Land::Zubrus);
    expect(hiddenParty.join(BattleCreature(Clan::Yellow, Creature::Shadow, 390)),
           "Monacle invisibility fixture must join");
    lakkho.army.push_back(hiddenParty);
    GameData::gamers.push_back(lakkho);

    LocalPlayer dayla;
    dayla.avatar = Avatar::Dayla;
    dayla.clan = Clan::Purple;
    dayla.wind = Wind::North;
    GameData::gamers.push_back(dayla);

    const LocalData ordinaryView = GameData::toLocalData(Avatar(Avatar::Orachi));
    const LocalData monacleView = GameData::toLocalData(Avatar(Avatar::Ziag));
    expect(!ordinaryView.playerOfAvatar(Avatar(Avatar::Lakkho)).army.findBattleUnitConst(390),
           "an ordinary avatar must not receive hidden creatures in local map data");
    expect(monacleView.playerOfAvatar(Avatar(Avatar::Lakkho)).army.findBattleUnitConst(390),
           "Monacle must reveal invisible creatures anywhere on the map");
    expect(GameData::gamers.playerOfAvatar(Avatar(Avatar::Lakkho))->army.findBattleUnitConst(390),
           "visibility filtering must not mutate the authoritative army");
    GameData::gamers = savedPlayers;

    LocalPlayer javed;
    javed.avatar = Avatar::Javed;
    javed.clan = Clan::Red;
    javed.wind = Wind::South;
    javed.points = 500;
    expect(!javed.mahjongApplySpell(Spell(Spell::Silence), Avatar(Avatar::Orachi)) &&
           !javed.isAffectedSpell(Spell::Silence),
           "Telepath must reject newly cast Silence");

    javed.affected.insert(AffectedSpell(Spell(Spell::Silence), 3));
    expect(!javed.isSilenced(),
           "Telepath must ignore Silence restored from an older save");
    expect(javed.allowCastSpell(Spell(Spell::Healing)),
           "Telepath must retain spell casting while Silence is present");

    LocalPlayer chao = javed;
    chao.stones.add(GameStone(Stone::Sword1));
    chao.stones.add(GameStone(Stone::Sword3));
    expect(chao.isMahjongChao(Wind(Wind::East), Stone(Stone::Sword2)),
           "Telepath must retain Chao while Silence is present");

    LocalPlayer pung = javed;
    pung.stones.add(GameStone(Stone::Sword4));
    pung.stones.add(GameStone(Stone::Sword4));
    expect(pung.isMahjongPung(Wind(Wind::East), Stone(Stone::Sword4)),
           "Telepath must retain Pung while Silence is present");

    LocalPlayer kong = javed;
    kong.stones.add(GameStone(Stone::Sword5));
    kong.stones.add(GameStone(Stone::Sword5));
    kong.stones.add(GameStone(Stone::Sword5));
    expect(kong.isMahjongKong1(Wind(Wind::East), Stone(Stone::Sword5)),
           "Telepath must retain Kong while Silence is present");

    LocalPlayer game = javed;
    const Stone::stone_t winningStones[] = {
        Stone::Skull1, Stone::Skull2, Stone::Skull3,
        Stone::Sword1, Stone::Sword2, Stone::Sword3,
        Stone::Number1, Stone::Number2, Stone::Number3,
        Stone::Dragon1, Stone::Dragon1, Stone::Dragon1,
        Stone::WindEast
    };
    for(const Stone::stone_t stone : winningStones) game.stones.add(GameStone(stone));
    expect(game.isWinMahjong(Wind(Wind::East), Wind(Wind::East), Stone(Stone::WindEast)),
           "Telepath must retain Game while Silence is present");

    LocalPlayer ordinaryWizard;
    ordinaryWizard.avatar = Avatar::Lakkho;
    ordinaryWizard.points = 500;
    expect(ordinaryWizard.mahjongApplySpell(Spell(Spell::Silence), Avatar(Avatar::Orachi)) &&
           ordinaryWizard.isSilenced() &&
           !ordinaryWizard.allowCastSpell(Spell(Spell::Healing)),
           "Silence must still block a non-Telepath wizard");
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

int runRecoverySelfTest()
{
    const char* directoryValue = std::getenv("FOUR_WINDS_RECOVERY_DIR");
    if(!directoryValue || !*directoryValue)
    {
        std::cerr << "FAIL: recovery test directory is missing\n";
        return 1;
    }

    const std::filesystem::path directory(directoryValue);
    std::error_code error;
    std::filesystem::remove_all(directory, error);

    for(int sequence = 1; sequence <= 4; ++sequence)
    {
        JsonObject metadata;
        metadata.addInteger("sequence", sequence);
        if(!Recovery::writeCheckpoint(directory.string(),
                                      std::string("state-") + std::to_string(sequence), metadata))
        {
            std::cerr << "FAIL: recovery checkpoint write failed at sequence " << sequence << '\n';
            return 1;
        }
    }

    bool valid = true;
    for(int slot = 0; slot < Recovery::SlotCount; ++slot)
    {
        std::string save;
        std::string metadataText;
        const int expected = 4 - slot;
        valid = valid && Systems::readFile2String(Recovery::savePath(directory.string(), slot), save) &&
            save == std::string("state-") + std::to_string(expected);
        valid = valid && Systems::readFile2String(Recovery::metadataPath(directory.string(), slot), metadataText) &&
            JsonContentString(metadataText).toObject().getInteger("sequence") == expected;
    }

    valid = valid && !Systems::isFile(Recovery::savePath(directory.string(), Recovery::SlotCount)) &&
        !Systems::isFile(Recovery::metadataPath(directory.string(), Recovery::SlotCount)) &&
        !Systems::isFile(Recovery::savePath(directory.string(), 0) + ".tmp") &&
        !Systems::isFile(Recovery::metadataPath(directory.string(), 0) + ".tmp");

    if(!valid)
    {
        std::cerr << "FAIL: recovery rotation or metadata pairing is invalid\n";
        return 1;
    }

    JsonObject orderedState;
    orderedState.addInteger("alpha", 1);
    orderedState.addInteger("omega", 2);
    JsonObject reversedState;
    reversedState.addInteger("omega", 2);
    reversedState.addInteger("alpha", 1);
    if(Recovery::stateHash(orderedState) != Recovery::stateHash(reversedState))
    {
        std::cerr << "FAIL: canonical state hash depends on object insertion order\n";
        return 1;
    }

    GameData::initPersons(Person(Avatar::Nucrus, Clan::Red, Wind::East));
    LocalPlayer* recoveryPlayer = GameData::gamers.playerOfWind(Wind::East);
    if(!recoveryPlayer)
    {
        std::cerr << "FAIL: production recovery player is missing\n";
        return 1;
    }
    BattleParty recoveryParty(recoveryPlayer->clan, Land::Corzen);
    if(!recoveryParty.join(combatCreature(recoveryPlayer->clan, Creature::Durlock,
                                          701, 2, 0, 2, 3, 1)))
    {
        std::cerr << "FAIL: production recovery replay party could not be created\n";
        return 1;
    }
    recoveryPlayer->army.push_back(recoveryParty);
    GameData::setAIDifficulty(AI::Difficulty::Hard);
    GameData::initAdventure();
    ActionList recoveryActions;
    if(!GameData::client2Adventure(recoveryPlayer->avatar,
                                   ClientUnitMoved(701, Land::None), recoveryActions))
    {
        std::cerr << "FAIL: production recovery replay action was rejected\n";
        return 1;
    }
    JsonObject gui;
    gui.addString("type", "RecoverySelfTest");
    if(!GameData::saveRecovery(gui, "production-self-test"))
    {
        std::cerr << "FAIL: production recovery checkpoint failed\n";
        return 1;
    }

    std::string productionSave;
    std::string productionMetadataText;
    valid = Systems::readFile2String(Recovery::savePath(directory.string(), 0), productionSave) &&
        Systems::readFile2String(Recovery::metadataPath(directory.string(), 0), productionMetadataText);
    const JsonObject productionState = JsonContentString(productionSave).toObject();
    const JsonObject productionMetadata = JsonContentString(productionMetadataText).toObject();
    const JsonArray* breadcrumbs = productionMetadata.getArray("recentBreadcrumbs");
    const JsonObject* metadataRng = productionMetadata.getObject("gameplayRng");
    const JsonObject* savedRng = productionState.getObject("gameplayRng");
    const JsonObject* replay = productionMetadata.getObject("replay");
    std::string replayError;
    valid = valid && productionState.isValid() &&
        productionState.getInteger("version") == FORMAT_VERSION_CURRENT &&
        productionMetadata.getInteger("schema") == 1 &&
        productionMetadata.getString("reason") == "production-self-test" &&
        productionMetadata.getString("gameRevision").size() > 6 &&
        productionMetadata.getString("engineRevision").size() > 6 &&
        productionMetadata.getString("fileHashFNV1a64") == Recovery::stateHash(productionSave) &&
        productionMetadata.getString("stateHashFNV1a64") == Recovery::stateHash(productionState) &&
        productionMetadata.getInteger("gamePart") == Menu::AdventurePart &&
        productionMetadata.getString("aiDifficulty") == "hard" &&
        metadataRng && savedRng && metadataRng->getString("algorithm") == GameplayRng::Algorithm &&
        metadataRng->getString("state") == savedRng->getString("state") &&
        replay && replay->getInteger("actionCount") == 1 &&
        replay->getBoolean("contiguousToCheckpoint") && Replay::run(*replay, &replayError) &&
        breadcrumbs && 0 < breadcrumbs->size();

    if(!valid)
    {
        std::cerr << "FAIL: production recovery metadata or state is invalid\n";
        return 1;
    }

    std::cout << "recovery checkpointing: ok\n";
    return 0;
}

#if defined(_WIN32)
int runWindowsCrashReportSelfTest(const char* executable)
{
    const char* directoryValue = std::getenv("FOUR_WINDS_DIAGNOSTICS_DIR");
    if(!directoryValue || !*directoryValue)
    {
        std::cerr << "FAIL: Windows crash test diagnostics directory is missing\n";
        return 1;
    }

    const std::filesystem::path directory(directoryValue);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    for(const auto & entry : std::filesystem::directory_iterator(directory, error))
    {
        if(entry.path().extension() == ".dmp")
            std::filesystem::remove(entry.path(), error);
    }
    std::filesystem::remove(directory / "crash-report.log", error);

    const std::string childExecutable = std::filesystem::absolute(executable).string();
    std::string command = "\"" + childExecutable + "\" --windows-crash-report-child";
    std::vector<char> commandLine(command.begin(), command.end());
    commandLine.push_back('\0');

    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if(!CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0,
                       nullptr, nullptr, &startup, &process))
    {
        std::cerr << "FAIL: Windows crash test child could not start\n";
        return 1;
    }

    const DWORD waitResult = WaitForSingleObject(process.hProcess, 30000);
    DWORD exitCode = STILL_ACTIVE;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    bool dumpFound = false;
    for(const auto & entry : std::filesystem::directory_iterator(directory, error))
    {
        if(entry.path().extension() == ".dmp" && entry.file_size(error) > 0)
            dumpFound = true;
    }

    std::string report;
    const bool reportValid = Systems::readFile2String(
        (directory / "crash-report.log").string(), report) &&
        report.find("[FATAL WINDOWS EXCEPTION]") != std::string::npos &&
        report.find("code=0xC0000005") != std::string::npos &&
        report.find("status=written") != std::string::npos;

    if(waitResult != WAIT_OBJECT_0 || exitCode == 0 || !dumpFound || !reportValid)
    {
        std::cerr << "FAIL: Windows native crash capture is incomplete"
                  << ", wait=" << waitResult << ", exit=" << exitCode
                  << ", dump=" << dumpFound << ", report=" << reportValid << '\n';
        return 1;
    }

    std::cout << "windows native crash reporting: ok\n";
    return 0;
}
#endif

void testMahjongCastDeadTargetMessage()
{
    const LocalPlayers savedPlayers = GameData::gamers;
    const Wind savedCurrentWind = GameData::currentWind;
    GameData::gamers.clear();

    LocalPlayer targetOwner;
    targetOwner.avatar = Avatar::Lakkho;
    targetOwner.clan = Clan::Yellow;
    targetOwner.wind = Wind::South;
    BattleParty targetParty(Clan::Yellow, Land::Zubrus);
    expect(targetParty.join(combatCreature(Clan::Yellow, Creature::Durlock,
                                           490, 1, 0, 1, 1)),
           "lethal spell message fixture must join");
    targetOwner.army.push_back(targetParty);
    GameData::gamers.push_back(targetOwner);

    LocalData observerSnapshot;
    observerSnapshot.players[0] = targetOwner;

    BattleTargets targets;
    targets << GameData::findBattleCreature(490);
    const MahjongCast message(Wind::East, Spell(Spell::LightningBolt),
                              Land::Zubrus, targets, std::vector<int>());
    expect(message.targetUnits().size() == 1 && message.targetUnits().front() == 490,
           "Mahjong cast messages must preserve target ids independently of live state");

    BattleCreature* liveTarget = GameData::findBattleCreature(490);
    expect(liveTarget != nullptr, "lethal spell message target must begin in live state");
    if(liveTarget) liveTarget->applyDamage(1);
    GameData::gamers.front().army.removeUnloyalty();

    expect(GameData::findBattleCreature(490) == nullptr,
           "lethal spell fixture must remove the target before the client reads its message");
    expect(message.targets().empty(),
           "decoding a stale spell target must not throw or resolve a removed live unit");
    expect(observerSnapshot.findBattleUnitConst(490) != nullptr,
           "the observer snapshot must retain target details for spell presentation");

    GameData::gamers.clear();
    LocalPlayer caster;
    caster.avatar = Avatar::Dayla;
    caster.clan = Clan::Purple;
    caster.wind = Wind::East;
    caster.points = 1000;
    GameData::gamers.push_back(caster);

    targetOwner.avatar = Avatar::Logun;
    targetOwner.clan = Clan::Yellow;
    targetOwner.wind = Wind::South;
    targetOwner.army.clear();
    BattleParty lethalParty(Clan::Yellow, Land::TowerOf4Winds);
    expect(lethalParty.join(combatCreature(Clan::Yellow, Creature::Durlock,
                                           491, 1, 0, 1, 1)),
           "lethal spell dispatch fixture must join");
    targetOwner.army.push_back(lethalParty);
    GameData::gamers.push_back(targetOwner);
    GameData::currentWind = Wind(Wind::East);

    ActionList emitted;
    const ClientCastSpell lethalCast(Spell(Spell::LightningBolt),
                                     Land(Land::TowerOf4Winds), 491, true);
    expect(GameData::client2Mahjong(Avatar(Avatar::Dayla), lethalCast, emitted),
           "lethal Lightning Bolt dispatch must complete without allocation failure");
    auto info = std::find_if(emitted.begin(), emitted.end(), [](const ActionMessage & action)
    {
        return action.type() == Action::MahjongInfo;
    });
    expect(info != emitted.end() && info->getString("info").find("vanquished") != std::string::npos,
           "lethal spell dispatch must own a valid vanquished-unit message");
    expect(GameData::findBattleCreature(491) == nullptr,
           "lethal Lightning Bolt dispatch must remove its defeated target");

    GameData::gamers = savedPlayers;
    GameData::currentWind = savedCurrentWind;
}

int runCapturedLightningRepro(const char* savePath)
{
    if(!savePath || !GameData::loadGame(savePath))
    {
        std::cerr << "FAIL: captured recovery save could not be loaded\n";
        return 1;
    }

    ActionList actions;
    if(!GameData::mahjong2Client(Avatar(Avatar::Logun), actions))
    {
        std::cerr << "FAIL: captured pre-crash Mahjong prompt was not restored\n";
        return 1;
    }

    actions.clear();
    if(!GameData::client2Mahjong(Avatar(Avatar::Logun), ClientButtonPass(), actions))
    {
        std::cerr << "FAIL: captured pre-crash pass action was rejected\n";
        return 1;
    }

    actions.clear();
    if(!GameData::mahjong2Client(Avatar(Avatar::Logun), actions))
    {
        std::cerr << "FAIL: captured post-Pung AI turn did not advance\n";
        return 1;
    }

    const bool vanquishedMessage = std::any_of(actions.begin(), actions.end(),
        [](const ActionMessage & action)
        {
            return action.type() == Action::MahjongInfo &&
                action.getString("info").find("vanquished") != std::string::npos;
        });
    if(!vanquishedMessage || GameData::findBattleCreature(1))
    {
        std::cerr << "FAIL: captured continuation did not execute lethal Lightning Bolt\n";
        for(const ActionMessage & action : actions)
            std::cerr << "  action=" << action.toString() << '\n';
        return 1;
    }

    std::cout << "captured Lightning Bolt replay: ok\n";
    return 0;
}
}

int main(int argc, char** argv)
{
#if defined(_WIN32)
    if(1 < argc && std::string(argv[1]) == "--windows-crash-report-child")
    {
        CrashReport::install("four-winds-reborn-native-test");
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        return 2;
    }

    if(1 < argc && std::string(argv[1]) == "--windows-crash-report-self-test")
        return runWindowsCrashReportSelfTest(argv[0]);
#endif

    if(1 < argc && std::string(argv[1]) == "--crash-report-self-test")
    {
        CrashReport::install("four-winds-reborn-test");
        CrashReport::breadcrumb("crash reporter self-test action");
        CrashReport::breadcrumb("crash reporter self-test phase transition");
        CrashReport::reportException("crash reporter self-test exception");
        const std::string report = CrashReport::filePath();
        CrashReport::shutdown();

        std::string content;
        bool valid = Systems::readFile2String(report, content) &&
            content.find("[ACTION] seq=1 crash reporter self-test action") != std::string::npos &&
            content.find("[ACTION] seq=2 crash reporter self-test phase transition") != std::string::npos &&
            content.find("[FATAL EXCEPTION] crash reporter self-test exception") != std::string::npos &&
            content.find("[SESSION END] failure") != std::string::npos &&
            CrashReport::breadcrumbSequence() == 2 &&
            CrashReport::recentBreadcrumbs().size() == 2;
#if defined(__APPLE__) || defined(__linux__)
        valid = valid && content.find("Stack trace:") != std::string::npos;
#endif
        if(!valid)
        {
            std::cerr << "FAIL: crash report is incomplete: " << report << '\n';
            return 1;
        }

        std::cout << "crash reporting: ok\n";
        return 0;
    }

    const Engine::exception diagnosticException("mahjongTick", "invalid state");
    expect(diagnosticException.function() == "mahjongTick" &&
           diagnosticException.message() == "invalid state",
           "engine exceptions must retain diagnostic context");

#if defined(_WIN32)
    const std::string windowsHome = Systems::homeDirectory("four-winds-reborn");
    expect(Systems::basename(windowsHome) == "four-winds-reborn",
           "Windows application data path must retain its application subdirectory");
#endif

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

    if(2 < argc && std::string(argv[1]) == "--captured-lightning-repro")
        return runCapturedLightningRepro(argv[2]);

    if(1 < argc && std::string(argv[1]) == "--recovery-self-test")
        return runRecoverySelfTest();

    if(1 < argc && std::string(argv[1]) == "--fixed-seed-replay")
        return runFixedSeedReplaySelfTest();

    if(1 < argc && std::string(argv[1]) == "--balance-only")
    {
        const int balanceFailures = runAvatarBalanceTests();
        if(balanceFailures) return 1;
        std::cout << "avatar balance metrics: ok\n";
        return 0;
    }

    testDifficultyRules();
    testSelectedPartyMovement();
    testGateMovement();
    testLuckAbility();
    testAvatarPassives();
    testSpellCastingAI();
    testMahjongCastDeadTargetMessage();
    testAdventureProfiles();
    testAdventureCoordination();
    testBattleAI();
    testGameplayRngState();
    testActionReplay();

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
