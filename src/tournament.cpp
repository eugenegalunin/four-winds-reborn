#include "tournament.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <sstream>

namespace
{
const std::array<const char*, MatchScore::CategoryCount> categoryNames = {
    "territory", "summon_circles", "units", "spell_points", "land_claims"
};

int jsonInteger(std::size_t value)
{
    return value <= static_cast<std::size_t>(std::numeric_limits<int>::max()) ?
        static_cast<int>(value) : std::numeric_limits<int>::max();
}

bool supportsClan(const Avatar & avatar, const Clan & clan)
{
    const Clans & supported = GameData::avatarInfo(avatar).clans;
    return std::find(supported.begin(), supported.end(), clan) != supported.end();
}

const MatchScore::PlayerResult* findScore(const MatchScore::Results & scores,
                                          const Avatar & avatar)
{
    const auto found = std::find_if(scores.begin(), scores.end(),
        [&avatar](const MatchScore::PlayerResult & score)
        {
            return score.person.avatar == avatar;
        });
    return found == scores.end() ? nullptr : &*found;
}

std::pair<double, double> wilsonInterval(std::size_t successes, std::size_t samples)
{
    if(samples == 0) return { 0, 0 };

    constexpr double z = 1.959963984540054;
    const double n = static_cast<double>(samples);
    const double p = static_cast<double>(successes) / n;
    const double z2 = z * z;
    const double denominator = 1.0 + z2 / n;
    const double center = (p + z2 / (2.0 * n)) / denominator;
    const double spread = z * std::sqrt((p * (1.0 - p) + z2 / (4.0 * n)) / n) /
        denominator;
    return { std::max(0.0, center - spread), std::min(1.0, center + spread) };
}

JsonObject personScoreJson(const MatchScore::PlayerResult & score)
{
    JsonObject object;
    object.addString("avatar", score.person.avatar.toString());
    object.addString("clan", score.person.clan.toString());
    object.addString("wind", score.person.wind.toString());
    object.addInteger("final_rank", score.finalRank);
    object.addInteger("total_score", score.totalScore);

    JsonObject categories;
    for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
    {
        JsonObject value;
        value.addInteger("score", score.categories[category].score);
        value.addInteger("rank", score.categories[category].rank);
        value.addInteger("standing_points", score.categories[category].standingPoints);
        categories.addObject(categoryNames[category], value);
    }
    object.addObject("categories", categories);
    return object;
}

JsonArray scoresJson(const MatchScore::Results & scores)
{
    JsonArray array;
    for(const MatchScore::PlayerResult & score : scores)
        array.addObject(personScoreJson(score));
    return array;
}

std::string csvCell(const std::string & value)
{
    if(value.find_first_of(",\"\r\n") == std::string::npos) return value;

    std::string escaped = "\"";
    for(char character : value)
    {
        if(character == '"') escaped += '"';
        escaped += character;
    }
    escaped += '"';
    return escaped;
}

std::string fixed(double value, int precision = 3)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}
}

std::size_t Tournament::Result::completedMatches(void) const
{
    return static_cast<std::size_t>(std::count_if(matches.begin(), matches.end(),
        [](const MatchRecord & record){ return record.result.completed(); }));
}

bool Tournament::Result::completed(void) const
{
    return error.empty() && !matches.empty() && completedMatches() == schedule.matches.size();
}

const std::vector<std::uint64_t> & Tournament::publishedSeeds(void)
{
    static const std::vector<std::uint64_t> seeds = {
        UINT64_C(0x15a50001), UINT64_C(0x15a50002),
        UINT64_C(0x15a50003), UINT64_C(0x15a50004),
        UINT64_C(0x4f575201), UINT64_C(0x4f575202),
        UINT64_C(0x4f575203), UINT64_C(0x4f575204)
    };
    return seeds;
}

