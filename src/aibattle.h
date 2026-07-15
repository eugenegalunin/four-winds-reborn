#ifndef _RWNA_AIBATTLE_
#define _RWNA_AIBATTLE_

#include "aiprofile.h"

namespace AI
{
    enum class BattleAttackMode
    {
        Ranged,
        Melee
    };

    struct BattleAttackPlan
    {
        int attacker;
        std::vector<int> targets;
        int score;

        BattleAttackPlan() : attacker(-1), score(0) {}
        bool isValid(void) const { return 0 < attacker && !targets.empty(); }
    };

    struct BattleRoundPlan
    {
        std::vector<int> initiative;
        std::vector<BattleAttackPlan> attacks;

        bool isValid(void) const { return !attacks.empty(); }
    };

    struct BattleForecast
    {
        int samples;
        int captures;
        int captureChance;
        int attackerSurvival;
        int defenderSurvival;

        BattleForecast()
            : samples(0), captures(0), captureChance(0),
              attackerSurvival(0), defenderSurvival(0) {}
        bool isValid(void) const { return 0 < samples; }
    };

    int battleInitiativeScore(const BattleCreature &, BattleAttackMode,
                              BehaviorProfile, bool opening = false);
    std::vector<int> chooseBattleInitiative(const BattleParty &, BattleAttackMode,
                                             BehaviorProfile, bool opening = false);
    int chooseBattleTarget(const BattleUnit &, const BattleParty &, BattleAttackMode,
                           BehaviorProfile);
    BattleRoundPlan chooseRangedBattlePlan(const BattleParty &, const BattleParty &,
                                            BehaviorProfile);
    BattleAttackPlan chooseMeleeBattlePlan(const BattleParty &, const BattleParty &,
                                            BehaviorProfile);
    BattleAttackPlan chooseTownBattlePlan(const BattleParty &, const BattleTown &,
                                           BehaviorProfile);
    BattleForecast forecastBattle(const BattleParty &, const BattleTown &, const BattleParty*,
                                  BehaviorProfile, BehaviorProfile, int samples = 16);
}

#endif
