#include <algorithm>
#include <map>
#include <set>
#include <sstream>

#include "aistrategy.h"

namespace
{
    int visibleRuleStoneCount(const WinRule & rule, const Stone & stone)
    {
        if(rule.isHidden()) return 0;
        if(rule.isChao())
            return stone == rule.stone() || stone == rule.stone().next() ||
                   stone == rule.stone().next().next() ? 1 : 0;
        if(rule.isPung() || rule.isKong())
            return stone == rule.stone() ? rule.count() : 0;
        return 0;
    }

    int uncastedStoneCount(const GameStones & hand, const Stone & stone)
    {
        return static_cast<int>(std::count_if(hand.begin(), hand.end(), [&](const Stone & value)
        {
            return value == stone && !GameStone(value).isCasted();
        }));
    }

    int goalId(const AI::RuneGoal & goal)
    {
        return goal.type == AI::StrategicGoalType::Summon ? goal.creature() : goal.spell();
    }

    int adjacentVisibleThreat(const AI::AIObservation & observation, const Land & land)
    {
        int threat = 0;
        const std::vector<Land> & borders = GameData::landInfo(land).borders;
        for(const LocalPlayer & visible : observation.state().players)
        {
            if(visible.clan == observation.player().clan) continue;
            for(const Land & border : borders)
            {
                const BattleParty* party = visible.army.findPartyConst(border);
                if(!party) continue;
                const BaseStat stat = party->toBaseStatSummary();
                threat += stat.attack * 2 + stat.ranger * 2 + stat.defense + stat.loyalty;
            }
        }
        return threat;
    }

    int resultingPartySynergy(const BattleParty* party, const CreatureInfo & info)
    {
        if(!party) return 0;

        const BaseStat current = party->toBaseStatSummary();
        int synergy = party->count() * 30;
        if(0 < info.stat.ranger && current.ranger == 0) synergy += 70;
        if(info.specials.check(Speciality::SeeInvisible) &&
           party->toBattleCreatures(Specials() << Speciality::SeeInvisible, true).empty())
            synergy += 55;

        const int currentMove = party->movePoint();
        if(0 < currentMove)
        {
            if(info.stat.move < currentMove) synergy -= (currentMove - info.stat.move) * 45;
            else synergy += 20;
        }

        if(info.specials.check(Speciality::Merge))
        {
            const BattleCreatures members = party->toBattleCreatures();
            const int matching = static_cast<int>(std::count_if(
                members.begin(), members.end(), [&](const BattleCreature* creature)
                {
                    return creature && creature->id() == info.id.id() &&
                           creature->haveSpeciality(Speciality::Merge);
                }));
            synergy += matching * 220;
        }
        return synergy;
    }

    AI::RuneGoal buildRuneGoal(const AI::AIObservation & observation,
                               AI::StrategicGoalType type, const Creature & creature,
                               const Spell & spell, const std::string & name,
                               const Stones & required, int manaRequired,
                               int goalValue, int drawHorizon)
    {
        AI::RuneGoal goal;
        goal.type = type;
        goal.creature = creature;
        goal.spell = spell;
        goal.name = name;
        goal.required = required;
        goal.manaRequired = manaRequired;
        goal.manaGap = std::max(0, manaRequired - observation.player().points);
        goal.goalValue = goalValue;

        std::map<int, int> used;
        for(const Stone & stone : required)
        {
            if(used[stone()] < uncastedStoneCount(observation.player().stones, stone))
                ++goal.matchedRunes;
            else
                goal.missing.push_back(stone);
            ++used[stone()];
        }

        std::map<int, int> missingCopies;
        for(const Stone & stone : goal.missing) ++missingCopies[stone()];

        int probability = goal.missing.empty() ? 100 : 100;
        const int unseenPool = std::max(1, observation.state().stoneLastCount);
        for(const auto & entry : missingCopies)
        {
            const Stone stone(static_cast<Stone::stone_t>(entry.first));
            const int unseenCopies = std::max(0, 4 - observation.visibleStoneCount(stone));
            goal.visibleRemaining += unseenCopies;

            for(int copy = 0; copy < entry.second; ++copy)
            {
                const int available = std::max(0, unseenCopies - copy);
                const int drawChance = std::clamp(available * drawHorizon * 100 / unseenPool, 0, 100);
                probability = probability * drawChance / 100;
            }
        }
        goal.completionProbability = goal.missing.empty() ? 100 : probability;

        goal.runeGain = goal.matchedRunes * 65 - static_cast<int>(goal.missing.size()) * 35;
        goal.availabilityValue = goal.completionProbability / 2;
        goal.score = goal.goalValue + goal.runeGain + goal.availabilityValue - goal.manaGap / 5;
        return goal;
    }
}