Avatars Tournament::baselineAvatars(void)
{
    return Avatars(std::vector<Avatar>{
        Avatar(Avatar::Nucrus), Avatar(Avatar::Lakkho),
        Avatar(Avatar::Ziag), Avatar(Avatar::Dayla)
    });
}

Tournament::Schedule Tournament::buildSchedule(const Config & config)
{
    Schedule schedule;
    if(config.seeds.empty())
    {
        schedule.error = "at least one seed is required";
        return schedule;
    }
    if(config.avatars.size() != winds_all.size())
    {
        schedule.error = "a tournament roster requires exactly four avatars";
        return schedule;
    }
    if(config.maximumTicks == 0 || config.maximumUnchangedTicks == 0 ||
       config.stateHashInterval == 0)
    {
        schedule.error = "watchdog limits must be positive";
        return schedule;
    }

    std::set<int> avatarIds;
    for(const Avatar & avatar : config.avatars)
    {
        if(!avatar.isValid() || avatar == Avatar(Avatar::Random) ||
           !avatarIds.insert(avatar()).second)
        {
            schedule.error = "avatars must be concrete and unique";
            return schedule;
        }
    }

    std::set<std::uint64_t> seedIds;
    for(std::uint64_t seed : config.seeds)
    {
        if(!seedIds.insert(seed).second)
        {
            schedule.error = "published tournament seeds must be unique";
            return schedule;
        }
    }

    std::array<Clan, 4> clans = {
        Clan(Clan::Red), Clan(Clan::Yellow), Clan(Clan::Aqua), Clan(Clan::Purple)
    };
    std::vector<std::array<Clan, 4>> legalClans;
    do
    {
        bool legal = true;
        for(std::size_t index = 0; index < config.avatars.size(); ++index)
        {
            if(!supportsClan(config.avatars[index], clans[index]))
            {
                legal = false;
                break;
            }
        }
        if(legal) legalClans.push_back(clans);
    }
    while(std::next_permutation(clans.begin(), clans.end()));

    if(legalClans.empty())
    {
        schedule.error = "the roster has no legal one-avatar-per-clan assignment";
        return schedule;
    }
    schedule.legalClanAssignments = legalClans.size();

    const std::array<Wind, 4> winds = {
        Wind(Wind::East), Wind(Wind::South), Wind(Wind::West), Wind(Wind::North)
    };
    for(std::size_t seedIndex = 0; seedIndex < config.seeds.size(); ++seedIndex)
    {
        for(std::size_t clanAssignment = 0;
            clanAssignment < legalClans.size(); ++clanAssignment)
        {
            for(std::size_t rotation = 0; rotation < winds.size(); ++rotation)
            {
                PlannedMatch planned;
                planned.seedIndex = seedIndex;
                planned.clanAssignment = clanAssignment;
                planned.seatRotation = rotation;
                planned.match.seed = config.seeds[seedIndex];
                planned.match.difficulty = config.difficulty;
                planned.match.maximumTicks = config.maximumTicks;
                planned.match.maximumUnchangedTicks = config.maximumUnchangedTicks;
                planned.match.stateHashInterval = config.stateHashInterval;

                for(std::size_t seat = 0; seat < winds.size(); ++seat)
                {
                    const std::size_t avatarIndex =
                        (seat + config.avatars.size() - rotation) % config.avatars.size();
                    Person person(config.avatars[avatarIndex],
                                  legalClans[clanAssignment][avatarIndex], winds[seat]);
                    person.setAI(true);
                    planned.match.persons.push_back(person);
                }
                schedule.matches.push_back(planned);
            }
        }
    }
    return schedule;
}

