#ifndef FOUR_WINDS_RECOVERY_H
#define FOUR_WINDS_RECOVERY_H

#include <string>

#include "swe/swe_json.h"

namespace Recovery
{
    constexpr int SlotCount = 3;

    std::string defaultDirectory(void);
    std::string stateHash(const std::string & saveData);
    std::string stateHash(const SWE::JsonValue & state);
    std::string savePath(const std::string & directory, int slot);
    std::string metadataPath(const std::string & directory, int slot);
    bool writeCheckpoint(const std::string & directory, const std::string & saveData,
                         const SWE::JsonObject & metadata);
}

#endif
