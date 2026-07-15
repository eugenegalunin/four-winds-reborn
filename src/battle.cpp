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

namespace
{
    using RangedPlan = std::vector<std::pair<const BattleUnit*, BattleCreature*>>;

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

    BattleStrikes applyRangedPlan(const RangedPlan & plan, bool logEffects)
    {
        BattleStrikes strikes;
        for(const auto & attack : plan)
            strikes << applyRangedAttack(*attack.first, *attack.second, logEffects);
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
                              bool logEffects)
    {
        BattleStrikes strikes;
        const AI::BattleAttackPlan plan = AI::chooseMeleeBattlePlan(allies, enemies, profile);
        BattleCreature* attacker = plan.isValid() ? allies.findBattleUnit(plan.attacker) : nullptr;
        if(!attacker) return strikes;

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
                             bool logEffects)
    {
        BattleStrikes strikes;
        const AI::BattleAttackPlan plan = AI::chooseTownBattlePlan(attackers, town, profile);
        BattleCreature* attacker = plan.isValid() ? attackers.findBattleUnit(plan.attacker) : nullptr;
        if(attacker)
            strikes << applyMeleeAttack(*attacker, town, supportBonus(attackers),
                                        randomRoll, logEffects);
        return strikes;
    }

    BattleStrikes strikeFromTown(BattleTown & town, BattleParty & attackers,
                                 AI::BehaviorProfile profile, bool ranged,
                                 const Battle::RandomRoll & randomRoll, bool logEffects)
    {
        BattleStrikes strikes;
        const AI::BattleAttackMode mode = ranged ? AI::BattleAttackMode::Ranged :
                                                   AI::BattleAttackMode::Melee;
        const int targetId = AI::chooseBattleTarget(town, attackers, mode, profile);
        BattleCreature* target = attackers.findBattleUnit(targetId);
        if(!target) return strikes;

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
        return Tools::rand(minimum, maximum);
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

    // Hellblast is simultaneous at combat entry.
    if(defenders)
    {
        BattleCreature* attackerCaster = hellBlastCaster(attackers);
        BattleCreature* defenderCaster = hellBlastCaster(*defenders);
        strikes << applyHellBlast(attackerCaster, *defenders);
        strikes << applyHellBlast(defenderCaster, attackers);
        attackers.removeUnloyalty();
        defenders->removeUnloyalty();
    }

    BattleCreature* initialLeader = partyLeader(attackers, attackersProfile, true);
    const bool attackersFirst = initialLeader && initialLeader->haveSpeciality(Speciality::FirstStrike);

    // Territory missiles resolve before creature missiles.
    if(town.isRanger())
    {
        strikes << strikeFromTown(town, attackers, defendersProfile, true,
                                  randomRoll, logEffects);
        attackers.removeUnloyalty();
    }

    // Creature missile attacks are planned before any damage is applied.
    if(defenders && !attackersFirst && attackers.count() && defenders->count())
    {
        const RangedPlan attackerPlan = makeRangedPlan(attackers, *defenders, attackersProfile);
        const RangedPlan defenderPlan = makeRangedPlan(*defenders, attackers, defendersProfile);
        strikes << applyRangedPlan(attackerPlan, logEffects);
        strikes << applyRangedPlan(defenderPlan, logEffects);
        attackers.removeUnloyalty();
        defenders->removeUnloyalty();
    }

    while(attackers.count() && town.isAlive())
    {
        if(defenders && defenders->count())
        {
            if(attackersFirst)
            {
                strikes << strikeParty(attackers, *defenders, attackersProfile,
                                       randomRoll, logEffects);
                attackers.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackers.count() && defenders->count())
                    strikes << strikeParty(*defenders, attackers, defendersProfile,
                                           randomRoll, logEffects);
            }
            else
            {
                strikes << strikeParty(*defenders, attackers, defendersProfile,
                                       randomRoll, logEffects);
                attackers.removeUnloyalty();
                defenders->removeUnloyalty();
                if(attackers.count() && defenders->count())
                    strikes << strikeParty(attackers, *defenders, attackersProfile,
                                           randomRoll, logEffects);
            }

            attackers.removeUnloyalty();
            defenders->removeUnloyalty();
            continue;
        }

        if(attackersFirst)
        {
            strikes << strikeTown(attackers, town, attackersProfile, randomRoll, logEffects);
            attackers.removeUnloyalty();
            if(attackers.count() && town.isAlive())
                strikes << strikeFromTown(town, attackers, defendersProfile, false,
                                          randomRoll, logEffects);
        }
        else
        {
            strikes << strikeFromTown(town, attackers, defendersProfile, false,
                                      randomRoll, logEffects);
            attackers.removeUnloyalty();
            if(attackers.count() && town.isAlive())
                strikes << strikeTown(attackers, town, attackersProfile, randomRoll, logEffects);
        }
        attackers.removeUnloyalty();
    }

    return strikes;
}
