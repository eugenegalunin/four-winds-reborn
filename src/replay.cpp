#include "replay.h"

#include <algorithm>
#include <exception>

#include "aiprofile.h"
#include "gamedata.h"
#include "recovery.h"

namespace
{
constexpr std::size_t MaximumJournalSteps = 64;

JsonObject journalInitialState;
std::vector<Replay::Step> journalSteps;
std::string journalTailHash;
bool recordingEnabled = true;
std::size_t journalStepLimit = MaximumJournalSteps;

bool isAdventureAction(int type)
{
    return type == Action::ClientUnitMoved || type == Action::ClientLandClaim ||
           type == Action::ClientAdventureUndo || type == Action::ClientBattleReady ||
           type == Action::ClientBattleChoice;
}

struct RecordingPause
{
    const bool previous;

    RecordingPause() : previous(recordingEnabled) { recordingEnabled = false; }
    ~RecordingPause() { recordingEnabled = previous; }
};

struct BehaviorProfileReplayScope
{
    const bool previousEnabled;
    const AI::BehaviorProfile previousProfile;

    BehaviorProfileReplayScope(bool force, AI::BehaviorProfile profile) :
        previousEnabled(AI::behaviorProfileOverrideEnabled()),
        previousProfile(AI::behaviorProfileOverride())
    {
        if(force) AI::setBehaviorProfileOverride(profile);
        else AI::clearBehaviorProfileOverride();
    }

    ~BehaviorProfileReplayScope()
    {
        if(previousEnabled) AI::setBehaviorProfileOverride(previousProfile);
        else AI::clearBehaviorProfileOverride();
    }
};
}

JsonObject Replay::Step::toJsonObject(void) const
{
    JsonObject result;
    if(isSystem())
    {
        result.addString("systemOperation", systemOperation);
        result.addBoolean("expectedAccepted", expectedAccepted);
        if(!expectedException.empty())
            result.addString("expectedException", expectedException);
    }
    else
    {
        result.addString("avatar", avatar.toString());
        result.addObject("action", action);
    }
    result.addString("expectedStateHash", expectedStateHash);
    return result;
}

std::unique_ptr<ClientMessage> Replay::clientMessageFromJson(const JsonObject & action)
{
    switch(action.getInteger("type"))
    {
        case Action::ClientReady: return std::make_unique<ClientReady>();
        case Action::ClientButtonGame: return std::make_unique<ClientButtonGame>();
        case Action::ClientButtonPass: return std::make_unique<ClientButtonPass>();
        case Action::ClientChaoVariant:
            return std::make_unique<ClientChaoVariant>(action.getInteger("variant"));
        case Action::ClientButtonPung: return std::make_unique<ClientButtonPung>();
        case Action::ClientButtonKong1: return std::make_unique<ClientButtonKong1>();
        case Action::ClientButtonKong2: return std::make_unique<ClientButtonKong2>();
        case Action::ClientDropIndex:
            return std::make_unique<ClientDropIndex>(action.getInteger("dropIndex"));
        case Action::ClientSayGame: return std::make_unique<ClientSayGame>();
        case Action::ClientSayChao: return std::make_unique<ClientSayChao>();
        case Action::ClientSayPung: return std::make_unique<ClientSayPung>();
        case Action::ClientSayKong:
            return std::make_unique<ClientSayKong>(action.getInteger("kongType"));
        case Action::ClientSummonCreature:
            return std::make_unique<ClientSummonCreature>(Creature(action.getString("creature")),
                Land(action.getString("land")), action.getBoolean("force"));
        case Action::ClientCastSpell:
        {
            const Spell spell(action.getString("spell"));
            if(action.hasKey("target"))
                return std::make_unique<ClientCastSpell>(spell, Avatar(action.getString("target")));
            if(action.hasKey("land") || action.hasKey("unit") || action.hasKey("force"))
                return std::make_unique<ClientCastSpell>(spell, Land(action.getString("land")),
                    action.getInteger("unit"), action.getBoolean("force"));
            return std::make_unique<ClientCastSpell>(spell);
        }
        case Action::ClientUnitMoved:
            return std::make_unique<ClientUnitMoved>(action.getInteger("unit"),
                Land(action.getString("land")));
        case Action::ClientLandClaim:
            return std::make_unique<ClientLandClaim>(Land(action.getString("land")));
        case Action::ClientAdventureUndo: return std::make_unique<ClientAdventureUndo>();
        case Action::ClientBattleReady: return std::make_unique<ClientBattleReady>();
        case Action::ClientBattleChoice:
            return std::make_unique<ClientBattleChoice>(action.getInteger("actor", -1),
                action.getInteger("target", -1), action.getBoolean("autoResolve"));
        case Action::ClientLuckChoice:
            return std::make_unique<ClientLuckChoice>(action.getInteger("index", -1));
        default: return nullptr;
    }
}

