#ifndef FOUR_WINDS_REPLAY_H
#define FOUR_WINDS_REPLAY_H

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "actions.h"

namespace Replay
{
    struct JournalInfo
    {
        int         schema = 0;
        int         actionCount = 0;
        int         gamePart = 0;
        long long   startedAtEpoch = 0;
        long long   savedAtEpoch = 0;
        std::string difficulty;
        std::string rulesetId;
        int         rulesetVersion = 0;
        std::string contentPackageId;
        int         contentPackageVersion = 0;
        bool        contiguousToCheckpoint = false;
        bool        developerAssisted = false;
    };

    struct Step
    {
        Avatar avatar;
        JsonObject action;
        std::string systemOperation;
        bool expectedAccepted = true;
        std::string expectedException;
        std::string expectedStateHash;

        Step(const Avatar & actor, const ClientMessage & message, const std::string & hash)
            : avatar(actor), action(message), expectedStateHash(hash) {}

        Step(const Avatar & actor, const JsonObject & message, const std::string & hash)
            : avatar(actor), action(message), expectedStateHash(hash) {}

        Step(const std::string & operation, const std::string & hash, bool accepted = true,
             const std::string & exception = std::string())
            : systemOperation(operation), expectedAccepted(accepted),
              expectedException(exception), expectedStateHash(hash) {}

        bool isSystem(void) const { return !systemOperation.empty(); }

        JsonObject toJsonObject(void) const;
    };

    std::unique_ptr<ClientMessage> clientMessageFromJson(const JsonObject & action);
    std::string authoritativeStateHash(void);
    bool actionRecordingEnabled(void);
    std::size_t actionJournalLimit(void);
    void setActionJournalLimit(std::size_t maximumSteps);
    void clearActionJournal(void);
    std::size_t actionJournalSize(void);
    void recordAcceptedAction(const JsonObject & beforeState, const Avatar & actor,
                              const ClientMessage & action, const JsonObject & afterState);
    void recordSystemTransition(const JsonObject & beforeState, const std::string & operation,
                                const JsonObject & afterState);
    bool recordSystemOperation(const std::string & operation,
                               const std::function<bool(void)> & apply);
    JsonObject actionJournal(const JsonObject & checkpointState);
    bool inspectJournal(const JsonObject & journal, JournalInfo & info,
                        std::string* error = nullptr);
    bool run(const JsonObject & initialState, const std::vector<Step> & steps,
             std::string* error = nullptr);
    bool run(const JsonObject & journal, std::string* error = nullptr);
}

#endif
