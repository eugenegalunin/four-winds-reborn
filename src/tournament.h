#ifndef FOUR_WINDS_TOURNAMENT_H
#define FOUR_WINDS_TOURNAMENT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "simulation.h"

namespace Tournament
{
    struct Config
    {
        std::vector<std::uint64_t> seeds;
        Avatars avatars;
        AI::Difficulty difficulty = AI::Difficulty::Normal;
        std::size_t maximumTicks = 100000;
        std::size_t maximumUnchangedTicks = 4;
        std::size_t stateHashInterval = 64;
        bool stopOnFailure = false;
    };

    struct PlannedMatch
    {
        std::size_t seedIndex = 0;
        std::size_t clanAssignment = 0;
        std::size_t seatRotation = 0;
        Simulation::MatchConfig match;
    };

    struct Schedule
    {
        std::size_t legalClanAssignments = 0;
        std::vector<PlannedMatch> matches;
        std::string error;

        bool valid(void) const { return error.empty() && !matches.empty(); }
    };

    struct MatchRecord
    {
        PlannedMatch plan;
        Simulation::MatchResult result;
    };

    struct AvatarSummary
    {
        Avatar avatar;
        std::size_t scheduled = 0;
        std::size_t completed = 0;
        std::size_t rankOneFinishes = 0;
        double rankOneRate = 0;
        double rankOneRateLow95 = 0;
        double rankOneRateHigh95 = 0;
        double meanFinalRank = 0;
        double meanTotalScore = 0;
        std::array<double, MatchScore::CategoryCount> meanCategoryScores{};
        std::array<std::size_t, Simulation::MatchStageCount> stageSamples{};
        std::array<std::array<double, MatchScore::CategoryCount>,
                   Simulation::MatchStageCount> meanStageScores{};
    };

    struct Result
    {
        Config config;
        Schedule schedule;
        std::vector<MatchRecord> matches;
        std::vector<AvatarSummary> summaries;
        std::string error;

        std::size_t completedMatches(void) const;
        bool completed(void) const;
    };

    using ProgressCallback =
        std::function<void(std::size_t, std::size_t, const MatchRecord &)>;

    const std::vector<std::uint64_t> & publishedSeeds(void);
    Avatars baselineAvatars(void);
    Schedule buildSchedule(const Config &);
    Result run(const Config &, const ProgressCallback & progress = ProgressCallback());
    void summarize(Result &);

    JsonObject toJsonObject(const Result &);
    std::string toCsv(const Result &);
    std::string toText(const Result &);
    bool saveReports(const Result &, const std::string & directory,
                     std::string* error = nullptr);
}

#endif