AI::AIObservation::AIObservation(const LocalData & value) : local(value)
{
}

int AI::AIObservation::visibleStoneCount(const Stone & stone) const
{
    int result = static_cast<int>(std::count(local.trashSet.begin(), local.trashSet.end(), stone));
    for(const LocalPlayer & visible : local.players)
    {
        result += static_cast<int>(std::count(visible.stones.begin(), visible.stones.end(), stone));
        if(visible.newStone == stone) ++result;
        for(const WinRule & rule : visible.rules)
            result += visibleRuleStoneCount(rule, stone);
    }
    return result;
}

bool AI::AIObservation::visibleCreature(const Creature & creature) const
{
    return std::any_of(local.players.begin(), local.players.end(), [&](const LocalPlayer & visible)
    {
        return visible.army.findCreature(creature);
    });
}

AI::RuneGoal::RuneGoal()
    : type(StrategicGoalType::Summon), matchedRunes(0), visibleRemaining(0),
      completionProbability(0), manaRequired(0), manaGap(0), goalValue(0),
      runeGain(0), availabilityValue(0), score(0)
{
}

bool AI::RuneGoal::requires(const Stone & stone) const
{
    return required.end() != std::find(required.begin(), required.end(), stone);
}

std::string AI::RuneGoal::trace(void) const
{
    std::ostringstream os;
    os << "goal=" << name << ", kind=" <<
        (type == StrategicGoalType::Summon ? "summon" : "spell") <<
        ", score=" << score << ", goal_value=" << goalValue <<
        ", rune_gain=" << runeGain << ", availability=" << availabilityValue <<
        ", progress=" << matchedRunes << "/" << required.size() <<
        ", completion=" << completionProbability << "%" <<
        ", visible_remaining=" << visibleRemaining <<
        ", mana_gap=" << manaGap;
    return os.str();
}

AI::StrategicIntent::StrategicIntent()
    : profile(BehaviorProfile::Balanced), difficulty(Difficulty::Normal)
{
}

AI::SummonCandidate::SummonCandidate()
    : staticValue(0), partySynergy(0), frontierValue(0), score(0)
{
}

std::string AI::SummonCandidate::trace(void) const
{
    std::ostringstream os;
    os << "summon=" << (creature.isValid() ? GameData::creatureInfo(creature).name : "none")
       << ", destination=" << destination.toString() << ", score=" << score
       << ", static=" << staticValue << ", party_synergy=" << partySynergy
       << ", frontier=" << frontierValue;
    return os.str();
}

int AI::StrategicIntent::runeValue(const Stone & stone) const
{
    int value = 0;
    for(std::size_t index = 0; index < runeGoals.size(); ++index)
    {
        const RuneGoal & goal = runeGoals[index];
        if(!goal.requires(stone)) continue;

        const int rankWeight = static_cast<int>(runeGoals.size() - index);
        const int goalWeight = std::max(8, std::max(0, goal.score) /
                                           std::max(1, static_cast<int>(goal.required.size())));
        value += goalWeight * rankWeight / std::max(1, static_cast<int>(runeGoals.size()));
    }
    return value;
}

std::string AI::StrategicIntent::trace(void) const
{
    std::ostringstream os;
    os << "profile=" << behaviorProfileName(profile)
       << ", difficulty=" << difficultyName(difficulty);
    for(const RuneGoal & goal : runeGoals) os << "; " << goal.trace();
    return os.str();
}

AI::AIObservation AI::observePlayer(const Avatar & avatar)
{
    return AIObservation(GameData::toLocalData(avatar));
}

