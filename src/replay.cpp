#include "replay.h"

#include "gamedata.h"
#include "recovery.h"

namespace
{
constexpr std::size_t MaximumJournalSteps = 64;

JsonObject journalInitialState;
std::vector<Replay::Step> journalSteps;
std::string journalTailHash;
bool recordingEnabled = true;

bool isAdventureAction(int type)
{
    return type == Action::ClientUnitMoved || type == Action::ClientLandClaim ||
           type == Action::ClientAdventureUndo || type == Action::ClientBattleReady;
}

struct RecordingPause
{
    const bool previous;

    RecordingPause() : previous(recordingEnabled) { recordingEnabled = false; }
    ~RecordingPause() { recordingEnabled = previous; }
};
}

JsonObject Replay::Step::toJsonObject(void) const
{
    JsonObject result;
    result.addString("avatar", avatar.toString());
    result.addObject("action", action);
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
       MaximumJournalSteps <= journalSteps.size())
    {
        journalInitialState = beforeState;
        journalSteps.clear();
    }

    journalTailHash = Recovery::stateHash(afterState);
    journalSteps.emplace_back(actor, JsonObject(action), journalTailHash);
}

JsonObject Replay::actionJournal(const JsonObject & checkpointState)
{
    JsonObject result;
    result.addInteger("schema", 1);
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
        std::unique_ptr<ClientMessage> action = clientMessageFromJson(step.action);
        if(!action)
        {
            if(error) *error = "unknown action at step " + std::to_string(index);
            return false;
        }

        ActionList emitted;
        const bool accepted = isAdventureAction(action->type()) ?
            GameData::client2Adventure(step.avatar, *action, emitted) :
            GameData::client2Mahjong(step.avatar, *action, emitted);
        if(!accepted)
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

    std::vector<Step> steps;
    steps.reserve(encodedSteps->size());
    for(std::size_t index = 0; index < encodedSteps->size(); ++index)
    {
        const JsonObject* encoded = encodedSteps->getObject(index);
        const JsonObject* action = encoded ? encoded->getObject("action") : nullptr;
        const Avatar actor(encoded ? encoded->getString("avatar") : std::string());
        const std::string expected = encoded ? encoded->getString("expectedStateHash") : std::string();
        if(!encoded || !action || !actor.isValid() || expected.empty())
        {
            if(error) *error = "invalid journal step " + std::to_string(index);
            return false;
        }
        steps.emplace_back(actor, *action, expected);
    }

    return run(*initialState, steps, error);
}
