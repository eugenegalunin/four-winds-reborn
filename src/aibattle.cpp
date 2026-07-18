#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <random>

#include "aibattle.h"
#include "battle.h"

namespace
{
    int specialityValue(const BattleCreature & creature)
    {
        int value = 0;
        if(creature.haveSpeciality(Speciality::FirstStrike)) value += 180;
        if(creature.haveSpeciality(Speciality::Swarm)) value += 160;
        if(creature.haveSpeciality(Speciality::MightyBlow)) value += 100;
        if(creature.haveSpeciality(Speciality::FireShield)) value += 80;
        if(creature.haveSpeciality(Speciality::Merge)) value += 70;
        if(creature.haveSpeciality(Speciality::Regeneration)) value += 60;
        if(creature.haveSpeciality(Speciality::CastHellblast)) value += 50;
        if(creature.haveSpeciality(Speciality::IgnoreMissiles)) value += 45;
        if(creature.haveSpeciality(Speciality::MagicResistence)) value += 40;
        if(creature.haveSpeciality(Speciality::Devotion)) value += 35;
        return value;
    }

    int shieldReduction(const BattleCreature & target)
    {
        return target.isAffectedSpell(Spell::ForceShield) ?
               GameData::spellInfo(Spell::ForceShield).value : 0;
    }

    int estimatedDamage(const BattleUnit & attacker, const BattleCreature & target,
                        AI::BattleAttackMode mode, int modifier)
    {
        if(mode == AI::BattleAttackMode::Ranged)
            return Battle::rangedDamage(attacker.ranger(), shieldReduction(target));

        int damage = attacker.attack() + modifier - target.defense();
        if(attacker.haveSpeciality(Speciality::MightyBlow)) damage += 1;
        return std::max(1, damage);
    }

    int targetScore(const BattleUnit & attacker, const BattleCreature & target,
                    AI::BattleAttackMode mode, const AI::BehaviorRules & rules,
                    int overkillPenalty, int remainingLoyalty, int modifier)
    {
        if(remainingLoyalty <= 0) return std::numeric_limits<int>::min();
        if(mode == AI::BattleAttackMode::Ranged &&
           target.haveSpeciality(Speciality::IgnoreMissiles))
            return std::numeric_limits<int>::min();

        const int damage = estimatedDamage(attacker, target, mode, modifier);
        const bool lethal = remainingLoyalty <= damage;
        const int threat = target.attack() * 2 + target.ranger() * 3 + target.loyalty();
        const int wounded = std::max(0, target.baseLoyalty() - remainingLoyalty);
        const int overkill = std::max(0, damage - remainingLoyalty);
        int score = (lethal ? rules.battleKillBonus : 0) +
                    threat * rules.battleThreatWeight / 100 +
                    specialityValue(target) * rules.battleSpecialWeight / 100 +
                    wounded * 20 - target.defense() * 5 -
                    overkill * overkillPenalty;

        if(mode == AI::BattleAttackMode::Melee &&
           target.haveSpeciality(Speciality::FireShield))
            score -= rules.battleLoyaltyWeight;
        return score;
    }

    std::vector<const BattleCreature*> rankedTargets(const BattleUnit & attacker,
                                                      const BattleParty & targets,
                                                      AI::BattleAttackMode mode,
                                                      AI::BehaviorProfile profile,
                                                      const std::map<int, int>* projected = nullptr,
                                                      int modifier = 0)
    {
        const AI::BehaviorRules & rules = AI::behaviorRules(profile);
        const int overkillPenalty = AI::battleOverkillPenalty(profile,
                                                               GameData::aiDifficulty());
        std::vector<const BattleCreature*> result;
        for(const BattleCreature* target : targets.toBattleCreatures())
        {
            const auto projection = projected ? projected->find(target->battleUnit()) :
                                                std::map<int, int>::const_iterator();
            const int loyalty = projected && projection != projected->end() ?
                                projection->second : target->loyalty();
            const int targetModifier = modifier -
                (mode == AI::BattleAttackMode::Melee ? Battle::mergeDefenseBonus(targets, *target) : 0);
            if(targetScore(attacker, *target, mode, rules, overkillPenalty,
                           loyalty, targetModifier) !=
               std::numeric_limits<int>::min())
                result.push_back(target);
        }

        std::sort(result.begin(), result.end(), [&](const BattleCreature* left,
                                                    const BattleCreature* right)
        {
            const int leftLoyalty = projected ? projected->at(left->battleUnit()) : left->loyalty();
            const int rightLoyalty = projected ? projected->at(right->battleUnit()) : right->loyalty();
            const int leftModifier = modifier -
                (mode == AI::BattleAttackMode::Melee ? Battle::mergeDefenseBonus(targets, *left) : 0);
            const int rightModifier = modifier -
                (mode == AI::BattleAttackMode::Melee ? Battle::mergeDefenseBonus(targets, *right) : 0);
            const int leftScore = targetScore(attacker, *left, mode, rules, overkillPenalty,
                                              leftLoyalty, leftModifier);
            const int rightScore = targetScore(attacker, *right, mode, rules, overkillPenalty,
                                               rightLoyalty, rightModifier);
            return leftScore == rightScore ? left->battleUnit() < right->battleUnit() :
                                             leftScore > rightScore;
        });
        return result;
    }

