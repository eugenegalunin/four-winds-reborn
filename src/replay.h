#ifndef FOUR_WINDS_REPLAY_H
#define FOUR_WINDS_REPLAY_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "actions.h"

namespace Replay
{
    struct Step
    {
        Avatar avatar;
        JsonObject action;
        std::string expectedStateHash;

        Step(const Avatar & actor, const ClientMessage & message, const std::string & hash)
            : avatar(actor), action(message), expectedStateHash(hash) {}

        Step(const Avatar & actor, const JsonObject & message, const std::string & hash)
            : avatar(actor), action(message), expectedStateHash(hash) {}

        JsonObject toJsonObject(void) const;
    };

    std::unique_ptr<ClientMessage> clientMessageFromJson(const JsonObject & action);
    std::string authoritativeStateHash(void);
    bool actionRecordingEnabled(void);
    void clearActionJournal(void);
    std::size_t actionJournalSize(void);
    void recordAcceptedAction(const JsonObject & beforeState, const Avatar & actor,
                              const ClientMessage & action, const JsonObject & afterState);
    JsonObject actionJournal(const JsonObject & checkpointState);
    bool run(const JsonObject & initialState, const std::vector<Step> & steps,
             std::string* error = nullptr);
    bool run(const JsonObject & journal, std::string* error = nullptr);
}

#endif
