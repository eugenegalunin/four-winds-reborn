#include "tournament.h"

#include <algorithm>
#include <cstdlib>
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

const char* behaviorProfileName(const Simulation::MatchConfig & config)
{
    return config.forceBehaviorProfile ?
        AI::behaviorProfileName(config.behaviorProfile) : "native";
}

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
    object.addString("clan_name", score.person.clan.canonicalName());
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

JsonArray countedNamesJson(const std::map<std::string, std::size_t> & values)
{
    JsonArray result;
    for(const auto & value : values)
    {
        JsonObject entry;
        entry.addString("name", value.first);
        entry.addInteger("count", jsonInteger(value.second));
        result.addObject(entry);
    }
    return result;
}

JsonObject playerTelemetryJson(const Simulation::MatchResult::PlayerTelemetry & player)
{
    JsonObject result;
    result.addString("avatar", player.avatar.toString());
    result.addInteger("summons", jsonInteger(player.summons));
    result.addInteger("spells_cast", jsonInteger(player.spellsCast));
    result.addInteger("casualties", jsonInteger(player.casualties));
    result.addInteger("dismissals", jsonInteger(player.dismissals));
    result.addInteger("peak_units", jsonInteger(player.peakUnits));
    result.addInteger("final_units", jsonInteger(player.finalUnits));
    result.addArray("summoned_creatures", countedNamesJson(player.summonedCreatures));
    result.addArray("cast_spells", countedNamesJson(player.castSpells));

    JsonArray army;
    for(const Simulation::MatchResult::PartyComposition & party : player.finalArmy)
    {
        JsonObject encoded;
        encoded.addString("land", party.land.toString());
        JsonArray creatures;
        for(const std::string & creature : party.creatures)
            creatures.addString(creature);
        encoded.addArray("creatures", creatures);
        army.addObject(encoded);
    }
    result.addArray("final_army", army);
    return result;
}

JsonObject eventJson(const Simulation::MatchResult::Event & event)
{
    JsonObject result;
    result.addString("type", event.type);
    result.addInteger("tick", jsonInteger(event.tick));
    result.addInteger("adventure_phase", jsonInteger(event.adventurePhase));
    result.addString("avatar", event.avatar.toString());
    result.addString("subject", event.subject);
    if(event.land.isValid()) result.addString("land", event.land.toString());
    if(event.unit) result.addInteger("unit", event.unit);
    return result;
}

JsonObject matchJson(const Tournament::MatchRecord & record, std::size_t run,
                     bool includeReplay)
{
    JsonObject match;
    match.addInteger("run", jsonInteger(run));
    match.addInteger("seed_index", jsonInteger(record.plan.seedIndex));
    match.addString("seed", std::to_string(record.result.seed));
    match.addInteger("clan_assignment", jsonInteger(record.plan.clanAssignment));
    match.addInteger("seat_rotation", jsonInteger(record.plan.seatRotation));
    match.addString("difficulty", AI::difficultyName(record.plan.match.difficulty));
    match.addString("behavior_profile", behaviorProfileName(record.plan.match));
    match.addString("status", Simulation::statusName(record.result.status));
    match.addInteger("ticks", jsonInteger(record.result.ticks));
    match.addInteger("unchanged_ticks", jsonInteger(record.result.unchangedTicks));
    match.addInteger("mahjong_hands", jsonInteger(record.result.mahjongHands));
    match.addInteger("adventure_phases", jsonInteger(record.result.adventurePhases));
    match.addInteger("emitted_actions", jsonInteger(record.result.emittedActions));
    match.addString("rng_draws", std::to_string(record.result.rngDraws));
    match.addString("final_state_hash", record.result.finalStateHash);
    match.addString("error", record.result.error);
    match.addBoolean("full_replay_captured", record.result.fullReplayCaptured);
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

    JsonArray telemetry;
    for(const auto & player : record.result.players)
        telemetry.addObject(playerTelemetryJson(player));
    match.addArray("player_telemetry", telemetry);

    JsonArray events;
    for(const auto & event : record.result.events) events.addObject(eventJson(event));
    match.addArray("events", events);

    JsonArray reasons;
    for(const std::string & reason : record.replayRetentionReasons)
        reasons.addString(reason);
    match.addArray("replay_retention_reasons", reasons);
    if(includeReplay && record.result.actionReplay.isValid())
        match.addObject("action_replay", record.result.actionReplay);
    return match;
}