    AI::BattleAttackPlan meleePlanFor(const BattleCreature & attacker,
                                      const BattleParty & allies,
                                      const BattleParty & enemies,
                                      AI::BehaviorProfile profile)
    {
        AI::BattleAttackPlan plan;
        const int modifier = std::max(0, allies.count() - 1);
        const auto targets = rankedTargets(attacker, enemies, AI::BattleAttackMode::Melee,
                                           profile, nullptr, modifier);
        if(targets.empty()) return plan;

        plan.attacker = attacker.battleUnit();
        if(attacker.haveSpeciality(Speciality::Swarm))
        {
            for(const BattleCreature* target : targets) plan.targets.push_back(target->battleUnit());
        }
        else
        {
            plan.targets.push_back(targets.front()->battleUnit());
        }
        plan.score = AI::battleInitiativeScore(attacker, AI::BattleAttackMode::Melee, profile) +
                     targetScore(attacker, *targets.front(), AI::BattleAttackMode::Melee,
                                 AI::behaviorRules(profile), AI::battleOverkillPenalty(
                                     profile, GameData::aiDifficulty()), targets.front()->loyalty(),
                                 modifier - Battle::mergeDefenseBonus(enemies, *targets.front()));
        return plan;
    }

    int partyLoyalty(const BattleParty & party)
    {
        int result = 0;
        for(const BattleCreature* creature : party.toBattleCreatures())
            result += std::max(0, creature->loyalty());
        return result;
    }

    void mixSeed(std::uint32_t & seed, int value)
    {
        seed ^= static_cast<std::uint32_t>(value);
        seed *= 16777619u;
    }

    void mixPartySeed(std::uint32_t & seed, const BattleParty & party)
    {
        mixSeed(seed, party.land()());
        mixSeed(seed, party.clan()());
        for(const BattleCreature* creature : party.toBattleCreatures())
        {
            mixSeed(seed, creature->battleUnit());
            mixSeed(seed, creature->attack());
            mixSeed(seed, creature->ranger());
            mixSeed(seed, creature->defense());
            mixSeed(seed, creature->loyalty());
        }
    }

    int deterministicUniform(std::mt19937 & generator, int minimum, int maximum)
    {
        if(minimum > maximum) std::swap(minimum, maximum);

        const std::uint64_t range = static_cast<std::uint64_t>(
            static_cast<std::int64_t>(maximum) - minimum) + 1;
        const std::uint64_t engineRange =
            static_cast<std::uint64_t>(std::mt19937::max()) + 1;
        const std::uint64_t limit = engineRange - engineRange % range;

        std::uint64_t value = 0;
        do value = generator(); while(value >= limit);
        return minimum + static_cast<int>(value % range);
    }

    std::uint32_t forecastSeed(const BattleParty & attackers, const BattleTown & town,
                               const BattleParty* defenders, AI::BehaviorProfile attackersProfile,
                               AI::BehaviorProfile defendersProfile, int samples)
    {
        std::uint32_t seed = 2166136261u;
        mixPartySeed(seed, attackers);
        mixSeed(seed, town.land()());
        mixSeed(seed, town.attack());
        mixSeed(seed, town.ranger());
        mixSeed(seed, town.defense());
        mixSeed(seed, town.loyalty());
        if(defenders) mixPartySeed(seed, *defenders);
        mixSeed(seed, static_cast<int>(attackersProfile));
        mixSeed(seed, static_cast<int>(defendersProfile));
        mixSeed(seed, samples);
        return seed;
    }
}

