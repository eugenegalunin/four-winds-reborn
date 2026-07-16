#include <algorithm>
#include <cctype>

#include "aiprofile.h"

namespace
{
    AI::BehaviorRules balancedRules()
    {
        AI::BehaviorRules rules{};
        rules.damageWeight = 100;
        rules.controlWeight = 100;
        rules.supportWeight = 100;
        rules.economyWeight = 100;
        rules.mobilityWeight = 100;
        rules.spellCostPenalty = 20;
        rules.manaReserve = 40;
        rules.reservePenalty = 100;
        rules.minimumSpellScore = 25;
        rules.summonOverrideScore = 110;
        rules.planningHorizon = 2;
        rules.futureDiscount = 65;
        rules.comboWeight = 100;
        rules.summonAttackWeight = 100;
        rules.summonDefenseWeight = 100;
        rules.summonMovementWeight = 100;
        rules.summonUtilityWeight = 100;
        rules.summonCostPenalty = 16;
        rules.claimValueWeight = 0;
        rules.claimCostPenalty = 100;
        rules.claimPowerBonus = 80;
        rules.claimFrontierWeight = 15;
        rules.claimFriendlyBorderWeight = 10;
        rules.claimPointReserve = 0;
        rules.targetValueWeight = 200;
        rules.targetDistancePenalty = 35;
        rules.targetDefensePenalty = 3;
        rules.targetEmptyBonus = 180;
        rules.targetDefendedBonus = 0;
        rules.targetWinnableBonus = 300;
        rules.targetPowerBonus = 80;
        rules.targetThreatBonus = 60;
        rules.targetFrontierWeight = 10;
        rules.minimumBattleWinChance = 55;
        rules.targetCaptureChanceWeight = 5;
        rules.targetSurvivalWeight = 2;
        rules.minimumThreatCaptureChance = 45;
        rules.defensiveReservePercent = 40;
        rules.threatValueWeight = 100;
        rules.threatPowerBonus = 150;
        rules.threatDistancePenalty = 20;
        rules.maxPartiesPerTarget = 1;
        rules.battleAttackWeight = 100;
        rules.battleDefenseWeight = 55;
        rules.battleLoyaltyWeight = 40;
        rules.battleRangedWeight = 75;
        rules.battleSpecialWeight = 100;
        rules.battleKillBonus = 500;
        rules.battleThreatWeight = 35;
        rules.battleOverkillPenalty = 80;
        rules.battleFirstStrikeBonus = 1000;
        rules.battleSwarmBonus = 300;
        return rules;
    }
}

const AI::DifficultyRules & AI::difficultyRules(Difficulty difficulty)
{
    static const DifficultyRules easy{ 8, 0, 1, -1, 2, 2 };
    static const DifficultyRules normal{ 16, 0, 3, 0, 4, 4 };
    static const DifficultyRules hard{ 48, 1, 4, 1, 6, 6 };

    switch(difficulty)
    {
        case Difficulty::Easy: return easy;
        case Difficulty::Hard: return hard;
        default: return normal;
    }
}

const char* AI::difficultyName(Difficulty difficulty)
{
    switch(difficulty)
    {
        case Difficulty::Easy: return "easy";
        case Difficulty::Hard: return "hard";
        default: return "normal";
    }
}

AI::Difficulty AI::difficultyFromString(const std::string & value)
{
    std::string difficulty = value;
    std::transform(difficulty.begin(), difficulty.end(), difficulty.begin(), [](unsigned char ch)
    {
        return static_cast<char>(std::tolower(ch));
    });

    if(difficulty == "easy") return Difficulty::Easy;
    if(difficulty == "hard") return Difficulty::Hard;
    return Difficulty::Normal;
}

AI::Difficulty AI::nextDifficulty(Difficulty difficulty)
{
    switch(difficulty)
    {
        case Difficulty::Easy: return Difficulty::Normal;
        case Difficulty::Normal: return Difficulty::Hard;
        default: return Difficulty::Easy;
    }
}

int AI::spellPlanningHorizon(BehaviorProfile profile, Difficulty difficulty)
{
    const DifficultyRules & level = difficultyRules(difficulty);
    return std::clamp(behaviorRules(profile).planningHorizon + level.spellHorizonAdjustment,
                      1, level.maximumSpellHorizon);
}

int AI::maximumPartiesPerTarget(BehaviorProfile profile, Difficulty difficulty)
{
    return std::clamp(behaviorRules(profile).maxPartiesPerTarget +
                      difficultyRules(difficulty).partyCoordinationAdjustment, 1, 3);
}

