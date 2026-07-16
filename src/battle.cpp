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

    BattleCreature* partyLeader(BattleParty & party, AI::BehaviorProfile profile, bool opening)
    {
        const std::vector<int> initiative =
            AI::chooseBattleInitiative(party, AI::BattleAttackMode::Melee, profile, opening);
        return initiative.empty() ? nullptr : party.findBattleUnit(initiative.front());
    }

    int supportBonus(const BattleParty & party)
    {
        return std::max(0, party.count() - 1);
    }

    RangedPlan makeRangedPlan(const BattleParty & shooters, BattleParty & targets,
                              AI::BehaviorProfile profile)
    {
        RangedPlan plan;
        const AI::BattleRoundPlan decisions = AI::chooseRangedBattlePlan(shooters, targets, profile);
        for(const AI::BattleAttackPlan & decision : decisions.attacks)
        {
            const BattleCreature* shooter = shooters.findBattleUnitConst(decision.attacker);
            BattleCreature* target = decision.targets.empty() ? nullptr :
                                     targets.findBattleUnit(decision.targets.front());
            if(shooter && target) plan.emplace_back(shooter, target);
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

    BattleStrikes strikeParty(BattleParty & allies, BattleParty & enemies,
                              AI::BehaviorProfile profile, const Battle::RandomRoll & randomRoll,
                              bool logEffects, const char* side, int round)
    {
        BattleStrikes strikes;
        const AI::BattleAttackPlan plan = AI::chooseMeleeBattlePlan(allies, enemies, profile);
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

    BattleStrikes strikeTown(BattleParty & attackers, BattleTown & town,
                             AI::BehaviorProfile profile, const Battle::RandomRoll & randomRoll,
                             bool logEffects, int round)
    {
        BattleStrikes strikes;
        const AI::BattleAttackPlan plan = AI::chooseTownBattlePlan(attackers, town, profile);
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
    BattleStrikes strikes;
    if(logEffects)
        battleStateBreadcrumb("Battle stage=begin", attackers, town, defenders);

    // Hellblast is simultaneous at combat entry.
    if(defenders)
    {
        BattleCreature* attackerCaster = hellBlastCaster(attackers);
        BattleCreature* defenderCaster = hellBlastCaster(*defenders);
        if(logEffects)
            CrashReport::breadcrumb(std::string("Battle phase=hellblast attacker_caster=")
                .append(attackerCaster ? std::to_string(attackerCaster->battleUnit()) : "none")
                .append(" defender_caster=")
                .append(defenderCaster ? std::to_string(defenderCaster->battleUnit()) : "none"));
        strikes << applyHellBlast(attackerCaster, *defenders);
        strikes << applyHellBlast(defenderCaster, attackers);
        attackers.removeUnloyalty();
        defenders->removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb("Battle phase=hellblast stage=after", attackers, town, defenders);
    }

    BattleCreature* initialLeader = partyLeader(attackers, attackersProfile, true);
    const bool attackersFirst = initialLeader && initialLeader->haveSpeciality(Speciality::FirstStrike);

    // Territory missiles resolve before creature missiles.
    if(town.isRanger())
    {
        strikes << strikeFromTown(town, attackers, defendersProfile, true,
                                  randomRoll, logEffects, 0);
        attackers.removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb("Battle phase=town_ranged stage=after", attackers, town, defenders);
    }

    // Creature missile attacks are planned before any damage is applied.
    if(defenders && !attackersFirst && attackers.count() && defenders->count())
    {
        const RangedPlan attackerPlan = makeRangedPlan(attackers, *defenders, attackersProfile);
        const RangedPlan defenderPlan = makeRangedPlan(*defenders, attackers, defendersProfile);
        strikes << applyRangedPlan(attackerPlan, logEffects, "attacker");
        strikes << applyRangedPlan(defenderPlan, logEffects, "defender");
        attackers.removeUnloyalty();
        defenders->removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb("Battle phase=creature_ranged stage=after", attackers, town, defenders);
    }

    int round = 0;
    while(attackers.count() && town.isAlive())
    {
	++round;
	if(logEffects)
	    battleStateBreadcrumb(std::string("Battle phase=melee round=") +
	        std::to_string(round) + " stage=before", attackers, town, defenders);

        if(defenders && defenders->count())
        {
            if(attackersFirst)
            {
                strikes << strikeParty(attackers, *defenders, attackersProfile,
                                       randomRoll, logEffects, "attacker", round);
                attackers.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackers.count() && defenders->count())
                    strikes << strikeParty(*defenders, attackers, defendersProfile,
                                           randomRoll, logEffects, "defender", round);
            }
            else
            {
                strikes << strikeParty(*defenders, attackers, defendersProfile,
                                       randomRoll, logEffects, "defender", round);
                attackers.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackers.count() && defenders->count())
                    strikes << strikeParty(attackers, *defenders, attackersProfile,
                                           randomRoll, logEffects, "attacker", round);
            }

            attackers.removeUnloyalty();
            defenders->removeUnloyalty();
            if(logEffects)
                battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                    std::to_string(round) + " stage=after", attackers, town, defenders);
            continue;
        }

        if(attackersFirst)
        {
            strikes << strikeTown(attackers, town, attackersProfile, randomRoll, logEffects, round);
            attackers.removeUnloyalty();
            if(attackers.count() && town.isAlive())
                strikes << strikeFromTown(town, attackers, defendersProfile, false,
                                          randomRoll, logEffects, round);
        }
        else
        {
            strikes << strikeFromTown(town, attackers, defendersProfile, false,
                                      randomRoll, logEffects, round);
            attackers.removeUnloyalty();
            if(attackers.count() && town.isAlive())
                strikes << strikeTown(attackers, town, attackersProfile, randomRoll, logEffects, round);
        }
        attackers.removeUnloyalty();
        if(logEffects)
            battleStateBreadcrumb(std::string("Battle phase=melee round=") +
                std::to_string(round) + " stage=after", attackers, town, defenders);
    }

    if(logEffects)
        battleStateBreadcrumb(std::string("Battle stage=end rounds=") +
            std::to_string(round) + " strikes=" + std::to_string(strikes.size()),
            attackers, town, defenders);

    return strikes;
}
