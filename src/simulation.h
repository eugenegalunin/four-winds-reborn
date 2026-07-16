#ifndef FOUR_WINDS_SIMULATION_H
#define FOUR_WINDS_SIMULATION_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "aiprofile.h"
#include "gamedata.h"
#include "matchscore.h"

namespace Simulation
{
    enum class MatchStage : std::size_t
    {
        Early,
        Middle,
        Late,
        Count
    };

    constexpr std::size_t MatchStageCount = static_cast<std::size_t>(MatchStage::Count);

    enum class MatchStatus
    {
        Complete,
        InvalidConfiguration,
        InitializationFailed,
        Stalled,
        TickLimit,
        Exception
    };

    struct MatchConfig
    {
        std::uint64_t seed = UINT64_C(1);
        AI::Difficulty difficulty = AI::Difficulty::Normal;
        Persons persons;
        std::size_t maximumTicks = 100000;
        std::size_t maximumUnchangedTicks = 4;
        std::size_t stateHashInterval = 64;
    };

    struct MatchResult
    {
        struct StageSnapshot
        {
            std::size_t adventurePhase = 0;
            MatchScore::Results score;

            bool captured(void) const { return !score.empty(); }
        };

        MatchStatus status = MatchStatus::InvalidConfiguration;
        std::uint64_t seed = 0;
        std::uint64_t rngDraws = 0;
        std::size_t ticks = 0;
        std::size_t unchangedTicks = 0;
        std::size_t mahjongHands = 0;
        std::size_t adventurePhases = 0;
        std::size_t emittedActions = 0;
        std::string finalStateHash;
        std::string error;
        JsonObject replayTail;
        MatchScore::Results score;
        std::array<StageSnapshot, MatchStageCount> stages{};

        bool completed(void) const { return status == MatchStatus::Complete; }
    };

    constexpr std::size_t index(MatchStage stage)
    {
        return static_cast<std::size_t>(stage);
    }

    const char* stageName(MatchStage);
    const char* statusName(MatchStatus);
    MatchResult runMatch(const MatchConfig &);
}

#endif