bool parseUnsigned(const std::string & text, std::uint64_t & value)
{
    if(text.empty()) return false;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    if(!end || *end != '\0') return false;
    value = static_cast<std::uint64_t>(parsed);
    return true;
}

bool parseStatus(const std::string & name, Simulation::MatchStatus & status)
{
    const std::array<Simulation::MatchStatus, 6> values = {{
        Simulation::MatchStatus::Complete,
        Simulation::MatchStatus::InvalidConfiguration,
        Simulation::MatchStatus::InitializationFailed,
        Simulation::MatchStatus::Stalled,
        Simulation::MatchStatus::TickLimit,
        Simulation::MatchStatus::Exception
    }};
    for(const auto value : values)
    {
        if(name == Simulation::statusName(value))
        {
            status = value;
            return true;
        }
    }
    return false;
}

bool parseScores(const JsonArray* encoded, MatchScore::Results & scores)
{
    scores.clear();
    if(!encoded) return false;
    for(std::size_t index = 0; index < encoded->size(); ++index)
    {
        const JsonObject* object = encoded->getObject(index);
        const JsonObject* categories = object ? object->getObject("categories") : nullptr;
        if(!object || !categories) return false;

        MatchScore::PlayerResult score;
        score.person = Person(Avatar(object->getString("avatar")),
                              Clan(object->getString("clan")),
                              Wind(object->getString("wind")));
        score.person.setAI(true);
        score.finalRank = object->getInteger("final_rank");
        score.totalScore = object->getInteger("total_score");
        if(!score.person.avatar.isValid() || !score.person.clan.isValid() ||
           !score.person.wind.isValid()) return false;

        for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
        {
            const JsonObject* value = categories->getObject(categoryNames[category]);
            if(!value) return false;
            score.categories[category].score = value->getInteger("score");
            score.categories[category].rank = value->getInteger("rank");
            score.categories[category].standingPoints =
                value->getInteger("standing_points");
        }
        scores.push_back(score);
    }
    return true;
}

bool parseCountedNames(const JsonArray* encoded, std::map<std::string, std::size_t> & values)
{
    values.clear();
    if(!encoded) return false;
    for(std::size_t index = 0; index < encoded->size(); ++index)
    {
        const JsonObject* entry = encoded->getObject(index);
        const std::string name = entry ? entry->getString("name") : std::string();
        const int count = entry ? entry->getInteger("count", -1) : -1;
        if(name.empty() || count < 0) return false;
        values[name] = static_cast<std::size_t>(count);
    }
    return true;
}

double median(std::vector<double> values)
{
    if(values.empty()) return 0;
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] :
        (values[middle - 1] + values[middle]) / 2.0;
}

std::pair<double, double> iqrBounds(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    std::vector<double> lower(values.begin(), values.begin() + middle);
    std::vector<double> upper(values.begin() + middle + (values.size() % 2), values.end());
    const double q1 = median(lower);
    const double q3 = median(upper);
    const double spread = q3 - q1;
    return { q1 - 1.5 * spread, q3 + 1.5 * spread };
}

void appendReason(Tournament::MatchRecord & record, const std::string & reason)
{
    if(std::find(record.replayRetentionReasons.begin(),
                 record.replayRetentionReasons.end(), reason) ==
       record.replayRetentionReasons.end())
        record.replayRetentionReasons.push_back(reason);
}