int AI::battleInitiativeScore(const BattleCreature & creature, BattleAttackMode mode,
                              BehaviorProfile profile, bool opening)
{
    const BehaviorRules & rules = behaviorRules(profile);
    int initiativeSpeciality = specialityValue(creature);
    if(creature.haveSpeciality(Speciality::FirstStrike)) initiativeSpeciality -= 180;
    int score = creature.attack() * rules.battleAttackWeight +
                creature.defense() * rules.battleDefenseWeight +
                creature.loyalty() * rules.battleLoyaltyWeight +
                initiativeSpeciality * rules.battleSpecialWeight / 100;

    if(mode == BattleAttackMode::Ranged)
        score += creature.ranger() * rules.battleRangedWeight;
    if(mode == BattleAttackMode::Melee && creature.haveSpeciality(Speciality::Swarm))
        score += rules.battleSwarmBonus;
    if(opening && creature.haveSpeciality(Speciality::FirstStrike))
        score += rules.battleFirstStrikeBonus;
    return score;
}

std::vector<int> AI::chooseBattleInitiative(const BattleParty & party, BattleAttackMode mode,
                                             BehaviorProfile profile, bool opening)
{
    BattleCreatures creatures = party.toBattleCreatures();
    creatures.erase(std::remove_if(creatures.begin(), creatures.end(), [&](const BattleCreature* creature)
    {
        return mode == BattleAttackMode::Ranged && !creature->isRanger();
    }), creatures.end());
    std::sort(creatures.begin(), creatures.end(), [&](const BattleCreature* left,
                                                      const BattleCreature* right)
    {
        if(opening)
        {
            const bool leftFirst = left->haveSpeciality(Speciality::FirstStrike);
            const bool rightFirst = right->haveSpeciality(Speciality::FirstStrike);
            if(leftFirst != rightFirst) return leftFirst;
        }
        const int leftScore = battleInitiativeScore(*left, mode, profile, opening);
        const int rightScore = battleInitiativeScore(*right, mode, profile, opening);
        return leftScore == rightScore ? left->battleUnit() < right->battleUnit() :
                                         leftScore > rightScore;
    });

    std::vector<int> result;
    for(const BattleCreature* creature : creatures) result.push_back(creature->battleUnit());
    return result;
}

int AI::chooseBattleTarget(const BattleUnit & attacker, const BattleParty & targets,
                           BattleAttackMode mode, BehaviorProfile profile)
{
    const auto ranked = rankedTargets(attacker, targets, mode, profile);
    return ranked.empty() ? -1 : ranked.front()->battleUnit();
}

AI::BattleRoundPlan AI::chooseRangedBattlePlan(const BattleParty & shooters,
                                                const BattleParty & targets,
                                                BehaviorProfile profile)
{
    BattleRoundPlan plan;
    plan.initiative = chooseBattleInitiative(shooters, BattleAttackMode::Ranged, profile);

    std::map<int, int> projectedLoyalty;
    for(const BattleCreature* target : targets.toBattleCreatures())
        projectedLoyalty[target->battleUnit()] = target->loyalty();

    for(int unit : plan.initiative)
    {
        const BattleCreature* shooter = shooters.findBattleUnitConst(unit);
        if(!shooter) continue;
        const auto ranked = rankedTargets(*shooter, targets, BattleAttackMode::Ranged,
                                          profile, &projectedLoyalty);
        if(ranked.empty()) continue;

        const BattleCreature* target = ranked.front();
        BattleAttackPlan attack;
        attack.attacker = shooter->battleUnit();
        attack.targets.push_back(target->battleUnit());
        attack.score = targetScore(*shooter, *target, BattleAttackMode::Ranged,
                                   behaviorRules(profile), battleOverkillPenalty(
                                       profile, GameData::aiDifficulty()),
                                   projectedLoyalty[target->battleUnit()], 0);
        plan.attacks.push_back(attack);
        projectedLoyalty[target->battleUnit()] -=
            estimatedDamage(*shooter, *target, BattleAttackMode::Ranged, 0);
    }
    return plan;
}