const AI::BehaviorRules & AI::behaviorRules(BehaviorProfile profile)
{
    static const BehaviorRules balanced = balancedRules();
    static const BehaviorRules aggressive = []
    {
        BehaviorRules rules = balancedRules();
        rules.damageWeight = 140;
        rules.controlWeight = 95;
        rules.supportWeight = 110;
        rules.economyWeight = 70;
        rules.mobilityWeight = 120;
        rules.spellCostPenalty = 10;
        rules.manaReserve = 0;
        rules.reservePenalty = 0;
        rules.minimumSpellScore = 20;
        rules.summonOverrideScore = 100;
        rules.comboWeight = 125;
        rules.summonAttackWeight = 135;
        rules.summonDefenseWeight = 85;
        rules.summonMovementWeight = 110;
        rules.summonUtilityWeight = 80;
        rules.summonCostPenalty = 10;
        rules.claimValueWeight = 140;
        rules.claimCostPenalty = 20;
        rules.claimPowerBonus = 180;
        rules.claimFrontierWeight = 45;
        rules.claimFriendlyBorderWeight = 0;
        rules.claimPointReserve = 0;
        rules.targetValueWeight = 100;
        rules.targetDistancePenalty = 20;
        rules.targetDefensePenalty = 1;
        rules.targetEmptyBonus = 70;
        rules.targetDefendedBonus = 320;
        rules.targetWinnableBonus = 350;
        rules.targetPowerBonus = 120;
        rules.targetThreatBonus = 140;
        rules.targetFrontierWeight = 20;
        rules.minimumBattleWinChance = 35;
        rules.targetCaptureChanceWeight = 6;
        rules.targetSurvivalWeight = 1;
        rules.minimumThreatCaptureChance = 75;
        rules.defensiveReservePercent = 25;
        rules.threatValueWeight = 70;
        rules.threatPowerBonus = 80;
        rules.threatDistancePenalty = 10;
        rules.maxPartiesPerTarget = 2;
        rules.battleAttackWeight = 140;
        rules.battleDefenseWeight = 35;
        rules.battleLoyaltyWeight = 25;
        rules.battleRangedWeight = 100;
        rules.battleSpecialWeight = 90;
        rules.battleKillBonus = 650;
        rules.battleThreatWeight = 50;
        rules.battleOverkillPenalty = 40;
        rules.battleFirstStrikeBonus = 1100;
        rules.battleSwarmBonus = 400;
        return rules;
    }();
    static const BehaviorRules economic = []
    {
        BehaviorRules rules = balancedRules();
        rules.damageWeight = 75;
        rules.controlWeight = 75;
        rules.supportWeight = 105;
        rules.economyWeight = 145;
        rules.mobilityWeight = 90;
        rules.spellCostPenalty = 55;
        rules.manaReserve = 180;
        rules.minimumSpellScore = 35;
        rules.summonOverrideScore = 155;
        rules.planningHorizon = 3;
        rules.futureDiscount = 75;
        rules.comboWeight = 90;
        rules.summonAttackWeight = 75;
        rules.summonDefenseWeight = 110;
        rules.summonCostPenalty = 35;
        rules.claimValueWeight = 20;
        rules.claimCostPenalty = 130;
        rules.claimPowerBonus = 20;
        rules.claimFrontierWeight = 5;
        rules.claimFriendlyBorderWeight = 25;
        rules.claimPointReserve = 200;
        rules.targetValueWeight = 250;
        rules.targetDistancePenalty = 55;
        rules.targetDefensePenalty = 6;
        rules.targetEmptyBonus = 300;
        rules.targetDefendedBonus = -250;
        rules.targetWinnableBonus = 180;
        rules.targetPowerBonus = 80;
        rules.targetThreatBonus = 20;
        rules.targetFrontierWeight = 5;
        rules.minimumBattleWinChance = 75;
        rules.targetCaptureChanceWeight = 4;
        rules.targetSurvivalWeight = 5;
        rules.minimumThreatCaptureChance = 30;
        rules.defensiveReservePercent = 60;
        rules.threatValueWeight = 130;
        rules.threatPowerBonus = 180;
        rules.threatDistancePenalty = 25;
        rules.maxPartiesPerTarget = 1;
        rules.battleAttackWeight = 80;
        rules.battleDefenseWeight = 110;
        rules.battleLoyaltyWeight = 90;
        rules.battleRangedWeight = 70;
        rules.battleSpecialWeight = 80;
        rules.battleKillBonus = 600;
        rules.battleThreatWeight = 25;
        rules.battleOverkillPenalty = 160;
        rules.battleFirstStrikeBonus = 900;
        rules.battleSwarmBonus = 200;
        return rules;
    }();
    static const BehaviorRules control = []
    {
        BehaviorRules rules = balancedRules();
        rules.damageWeight = 90;
        rules.controlWeight = 145;
        rules.supportWeight = 90;
        rules.economyWeight = 105;
        rules.spellCostPenalty = 25;
        rules.manaReserve = 80;
        rules.reservePenalty = 75;
        rules.summonOverrideScore = 105;
        rules.planningHorizon = 3;
        rules.futureDiscount = 80;
        rules.comboWeight = 135;
        rules.summonAttackWeight = 85;
        rules.summonDefenseWeight = 115;
        rules.summonUtilityWeight = 155;
        rules.summonCostPenalty = 18;
        rules.claimValueWeight = 80;
        rules.claimCostPenalty = 70;
        rules.claimPowerBonus = 300;
        rules.claimFrontierWeight = 35;
        rules.claimFriendlyBorderWeight = 20;
        rules.claimPointReserve = 100;
        rules.targetValueWeight = 180;
        rules.targetDistancePenalty = 30;
        rules.targetDefensePenalty = 3;
        rules.targetEmptyBonus = 100;
        rules.targetDefendedBonus = 80;
        rules.targetWinnableBonus = 260;
        rules.targetPowerBonus = 400;
        rules.targetThreatBonus = 220;
        rules.targetFrontierWeight = 45;
        rules.minimumBattleWinChance = 60;
        rules.targetCaptureChanceWeight = 6;
        rules.targetSurvivalWeight = 3;
        rules.minimumThreatCaptureChance = 25;
        rules.defensiveReservePercent = 55;
        rules.threatValueWeight = 150;
        rules.threatPowerBonus = 300;
        rules.threatDistancePenalty = 15;
        rules.maxPartiesPerTarget = 1;
        rules.battleAttackWeight = 90;
        rules.battleDefenseWeight = 70;
        rules.battleLoyaltyWeight = 55;
        rules.battleRangedWeight = 90;
        rules.battleSpecialWeight = 160;
        rules.battleKillBonus = 450;
        rules.battleThreatWeight = 80;
        rules.battleOverkillPenalty = 100;
        rules.battleFirstStrikeBonus = 1200;
        rules.battleSwarmBonus = 450;
        return rules;
    }();

    switch(profile)
    {
        case BehaviorProfile::Aggressive: return aggressive;
        case BehaviorProfile::Economic: return economic;
        case BehaviorProfile::Control: return control;
        default: return balanced;
    }
}

