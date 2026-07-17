#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "aispell.h"
#include "battle.h"

namespace GameData
{
    extern LocalPlayers gamers;
}

namespace
{
    constexpr int SummonBudget = 700;
    constexpr int PartyLimit = 3;
    constexpr int LoyaltyScore = 14;
    constexpr int UtilityScore = 12;
    constexpr int OptionScore = 8;
    constexpr int ProjectedLuckDraws = 6;
    constexpr int SilenceDenialScore = 70;

    int failures = 0;

    struct RosterPlan
    {
        int score = 0;
        int cost = 0;
        Creatures creatures;
    };

    struct AvatarBalanceRow
    {
        Avatar avatar;
        int summonScore = 0;
        int summonCost = 0;
        int spellScore = 0;
        int passiveScore = 0;
        int optionsScore = 0;
        int baseline = 0;
        Spell bestSpell;
        Creatures party;
    };

    void expect(bool condition, const char* message)
    {
        if(condition) return;

        std::cerr << "BALANCE FAIL: " << message << '\n';
        ++failures;
    }

    class GameStateGuard
    {
        LocalPlayers players;
        AI::Difficulty difficulty;

    public:
        GameStateGuard()
            : players(GameData::gamers), difficulty(GameData::aiDifficulty())
        {
        }

        ~GameStateGuard()
        {
            GameData::gamers = players;
            GameData::setAIDifficulty(difficulty);
        }
    };

    void searchRoster(const AvatarInfo & avatar, int start, int budget, int score,
                      Creatures & selected, RosterPlan & best)
    {
        if(!selected.empty() && best.score < score)
        {
            best.score = score;
            best.cost = SummonBudget - budget;
            best.creatures = selected;
        }

        if(static_cast<int>(selected.size()) == PartyLimit) return;

        for(int index = start; index < static_cast<int>(avatar.creatures.size()); ++index)
        {
            const Creature creature = avatar.creatures[index];
            const CreatureInfo & info = GameData::creatureInfo(creature);
            if(info.cost > budget) continue;

            const bool repeated = std::find(selected.begin(), selected.end(), creature) != selected.end();
            if(repeated && info.unique) continue;

            selected.push_back(creature);
            searchRoster(avatar, info.unique ? index + 1 : index, budget - info.cost,
                         score + AI::creatureSummonScore(creature, AI::BehaviorProfile::Balanced),
                         selected, best);
            selected.pop_back();
        }
    }

    RosterPlan bestRoster(const AvatarInfo & avatar)
    {
        RosterPlan result;
        Creatures selected;
        searchRoster(avatar, 0, SummonBudget, 0, selected, result);
        return result;
    }

    int avatarPassiveScore(const AvatarInfo & avatar, const RosterPlan & roster)
    {
        const int partySize = static_cast<int>(roster.creatures.size());
        switch(avatar.ability())
        {
            case Ability::Bard:
                return partySize * 2 * LoyaltyScore;

            case Ability::Monacle:
                return partySize * UtilityScore;

            case Ability::Luck:
                return ProjectedLuckDraws * OptionScore;

            case Ability::Telepath:
                return SilenceDenialScore;

            // Catastrophic is already represented by Hell Blast in spellScore.
            default:
                return 0;
        }
    }

    BattleCreature battleCreature(const Clan & clan, const Creature & creature, int uid)
    {
        return BattleCreature(clan, creature, uid);
    }

    std::vector<Avatar> opponentAvatars(const Avatar & tested)
    {
        std::vector<Avatar> result;
        for(const auto id : avatars_all)
        {
            const Avatar avatar(id);
            if(avatar != tested) result.push_back(avatar);
            if(result.size() == 3) break;
        }
        return result;
    }