void markReplayRetention(Tournament::Result & tournament)
{
    for(auto & record : tournament.matches)
    {
        record.replayRetentionReasons.clear();
        if(!record.result.completed())
            appendReason(record, std::string("failure:") +
                         Simulation::statusName(record.result.status));
    }

    std::vector<double> ticks;
    for(const auto & record : tournament.matches)
        if(record.result.completed()) ticks.push_back(static_cast<double>(record.result.ticks));
    if(8 <= ticks.size())
    {
        const auto bounds = iqrBounds(ticks);
        for(auto & record : tournament.matches)
        {
            if(record.result.completed() &&
               (record.result.ticks < bounds.first || bounds.second < record.result.ticks))
                appendReason(record, "outlier:match_ticks_iqr");
        }
    }

    for(const Avatar & avatar : tournament.config.avatars)
    {
        std::vector<double> totals;
        for(const auto & record : tournament.matches)
        {
            const MatchScore::PlayerResult* score = findScore(record.result.score, avatar);
            if(record.result.completed() && score) totals.push_back(score->totalScore);
        }
        if(totals.size() < 8) continue;
        const auto bounds = iqrBounds(totals);
        for(auto & record : tournament.matches)
        {
            const MatchScore::PlayerResult* score = findScore(record.result.score, avatar);
            if(record.result.completed() && score &&
               (score->totalScore < bounds.first || bounds.second < score->totalScore))
                appendReason(record, "outlier:total_score_iqr:" + avatar.toString());
        }
    }
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
                planned.match.forceBehaviorProfile = config.forceBehaviorProfile;
                planned.match.behaviorProfile = config.behaviorProfile;
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

Tournament::Result Tournament::assemble(const Config & config,
                                        const std::vector<MatchRecord> & records)
{
    Result tournament;
    tournament.config = config;
    tournament.schedule = buildSchedule(config);
    if(!tournament.schedule.valid())
    {
        tournament.error = tournament.schedule.error;
        return tournament;
    }
    if(records.size() != tournament.schedule.matches.size())
    {
        tournament.error = "isolated match record count does not match the schedule";
        return tournament;
    }

    tournament.matches = records;
    for(std::size_t index = 0; index < records.size(); ++index)
    {
        const PlannedMatch & expected = tournament.schedule.matches[index];
        const PlannedMatch & actual = records[index].plan;
        if(actual.seedIndex != expected.seedIndex ||
           actual.clanAssignment != expected.clanAssignment ||
           actual.seatRotation != expected.seatRotation ||
           actual.match.seed != expected.match.seed ||
           actual.match.difficulty != expected.match.difficulty ||
           actual.match.forceBehaviorProfile != expected.match.forceBehaviorProfile ||
           (actual.match.forceBehaviorProfile &&
            actual.match.behaviorProfile != expected.match.behaviorProfile))
        {
            tournament.error = "isolated match record order does not match the schedule";
            tournament.matches.clear();
            return tournament;
        }
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
            const auto telemetry = std::find_if(record.result.players.begin(),
                record.result.players.end(), [&score](const auto & value)
                {
                    return value.avatar == score.person.avatar;
                });
            if(telemetry != record.result.players.end())
            {
                summary->meanSummons += telemetry->summons;
                summary->meanSpellsCast += telemetry->spellsCast;
                summary->meanCasualties += telemetry->casualties;
                summary->meanDismissals += telemetry->dismissals;
                summary->meanPeakUnits += telemetry->peakUnits;
                summary->meanFinalUnits += telemetry->finalUnits;
            }
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
            summary.meanSummons /= completed;
            summary.meanSpellsCast /= completed;
            summary.meanCasualties /= completed;
            summary.meanDismissals /= completed;
            summary.meanPeakUnits /= completed;
            summary.meanFinalUnits /= completed;
            for(double & score : summary.meanCategoryScores) score /= completed;
        }

        for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
        {
            if(!summary.stageSamples[stage]) continue;
            const double samples = static_cast<double>(summary.stageSamples[stage]);
            for(double & score : summary.meanStageScores[stage]) score /= samples;
        }
    }
    markReplayRetention(tournament);
}

JsonObject Tournament::matchRecordJson(const MatchRecord & record,
                                       std::size_t scheduleIndex,
                                       bool includeReplay)
{
    JsonObject root;
    root.addInteger("schema_version", 2);
    root.addInteger("schedule_index", jsonInteger(scheduleIndex));
    root.addObject("match", matchJson(record, scheduleIndex, includeReplay));
    return root;
}

