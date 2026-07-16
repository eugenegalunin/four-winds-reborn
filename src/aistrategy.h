#ifndef _RWNA_AISTRATEGY_
#define _RWNA_AISTRATEGY_

#include "aiprofile.h"

namespace AI
{
    enum class StrategicGoalType
    {
        Summon,
        Spell
    };

    class AIObservation
    {
        LocalData local;

    public:
        explicit AIObservation(const LocalData &);

        const LocalData & state(void) const { return local; }
        const LocalPlayer & player(void) const { return local.myPlayer(); }
        int visibleStoneCount(const Stone &) const;
        bool visibleCreature(const Creature &) const;
    };

    struct RuneGoal
    {
        StrategicGoalType type;
        Creature creature;
        Spell spell;
        std::string name;
        Stones required;
        Stones missing;
        int matchedRunes;
        int visibleRemaining;
        int completionProbability;
        int manaRequired;
        int manaGap;
        int goalValue;
        int runeGain;
        int availabilityValue;
        int score;

        RuneGoal();

        bool requires(const Stone &) const;
        std::string trace(void) const;
    };

    struct StrategicIntent
    {
        BehaviorProfile profile;
        Difficulty difficulty;
        std::vector<RuneGoal> runeGoals;

        StrategicIntent();

        int runeValue(const Stone &) const;
        std::string trace(void) const;
    };

    struct SummonCandidate
    {
        Creature creature;
        Land destination;
        int staticValue;
        int partySynergy;
        int frontierValue;
        int score;

        SummonCandidate();

        bool isValid(void) const { return creature.isValid() && destination.isValid(); }
        std::string trace(void) const;
    };

    AIObservation observePlayer(const Avatar &);
    StrategicIntent chooseStrategicIntent(const AIObservation &, BehaviorProfile, Difficulty);
    std::vector<SummonCandidate> rankSummonCandidates(const AIObservation &, const Creatures &,
                                                      BehaviorProfile);
}

#endif