Tournament::Result Tournament::run(const Config & config, const ProgressCallback & progress)
{
    Result tournament;
    tournament.config = config;
    tournament.schedule = buildSchedule(config);
    if(!tournament.schedule.valid())
    {
        tournament.error = tournament.schedule.error;
        return tournament;
    }

    tournament.matches.reserve(tournament.schedule.matches.size());
    for(const PlannedMatch & planned : tournament.schedule.matches)
    {
        MatchRecord record;
        record.plan = planned;
        record.result = Simulation::runMatch(planned.match);
        tournament.matches.push_back(record);

        if(progress)
            progress(tournament.matches.size(), tournament.schedule.matches.size(),
                     tournament.matches.back());

        if(config.stopOnFailure && !record.result.completed()) break;
    }

    summarize(tournament);
    return tournament;
}

void Tournament::summarize(Result & tournament)
{
    tournament.summaries.clear();
    for(const Avatar & avatar : tournament.config.avatars)
    {
        AvatarSummary summary;
        summary.avatar = avatar;
        tournament.summaries.push_back(summary);
    }

    for(const MatchRecord & record : tournament.matches)
    {
        for(const Person & person : record.plan.match.persons)
        {
            auto summary = std::find_if(tournament.summaries.begin(), tournament.summaries.end(),
                [&person](const AvatarSummary & value){ return value.avatar == person.avatar; });
            if(summary != tournament.summaries.end()) ++summary->scheduled;
        }

        if(!record.result.completed()) continue;
        for(const MatchScore::PlayerResult & score : record.result.score)
        {
            auto summary = std::find_if(tournament.summaries.begin(), tournament.summaries.end(),
                [&score](const AvatarSummary & value){ return value.avatar == score.person.avatar; });
            if(summary == tournament.summaries.end()) continue;

            ++summary->completed;
            if(score.finalRank == 1) ++summary->rankOneFinishes;
            summary->meanFinalRank += score.finalRank;
            summary->meanTotalScore += score.totalScore;
            for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
                summary->meanCategoryScores[category] += score.categories[category].score;

            for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
            {
                const Simulation::MatchResult::StageSnapshot & snapshot =
                    record.result.stages[stage];
                const MatchScore::PlayerResult* stageScore =
                    findScore(snapshot.score, score.person.avatar);
                if(!stageScore) continue;

                ++summary->stageSamples[stage];
                for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
                    summary->meanStageScores[stage][category] +=
                        stageScore->categories[category].score;
            }
        }
    }

    for(AvatarSummary & summary : tournament.summaries)
    {
        if(summary.completed)
        {
            const double completed = static_cast<double>(summary.completed);
            summary.rankOneRate = static_cast<double>(summary.rankOneFinishes) / completed;
            const auto interval = wilsonInterval(summary.rankOneFinishes, summary.completed);
            summary.rankOneRateLow95 = interval.first;
            summary.rankOneRateHigh95 = interval.second;
            summary.meanFinalRank /= completed;
            summary.meanTotalScore /= completed;
            for(double & score : summary.meanCategoryScores) score /= completed;
        }

        for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
        {
            if(!summary.stageSamples[stage]) continue;
            const double samples = static_cast<double>(summary.stageSamples[stage]);
            for(double & score : summary.meanStageScores[stage]) score /= samples;
        }
    }
}