    void setupSpellBoard(const Avatar & tested)
    {
        GameData::gamers.clear();

        LocalPlayer candidate;
        candidate.avatar = tested;
        candidate.clan = Clan::Red;
        candidate.wind = Wind::East;
        candidate.points = 500;

        BattleParty allies(Clan::Red, Land::Corzen);
        BattleCreature wounded = battleCreature(Clan::Red, Creature::Durlock, 1001);
        wounded.applyDamage(2);
        allies.join(wounded);
        allies.join(battleCreature(Clan::Red, Creature::FireGiant, 1002));
        candidate.army.push_back(allies);
        GameData::gamers.push_back(candidate);

        const std::vector<Avatar> opponents = opponentAvatars(tested);
        const Clan::clan_t clans[] = { Clan::Yellow, Clan::Aqua, Clan::Purple };
        const Wind::wind_t winds[] = { Wind::South, Wind::West, Wind::North };

        for(int index = 0; index < 3; ++index)
        {
            LocalPlayer opponent;
            opponent.avatar = opponents[index];
            opponent.clan = Clan(clans[index]);
            opponent.wind = Wind(winds[index]);
            opponent.points = 500;

            if(index == 0)
            {
                BattleParty targets(opponent.clan, Land::Zubrus);
                targets.join(battleCreature(opponent.clan, Creature::SkeletonHorde, 1101));
                targets.join(battleCreature(opponent.clan, Creature::Durlock, 1102));
                targets.join(battleCreature(opponent.clan, Creature::Wraith, 1103));
                opponent.army.push_back(targets);
            }

            GameData::gamers.push_back(opponent);
        }
    }

    AI::SpellCastPlan spellPlan(const Avatar & avatar, const Spells & spells,
                                AI::BehaviorProfile profile = AI::BehaviorProfile::Balanced)
    {
        setupSpellBoard(avatar);
        return AI::chooseSpellCast(GameData::gamers.front(), spells, profile, spells);
    }

    bool planContains(const AI::SpellCastPlan & plan, const Spell & spell)
    {
        if(plan.spell == spell) return true;
        return std::any_of(plan.followUps.begin(), plan.followUps.end(), [&](const AI::SpellCastStep & step)
        {
            return step.spell == spell;
        });
    }

    int partySpellDamage(const Spell & spell, int targets)
    {
        BattleParty party(Clan::Yellow, Land::Zubrus);
        for(int index = 0; index < targets; ++index)
            expect(party.join(battleCreature(Clan::Yellow, Creature::Durlock, 1200 + index)),
                   "Hell Blast damage fixture must fit in a party");

        int before = 0;
        int after = 0;
        for(BattleCreature* creature : party.toBattleCreatures()) before += creature->loyalty();
        for(BattleCreature* creature : party.toBattleCreatures()) creature->applySpell(spell);
        for(BattleCreature* creature : party.toBattleCreatures()) after += creature->loyalty();
        return before - after;
    }

    long double combinations(int total, int selected)
    {
        if(selected < 0 || total < selected) return 0.0L;
        selected = std::min(selected, total - selected);
        long double result = 1.0L;
        for(int index = 1; index <= selected; ++index)
            result = result * (total - selected + index) / index;
        return result;
    }

    long double openingCastProbability(const CroupierSet & wall, const Stones & cost)
    {
        if(cost.size() != 2 || cost[0] == cost[1]) return 0.0L;

        const int total = static_cast<int>(wall.bank.size());
        const int first = std::count(wall.bank.begin(), wall.bank.end(), cost[0]);
        const int second = std::count(wall.bank.begin(), wall.bank.end(), cost[1]);
        const long double allHands = combinations(total, GAME_SET_COUNT);
        const long double missingFirst = combinations(total - first, GAME_SET_COUNT);
        const long double missingSecond = combinations(total - second, GAME_SET_COUNT);
        const long double missingBoth = combinations(total - first - second, GAME_SET_COUNT);
        return 1.0L - (missingFirst + missingSecond - missingBoth) / allHands;
    }

    std::string partyName(const Creatures & creatures)
    {
        std::ostringstream stream;
        for(auto it = creatures.begin(); it != creatures.end(); ++it)
        {
            if(it != creatures.begin()) stream << '/';
            stream << GameData::creatureInfo(*it).name;
        }
        return stream.str();
    }

