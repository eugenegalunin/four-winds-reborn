#ifndef _RWNA_AIPROFILE_
#define _RWNA_AIPROFILE_

#include "gamedata.h"

namespace AI
{
    enum class Difficulty
    {
        Easy,
        Normal,
        Hard
    };

    enum class BehaviorProfile
    {
        Balanced,
        Aggressive,
        Economic,
        Control
    };

    struct BehaviorRules
    {
        int damageWeight;
        int controlWeight;
        int supportWeight;
        int economyWeight;
        int mobilityWeight;
        int spellCostPenalty;
        int manaReserve;
        int reservePenalty;
        int minimumSpellScore;
        int summonOverrideScore;
        int planningHorizon;
        int futureDiscount;
        int comboWeight;

        int summonAttackWeight;
        int summonDefenseWeight;
        int summonMovementWeight;
        int summonUtilityWeight;
        int summonCostPenalty;

        int claimValueWeight;
        int claimCostPenalty;
        int claimPowerBonus;
        int claimFrontierWeight;
        int claimFriendlyBorderWeight;
        int claimPointReserve;

        int targetValueWeight;
        int targetDistancePenalty;
        int targetDefensePenalty;
        int targetEmptyBonus;
        int targetDefendedBonus;
        int targetWinnableBonus;
        int targetPowerBonus;
        int targetThreatBonus;
        int targetFrontierWeight;
        int minimumBattleWinChance;
        int targetCaptureChanceWeight;
        int targetSurvivalWeight;

        int minimumThreatCaptureChance;
        int defensiveReservePercent;
        int threatValueWeight;
        int threatPowerBonus;
        int threatDistancePenalty;
        int maxPartiesPerTarget;

        int battleAttackWeight;
        int battleDefenseWeight;
        int battleLoyaltyWeight;
        int battleRangedWeight;
        int battleSpecialWeight;
        int battleKillBonus;
        int battleThreatWeight;
        int battleOverkillPenalty;
        int battleFirstStrikeBonus;
        int battleSwarmBonus;
    };

    struct DifficultyRules
    {
        int battleForecastSamples;
        int spellHorizonAdjustment;
        int maximumSpellHorizon;
        int partyCoordinationAdjustment;
    };

    const char*             difficultyName(Difficulty);
    Difficulty              difficultyFromString(const std::string &);
    Difficulty              nextDifficulty(Difficulty);
    const DifficultyRules & difficultyRules(Difficulty);
    int                     spellPlanningHorizon(BehaviorProfile, Difficulty);
    int                     maximumPartiesPerTarget(BehaviorProfile, Difficulty);
    const char*             behaviorProfileName(BehaviorProfile);
    BehaviorProfile         behaviorProfileFromString(const std::string &);
    BehaviorProfile         behaviorProfile(const RemotePlayer &);
    const BehaviorRules &   behaviorRules(BehaviorProfile);
    int                     creatureSummonScore(const Creature &, BehaviorProfile);
}

#endif
