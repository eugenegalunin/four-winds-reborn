#include "simulation.h"

#include <algorithm>
#include <exception>
#include <set>

#include "gameplayrng.h"
#include "replay.h"

namespace
{
bool validConfiguration(const Simulation::MatchConfig & config, std::string* error)
{
    if(config.persons.size() != winds_all.size())
    {
        if(error) *error = "a match requires exactly four players";
        return false;
    }
    if(config.maximumTicks == 0 || config.maximumUnchangedTicks == 0 ||
       config.stateHashInterval == 0)
    {
        if(error) *error = "watchdog limits must be positive";
        return false;
    }

    std::set<int> avatars;
    std::set<int> clans;
    std::set<int> winds;
    for(const Person & person : config.persons)
    {
        if(!person.isAI())
        {
            if(error) *error = "headless matches require every player to be AI";
            return false;
        }
        if(!person.avatar.isValid() || person.avatar == Avatar(Avatar::Random) ||
           !person.clan.isValid() || !person.wind.isValid())
        {
            if(error) *error = "every player needs a concrete avatar, clan and wind";
            return false;
        }
        if(!avatars.insert(person.avatar()).second || !clans.insert(person.clan()).second ||
           !winds.insert(person.wind()).second)
        {
            if(error) *error = "avatars, clans and winds must be unique";
            return false;
        }

        const AvatarInfo & avatar = GameData::avatarInfo(person.avatar);
        if(std::find(avatar.clans.begin(), avatar.clans.end(), person.clan) == avatar.clans.end())
        {
            if(error) *error = "an avatar was assigned to an unsupported clan";
            return false;
        }
    }
    return true;
}

void finishResult(Simulation::MatchResult & result)
{
    result.rngDraws = GameplayRng::draws();
    result.finalStateHash = Replay::authoritativeStateHash();
    result.replayTail = Replay::actionJournal(GameData::authoritativeState());
    result.score = MatchScore::current();
}
}

const char* Simulation::statusName(MatchStatus status)
{
    switch(status)
    {
        case MatchStatus::Complete: return "complete";
        case MatchStatus::InvalidConfiguration: return "invalid_configuration";
        case MatchStatus::InitializationFailed: return "initialization_failed";
        case MatchStatus::Stalled: return "stalled";
        case MatchStatus::TickLimit: return "tick_limit";
        case MatchStatus::Exception: return "exception";
    }
    return "unknown";
}

Simulation::MatchResult Simulation::runMatch(const MatchConfig & config)
{
    MatchResult result;
    result.seed = config.seed;

    if(!validConfiguration(config, &result.error)) return result;

    try
    {
        GameplayRng::seed(config.seed);
        GameData::setAIDifficulty(config.difficulty);
        if(!GameData::initPersons(config.persons))
        {
            result.status = MatchStatus::InitializationFailed;
            result.error = "GameData rejected the exact player configuration";
            return result;
        }
        if(!GameData::initMahjong())
        {
            result.status = MatchStatus::InitializationFailed;
            result.error = "the first Mahjong phase did not initialize";
            finishResult(result);
            return result;
        }

        std::string checkpointHash = Replay::authoritativeStateHash();
        while(result.ticks < config.maximumTicks)
        {
            ++result.ticks;
            ActionList actions;
            bool advanced = false;

            switch(GameData::loadedGamePart())
            {
                case Menu::MahjongPart:
                    advanced = GameData::mahjong2Client(GameData::currentPerson().avatar, actions);
                    break;

                case Menu::MahjongSummaryPart:
                    ++result.mahjongHands;
                    advanced = GameData::initAdventure();
                    break;

                case Menu::AdventurePart:
                    advanced = GameData::adventure2Client(GameData::currentPerson().avatar, actions);
                    break;

                case Menu::BattleSummaryPart:
                    ++result.adventurePhases;
                    if(GameData::isGameOver())
                    {
                        GameData::setGamePart(Menu::GameSummaryPart);
                        result.status = MatchStatus::Complete;
                        finishResult(result);
                        return result;
                    }
                    advanced = GameData::initMahjong();
                    break;

                default:
                    result.status = MatchStatus::Stalled;
                    result.error = "unexpected game phase " +
                        std::to_string(GameData::loadedGamePart());
                    finishResult(result);
                    return result;
            }

            result.emittedActions += actions.size();
            if(!advanced)
                ++result.unchangedTicks;
            else
                result.unchangedTicks = 0;

            if(config.maximumUnchangedTicks <= result.unchangedTicks)
            {
                result.status = MatchStatus::Stalled;
                result.error = !advanced ? "phase driver rejected progress" :
                    "authoritative state stopped changing";
                finishResult(result);
                return result;
            }

            if(result.ticks % config.stateHashInterval == 0)
            {
                const std::string currentHash = Replay::authoritativeStateHash();
                if(currentHash == checkpointHash)
                {
                    result.status = MatchStatus::Stalled;
                    result.error = "authoritative state did not change during a watchdog interval";
                    finishResult(result);
                    return result;
                }
                checkpointHash = currentHash;
            }
        }

        result.status = MatchStatus::TickLimit;
        result.error = "maximum match tick count reached";
        finishResult(result);
        return result;
    }
    catch(const std::exception & exception)
    {
        result.status = MatchStatus::Exception;
        result.error = exception.what();
    }
    catch(...)
    {
        result.status = MatchStatus::Exception;
        result.error = "unknown exception";
    }

    finishResult(result);
    return result;
}
