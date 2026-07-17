/***************************************************************************
 *   Four Winds Reborn player save storage                                 *
 ***************************************************************************/

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#ifdef ERROR
#undef ERROR
#endif
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

#include "settings.h"
#include "recovery.h"
#include "savegames.h"
#include "swe/swe_cstring.h"
#include "swe/swe_systems.h"

using namespace SWE;

namespace
{
namespace fs = std::filesystem;

long long parseEpoch(const std::string & value)
{
    if(value.empty()) return 0;
    try
    {
        return std::stoll(value);
    }
    catch(...)
    {
        return 0;
    }
}

bool endsWithSave(const std::string & path)
{
    const std::string lower = String::toLower(path);
    return 4 <= lower.size() && lower.substr(lower.size() - 4) == ".sav";
}

bool replaceFile(const std::string & source, const std::string & destination)
{
#if defined(_WIN32)
    const fs::path sourcePath = fs::u8path(source);
    const fs::path destinationPath = fs::u8path(destination);
    return MoveFileExW(sourcePath.c_str(), destinationPath.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(source.c_str(), destination.c_str()) == 0;
#endif
}

bool copyFile(const std::string & source, const std::string & destination)
{
    std::error_code ec;
    fs::copy_file(fs::u8path(source), fs::u8path(destination),
                  fs::copy_options::overwrite_existing, ec);
    return !ec;
}

bool writeState(const std::string & destination, const JsonObject & state,
                const std::string & backupSuffix, std::string* error)
{
    const std::string temporary = destination + ".tmp";
    std::error_code ec;
    fs::remove(fs::u8path(temporary), ec);

    if(!Systems::saveString2File(state.toString(), temporary))
    {
        if(error) *error = "unable to write the temporary save";
        return false;
    }

    std::string validationError;
    if(!Recovery::validateSaveFile(temporary, &validationError))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = std::string("temporary save validation failed: ") + validationError;
        return false;
    }

    if(Systems::isFile(destination) && !backupSuffix.empty() &&
       !copyFile(destination, destination + backupSuffix))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = "unable to preserve the previous save";
        return false;
    }

    if(!replaceFile(temporary, destination))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = "unable to replace the destination save";
        return false;
    }

    if(error) error->clear();
    return true;
}

std::string newManualPath(const std::string & directory)
{
    const long long epoch = static_cast<long long>(std::time(nullptr));
    for(int suffix = 0; suffix < 1000; ++suffix)
    {
        std::string file = std::string("manual-") + std::to_string(epoch);
        if(suffix) file += std::string("-") + std::to_string(suffix);
        file += ".sav";
        const std::string path = Systems::concatePath(directory, file);
        if(!Systems::isFile(path)) return path;
    }
    return std::string();
}
}

std::string SaveGames::defaultDirectory(void)
{
    if(const char* overrideDirectory = Systems::environment("FOUR_WINDS_SAVE_DIR"))
        return overrideDirectory;
    return Systems::concatePath(Settings::shareDir(), "saves");
}

std::string SaveGames::normalizedName(const std::string & value)
{
    std::string result = value;
    while(!result.empty() && std::isspace(static_cast<unsigned char>(result.front())))
        result.erase(result.begin());
    while(!result.empty() && std::isspace(static_cast<unsigned char>(result.back())))
        result.pop_back();
    return result;
}

std::vector<SaveGames::Info> SaveGames::inspect(const std::string & directory)
{
    std::vector<Info> result;
    if(!Systems::isDirectory(directory)) return result;

    for(const std::string & path : Systems::readDir(directory, true))
    {
        if(!Systems::isFile(path) || !endsWithSave(path)) continue;

        Info info;
        info.path = path;

        std::string contents;
        std::string validationError;
        info.valid = Recovery::validateSaveFile(path, &validationError, &contents);
        info.error = validationError;
        if(contents.empty()) Systems::readFile2String(path, contents);

        const JsonObject state = JsonContentString(contents).toObject();
        info.name = normalizedName(state.getString("save:name"));
        if(info.name.empty()) info.name = Systems::basename(path);
        info.savedAtEpoch = parseEpoch(state.getString("save:savedAtEpoch"));
        info.gamePart = state.getInteger("gamepart");
        info.difficulty = state.getString("ai:difficulty");
        result.push_back(std::move(info));
    }

    std::stable_sort(result.begin(), result.end(), [](const Info & left, const Info & right)
    {
        if(left.valid != right.valid) return left.valid;
        if(left.savedAtEpoch != right.savedAtEpoch) return left.savedAtEpoch > right.savedAtEpoch;
        return String::toLower(left.name) < String::toLower(right.name);
    });
    return result;
}

