/***************************************************************************
 *   Copyright (C) 2020 by RuneWarsNA team <runewars.newage@gmail.com>     *
 *                                                                         *
 *   Part of the RuneWars: NewAge engine.                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <algorithm>

#include "aibattle.h"
#include "battle.h"
#include "crashreport.h"
#include "gameplayrng.h"

namespace
{
    using RangedPlan = std::vector<std::pair<const BattleUnit*, BattleCreature*>>;

    std::string targetIds(const std::vector<int> & targets)
    {
        std::string result;
        for(int target : targets)
        {
            if(!result.empty()) result += ',';
            result += std::to_string(target);
        }
        return result.empty() ? std::string("none") : result;
    }

    void battleStateBreadcrumb(const std::string & event, const BattleParty & attackers,
                               const BattleTown & town, const BattleParty* defenders)
    {
        CrashReport::breadcrumb(event + " attackers=" + std::to_string(attackers.count()) +
            " defenders=" + std::to_string(defenders ? defenders->count() : 0) +
            " town_alive=" + (town.isAlive() ? "true" : "false") +
            " town_loyalty=" + std::to_string(town.loyalty()));
    }

    int supportBonus(const BattleParty & party)
    {
        return std::max(0, party.count() - 1);
    }

    AI::BattleRoundPlan makeCompleteRangedPlan(const BattleParty & shooters,
                                               const BattleParty & targets,
                                               AI::BehaviorProfile profile)
    {
        const AI::BattleRoundPlan planned =
            AI::chooseRangedBattlePlan(shooters, targets, profile);
        AI::BattleRoundPlan result;
        result.initiative = planned.initiative;

        for(int actor : result.initiative)
        {
            const auto found = std::find_if(planned.attacks.begin(), planned.attacks.end(),
                [actor](const AI::BattleAttackPlan & attack)
                {
                    return attack.attacker == actor && !attack.targets.empty();
                });
            if(found != planned.attacks.end())
            {
                result.attacks.push_back(*found);
                continue;
            }

            const BattleCreature* shooter = shooters.findBattleUnitConst(actor);
            const int target = shooter ? AI::chooseBattleTarget(
                *shooter, targets, AI::BattleAttackMode::Ranged, profile) : -1;
            if(0 < target)
            {
                AI::BattleAttackPlan fallback;
                fallback.attacker = actor;
                fallback.targets.push_back(target);
                result.attacks.push_back(fallback);
            }
        }
        return result;
    }

    RangedPlan makeRangedPlan(const BattleParty & shooters, BattleParty & targets,
                              AI::BehaviorProfile profile)
    {
        RangedPlan plan;
        const AI::BattleRoundPlan decisions =
            makeCompleteRangedPlan(shooters, targets, profile);
        for(const AI::BattleAttackPlan & decision : decisions.attacks)
        {
            const BattleCreature* shooter = shooters.findBattleUnitConst(decision.attacker);
            BattleCreature* target = decision.targets.empty() ? nullptr :
                                     targets.findBattleUnit(decision.targets.front());
            if(shooter && target) plan.emplace_back(shooter, target);
        }
        return plan;
    }

    RangedPlan makeRangedPlan(const BattleParty & shooters, BattleParty & targets,
                              const std::vector<int> & actors,
                              const std::vector<int> & selectedTargets)
    {
        RangedPlan plan;
        const std::size_t count = std::min(actors.size(), selectedTargets.size());
        for(std::size_t index = 0; index < count; ++index)
        {
            const BattleCreature* shooter = shooters.findBattleUnitConst(actors[index]);
            BattleCreature* target = targets.findBattleUnit(selectedTargets[index]);
            if(shooter && shooter->isRanger() && target &&
               !target->haveSpeciality(Speciality::IgnoreMissiles))
                plan.emplace_back(shooter, target);
        }
        return plan;
    }

    int calculateDamage(const BattleUnit & attacker, const BattleUnit & defender, int modifier,
                        const Battle::RandomRoll & randomRoll, bool logEffects)
    {
        int mightyBlow = 0;
        bool mightyBlowHit = false;

        if(attacker.haveSpeciality(Speciality::MightyBlow))
        {
            const SpecialityMightyBlow blow;
            mightyBlowHit = blow.chance() >= randomRoll(1, 100);
            if(mightyBlowHit)
            {
                if(logEffects) VERBOSE("Speciality: Mighty Blow!");
                mightyBlow = blow.strength();
            }
        }

        int damage = attacker.attack() + mightyBlow + modifier - defender.defense();
        if(0 < damage) return damage;
        if(mightyBlowHit) return 1;

        return Battle::meleeHitChance(attacker.attack() + modifier, defender.defense()) >=
               randomRoll(1, 100) ? 1 : 0;
    }

    BattleStrike applyRangedAttack(const BattleUnit & attacker, BattleCreature & target,
                                   bool logEffects)
    {
        int reduction = 0;
        if(target.isAffectedSpell(Spell::ForceShield))
        {
            if(logEffects) VERBOSE("Affected spell: Force Shield!");
            reduction = GameData::spellInfo(Spell::ForceShield).value;
        }

        const int damage = Battle::rangedDamage(attacker.ranger(), reduction);
        target.applyDamage(damage);
        return BattleStrike(attacker, damage, target, BattleStrike::Ranger);
    }

    BattleStrikes applyRangedPlan(const RangedPlan & plan, bool logEffects, const char* side)
    {
        BattleStrikes strikes;
        for(const auto & attack : plan)
        {
            if(logEffects)
                CrashReport::breadcrumb(std::string("Battle phase=creature_ranged side=")
                    .append(side).append(" attacker=").append(std::to_string(attack.first->battleUnit()))
                    .append(" target=").append(std::to_string(attack.second->battleUnit())));
            strikes << applyRangedAttack(*attack.first, *attack.second, logEffects);
        }
        return strikes;
    }

    BattleStrikes applyMeleeAttack(BattleUnit & attacker, BattleUnit & target, int modifier,
                                   const Battle::RandomRoll & randomRoll, bool logEffects)
    {
        BattleStrikes strikes;
        const int damage = calculateDamage(attacker, target, modifier, randomRoll, logEffects);
        target.applyDamage(damage);
        strikes << BattleStrike(attacker, damage, target, BattleStrike::Melee);

        if(!attacker.isTown() && target.haveSpeciality(Speciality::FireShield))
        {
            if(logEffects) VERBOSE("Speciality: Fire Shield!");
            attacker.applyDamage(1);
            strikes << BattleStrike(target, 1, attacker, BattleStrike::FireShield);
        }
        return strikes;
    }

    BattleStrikes strikePartyPlan(BattleParty & allies, BattleParty & enemies,
                                  const AI::BattleAttackPlan & plan,
                                  const Battle::RandomRoll & randomRoll,
                                  bool logEffects, const char* side, int round)
    {
        BattleStrikes strikes;
        BattleCreature* attacker = plan.isValid() ? allies.findBattleUnit(plan.attacker) : nullptr;
        if(!attacker) return strikes;

        if(logEffects)
            CrashReport::breadcrumb(std::string("Battle phase=melee round=")
                .append(std::to_string(round)).append(" side=").append(side)
                .append(" attacker=").append(std::to_string(plan.attacker))
                .append(" targets=").append(targetIds(plan.targets))
                .append(" allies_before=").append(std::to_string(allies.count()))
                .append(" enemies_before=").append(std::to_string(enemies.count())));

        for(int unit : plan.targets)
        {
            if(!attacker->isAlive()) break;
            BattleCreature* target = enemies.findBattleUnit(unit);
            if(!target || !target->isAlive()) continue;
            const int modifier = supportBonus(allies) - Battle::mergeDefenseBonus(enemies, *target);
            strikes << applyMeleeAttack(*attacker, *target, modifier, randomRoll, logEffects);
        }
        return strikes;
    }

    BattleStrikes strikeParty(BattleParty & allies, BattleParty & enemies,
                              AI::BehaviorProfile profile, const Battle::RandomRoll & randomRoll,
                              bool logEffects, const char* side, int round)
    {
        return strikePartyPlan(allies, enemies,
                               AI::chooseMeleeBattlePlan(allies, enemies, profile),
                               randomRoll, logEffects, side, round);
    }

    BattleStrikes strikeTownPlan(BattleParty & attackers, BattleTown & town,
                                 const AI::BattleAttackPlan & plan,
                                 const Battle::RandomRoll & randomRoll,
                                 bool logEffects, int round)
    {
        BattleStrikes strikes;
        BattleCreature* attacker = plan.isValid() ? attackers.findBattleUnit(plan.attacker) : nullptr;
        if(attacker)
        {
            if(logEffects)
                CrashReport::breadcrumb(std::string("Battle phase=town_melee round=")
                    .append(std::to_string(round)).append(" side=attacker attacker=")
                    .append(std::to_string(plan.attacker)).append(" target=town"));
            strikes << applyMeleeAttack(*attacker, town, supportBonus(attackers),
                                        randomRoll, logEffects);
        }
        return strikes;
    }

    BattleStrikes strikeTown(BattleParty & attackers, BattleTown & town,
                             AI::BehaviorProfile profile, const Battle::RandomRoll & randomRoll,
                             bool logEffects, int round)
    {
        return strikeTownPlan(attackers, town,
                              AI::chooseTownBattlePlan(attackers, town, profile),
                              randomRoll, logEffects, round);
    }

    BattleStrikes strikeFromTown(BattleTown & town, BattleParty & attackers,
                                 AI::BehaviorProfile profile, bool ranged,
                                 const Battle::RandomRoll & randomRoll, bool logEffects, int round)
    {
        BattleStrikes strikes;
        const AI::BattleAttackMode mode = ranged ? AI::BattleAttackMode::Ranged :
                                                   AI::BattleAttackMode::Melee;
        const int targetId = AI::chooseBattleTarget(town, attackers, mode, profile);
        BattleCreature* target = attackers.findBattleUnit(targetId);
        if(!target) return strikes;

        if(logEffects)
            CrashReport::breadcrumb(std::string("Battle phase=")
                .append(ranged ? "town_ranged" : "town_melee")
                .append(" round=").append(std::to_string(round))
                .append(" side=defender attacker=town target=").append(std::to_string(targetId)));

        if(ranged)
            strikes << applyRangedAttack(town, *target, logEffects);
        else
            strikes << applyMeleeAttack(town, *target,
                                        -Battle::mergeDefenseBonus(attackers, *target),
                                        randomRoll, logEffects);
        return strikes;
    }

    BattleCreature* hellBlastCaster(BattleParty & party)
    {
        BattleCreatures casters = party.toBattleCreatures(Specials() << Speciality::CastHellblast, true);
        return casters.empty() ? nullptr : casters.front();
    }

    BattleStrikes applyHellBlast(BattleCreature* caster, BattleParty & targets)
    {
        BattleStrikes strikes;
        if(!caster) return strikes;

        const int damage = std::max(1, std::abs(GameData::spellInfo(Spell::HellBlast).effect.loyalty));
        for(auto target : targets.toBattleCreatures())
        {
            target->applyDamage(damage);
            strikes << BattleStrike(*caster, damage, *target, BattleStrike::Melee);
        }
        return strikes;
    }
}

int Battle::rangedDamage(int strength, int reduction)
{
    return std::max(0, strength - std::max(0, reduction));
}

int Battle::meleeHitChance(int attack, int defense)
{
    if(attack > defense) return 100;
    const int difference = std::max(0, defense - attack);
    return std::max(1, 100 >> (difference + 1));
}

int Battle::mergeDefenseBonus(const BattleParty & party, const BattleCreature & target)
{
    if(!target.haveSpeciality(Speciality::Merge)) return 0;

    int matching = 0;
    for(auto creature : party.toBattleCreatures())
        if(creature->id() == target.id() && creature->haveSpeciality(Speciality::Merge)) matching++;

    return std::max(0, matching - 1);
}

Battle::AttackPreview Battle::previewAttack(const BattleParty & attackers,
                                             const BattleTown & town,
                                             const BattleParty* defenders,
                                             int actorId, int targetId,
                                             bool ranged)
{
    AttackPreview result;
    const BattleCreature* actor = attackers.findBattleUnitConst(actorId);
    const BattleCreature* targetCreature = defenders ?
        defenders->findBattleUnitConst(targetId) : nullptr;
    const BattleUnit* target = targetCreature;
    if(!target && town.isBattleUnit(targetId)) target = &town;
    if(!actor || !target) return result;

    result.ranged = ranged;
    if(ranged)
    {
        if(!actor->isRanger() || !targetCreature ||
           targetCreature->haveSpeciality(Speciality::IgnoreMissiles))
            return result;

        const int reduction = targetCreature->isAffectedSpell(Spell::ForceShield) ?
            GameData::spellInfo(Spell::ForceShield).value : 0;
        result.damage = rangedDamage(actor->ranger(), reduction);
        result.hitChance = 100;
        result.valid = true;
        return result;
    }

    int modifier = supportBonus(attackers);
    if(targetCreature) modifier -= mergeDefenseBonus(*defenders, *targetCreature);
    const int normalDamage = actor->attack() + modifier - target->defense();
    result.damage = 0 < normalDamage ? normalDamage : 1;
    result.hitChance = 0 < normalDamage ? 100 :
        meleeHitChance(actor->attack() + modifier, target->defense());

    if(actor->haveSpeciality(Speciality::MightyBlow))
    {
        const SpecialityMightyBlow blow;
        result.mightyDamage = std::max(1, normalDamage + blow.strength());
        result.mightyChance = blow.chance();
    }
    result.valid = true;
    return result;
}

Battle::Session::Session()
    : attackersProfile(AI::BehaviorProfile::Balanced),
      defendersProfile(AI::BehaviorProfile::Balanced),
      currentPhase(Phase::None), defendersPresent(false),
      attackersFirst(false), round(0), openingLeader(-1)
{
}

Battle::Session::Session(const BattleParty & attackers, const BattleTown & town,
                         const BattleParty* defenders,
                         AI::BehaviorProfile attackerBehavior,
                         AI::BehaviorProfile defenderBehavior)
    : attackersState(attackers),
      defendersState(defenders ? *defenders : BattleParty()),
      townState(town), attackersProfile(attackerBehavior),
      defendersProfile(defenderBehavior), currentPhase(Phase::None),
      defendersPresent(defenders != nullptr), attackersFirst(false), round(0),
      openingLeader(-1)
{
}

bool Battle::Session::prepare(const RandomRoll & randomRoll, bool logEffects)
{
    if(currentPhase != Phase::None) return isValid();
    if(!isValid()) return false;

    BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
    if(logEffects)
        battleStateBreadcrumb("Battle stage=begin", attackersState, townState, defenders);

    // Hellblast is simultaneous at combat entry.
    if(defenders)
    {
        BattleCreature* attackerCaster = hellBlastCaster(attackersState);
        BattleCreature* defenderCaster = hellBlastCaster(*defenders);
        if(logEffects)
            CrashReport::breadcrumb(std::string("Battle phase=hellblast attacker_caster=")
                .append(attackerCaster ? std::to_string(attackerCaster->battleUnit()) : "none")
                .append(" defender_caster=")
                .append(defenderCaster ? std::to_string(defenderCaster->battleUnit()) : "none"));
        battleStrikes << applyHellBlast(attackerCaster, *defenders);
        battleStrikes << applyHellBlast(defenderCaster, attackersState);
        attackersState.removeUnloyalty();
        defenders->removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb("Battle phase=hellblast stage=after",
                                  attackersState, townState, defenders);
    }

    if(attackersState.count() && townState.isAlive())
        currentPhase = Phase::OpeningLeader;
    else
        resolveRemaining(randomRoll, logEffects);
    return true;
}

const char* Battle::Session::phaseName(void) const
{
    switch(currentPhase)
    {
        case Phase::OpeningLeader: return "opening_leader";
        case Phase::AttackerRangedChoice: return "attacker_ranged";
        case Phase::AttackerChoice: return "attacker_melee";
        case Phase::Complete: return "complete";
        default: return "none";
    }
}

void Battle::Session::advanceToMelee(const RandomRoll & randomRoll, bool logEffects)
{
    BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
    currentPhase = Phase::None;

    if(attackersState.count() && townState.isAlive())
    {
        ++round;
        if(logEffects)
            battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                std::to_string(round) + " stage=before", attackersState, townState, defenders);

        if(defenders && defenders->count())
        {
            if(attackersFirst)
            {
                currentPhase = Phase::AttackerChoice;
                return;
            }

            battleStrikes << strikeParty(*defenders, attackersState, defendersProfile,
                                         randomRoll, logEffects, "defender", round);
            attackersState.removeUnloyalty();
            defenders->removeUnloyalty();
            if(attackersState.count() && defenders->count())
            {
                currentPhase = Phase::AttackerChoice;
                return;
            }
        }
        else
        {
            if(attackersFirst)
            {
                currentPhase = Phase::AttackerChoice;
                return;
            }

            battleStrikes << strikeFromTown(townState, attackersState, defendersProfile, false,
                                            randomRoll, logEffects, round);
            attackersState.removeUnloyalty();
            if(attackersState.count() && townState.isAlive())
            {
                currentPhase = Phase::AttackerChoice;
                return;
            }
        }

        if(logEffects)
            battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                std::to_string(round) + " stage=after", attackersState, townState, defenders);
    }

    // A defender may have died to Fire Shield during its half of the round.
    // Continue to the next manual attacker decision instead of silently
    // finishing the remainder of the battle.
    if(attackersState.count() && townState.isAlive())
    {
        advanceToMelee(randomRoll, logEffects);
        return;
    }

    resolveRemaining(randomRoll, logEffects);
}

void Battle::Session::resolveRangedChoices(const RandomRoll & randomRoll, bool logEffects)
{
    BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
    if(defenders && attackersState.count() && defenders->count())
    {
        // Both plans retain pointers into the untouched pre-volley state. Apply
        // them only after every human assignment has been collected.
        const RangedPlan attackerPlan = makeRangedPlan(attackersState, *defenders,
                                                        rangedChoiceActors,
                                                        rangedChoiceTargets);
        const RangedPlan defenderPlan = makeRangedPlan(*defenders, attackersState,
                                                        defendersProfile);
        battleStrikes << applyRangedPlan(attackerPlan, logEffects, "attacker");
        battleStrikes << applyRangedPlan(defenderPlan, logEffects, "defender");
        attackersState.removeUnloyalty();
        defenders->removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb("Battle phase=creature_ranged stage=after",
                                  attackersState, townState, defenders);
    }
    currentPhase = Phase::None;
    advanceToMelee(randomRoll, logEffects);
}

std::vector<int> Battle::Session::legalActors(void) const
{
    std::vector<int> result;
    if(!awaitsChoice()) return result;

    if(currentPhase == Phase::AttackerRangedChoice)
    {
        const std::size_t index = rangedChoiceActors.size();
        if(index < rangedActors.size()) result.push_back(rangedActors[index]);
        return result;
    }

    for(const BattleCreature* creature : attackersState.toBattleCreatures())
        result.push_back(creature->battleUnit());
    return result;
}

std::vector<int> Battle::Session::legalTargets(int attacker) const
{
    std::vector<int> result;
    if(!awaitsChoice() || !attackersState.findBattleUnitConst(attacker)) return result;

    if(currentPhase == Phase::OpeningLeader) return result;

    if(currentPhase == Phase::AttackerRangedChoice)
    {
        const std::size_t index = rangedChoiceActors.size();
        const BattleCreature* shooter = attackersState.findBattleUnitConst(attacker);
        if(index >= rangedActors.size() || rangedActors[index] != attacker ||
           !shooter || !shooter->isRanger() || !defendersPresent)
            return result;

        for(const BattleCreature* creature : defendersState.toBattleCreatures())
            if(!creature->haveSpeciality(Speciality::IgnoreMissiles))
                result.push_back(creature->battleUnit());
        return result;
    }

    if(defendersPresent && defendersState.count())
    {
        for(const BattleCreature* creature : defendersState.toBattleCreatures())
            result.push_back(creature->battleUnit());
    }
    else if(townState.isAlive())
    {
        result.push_back(townState.battleUnit());
    }
    return result;
}

std::pair<int, int> Battle::Session::recommendedChoice(void) const
{
    if(!awaitsChoice()) return {-1, -1};

    if(currentPhase == Phase::OpeningLeader)
    {
        const std::vector<int> initiative = AI::chooseBattleInitiative(
            attackersState, AI::BattleAttackMode::Melee, attackersProfile, true);
        return initiative.empty() ? std::make_pair(-1, -1) :
                                    std::make_pair(initiative.front(), -1);
    }

    if(currentPhase == Phase::AttackerRangedChoice)
    {
        const std::size_t index = rangedChoiceActors.size();
        if(index >= rangedActors.size() || index >= rangedRecommendations.size())
            return {-1, -1};
        return {rangedActors[index], rangedRecommendations[index]};
    }

    const AI::BattleAttackPlan plan = defendersPresent && defendersState.count() ?
        AI::chooseMeleeBattlePlan(attackersState, defendersState, attackersProfile) :
        AI::chooseTownBattlePlan(attackersState, townState, attackersProfile);
    return plan.isValid() ? std::make_pair(plan.attacker, plan.targets.front()) :
                            std::make_pair(-1, -1);
}

bool Battle::Session::choose(int attackerId, int targetId,
                             const RandomRoll & randomRoll, bool logEffects)
{
    if(!awaitsChoice()) return false;
    const std::vector<int> actors = legalActors();
    if(std::find(actors.begin(), actors.end(), attackerId) == actors.end()) return false;

    if(currentPhase == Phase::OpeningLeader)
    {
        openingLeader = attackerId;
        const BattleCreature* leader = attackersState.findBattleUnitConst(openingLeader);
        attackersFirst = leader && leader->haveSpeciality(Speciality::FirstStrike);
        BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
        currentPhase = Phase::None;

        // Territory missiles resolve after the opening leader has been chosen
        // and before either side assigns creature missiles.
        if(townState.isRanger())
        {
            battleStrikes << strikeFromTown(townState, attackersState, defendersProfile, true,
                                            randomRoll, logEffects, 0);
            attackersState.removeUnloyalty();
            if(logEffects)
                battleStateBreadcrumb("Battle phase=town_ranged stage=after",
                                      attackersState, townState, defenders);
        }

        rangedActors.clear();
        rangedRecommendations.clear();
        rangedChoiceActors.clear();
        rangedChoiceTargets.clear();

        if(defenders && !attackersFirst && attackersState.count() && defenders->count())
        {
            const AI::BattleRoundPlan plan = makeCompleteRangedPlan(
                attackersState, *defenders, attackersProfile);
            for(const AI::BattleAttackPlan & attack : plan.attacks)
            {
                if(!attack.targets.empty())
                {
                    rangedActors.push_back(attack.attacker);
                    rangedRecommendations.push_back(attack.targets.front());
                }
            }

            if(!rangedActors.empty())
            {
                currentPhase = Phase::AttackerRangedChoice;
                return true;
            }

            // The attacker may have no legal shooter while the defender does.
            // Resolve the defender plan through the same simultaneous-volley path.
            resolveRangedChoices(randomRoll, logEffects);
            return true;
        }

        advanceToMelee(randomRoll, logEffects);
        return true;
    }

    const std::vector<int> targets = legalTargets(attackerId);
    if(std::find(targets.begin(), targets.end(), targetId) == targets.end()) return false;

    if(currentPhase == Phase::AttackerRangedChoice)
    {
        rangedChoiceActors.push_back(attackerId);
        rangedChoiceTargets.push_back(targetId);
        if(rangedChoiceActors.size() < rangedActors.size()) return true;

        resolveRangedChoices(randomRoll, logEffects);
        return true;
    }

    BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
    BattleCreature* attacker = attackersState.findBattleUnit(attackerId);
    AI::BattleAttackPlan plan;
    plan.attacker = attackerId;
    plan.targets.push_back(targetId);

    if(defenders && defenders->count() && attacker &&
       attacker->haveSpeciality(Speciality::Swarm))
    {
        const AI::BattleAttackPlan recommendation =
            AI::chooseMeleeBattlePlan(attackersState, *defenders, attackersProfile);
        if(recommendation.attacker == attackerId && !recommendation.targets.empty() &&
           recommendation.targets.front() == targetId)
        {
            plan.targets = recommendation.targets;
        }
        else
        {
            std::vector<int> remaining = targets;
            std::sort(remaining.begin(), remaining.end());
            for(int target : remaining)
                if(target != targetId) plan.targets.push_back(target);
        }
    }

    if(defenders && defenders->count())
        battleStrikes << strikePartyPlan(attackersState, *defenders, plan,
                                         randomRoll, logEffects, "attacker", round);
    else
        battleStrikes << strikeTownPlan(attackersState, townState, plan,
                                        randomRoll, logEffects, round);

    attackersState.removeUnloyalty();
    if(defenders) defenders->removeUnloyalty();

    // Finish the opposing half of the paused round when First Strike put the
    // attacker first. Otherwise the defender already acted in prepare().
    if(attackersFirst && attackersState.count())
    {
        if(defenders && defenders->count())
            battleStrikes << strikeParty(*defenders, attackersState, defendersProfile,
                                         randomRoll, logEffects, "defender", round);
        else if(townState.isAlive())
            battleStrikes << strikeFromTown(townState, attackersState, defendersProfile,
                                            false, randomRoll, logEffects, round);
    }

    attackersState.removeUnloyalty();
    if(defenders) defenders->removeUnloyalty();
    if(logEffects)
        battleStateBreadcrumb(std::string("Battle phase=melee round=") +
            std::to_string(round) + " stage=after", attackersState, townState, defenders);

    currentPhase = Phase::None;
    advanceToMelee(randomRoll, logEffects);
    return true;
}

void Battle::Session::resolveRemaining(const RandomRoll & randomRoll, bool logEffects)
{
    BattleParty* defenders = defendersPresent ? &defendersState : nullptr;
    while(attackersState.count() && townState.isAlive())
    {
        ++round;
        if(logEffects)
            battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                std::to_string(round) + " stage=before", attackersState, townState, defenders);

        if(defenders && defenders->count())
        {
            if(attackersFirst)
            {
                battleStrikes << strikeParty(attackersState, *defenders, attackersProfile,
                                             randomRoll, logEffects, "attacker", round);
                attackersState.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackersState.count() && defenders->count())
                    battleStrikes << strikeParty(*defenders, attackersState, defendersProfile,
                                                 randomRoll, logEffects, "defender", round);
            }
            else
            {
                battleStrikes << strikeParty(*defenders, attackersState, defendersProfile,
                                             randomRoll, logEffects, "defender", round);
                attackersState.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackersState.count() && defenders->count())
                    battleStrikes << strikeParty(attackersState, *defenders, attackersProfile,
                                                 randomRoll, logEffects, "attacker", round);
            }

            attackersState.removeUnloyalty();
            defenders->removeUnloyalty();
        }
        else if(attackersFirst)
        {
            battleStrikes << strikeTown(attackersState, townState, attackersProfile,
                                        randomRoll, logEffects, round);
            attackersState.removeUnloyalty();
            if(attackersState.count() && townState.isAlive())
                battleStrikes << strikeFromTown(townState, attackersState, defendersProfile,
                                                false, randomRoll, logEffects, round);
            attackersState.removeUnloyalty();
        }
        else
        {
            battleStrikes << strikeFromTown(townState, attackersState, defendersProfile,
                                            false, randomRoll, logEffects, round);
            attackersState.removeUnloyalty();
            if(attackersState.count() && townState.isAlive())
                battleStrikes << strikeTown(attackersState, townState, attackersProfile,
                                            randomRoll, logEffects, round);
            attackersState.removeUnloyalty();
        }

        if(logEffects)
            battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                std::to_string(round) + " stage=after", attackersState, townState, defenders);
    }

    currentPhase = Phase::Complete;
    if(logEffects)
        battleStateBreadcrumb(std::string("Battle stage=end rounds=") +
            std::to_string(round) + " strikes=" + std::to_string(battleStrikes.size()),
            attackersState, townState, defenders);
}

bool Battle::Session::autoResolve(const RandomRoll & randomRoll, bool logEffects)
{
    if(currentPhase == Phase::None && !prepare(randomRoll, logEffects)) return false;
    int noDamageChoices = 0;
    while(!isComplete())
    {
        if(!awaitsChoice()) return false;
        const std::pair<int, int> choice = recommendedChoice();
        const std::size_t strikeCount = battleStrikes.size();
        if(0 >= choice.first ||
           !choose(choice.first, choice.second, randomRoll, logEffects))
            return false;

        bool causedDamage = false;
        std::size_t added = battleStrikes.size() - strikeCount;
        for(auto strike = battleStrikes.rbegin(); added && strike != battleStrikes.rend();
            ++strike, --added)
        {
            if(0 < strike->damage)
            {
                causedDamage = true;
                break;
            }
        }

        noDamageChoices = causedDamage ? 0 : noDamageChoices + 1;
        if(1024 <= noDamageChoices)
        {
            ERROR("Battle Auto Resolve stopped after 1024 consecutive choices without damage");
            return false;
        }
    }
    return true;
}

JsonObject Battle::Session::toJsonObject(void) const
{
    JsonObject result;
    result.addInteger("schema", 1);
    result.addString("phase", phaseName());
    result.addBoolean("defendersPresent", defendersPresent);
    result.addBoolean("attackersFirst", attackersFirst);
    result.addInteger("round", round);
    result.addInteger("openingLeader", openingLeader);
    result.addString("attackersProfile", AI::behaviorProfileName(attackersProfile));
    result.addString("defendersProfile", AI::behaviorProfileName(defendersProfile));
    result.addArray("rangedActors", JsonPack::stdVector<int>(rangedActors));
    result.addArray("rangedRecommendations", JsonPack::stdVector<int>(rangedRecommendations));
    result.addArray("rangedChoiceActors", JsonPack::stdVector<int>(rangedChoiceActors));
    result.addArray("rangedChoiceTargets", JsonPack::stdVector<int>(rangedChoiceTargets));
    result.addObject("attackers", attackersState.toJsonObject());
    result.addObject("defenders", defendersState.toJsonObject());
    result.addObject("town", townState.toJsonObject());
    result.addArray("strikes", battleStrikes.toJsonArray());
    return result;
}

Battle::Session Battle::Session::fromJsonObject(const JsonObject & object)
{
    Session result;
    const std::string phaseName = object.getString("phase", "none");
    result.currentPhase = phaseName == "opening_leader" ? Phase::OpeningLeader :
                          phaseName == "attacker_ranged" ? Phase::AttackerRangedChoice :
                          (phaseName == "attacker_melee" || phaseName == "attacker_choice") ?
                              Phase::AttackerChoice :
                          phaseName == "complete" ? Phase::Complete : Phase::None;
    result.defendersPresent = object.getBoolean("defendersPresent");
    result.attackersFirst = object.getBoolean("attackersFirst");
    result.round = object.getInteger("round");
    result.openingLeader = object.getInteger("openingLeader", -1);
    result.attackersProfile = AI::behaviorProfileFromString(
        object.getString("attackersProfile", "balanced"));
    result.defendersProfile = AI::behaviorProfileFromString(
        object.getString("defendersProfile", "balanced"));
    result.rangedActors = object.getStdVector<int>("rangedActors");
    result.rangedRecommendations = object.getStdVector<int>("rangedRecommendations");
    result.rangedChoiceActors = object.getStdVector<int>("rangedChoiceActors");
    result.rangedChoiceTargets = object.getStdVector<int>("rangedChoiceTargets");

    const JsonObject* nested = object.getObject("attackers");
    if(nested) result.attackersState = BattleParty::fromJsonObject(*nested);
    nested = object.getObject("defenders");
    if(nested) result.defendersState = BattleParty::fromJsonObject(*nested);
    nested = object.getObject("town");
    if(nested) result.townState = BattleTown::fromJsonObject(*nested);
    const JsonArray* strikeArray = object.getArray("strikes");
    if(strikeArray) result.battleStrikes = BattleStrikes::fromJsonArray(*strikeArray);
    return result;
}

BattleStrikes Battle::doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders)
{
    return doAttackParty(attackers, town, defenders, AI::BehaviorProfile::Balanced,
                         AI::BehaviorProfile::Balanced);
}

BattleStrikes Battle::doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders,
                                    AI::BehaviorProfile attackersProfile,
                                    AI::BehaviorProfile defendersProfile)
{
    const RandomRoll randomRoll = [](int minimum, int maximum)
    {
        return GameplayRng::uniform(minimum, maximum);
    };
    return doAttackParty(attackers, town, defenders, attackersProfile, defendersProfile,
                         randomRoll, true);
}

BattleStrikes Battle::doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders,
                                    AI::BehaviorProfile attackersProfile,
                                    AI::BehaviorProfile defendersProfile,
                                    const RandomRoll & randomRoll, bool logEffects)
{
    Session session(attackers, town, defenders, attackersProfile, defendersProfile);
    if(!session.prepare(randomRoll, logEffects) ||
       !session.autoResolve(randomRoll, logEffects))
        return BattleStrikes();

    attackers = session.attackers();
    town = session.town();
    if(defenders) *defenders = session.defenders();
    return session.strikes();
}
