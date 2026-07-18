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
        int strategicGoalLimit;
        int strategicDrawHorizon;
        int strategicBranchLimit;
        int offensiveSpellPenalty;
        int manaReserveAdjustment;
        int runeValuePercent;
        int minimumBattleWinChanceAdjustment;
        int minimumThreatCaptureChanceAdjustment;
        int defensiveReserveAdjustment;
        int battleOverkillPenaltyPercent;
        int nearBestScorePercent;
        int nearBestDecisionPeriod;
        bool showPlayerBattleForecast;
    };

    const char*             difficultyName(Difficulty);
    bool                    difficultyFromString(const std::string &, Difficulty &);
    Difficulty              difficultyFromString(const std::string &);
    Difficulty              nextDifficulty(Difficulty);
    const DifficultyRules & difficultyRules(Difficulty);
    int                     spellPlanningHorizon(BehaviorProfile, Difficulty);
    int                     maximumPartiesPerTarget(BehaviorProfile, Difficulty);
    int                     manaReserve(BehaviorProfile, Difficulty);
    int                     minimumBattleWinChance(BehaviorProfile, Difficulty);
    int                     minimumThreatCaptureChance(BehaviorProfile, Difficulty);
    int                     defensiveReservePercent(BehaviorProfile, Difficulty);
    int                     battleOverkillPenalty(BehaviorProfile, Difficulty);
    bool                    preferNearBestChoice(Difficulty, int bestScore,
                                                 int alternativeScore, int decisionKey);
    const char*             behaviorProfileName(BehaviorProfile);
    bool                    behaviorProfileFromString(const std::string &, BehaviorProfile &);
    BehaviorProfile         behaviorProfileFromString(const std::string &);
    bool                    behaviorProfileOverrideEnabled(void);
    BehaviorProfile         behaviorProfileOverride(void);
    void                    setBehaviorProfileOverride(BehaviorProfile);
    void                    clearBehaviorProfileOverride(void);
    BehaviorProfile         behaviorProfile(const RemotePlayer &);
    const BehaviorRules &   behaviorRules(BehaviorProfile);
    int                     creatureSummonScore(const Creature &, BehaviorProfile);
}

#endif