AI::StrategicIntent AI::chooseStrategicIntent(const AIObservation & observation,
                                               BehaviorProfile profile, Difficulty difficulty)
{
    StrategicIntent intent;
    intent.profile = profile;
    intent.difficulty = difficulty;

    const LocalPlayer & player = observation.player();
    const AvatarInfo & avatar = GameData::avatarInfo(player.avatar);
    const DifficultyRules & budget = difficultyRules(difficulty);

    if(!player.army.isMaximumSummoning())
    {
        for(const Creature & creature : avatar.creatures)
        {
            const CreatureInfo & info = GameData::creatureInfo(creature);
            if(info.stones.empty() || (info.unique && observation.visibleCreature(creature))) continue;

            intent.runeGoals.push_back(buildRuneGoal(
                observation, StrategicGoalType::Summon, creature, Spell(), info.name,
                info.stones, info.cost, creatureSummonScore(creature, profile) / 8,
                budget.strategicDrawHorizon));
        }
    }

    std::set<int> knownSpells;
    auto addSpellGoal = [&](const Spell & spell)
    {
        if(!spell.isValid() || !knownSpells.insert(spell()).second) return;
        const SpellInfo & info = GameData::spellInfo(spell);
        if(info.stones.empty()) return;

        const int spellValue = 45 + info.cost / 2 +
                               static_cast<int>(info.stones.size()) * 12;
        intent.runeGoals.push_back(buildRuneGoal(
            observation, StrategicGoalType::Spell, Creature(), spell, info.name,
            info.stones, info.cost, spellValue, budget.strategicDrawHorizon));
    };
    for(const Spell & spell : avatar.spells) addSpellGoal(spell);
    for(const BattleCreature* creature : player.army.toBattleCreatures())
    {
        if(!creature) continue;
        for(const Speciality & speciality : GameData::creatureInfo(*creature).specials.toList())
            addSpellGoal(speciality.toSpell());
    }

    std::sort(intent.runeGoals.begin(), intent.runeGoals.end(), [](const RuneGoal & left,
                                                                  const RuneGoal & right)
    {
        if(left.score != right.score) return left.score > right.score;
        if(left.matchedRunes != right.matchedRunes) return left.matchedRunes > right.matchedRunes;
        if(left.type != right.type) return left.type == StrategicGoalType::Summon;
        return goalId(left) < goalId(right);
    });

    if(static_cast<int>(intent.runeGoals.size()) > budget.strategicGoalLimit)
        intent.runeGoals.resize(budget.strategicGoalLimit);
    return intent;
}

std::vector<AI::SummonCandidate> AI::rankSummonCandidates(const AIObservation & observation,
                                                          const Creatures & creatures,
                                                          BehaviorProfile profile)
{
    std::vector<SummonCandidate> result;
    const LocalPlayer & player = observation.player();
    Lands lands = Lands::thisClan(player.clan).powerOnly();

    for(const Creature & creature : creatures)
    {
        const CreatureInfo & info = GameData::creatureInfo(creature);
        if(info.unique && observation.visibleCreature(creature)) continue;

        for(const Land & land : lands)
        {
            const BattleParty* party = player.army.findPartyConst(land);
            if(party && !party->canJoin()) continue;

            SummonCandidate candidate;
            candidate.creature = creature;
            candidate.destination = land;
            candidate.staticValue = creatureSummonScore(creature, profile);
            candidate.partySynergy = resultingPartySynergy(party, info);

            const int threat = adjacentVisibleThreat(observation, land);
            candidate.frontierValue = GameData::landInfo(land).stat.point +
                std::min(180, threat * (info.stat.defense + info.stat.loyalty) / 20);
            candidate.score = candidate.staticValue + candidate.partySynergy +
                              candidate.frontierValue;
            result.push_back(candidate);
        }
    }

    std::sort(result.begin(), result.end(), [](const SummonCandidate & left,
                                               const SummonCandidate & right)
    {
        if(left.score != right.score) return left.score > right.score;
        if(left.creature() != right.creature()) return left.creature() < right.creature();
        return left.destination() < right.destination();
    });
    return result;
}
