#include "recovery.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <iomanip>
#include <list>
#include <sstream>
#include <vector>

#include "settings.h"
#include "swe/swe_systems.h"

namespace
{
bool moveFile(const std::string & source, const std::string & destination)
{
    if(!SWE::Systems::isFile(source)) return true;
    if(SWE::Systems::isFile(destination)) std::remove(destination.c_str());
    return std::rename(source.c_str(), destination.c_str()) == 0;
}

std::string numberedPath(const std::string & directory, int slot, const char* extension)
{
    return SWE::Systems::concatePath(directory,
        std::string("recovery-") + std::to_string(slot) + extension);
}

std::string fnv1a64(const std::string & value)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for(unsigned char byte : value)
    {
        hash ^= byte;
        hash *= UINT64_C(1099511628211);
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string canonicalJson(const SWE::JsonValue & value)
{
    if(value.getType() == SWE::JsonType::Array)
    {
        const auto & array = static_cast<const SWE::JsonArray &>(value);
        std::string result = "[";
        for(std::size_t index = 0; index < array.size(); ++index)
        {
            if(index) result += ',';
            const SWE::JsonValue* child = array.getValue(index);
            result += child ? canonicalJson(*child) : "null";
        }
        return result + ']';
    }

    if(value.getType() == SWE::JsonType::Object)
    {
        const auto & object = static_cast<const SWE::JsonObject &>(value);
        const std::list<std::string> objectKeys = object.keys();
        std::vector<std::string> keys(objectKeys.begin(), objectKeys.end());
        std::sort(keys.begin(), keys.end());

        std::string result = "{";
        for(std::size_t index = 0; index < keys.size(); ++index)
        {
            if(index) result += ',';
            result += SWE::JsonString(keys[index]).toString();
            result += ':';
            const SWE::JsonValue* child = object.getValue(keys[index]);
            result += child ? canonicalJson(*child) : "null";
        }
        return result + '}';
    }

    return value.toString();
}
}

std::string Recovery::defaultDirectory(void)
{
    if(const char* overrideDirectory = SWE::Systems::environment("FOUR_WINDS_RECOVERY_DIR"))
        return overrideDirectory;

    return SWE::Systems::concatePath(Settings::shareDir(), "recovery");
}

std::string Recovery::stateHash(const std::string & saveData)
{
    return fnv1a64(saveData);
}

std::string Recovery::stateHash(const SWE::JsonValue & state)
{
    return fnv1a64(canonicalJson(state));
}

std::string Recovery::savePath(const std::string & directory, int slot)
{
    return numberedPath(directory, slot, ".sav");
}

std::string Recovery::metadataPath(const std::string & directory, int slot)
{
    return numberedPath(directory, slot, ".json");
}

bool Recovery::writeCheckpoint(const std::string & directory, const std::string & saveData,
                               const SWE::JsonObject & metadata)
{
    if(directory.empty() || saveData.empty() || !metadata.isValid()) return false;
    if(!SWE::Systems::isDirectory(directory) && !SWE::Systems::makeDirectory(directory))
        return false;

    const std::string saveTemporary = savePath(directory, 0) + ".tmp";
    const std::string metadataTemporary = metadataPath(directory, 0) + ".tmp";
    std::remove(saveTemporary.c_str());
    std::remove(metadataTemporary.c_str());

    if(!SWE::Systems::saveString2File(saveData, saveTemporary)) return false;
    if(!SWE::Systems::saveString2File(metadata.toString(), metadataTemporary))
    {
        std::remove(saveTemporary.c_str());
        return false;
    }

    for(int slot = SlotCount - 2; 0 <= slot; --slot)
    {
        if(!moveFile(savePath(directory, slot), savePath(directory, slot + 1)) ||
           !moveFile(metadataPath(directory, slot), metadataPath(directory, slot + 1)))
            return false;
    }

    if(!moveFile(saveTemporary, savePath(directory, 0))) return false;
    if(!moveFile(metadataTemporary, metadataPath(directory, 0))) return false;
    return true;
}
