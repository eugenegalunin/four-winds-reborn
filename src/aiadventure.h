#ifndef _RWNA_AIADVENTURE_
#define _RWNA_AIADVENTURE_

#include "aiprofile.h"

namespace AI
{
    struct AdventureClaimPlan
    {
        Land land;
        int score;

        AdventureClaimPlan() : score(0) {}
        bool isValid(void) const { return land.isValid(); }
    };

    struct AdventureMovePlan
    {
        Land target;
        Land destination;
        int score;
        int attackStrength;
        int defenseStrength;
        int captureChance;
        int attackerSurvival;
        bool engagesEnemy;

        AdventureMovePlan()
            : score(0), attackStrength(0), defenseStrength(0), captureChance(0),
              attackerSurvival(0), engagesEnemy(false) {}
        bool isValid(void) const { return target.isValid() && destination.isValid(); }
    };

    struct AdventureThreat
    {
        Land land;
        Land enemyOrigin;
        int enemyStrength;
        int defenseStrength;
        int distance;
        int score;
        int captureChance;
        int attackerSurvival;

        AdventureThreat()
            : enemyStrength(0), defenseStrength(0), distance(0), score(0),
              captureChance(0), attackerSurvival(0) {}
        bool isValid(void) const { return land.isValid() && 0 < enemyStrength; }
    };

    struct AdventurePartyOrder
    {
        std::vector<int> units;
        Land origin;
        Land defend;
        AdventureMovePlan move;
        int score;
        bool hold;

        AdventurePartyOrder() : score(0), hold(true) {}
        bool isMove(void) const { return !hold && move.isValid(); }
    };

    struct AdventureTurnPlan
    {
        BehaviorProfile profile;
        std::vector<AdventureThreat> threats;
        std::vector<AdventurePartyOrder> orders;
        int reservedParties;

        AdventureTurnPlan() : profile(BehaviorProfile::Balanced), reservedParties(0) {}
    };

    AdventureClaimPlan chooseAdventureClaim(const RemotePlayer &, BehaviorProfile);
    AdventureMovePlan  chooseAdventureMove(const RemotePlayer &, const BattleParty &, BehaviorProfile);
    AdventureMovePlan  chooseAdventureMove(const RemotePlayer &, const BattleParty &, BehaviorProfile,
                                           const Lands & targets);
    std::vector<AdventureThreat> predictAdventureThreats(const RemotePlayer &, BehaviorProfile);
    AdventureTurnPlan chooseAdventureTurn(const RemotePlayer &, BehaviorProfile);
}

#endif