std::string Replay::authoritativeStateHash(void)
{
    const JsonObject state = GameData::authoritativeState();
    return Recovery::stateHash(state);
}

bool Replay::actionRecordingEnabled(void)
{
    return recordingEnabled;
}

std::size_t Replay::actionJournalLimit(void)
{
    return journalStepLimit;
}

void Replay::setActionJournalLimit(std::size_t maximumSteps)
{
    journalStepLimit = maximumSteps;
}

void Replay::clearActionJournal(void)
{
    journalInitialState.clear();
    journalSteps.clear();
    journalTailHash.clear();
}

std::size_t Replay::actionJournalSize(void)
{
    return journalSteps.size();
}

void Replay::recordAcceptedAction(const JsonObject & beforeState, const Avatar & actor,
                                  const ClientMessage & action, const JsonObject & afterState)
{
    if(!recordingEnabled) return;

    const std::string beforeHash = Recovery::stateHash(beforeState);
    if(!journalInitialState.isValid() || journalTailHash != beforeHash ||
       (journalStepLimit && journalStepLimit <= journalSteps.size()))
    {
        journalInitialState = beforeState;
        journalSteps.clear();
    }

    journalTailHash = Recovery::stateHash(afterState);
    journalSteps.emplace_back(actor, JsonObject(action), journalTailHash);
}

void Replay::recordSystemTransition(const JsonObject & beforeState,
                                    const std::string & operation,
                                    const JsonObject & afterState)
{
    if(!recordingEnabled || operation.empty()) return;

    const std::string beforeHash = Recovery::stateHash(beforeState);
    if(!journalInitialState.isValid() || journalTailHash != beforeHash ||
       (journalStepLimit && journalStepLimit <= journalSteps.size()))
    {
        journalInitialState = beforeState;
        journalSteps.clear();
    }

    journalTailHash = Recovery::stateHash(afterState);
    journalSteps.emplace_back(operation, journalTailHash);
}

bool Replay::recordSystemOperation(const std::string & operation,
                                   const std::function<bool(void)> & apply)
{
    if(!recordingEnabled || operation.empty()) return apply();

    const JsonObject beforeState = GameData::authoritativeState();
    bool accepted = false;
    std::string exceptionText;
    std::exception_ptr exception;
    try
    {
        RecordingPause pause;
        accepted = apply();
    }
    catch(const std::exception & failure)
    {
        exceptionText = failure.what();
        exception = std::current_exception();
    }
    catch(...)
    {
        exceptionText = "unknown exception";
        exception = std::current_exception();
    }
    const JsonObject afterState = GameData::authoritativeState();
    const std::string beforeHash = Recovery::stateHash(beforeState);
    if(!journalInitialState.isValid() || journalTailHash != beforeHash ||
       (journalStepLimit && journalStepLimit <= journalSteps.size()))
    {
        journalInitialState = beforeState;
        journalSteps.clear();
    }
    journalTailHash = Recovery::stateHash(afterState);
    journalSteps.emplace_back(operation, journalTailHash, accepted, exceptionText);
    if(exception) std::rethrow_exception(exception);
    return accepted;
}

JsonObject Replay::actionJournal(const JsonObject & checkpointState)
{
    JsonObject result;
    const bool hasSystemOperations = std::any_of(journalSteps.begin(), journalSteps.end(),
        [](const Step & step){ return step.isSystem(); });
    const bool forcedProfile = AI::behaviorProfileOverrideEnabled();
    result.addInteger("schema", forcedProfile ? 3 : (hasSystemOperations ? 2 : 1));
    result.addString("aiBehaviorProfile", forcedProfile ?
        AI::behaviorProfileName(AI::behaviorProfileOverride()) : "native");
    result.addInteger("actionCount", static_cast<int>(journalSteps.size()));

    const std::string checkpointHash = Recovery::stateHash(checkpointState);
    result.addString("checkpointStateHash", checkpointHash);
    result.addString("tailStateHash", journalTailHash);
    result.addBoolean("contiguousToCheckpoint",
                      !journalSteps.empty() && journalTailHash == checkpointHash);

    if(journalInitialState.isValid()) result.addObject("initialState", journalInitialState);
    JsonArray steps;
    for(const Step & step : journalSteps) steps.addObject(step.toJsonObject());
    result.addArray("steps", steps);
    return result;
}

