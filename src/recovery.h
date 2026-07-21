#ifndef FOUR_WINDS_RECOVERY_H
#define FOUR_WINDS_RECOVERY_H

#include <string>
#include <vector>

#include "swe/swe_json.h"

namespace Recovery
{
    constexpr int SlotCount = 3;

    struct CheckpointInfo
    {
        int         slot = -1;
        bool        valid = false;
        std::string saveFile;
        std::string metadataFile;
        std::string error;
        long long   savedAtEpoch = 0;
        std::string reason;
        std::string gameVersion;
        std::string gameRevision;
        int         gamePart = 0;
        std::string roundWind;
        std::string partWind;
        std::string currentWind;
        std::string aiDifficulty;
        std::string runeGameRulesetId;
        int         runeGameRulesetVersion = 0;
        int         stateBytes = 0;
    };

    bool enabled(void);
    void setEnabled(bool);
    std::string defaultDirectory(void);
    std::string stateHash(const std::string & saveData);
    std::string stateHash(const SWE::JsonValue & state);
    std::string savePath(const std::string & directory, int slot);
    std::string metadataPath(const std::string & directory, int slot);
    bool writeCheckpoint(const std::string & directory, const std::string & saveData,
                         const SWE::JsonObject & metadata);
    bool validateSaveState(const SWE::JsonObject & state, std::string* error = nullptr);
    bool validateSaveFile(const std::string & path, std::string* error = nullptr,
                          std::string* saveData = nullptr);
    CheckpointInfo inspectCheckpoint(const std::string & directory, int slot);
    std::vector<CheckpointInfo> inspectCheckpoints(const std::string & directory);
    bool promoteSave(const std::string & source, const std::string & destination,
                     std::string* error = nullptr);
}

#endif