std::vector<SaveGames::Info> SaveGames::inspect(void)
{
    return inspect(defaultDirectory());
}

std::string SaveGames::existingPath(const std::string & directory, const std::string & value)
{
    const std::string name = String::toLower(normalizedName(value));
    if(name.empty()) return std::string();

    for(const Info & info : inspect(directory))
        if(String::toLower(normalizedName(info.name)) == name)
            return info.path;
    return std::string();
}

std::string SaveGames::existingPath(const std::string & value)
{
    return existingPath(defaultDirectory(), value);
}

bool SaveGames::hasManualSaves(void)
{
    return !inspect().empty();
}

bool SaveGames::writeAutosave(const std::string & destination, const JsonObject & source,
                              std::string* error)
{
    JsonObject state = source;
    state.addString("save:kind", "autosave");
    state.addString("save:name", "Autosave");
    state.addString("save:savedAtEpoch", std::to_string(static_cast<long long>(std::time(nullptr))));
    return writeState(destination, state, ".previous", error);
}

bool SaveGames::writeManual(const std::string & directory, const JsonObject & source,
                            const std::string & value, bool overwrite,
                            std::string* savedFile, std::string* error)
{
    const std::string name = normalizedName(value);
    if(name.empty())
    {
        if(error) *error = "save name is empty";
        return false;
    }

    if(!Systems::isDirectory(directory) && !Systems::makeDirectory(directory))
    {
        if(error) *error = "unable to create the save directory";
        return false;
    }

    const std::string existing = existingPath(directory, name);
    if(!existing.empty() && !overwrite)
    {
        if(error) *error = "a save with this name already exists";
        return false;
    }

    const std::string destination = existing.empty() ? newManualPath(directory) : existing;
    if(destination.empty())
    {
        if(error) *error = "unable to allocate a save filename";
        return false;
    }

    JsonObject state = source;
    state.addString("save:kind", "manual");
    state.addString("save:name", name);
    state.addString("save:savedAtEpoch", std::to_string(static_cast<long long>(std::time(nullptr))));

    if(!writeState(destination, state, existing.empty() ? std::string() : ".before-overwrite", error))
        return false;

    if(savedFile) *savedFile = destination;
    if(error) error->clear();
    return true;
}

bool SaveGames::writeManual(const JsonObject & state, const std::string & name,
                            bool overwrite, std::string* savedFile, std::string* error)
{
    return writeManual(defaultDirectory(), state, name, overwrite, savedFile, error);
}

bool SaveGames::deleteManual(const std::string & directory, const std::string & file,
                             std::string* error)
{
    const fs::path directoryPath = fs::absolute(fs::u8path(directory)).lexically_normal();
    const fs::path savePath = fs::absolute(fs::u8path(file)).lexically_normal();
    if(savePath.parent_path() != directoryPath || !endsWithSave(file))
    {
        if(error) *error = "the selected file is outside the manual save directory";
        return false;
    }

    if(!Systems::isFile(file))
    {
        if(error) *error = "the selected save no longer exists";
        return false;
    }

    std::error_code ec;
    if(!fs::remove(savePath, ec) || ec)
    {
        if(error) *error = "unable to remove the manual save";
        return false;
    }

    // Remove the authoritative save first. If Windows has it locked, companion
    // files remain untouched and the user can retry safely.
    fs::path screenshotPath = savePath;
    screenshotPath.replace_extension(".png");
    const fs::path overwriteBackup = fs::u8path(file + ".before-overwrite");
    for(const fs::path & companion : { screenshotPath, overwriteBackup })
    {
        ec.clear();
        if(fs::exists(companion, ec))
        {
            ec.clear();
            fs::remove(companion, ec);
        }
    }

    if(error) error->clear();
    return true;
}

bool SaveGames::deleteManual(const std::string & file, std::string* error)
{
    return deleteManual(defaultDirectory(), file, error);
}