    void printRows(std::vector<AvatarBalanceRow> rows)
    {
        std::sort(rows.begin(), rows.end(), [](const AvatarBalanceRow & left,
                                               const AvatarBalanceRow & right)
        {
            return left.baseline > right.baseline;
        });

        std::cout << "\navatar balance baseline (passives modeled; Catastrophic is in spell score)\n";
        std::cout << "avatar      total  summon/cost/size  spell  passive  options  best spell       best party\n";
        for(const AvatarBalanceRow & row : rows)
        {
            const AvatarInfo & info = GameData::avatarInfo(row.avatar);
            const std::string spell = row.bestSpell.isValid() ?
                                      GameData::spellInfo(row.bestSpell).name : "none";
            std::cout << std::left << std::setw(11) << info.name
                      << std::right << std::setw(6) << row.baseline << "  "
                      << std::setw(5) << row.summonScore << '/' << std::setw(3) << row.summonCost
                      << '/' << row.party.size() << "  "
                      << std::setw(5) << row.spellScore << "  "
                      << std::setw(7) << row.passiveScore << "  "
                      << std::setw(7) << row.optionsScore << "  "
                      << std::left << std::setw(16) << spell << partyName(row.party) << '\n';
        }
        std::cout << "passive model: Bard=2 loyalty x party x 14; Monocle=party x 12 utility; "
                  << "Luck=6 draws x 8 option value; Telepath=70 Silence denial\n";
    }
}