bool Replay::run(const JsonObject & initialState, const std::vector<Step> & steps,
                 std::string* error)
{
    RecordingPause pause;
    if(!GameData::restoreState(initialState))
    {
        if(error) *error = "initial state rejected";
        return false;
    }

    for(std::size_t index = 0; index < steps.size(); ++index)
    {
        const Step & step = steps[index];
        bool accepted = false;
        if(step.isSystem())
        {
            std::string actualException;
            try
            {
                if(step.systemOperation == "init_mahjong")
                    accepted = GameData::initMahjong();
                else if(step.systemOperation == "init_adventure")
                    accepted = GameData::initAdventure();
                else if(step.systemOperation == "game_summary")
                {
                    GameData::setGamePart(Menu::GameSummaryPart);
                    accepted = true;
                }
                else if(step.systemOperation == "mahjong_tick")
                {
                    ActionList emitted;
                    accepted = GameData::mahjong2Client(
                        GameData::currentPerson().avatar, emitted);
                }
                else if(step.systemOperation == "adventure_tick")
                {
                    ActionList emitted;
                    accepted = GameData::adventure2Client(
                        GameData::currentPerson().avatar, emitted);
                }
                else
                {
                    if(error) *error = "unknown system operation at step " +
                        std::to_string(index);
                    return false;
                }
            }
            catch(const std::exception & failure)
            {
                actualException = failure.what();
            }
            catch(...)
            {
                actualException = "unknown exception";
            }

            if(actualException != step.expectedException)
            {
                if(error) *error = "system operation exception mismatch at step " +
                    std::to_string(index) + ": expected=" + step.expectedException +
                    " actual=" + actualException;
                return false;
            }
        }
        else
        {
            std::unique_ptr<ClientMessage> action = clientMessageFromJson(step.action);
            if(!action)
            {
                if(error) *error = "unknown action at step " + std::to_string(index);
                return false;
            }

            ActionList emitted;
            accepted = isAdventureAction(action->type()) ?
                GameData::client2Adventure(step.avatar, *action, emitted) :
                GameData::client2Mahjong(step.avatar, *action, emitted);
        }
        if(step.isSystem() && accepted != step.expectedAccepted)
        {
            if(error) *error = "system operation outcome mismatch at step " +
                std::to_string(index);
            return false;
        }
        if(!step.isSystem() && !accepted)
        {
            if(error) *error = "action rejected at step " + std::to_string(index);
            return false;
        }

        const std::string actualHash = authoritativeStateHash();
        if(actualHash != step.expectedStateHash)
        {
            if(error) *error = "state hash mismatch at step " + std::to_string(index) +
                ": expected=" + step.expectedStateHash + " actual=" + actualHash;
            return false;
        }
    }

    return true;
}

bool Replay::run(const JsonObject & journal, std::string* error)
{
    const JsonObject* initialState = journal.getObject("initialState");
    const JsonArray* encodedSteps = journal.getArray("steps");
    if(!initialState || !encodedSteps)
    {
        if(error) *error = "journal is missing initialState or steps";
        return false;
    }

    const std::string selectedProfile = journal.getString("aiBehaviorProfile", "native");
    AI::BehaviorProfile profile = AI::BehaviorProfile::Balanced;
    const bool forceProfile = selectedProfile != "native";
    if(forceProfile && !AI::behaviorProfileFromString(selectedProfile, profile))
    {
        if(error) *error = "journal has an invalid AI behavior profile: " + selectedProfile;
        return false;
    }
    BehaviorProfileReplayScope profileScope(forceProfile, profile);

    std::vector<Step> steps;
    steps.reserve(encodedSteps->size());
    for(std::size_t index = 0; index < encodedSteps->size(); ++index)
    {
        const JsonObject* encoded = encodedSteps->getObject(index);
        const JsonObject* action = encoded ? encoded->getObject("action") : nullptr;
        const std::string operation = encoded ?
            encoded->getString("systemOperation") : std::string();
        const bool expectedAccepted = encoded ?
            encoded->getBoolean("expectedAccepted", true) : true;
        const std::string expectedException = encoded ?
            encoded->getString("expectedException") : std::string();
        const Avatar actor(encoded ? encoded->getString("avatar") : std::string());
        const std::string expected = encoded ? encoded->getString("expectedStateHash") : std::string();
        if(!encoded || expected.empty() ||
           (operation.empty() && (!action || !actor.isValid())) ||
           (!operation.empty() && action))
        {
            if(error) *error = "invalid journal step " + std::to_string(index);
            return false;
        }
        if(operation.empty()) steps.emplace_back(actor, *action, expected);
        else steps.emplace_back(operation, expected, expectedAccepted, expectedException);
    }

    return run(*initialState, steps, error);
}