JsonObject Tournament::toJsonObject(const Result & tournament)
{
    JsonObject root;
    root.addInteger("schema_version", 1);
    root.addString("difficulty", AI::difficultyName(tournament.config.difficulty));
    root.addInteger("legal_clan_assignments",
                    jsonInteger(tournament.schedule.legalClanAssignments));
    root.addInteger("planned_matches", jsonInteger(tournament.schedule.matches.size()));
    root.addInteger("executed_matches", jsonInteger(tournament.matches.size()));
    root.addInteger("completed_matches", jsonInteger(tournament.completedMatches()));
    root.addString("error", tournament.error);

    JsonArray seeds;
    for(std::uint64_t seed : tournament.config.seeds)
        seeds.addString(std::to_string(seed));
    root.addArray("seeds", seeds);

    JsonArray roster;
    for(const Avatar & avatar : tournament.config.avatars)
        roster.addString(avatar.toString());
    root.addArray("roster", roster);

    JsonArray matches;
    for(std::size_t index = 0; index < tournament.matches.size(); ++index)
    {
        const MatchRecord & record = tournament.matches[index];
        JsonObject match;
        match.addInteger("run", jsonInteger(index));
        match.addInteger("seed_index", jsonInteger(record.plan.seedIndex));
        match.addString("seed", std::to_string(record.result.seed));
        match.addInteger("clan_assignment", jsonInteger(record.plan.clanAssignment));
        match.addInteger("seat_rotation", jsonInteger(record.plan.seatRotation));
        match.addString("status", Simulation::statusName(record.result.status));
        match.addInteger("ticks", jsonInteger(record.result.ticks));
        match.addInteger("mahjong_hands", jsonInteger(record.result.mahjongHands));
        match.addInteger("adventure_phases", jsonInteger(record.result.adventurePhases));
        match.addInteger("emitted_actions", jsonInteger(record.result.emittedActions));
        match.addString("rng_draws", std::to_string(record.result.rngDraws));
        match.addString("final_state_hash", record.result.finalStateHash);
        match.addString("error", record.result.error);
        match.addArray("players", scoresJson(record.result.score));

        JsonArray stages;
        for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
        {
            const auto stageId = static_cast<Simulation::MatchStage>(stage);
            const Simulation::MatchResult::StageSnapshot & snapshot =
                record.result.stages[stage];
            if(!snapshot.captured()) continue;

            JsonObject stageObject;
            stageObject.addString("stage", Simulation::stageName(stageId));
            stageObject.addInteger("adventure_phase", jsonInteger(snapshot.adventurePhase));
            stageObject.addArray("players", scoresJson(snapshot.score));
            stages.addObject(stageObject);
        }
        match.addArray("stages", stages);
        matches.addObject(match);
    }
    root.addArray("matches", matches);

    JsonArray summaries;
    for(const AvatarSummary & summary : tournament.summaries)
    {
        JsonObject object;
        object.addString("avatar", summary.avatar.toString());
        object.addInteger("scheduled", jsonInteger(summary.scheduled));
        object.addInteger("completed", jsonInteger(summary.completed));
        object.addInteger("rank_one_finishes", jsonInteger(summary.rankOneFinishes));
        object.addDouble("rank_one_rate", summary.rankOneRate);
        object.addDouble("rank_one_rate_low_95", summary.rankOneRateLow95);
        object.addDouble("rank_one_rate_high_95", summary.rankOneRateHigh95);
        object.addDouble("mean_final_rank", summary.meanFinalRank);
        object.addDouble("mean_total_score", summary.meanTotalScore);

        JsonObject categories;
        for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
            categories.addDouble(categoryNames[category], summary.meanCategoryScores[category]);
        object.addObject("mean_final_categories", categories);

        JsonArray stages;
        for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
        {
            JsonObject stageObject;
            stageObject.addString("stage", Simulation::stageName(
                static_cast<Simulation::MatchStage>(stage)));
            stageObject.addInteger("samples", jsonInteger(summary.stageSamples[stage]));
            for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
                stageObject.addDouble(categoryNames[category],
                                      summary.meanStageScores[stage][category]);
            stages.addObject(stageObject);
        }
        object.addArray("mean_stages", stages);
        summaries.addObject(object);
    }
    root.addArray("avatar_summary", summaries);
    return root;
}