int runAvatarBalanceTests(void)
{
    GameStateGuard guard;
    GameData::setAIDifficulty(AI::Difficulty::Normal);
    failures = 0;

    std::vector<AvatarBalanceRow> rows;
    for(const auto id : avatars_all)
    {
        const Avatar avatar(id);
        const AvatarInfo & info = GameData::avatarInfo(avatar);
        const RosterPlan roster = bestRoster(info);
        const AI::SpellCastPlan spells = spellPlan(avatar, info.spells);

        AvatarBalanceRow row;
        row.avatar = avatar;
        row.summonScore = roster.score;
        row.summonCost = roster.cost;
        row.spellScore = spells.score;
        row.passiveScore = avatarPassiveScore(info, roster);
        row.optionsScore = std::min<int>(12, info.creatures.size()) * OptionScore +
                           std::min<int>(12, info.spells.size()) * OptionScore;
        row.baseline = row.summonScore + row.spellScore + row.passiveScore + row.optionsScore;
        row.bestSpell = spells.spell;
        row.party = roster.creatures;
        rows.push_back(row);

        expect(info.creatures.size() >= 4, "every avatar must have at least four summon options");
        expect(info.spells.size() >= 3, "every avatar must have at least three spell options");
        expect(roster.score > 0 && 2 <= roster.creatures.size(),
               "every avatar must field at least two useful creatures under the common budget");
        expect(spells.isValid(), "every avatar must have a useful cast in the common spell scenario");
        const bool modeledPassive = info.ability == Ability(Ability::Bard) ||
                                    info.ability == Ability(Ability::Monacle) ||
                                    info.ability == Ability(Ability::Luck) ||
                                    info.ability == Ability(Ability::Telepath);
        expect(modeledPassive == (row.passiveScore > 0),
               "every non-spell avatar passive must contribute exactly once to the baseline");
    }

    std::vector<int> baselines;
    std::vector<int> summonScores;
    for(const AvatarBalanceRow & row : rows)
    {
        baselines.push_back(row.baseline);
        summonScores.push_back(row.summonScore);
    }
    std::sort(baselines.begin(), baselines.end());
    std::sort(summonScores.begin(), summonScores.end());
    const int medianBaseline = baselines[baselines.size() / 2];
    const int medianSummon = summonScores[summonScores.size() / 2];
    expect(baselines.front() * 100 >= medianBaseline * 55,
           "no avatar baseline may fall below 55 percent of the median");
    expect(baselines.back() * 100 <= medianBaseline * 180,
           "no avatar baseline may exceed 180 percent of the median");
    expect(summonScores.front() * 100 >= medianSummon * 65,
           "no summon roster may fall below 65 percent of the median roster");

    const AvatarInfo & niana = GameData::avatarInfo(Avatar(Avatar::Niana));
    const Spell hellBlast(Spell::HellBlast);
    const Spell lightning(Spell::LightningBolt);
    expect(niana.ability == Ability(Ability::Catasrophic),
           "Niana must retain the Catastrophic ability");
    expect(std::count(niana.spells.begin(), niana.spells.end(), hellBlast) == 1,
           "Catastrophic must grant Niana exactly one Hell Blast entry");
    for(const auto id : avatars_all)
    {
        if(id == Avatar::Niana) continue;
        const Spells & spells = GameData::avatarInfo(Avatar(id)).spells;
        expect(std::find(spells.begin(), spells.end(), hellBlast) == spells.end(),
               "Hell Blast must not be a native spell of a non-Catastrophic avatar");
    }

    const SpellInfo & hellInfo = GameData::spellInfo(hellBlast);
    const SpellInfo & lightningInfo = GameData::spellInfo(lightning);
    expect(hellInfo.target() == (SpellTarget::Enemy | SpellTarget::Party),
           "Hell Blast must remain limited to one enemy party");
    expect(hellInfo.cost == lightningInfo.cost && hellInfo.cost == 80,
           "Hell Blast and Lightning Bolt must retain the same 80-point mana cost");
    expect(hellInfo.stones.size() == 2,
           "Hell Blast burst damage must remain gated by two specific runes");
    expect(partySpellDamage(hellBlast, 1) == 2 && partySpellDamage(hellBlast, 2) == 4 &&
           partySpellDamage(hellBlast, 3) == 6,
           "Hell Blast must deal exactly two loyalty to each member of a party");
    expect(partySpellDamage(lightning, 1) == 1,
           "Lightning Bolt baseline must remain one loyalty to one creature");

    Spells onlyHellBlast;
    onlyHellBlast.push_back(hellBlast);
    Spells onlyLightning;
    onlyLightning.push_back(lightning);
    const AI::SpellCastPlan hellPlan = spellPlan(Avatar(Avatar::Niana), onlyHellBlast,
                                                  AI::BehaviorProfile::Aggressive);
    const AI::SpellCastPlan lightningPlan = spellPlan(Avatar(Avatar::Niana), onlyLightning,
                                                       AI::BehaviorProfile::Aggressive);
    const AI::SpellCastPlan fullPlan = spellPlan(Avatar(Avatar::Niana), niana.spells,
                                                  AI::BehaviorProfile::Aggressive);
    Spells nianaWithoutHellBlast = niana.spells;
    nianaWithoutHellBlast.erase(std::remove(nianaWithoutHellBlast.begin(),
                                            nianaWithoutHellBlast.end(), hellBlast),
                                nianaWithoutHellBlast.end());
    const AI::SpellCastPlan planWithoutHellBlast = spellPlan(Avatar(Avatar::Niana),
        nianaWithoutHellBlast, AI::BehaviorProfile::Aggressive);
    expect(hellPlan.isValid() && hellPlan.score > lightningPlan.score,
           "Niana AI must value Hell Blast above Lightning Bolt against a full party");
    expect(planContains(fullPlan, hellBlast),
           "Niana AI must include Hell Blast in its dense-party spell plan");
    expect(fullPlan.score > planWithoutHellBlast.score,
           "Catastrophic must materially improve Niana's dense-party spell plan");

    CroupierSet wall;
    const long double openingChance = openingCastProbability(wall, hellInfo.stones) * 100.0L;
    expect(10.0L < openingChance && openingChance < 11.0L,
           "Hell Blast opening-hand availability must stay near 10.6 percent");

    printRows(rows);
    std::cout << "Hell Blast: damage=2/4/6, mana=" << hellInfo.cost
              << ", opening readiness=" << std::fixed << std::setprecision(2)
              << static_cast<double>(openingChance) << "%"
              << ", AI score=" << hellPlan.score << " vs Lightning " << lightningPlan.score
              << ", Niana marginal=" << fullPlan.score - planWithoutHellBlast.score << '\n';

    return failures;
}