bool Tournament::saveMatchRecord(const MatchRecord & record, std::size_t scheduleIndex,
                                 const std::string & path, std::string* error)
{
    if(path.empty() ||
       !Systems::saveString2File(matchRecordJson(record, scheduleIndex, true).toString(), path))
    {
        if(error) *error = "could not write isolated match record: " + path;
        return false;
    }
    return true;
}

bool Tournament::loadMatchRecord(const std::string & path, const PlannedMatch & expected,
                                 std::size_t scheduleIndex, MatchRecord & record,
                                 std::string* error)
{
    JsonContentFile file(path);
    if(!file.isValid() || !file.isObject())
    {
        if(error) *error = "isolated match record is not valid JSON: " + path;
        return false;
    }
    const JsonObject root = file.toObject();
    const JsonObject* match = root.getObject("match");
    if(root.getInteger("schema_version") != 2 ||
       root.getInteger("schedule_index", -1) != jsonInteger(scheduleIndex) || !match)
    {
        if(error) *error = "isolated match record header mismatch: " + path;
        return false;
    }

    std::uint64_t seed = 0;
    Simulation::MatchStatus status;
    if(match->getInteger("run", -1) != jsonInteger(scheduleIndex) ||
       match->getInteger("seed_index", -1) != jsonInteger(expected.seedIndex) ||
       match->getInteger("clan_assignment", -1) != jsonInteger(expected.clanAssignment) ||
       match->getInteger("seat_rotation", -1) != jsonInteger(expected.seatRotation) ||
       match->getString("difficulty") != AI::difficultyName(expected.match.difficulty) ||
       match->getString("behavior_profile") != behaviorProfileName(expected.match) ||
       !parseUnsigned(match->getString("seed"), seed) || seed != expected.match.seed ||
       !parseStatus(match->getString("status"), status))
    {
        if(error) *error = "isolated match record schedule mismatch: " + path;
        return false;
    }

    MatchRecord loaded;
    loaded.plan = expected;
    loaded.result.status = status;
    loaded.result.seed = seed;
    loaded.result.ticks = static_cast<std::size_t>(match->getInteger("ticks"));
    loaded.result.unchangedTicks =
        static_cast<std::size_t>(match->getInteger("unchanged_ticks"));
    loaded.result.mahjongHands =
        static_cast<std::size_t>(match->getInteger("mahjong_hands"));
    loaded.result.adventurePhases =
        static_cast<std::size_t>(match->getInteger("adventure_phases"));
    loaded.result.emittedActions =
        static_cast<std::size_t>(match->getInteger("emitted_actions"));
    if(!parseUnsigned(match->getString("rng_draws"), loaded.result.rngDraws) ||
       !parseScores(match->getArray("players"), loaded.result.score))
    {
        if(error) *error = "isolated match record result payload is invalid: " + path;
        return false;
    }
    loaded.result.finalStateHash = match->getString("final_state_hash");
    loaded.result.error = match->getString("error");
    loaded.result.fullReplayCaptured = match->getBoolean("full_replay_captured");
    if(const JsonObject* replay = match->getObject("action_replay"))
        loaded.result.actionReplay = *replay;

    if(const JsonArray* stages = match->getArray("stages"))
    {
        for(std::size_t index = 0; index < stages->size(); ++index)
        {
            const JsonObject* encoded = stages->getObject(index);
            if(!encoded) continue;
            for(std::size_t stage = 0; stage < Simulation::MatchStageCount; ++stage)
            {
                const auto stageId = static_cast<Simulation::MatchStage>(stage);
                if(encoded->getString("stage") != Simulation::stageName(stageId)) continue;
                auto & snapshot = loaded.result.stages[stage];
                snapshot.adventurePhase = static_cast<std::size_t>(
                    encoded->getInteger("adventure_phase"));
                if(!parseScores(encoded->getArray("players"), snapshot.score))
                {
                    if(error) *error = "isolated match stage payload is invalid: " + path;
                    return false;
                }
            }
        }
    }

    const JsonArray* telemetry = match->getArray("player_telemetry");
    if(!telemetry)
    {
        if(error) *error = "isolated match telemetry is missing: " + path;
        return false;
    }
    for(std::size_t index = 0; index < telemetry->size(); ++index)
    {
        const JsonObject* encoded = telemetry->getObject(index);
        Simulation::MatchResult::PlayerTelemetry player;
        player.avatar = Avatar(encoded ? encoded->getString("avatar") : std::string());
        if(!encoded || !player.avatar.isValid())
        {
            if(error) *error = "isolated player telemetry is invalid: " + path;
            return false;
        }
        player.summons = static_cast<std::size_t>(encoded->getInteger("summons"));
        player.spellsCast = static_cast<std::size_t>(encoded->getInteger("spells_cast"));
        player.casualties = static_cast<std::size_t>(encoded->getInteger("casualties"));
        player.dismissals = static_cast<std::size_t>(encoded->getInteger("dismissals"));
        player.peakUnits = static_cast<std::size_t>(encoded->getInteger("peak_units"));
        player.finalUnits = static_cast<std::size_t>(encoded->getInteger("final_units"));
        if(!parseCountedNames(encoded->getArray("summoned_creatures"),
                              player.summonedCreatures) ||
           !parseCountedNames(encoded->getArray("cast_spells"), player.castSpells))
        {
            if(error) *error = "isolated event counters are invalid: " + path;
            return false;
        }

        const JsonArray* army = encoded->getArray("final_army");
        if(!army)
        {
            if(error) *error = "isolated final army is missing: " + path;
            return false;
        }
        for(std::size_t partyIndex = 0; partyIndex < army->size(); ++partyIndex)
        {
            const JsonObject* encodedParty = army->getObject(partyIndex);
            const JsonArray* creatures = encodedParty ?
                encodedParty->getArray("creatures") : nullptr;
            Simulation::MatchResult::PartyComposition party;
            party.land = Land(encodedParty ? encodedParty->getString("land") : std::string());
            if(!encodedParty || !party.land.isValid() || !creatures)
            {
                if(error) *error = "isolated party composition is invalid: " + path;
                return false;
            }
            for(std::size_t creature = 0; creature < creatures->size(); ++creature)
                party.creatures.push_back(creatures->getString(creature));
            player.finalArmy.push_back(party);
        }
        loaded.result.players.push_back(player);
    }

    const JsonArray* events = match->getArray("events");
    if(!events)
    {
        if(error) *error = "isolated event timeline is missing: " + path;
        return false;
    }
    for(std::size_t index = 0; index < events->size(); ++index)
    {
        const JsonObject* encoded = events->getObject(index);
        Simulation::MatchResult::Event event;
        if(!encoded) return false;
        event.type = encoded->getString("type");
        event.tick = static_cast<std::size_t>(encoded->getInteger("tick"));
        event.adventurePhase =
            static_cast<std::size_t>(encoded->getInteger("adventure_phase"));
        event.avatar = Avatar(encoded->getString("avatar"));
        event.subject = encoded->getString("subject");
        event.land = Land(encoded->getString("land"));
        event.unit = encoded->getInteger("unit");
        if(event.type.empty() || !event.avatar.isValid() || event.subject.empty())
        {
            if(error) *error = "isolated event timeline is invalid: " + path;
            return false;
        }
        loaded.result.events.push_back(event);
    }

    if(loaded.result.completed() && loaded.result.fullReplayCaptured &&
       (!loaded.result.actionReplay.isValid() ||
        !loaded.result.actionReplay.getBoolean("contiguousToCheckpoint")))
    {
        if(error) *error = "isolated full replay is missing or non-contiguous: " + path;
        return false;
    }
    record = loaded;
    return true;
}