std::string Tournament::toCsv(const Result & tournament)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "run,seed,seed_index,clan_assignment,seat_rotation,status,avatar,clan,wind,"
              "final_rank,total_score";
    for(const char* category : categoryNames) stream << ',' << category;
    for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
        for(const char* category : categoryNames)
            stream << ',' << Simulation::stageName(static_cast<Simulation::MatchStage>(stage))
                   << '_' << category;
    stream << ",ticks,mahjong_hands,adventure_phases,emitted_actions,rng_draws,"
              "final_state_hash,error\n";

    for(std::size_t runIndex = 0; runIndex < tournament.matches.size(); ++runIndex)
    {
        const MatchRecord & record = tournament.matches[runIndex];
        for(const Person & person : record.plan.match.persons)
        {
            const MatchScore::PlayerResult* score = findScore(record.result.score, person.avatar);
            stream << runIndex << ',' << record.result.seed << ',' << record.plan.seedIndex << ','
                   << record.plan.clanAssignment << ',' << record.plan.seatRotation << ','
                   << Simulation::statusName(record.result.status) << ','
                   << person.avatar.toString() << ',' << person.clan.toString() << ','
                   << person.wind.toString() << ',';
            if(score) stream << score->finalRank << ',' << score->totalScore;
            else stream << ',';

            for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
            {
                stream << ',';
                if(score) stream << score->categories[category].score;
            }

            for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
            {
                const MatchScore::PlayerResult* stageScore =
                    findScore(record.result.stages[stage].score, person.avatar);
                for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
                {
                    stream << ',';
                    if(stageScore) stream << stageScore->categories[category].score;
                }
            }

            stream << ',' << record.result.ticks
                   << ',' << record.result.mahjongHands
                   << ',' << record.result.adventurePhases
                   << ',' << record.result.emittedActions
                   << ',' << record.result.rngDraws
                   << ',' << csvCell(record.result.finalStateHash)
                   << ',' << csvCell(record.result.error) << '\n';
        }
    }
    return stream.str();
}

std::string Tournament::toText(const Result & tournament)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "Four Winds Reborn balance laboratory\n"
           << "Difficulty: " << AI::difficultyName(tournament.config.difficulty) << '\n'
           << "Seeds: ";
    for(std::size_t index = 0; index < tournament.config.seeds.size(); ++index)
    {
        if(index) stream << ", ";
        stream << tournament.config.seeds[index];
    }
    stream << "\nLegal clan assignments: " << tournament.schedule.legalClanAssignments
           << "\nMatches: " << tournament.completedMatches() << '/'
           << tournament.schedule.matches.size() << " complete\n\n"
           << "Avatar  Done  Rank1  Rate (95% Wilson)       Mean rank  Mean total\n";

    for(const AvatarSummary & summary : tournament.summaries)
    {
        stream << std::left << std::setw(8) << summary.avatar.toString()
               << std::right << std::setw(5) << summary.completed
               << std::setw(7) << summary.rankOneFinishes << "  "
               << std::setw(6) << fixed(summary.rankOneRate) << " ["
               << fixed(summary.rankOneRateLow95) << ", "
               << fixed(summary.rankOneRateHigh95) << "]"
               << std::setw(11) << fixed(summary.meanFinalRank, 2)
               << std::setw(12) << fixed(summary.meanTotalScore, 2) << '\n';
    }

    stream << "\nStage means are raw authoritative scores at Adventure phases 5, 10 and final.\n";
    if(!tournament.error.empty()) stream << "Error: " << tournament.error << '\n';
    return stream.str();
}

bool Tournament::saveReports(const Result & tournament, const std::string & directory,
                             std::string* error)
{
    if(directory.empty())
    {
        if(error) *error = "report directory is empty";
        return false;
    }
    if(!Systems::isDirectory(directory) && !Systems::makeDirectory(directory))
    {
        if(error) *error = "could not create report directory: " + directory;
        return false;
    }

    const std::array<std::pair<std::string, std::string>, 3> reports = {{
        { "balance-report.json", toJsonObject(tournament).toString() },
        { "balance-players.csv", toCsv(tournament) },
        { "balance-report.txt", toText(tournament) }
    }};
    for(const auto & report : reports)
    {
        const std::string path = Systems::concatePath(directory, report.first);
        if(!Systems::saveString2File(report.second, path))
        {
            if(error) *error = "could not write report: " + path;
            return false;
        }
    }
    return true;
}
