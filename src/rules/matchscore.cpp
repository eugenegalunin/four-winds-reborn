#include "matchscore.h"

#include <algorithm>

#include "gamedata.h"

namespace
{
int competitionRank(const std::vector<int> & values, int value)
{
    return 1 + static_cast<int>(std::count_if(values.begin(), values.end(),
        [value](int other){ return value < other; }));
}
}

MatchScore::PlayerInput MatchScore::observe(const RemotePlayer & player)
{
    PlayerInput input;
    input.person = player;

    for(const Land & land : player.lands())
    {
        const LandInfo & info = GameData::landInfo(land);
        input.scores[index(Category::Territory)] += std::max(0, info.stat.point);
        if(info.stat.power) ++input.scores[index(Category::SummonCircles)];
    }

    for(const BattleCreature* creature : player.army.toBattleCreatures())
    {
        if(creature)
            input.scores[index(Category::Units)] +=
                std::max(0, GameData::creatureInfo(*creature).cost);
    }

    input.scores[index(Category::SpellPoints)] = std::max(0, player.points);
    for(const auto clanId : clans_all)
    {
        const Clan clan(clanId);
        if(clan != player.clan)
            input.scores[index(Category::LandClaims)] +=
                std::max(0, player.landClaimPoints(clan));
    }

    return input;
}

MatchScore::Results MatchScore::calculate(const std::vector<PlayerInput> & inputs)
{
    Results results(inputs.size());
    if(inputs.empty()) return results;

    for(std::size_t player = 0; player < inputs.size(); ++player)
    {
        results[player].person = inputs[player].person;
        for(std::size_t category = 0; category < CategoryCount; ++category)
            results[player].categories[category].score =
                std::max(0, inputs[player].scores[category]);
    }

    for(std::size_t category = 0; category < CategoryCount; ++category)
    {
        std::vector<int> values;
        values.reserve(results.size());
        for(const PlayerResult & player : results)
            values.push_back(player.categories[category].score);

        for(PlayerResult & player : results)
        {
            CategoryResult & score = player.categories[category];
            score.rank = competitionRank(values, score.score);
            score.standingPoints =
                static_cast<int>(results.size()) - score.rank + 1;
            player.totalScore += score.standingPoints;
        }
    }

    std::vector<int> totals;
    totals.reserve(results.size());
    for(const PlayerResult & player : results) totals.push_back(player.totalScore);
    for(PlayerResult & player : results)
        player.finalRank = competitionRank(totals, player.totalScore);

    return results;
}

MatchScore::Results MatchScore::current(void)
{
    std::vector<PlayerInput> inputs;
    inputs.reserve(winds_all.size());

    const LocalPlayers & players = GameData::players();
    for(const auto windId : winds_all)
    {
        const Wind wind(windId);
        const auto player = std::find_if(players.begin(), players.end(),
            [&wind](const LocalPlayer & value){ return value.wind == wind; });
        if(player != players.end()) inputs.push_back(observe(*player));
    }

    return calculate(inputs);
}

std::vector<std::size_t> MatchScore::winnerIndices(const Results & results)
{
    std::vector<std::size_t> winners;
    for(std::size_t index = 0; index < results.size(); ++index)
    {
        if(results[index].finalRank == 1)
            winners.push_back(index);
    }

    return winners;
}