const char* AI::behaviorProfileName(BehaviorProfile profile)
{
    switch(profile)
    {
        case BehaviorProfile::Aggressive: return "aggressive";
        case BehaviorProfile::Economic: return "economic";
        case BehaviorProfile::Control: return "control";
        default: return "balanced";
    }
}

AI::BehaviorProfile AI::behaviorProfileFromString(const std::string & value)
{
    std::string profile = value;
    std::transform(profile.begin(), profile.end(), profile.begin(), [](unsigned char ch)
    {
        return static_cast<char>(std::tolower(ch));
    });

    if(profile == "aggressive") return BehaviorProfile::Aggressive;
    if(profile == "economic") return BehaviorProfile::Economic;
    if(profile == "control") return BehaviorProfile::Control;
    return BehaviorProfile::Balanced;
}

AI::BehaviorProfile AI::behaviorProfile(const RemotePlayer & player)
{
    return behaviorProfileFromString(GameData::avatarInfo(player.avatar).aiProfile);
}

int AI::creatureSummonScore(const Creature & creature, BehaviorProfile profile)
{
    const CreatureInfo & info = GameData::creatureInfo(creature);
    const BehaviorRules & rules = behaviorRules(profile);
    const int offense = info.stat.attack * 14 + info.stat.ranger * 12;
    const int defense = info.stat.defense * 13 + info.stat.loyalty * 14;
    const int movement = info.stat.move * 5;
    int utility = 0;

    for(const Speciality & speciality : info.specials.toList())
        utility += speciality.toSpell().isValid() ? 35 : 12;

    return offense * rules.summonAttackWeight / 100 +
           defense * rules.summonDefenseWeight / 100 +
           movement * rules.summonMovementWeight / 100 +
           utility * rules.summonUtilityWeight / 100 -
           info.cost * rules.summonCostPenalty / 100;
}