AI::BattleAttackPlan AI::chooseMeleeBattlePlan(const BattleParty & actors,
                                                const BattleParty & enemies,
                                                BehaviorProfile profile)
{
    BattleAttackPlan best;
    for(const BattleCreature* attacker : actors.toBattleCreatures())
    {
        const BattleAttackPlan candidate = meleePlanFor(*attacker, actors, enemies, profile);
        if(!candidate.isValid()) continue;
        if(!best.isValid() || candidate.score > best.score ||
           (candidate.score == best.score && candidate.attacker < best.attacker))
            best = candidate;
    }
    return best;
}

AI::BattleAttackPlan AI::chooseTownBattlePlan(const BattleParty & actors, const BattleTown & town,
                                               BehaviorProfile profile)
{
    BattleAttackPlan best;
    const int modifier = std::max(0, actors.count() - 1);
    for(const BattleCreature* attacker : actors.toBattleCreatures())
    {
        const int damage = std::max(1, attacker->attack() + modifier - town.defense());
        BattleAttackPlan candidate;
        candidate.attacker = attacker->battleUnit();
        candidate.targets.push_back(town.battleUnit());
        candidate.score = battleInitiativeScore(*attacker, BattleAttackMode::Melee, profile) +
                          damage * 100 +
                          (town.loyalty() <= damage ? behaviorRules(profile).battleKillBonus : 0);
        if(!best.isValid() || candidate.score > best.score ||
           (candidate.score == best.score && candidate.attacker < best.attacker))
            best = candidate;
    }
    return best;
}

AI::BattleForecast AI::forecastBattle(const BattleParty & attackers, const BattleTown & town,
                                       const BattleParty* defenders,
                                       BehaviorProfile attackersProfile,
                                       BehaviorProfile defendersProfile, int samples)
{
    BattleForecast forecast;
    forecast.samples = std::clamp(samples, 1, 128);

    const int initialAttackerLoyalty = std::max(1, partyLoyalty(attackers));
    const int initialDefenderLoyalty = std::max(1, std::max(0, town.loyalty()) +
                                                  (defenders ? partyLoyalty(*defenders) : 0));
    long long attackerLoyalty = 0;
    long long defenderLoyalty = 0;
    std::mt19937 generator(forecastSeed(attackers, town, defenders, attackersProfile,
                                        defendersProfile, forecast.samples));
    const Battle::RandomRoll randomRoll = [&](int minimum, int maximum)
    {
        return deterministicUniform(generator, minimum, maximum);
    };

    for(int sample = 0; sample < forecast.samples; ++sample)
    {
        BattleParty simulatedAttackers = attackers;
        BattleTown simulatedTown = town;
        BattleParty simulatedDefenders;
        BattleParty* simulatedDefendersPtr = nullptr;
        if(defenders)
        {
            simulatedDefenders = *defenders;
            simulatedDefendersPtr = &simulatedDefenders;
        }

        Battle::doAttackParty(simulatedAttackers, simulatedTown, simulatedDefendersPtr,
                              attackersProfile, defendersProfile, randomRoll, false);
        if(!simulatedTown.isAlive()) forecast.captures++;
        attackerLoyalty += partyLoyalty(simulatedAttackers);
        defenderLoyalty += std::max(0, simulatedTown.loyalty()) +
                           (simulatedDefendersPtr ? partyLoyalty(*simulatedDefendersPtr) : 0);
    }

    forecast.captureChance = (forecast.captures * 100 + forecast.samples / 2) /
                             forecast.samples;
    forecast.attackerSurvival = static_cast<int>((attackerLoyalty * 100 +
        static_cast<long long>(forecast.samples) * initialAttackerLoyalty / 2) /
        (static_cast<long long>(forecast.samples) * initialAttackerLoyalty));
    forecast.defenderSurvival = static_cast<int>((defenderLoyalty * 100 +
        static_cast<long long>(forecast.samples) * initialDefenderLoyalty / 2) /
        (static_cast<long long>(forecast.samples) * initialDefenderLoyalty));
    return forecast;
}
