/***************************************************************************
 *   Copyright (C) 2020 by RuneWarsNA team <runewars.newage@gmail.com>     *
 *                                                                         *
 *   Part of the RuneWars: NewAge engine:                                  *
 *   https://github.com/AndreyBarmaley/runewars.newage                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _RWNA_BATTLE_
#define _RWNA_BATTLE_

#include <functional>
#include <utility>
#include <vector>

#include "gametheme.h"

namespace AI
{
    enum class BehaviorProfile;
}

namespace Battle
{
    using RandomRoll = std::function<int(int, int)>;

    struct AttackPreview
    {
        bool valid;
        bool ranged;
        int damage;
        int hitChance;
        int mightyDamage;
        int mightyChance;

        AttackPreview()
            : valid(false), ranged(false), damage(0), hitChance(0),
              mightyDamage(0), mightyChance(0) {}
    };

    class Session
    {
    public:
        enum class Phase
        {
            None,
            OpeningLeader,
            AttackerRangedChoice,
            AttackerChoice,
            Complete
        };

    private:
        BattleParty            attackersState;
        BattleParty            defendersState;
        BattleTown             townState;
        BattleStrikes          battleStrikes;
        AI::BehaviorProfile    attackersProfile;
        AI::BehaviorProfile    defendersProfile;
        Phase                  currentPhase;
        bool                   defendersPresent;
        bool                   attackersFirst;
        int                    round;
        int                    openingLeader;
        std::vector<int>       rangedActors;
        std::vector<int>       rangedRecommendations;
        std::vector<int>       rangedChoiceActors;
        std::vector<int>       rangedChoiceTargets;

        void resolveRemaining(const RandomRoll &, bool logEffects);
        void advanceToMelee(const RandomRoll &, bool logEffects);
        void resolveRangedChoices(const RandomRoll &, bool logEffects);

    public:
        Session();
        Session(const BattleParty &, const BattleTown &, const BattleParty*,
                AI::BehaviorProfile, AI::BehaviorProfile);

        bool prepare(const RandomRoll &, bool logEffects = true);
        bool choose(int attacker, int target, const RandomRoll &, bool logEffects = true);
        bool autoResolve(const RandomRoll &, bool logEffects = true);

        bool isValid(void) const { return townState.isValid(); }
        bool awaitsChoice(void) const
        {
            return currentPhase == Phase::OpeningLeader ||
                   currentPhase == Phase::AttackerRangedChoice ||
                   currentPhase == Phase::AttackerChoice;
        }
        bool isComplete(void) const { return currentPhase == Phase::Complete; }
        bool hasDefenders(void) const { return defendersPresent; }
        Phase phase(void) const { return currentPhase; }
        const char* phaseName(void) const;
        int choiceNumber(void) const
        {
            if(currentPhase == Phase::OpeningLeader) return 1;
            if(currentPhase == Phase::AttackerRangedChoice)
                return static_cast<int>(rangedChoiceActors.size()) + 1;
            if(currentPhase == Phase::AttackerChoice) return round;
            return 0;
        }
        int choiceCount(void) const
        {
            return currentPhase == Phase::AttackerRangedChoice ?
                static_cast<int>(rangedActors.size()) : 0;
        }

        std::vector<int> legalActors(void) const;
        std::vector<int> legalTargets(int attacker) const;
        std::pair<int, int> recommendedChoice(void) const;

        const BattleParty & attackers(void) const { return attackersState; }
        const BattleParty & defenders(void) const { return defendersState; }
        const BattleTown & town(void) const { return townState; }
        const BattleStrikes & strikes(void) const { return battleStrikes; }

        JsonObject toJsonObject(void) const;
        static Session fromJsonObject(const JsonObject &);
    };

    int                 rangedDamage(int strength, int reduction);
    int                 meleeHitChance(int attack, int defense);
    int                 mergeDefenseBonus(const BattleParty &, const BattleCreature &);
    AttackPreview       previewAttack(const BattleParty &, const BattleTown &,
                                      const BattleParty*, int actor, int target,
                                      bool ranged);
    BattleStrikes       doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders);
    BattleStrikes       doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders,
                                      AI::BehaviorProfile attackersProfile,
                                      AI::BehaviorProfile defendersProfile);
    BattleStrikes       doAttackParty(BattleParty & attackers, BattleTown & town, BattleParty* defenders,
                                      AI::BehaviorProfile attackersProfile,
                                      AI::BehaviorProfile defendersProfile,
                                      const RandomRoll &, bool logEffects);
}

#endif