JsonObject Tournament::toJsonObject(const Result & tournament)
{
    JsonObject root;
    root.addInteger("schema_version", 2);
    root.addString("difficulty", AI::difficultyName(tournament.config.difficulty));
    root.addString("behavior_profile", tournament.config.forceBehaviorProfile ?
        AI::behaviorProfileName(tournament.config.behaviorProfile) : "native");
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
        matches.addObject(matchJson(tournament.matches[index], index, false));
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
        object.addDouble("mean_summons", summary.meanSummons);
        object.addDouble("mean_spells_cast", summary.meanSpellsCast);
        object.addDouble("mean_casualties", summary.meanCasualties);
        object.addDouble("mean_dismissals", summary.meanDismissals);
        object.addDouble("mean_peak_units", summary.meanPeakUnits);
        object.addDouble("mean_final_units", summary.meanFinalUnits);

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
    stream << "run,seed,seed_index,clan_assignment,seat_rotation,difficulty,behavior_profile,"
              "status,avatar,clan,clan_id,wind,"
              "final_rank,total_score,summons,spells_cast,casualties,dismissals,peak_units,final_units";
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
            const auto telemetry = std::find_if(record.result.players.begin(),
                record.result.players.end(), [&person](const auto & value)
                {
                    return value.avatar == person.avatar;
                });
            stream << runIndex << ',' << record.result.seed << ',' << record.plan.seedIndex << ','
                   << record.plan.clanAssignment << ',' << record.plan.seatRotation << ','
                   << AI::difficultyName(record.plan.match.difficulty) << ','
                   << behaviorProfileName(record.plan.match) << ','
                   << Simulation::statusName(record.result.status) << ','
                   << person.avatar.toString() << ',' << person.clan.canonicalName() << ','
                   << person.clan.toString() << ',' << person.wind.toString() << ',';
            if(score) stream << score->finalRank << ',' << score->totalScore;
            else stream << ',';
            if(telemetry != record.result.players.end())
                stream << ',' << telemetry->summons << ',' << telemetry->spellsCast
                       << ',' << telemetry->casualties << ',' << telemetry->dismissals
                       << ',' << telemetry->peakUnits << ',' << telemetry->finalUnits;
            else stream << ",,,,,,";

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
           << "Behavior profile: " << (tournament.config.forceBehaviorProfile ?
                AI::behaviorProfileName(tournament.config.behaviorProfile) : "native") << '\n'
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

    stream << "\nAvatar  Summons  Spells  Losses  Dismiss  Peak units  Final units\n";
    for(const AvatarSummary & summary : tournament.summaries)
    {
        stream << std::left << std::setw(8) << summary.avatar.toString()
               << std::right << std::setw(8) << fixed(summary.meanSummons, 2)
               << std::setw(8) << fixed(summary.meanSpellsCast, 2)
               << std::setw(8) << fixed(summary.meanCasualties, 2)
               << std::setw(9) << fixed(summary.meanDismissals, 2)
               << std::setw(12) << fixed(summary.meanPeakUnits, 2)
               << std::setw(13) << fixed(summary.meanFinalUnits, 2) << '\n';
    }

    stream << "\nStage means are raw authoritative scores at Adventure phases 5, 10 and final.\n";
    const std::size_t retained = static_cast<std::size_t>(std::count_if(
        tournament.matches.begin(), tournament.matches.end(), [](const MatchRecord & record)
        {
            return !record.replayRetentionReasons.empty();
        }));
    stream << "Failure/outlier replays selected: " << retained << '\n';
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

    const bool retainAny = std::any_of(tournament.matches.begin(), tournament.matches.end(),
        [](const MatchRecord & record)
        {
            return !record.replayRetentionReasons.empty() &&
                   record.result.actionReplay.isValid();
        });
    if(retainAny)
    {
        const std::string replayDirectory = Systems::concatePath(directory, "replays");
        if(!Systems::isDirectory(replayDirectory) && !Systems::makeDirectory(replayDirectory))
        {
            if(error) *error = "could not create retained replay directory: " + replayDirectory;
            return false;
        }
        for(std::size_t index = 0; index < tournament.matches.size(); ++index)
        {
            const MatchRecord & record = tournament.matches[index];
            if(record.replayRetentionReasons.empty() ||
               !record.result.actionReplay.isValid()) continue;

            std::ostringstream name;
            name << "match-" << std::setfill('0') << std::setw(6) << index << ".json";
            const std::string path = Systems::concatePath(replayDirectory, name.str());
            if(!Systems::saveString2File(matchRecordJson(record, index, true).toString(), path))
            {
                if(error) *error = "could not write retained replay: " + path;
                return false;
            }
        }
    }
    return true;
}
