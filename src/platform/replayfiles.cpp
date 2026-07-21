/***************************************************************************
 *   Four Winds Reborn player replay storage                              *
 ***************************************************************************/

#include "replayfiles.h"

#include <algorithm>
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
#include "swe/swe_cstring.h"
#include "swe/swe_systems.h"

using namespace SWE;

namespace
{
namespace fs = std::filesystem;

constexpr const char* ReplayExtension = ".fwr";

bool endsWithReplay(const std::string & path)
{
    const std::string lower = String::toLower(path);
    const std::string extension = ReplayExtension;
    return extension.size() <= lower.size() &&
           lower.substr(lower.size() - extension.size()) == extension;
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

bool isDirectReplay(const std::string & directory, const std::string & file)
{
    std::error_code ec;
    const fs::path directoryPath = fs::weakly_canonical(fs::u8path(directory), ec);
    if(ec) return false;
    const fs::path replayPath = fs::weakly_canonical(fs::u8path(file), ec);
    return !ec && replayPath.parent_path() == directoryPath && endsWithReplay(file);
}

bool readValidatedReplay(const std::string & source, std::string & contents,
                         Replay::JournalInfo & info, std::string* error)
{
    if(!Systems::isFile(source) || !endsWithReplay(source))
    {
        if(error) *error = "the selected file is not a .fwr replay";
        return false;
    }
    if(!Systems::readFile2String(source, contents))
    {
        if(error) *error = "unable to read the replay file";
        return false;
    }

    const JsonObject journal = JsonContentString(contents).toObject();
    std::string validationError;
    if(!journal.isValid() || !Replay::inspectJournal(journal, info, &validationError))
    {
        if(error) *error = journal.isValid() ? validationError :
            "replay file is not valid JSON";
        return false;
    }
    if(info.actionCount <= 0 || !info.contiguousToCheckpoint)
    {
        if(error) *error = "replay journal is empty or not contiguous";
        return false;
    }
    return true;
}

std::string sanitizedReplayName(const std::string & source)
{
    std::string stem = fs::u8path(source).stem().u8string();
    for(char & symbol : stem)
    {
        const unsigned char value = static_cast<unsigned char>(symbol);
        if(value < 0x20 || symbol == '<' || symbol == '>' || symbol == ':' ||
           symbol == '"' || symbol == '/' || symbol == '\\' || symbol == '|' ||
           symbol == '?' || symbol == '*')
            symbol = '-';
    }
    while(!stem.empty() && (stem.back() == ' ' || stem.back() == '.')) stem.pop_back();
    if(stem.empty()) stem = "imported-replay";
    return stem + ReplayExtension;
}

std::string uniqueReplayPath(const std::string & directory, const std::string & source)
{
    const fs::path name = fs::u8path(sanitizedReplayName(source));
    fs::path candidate = fs::u8path(directory) / name;
    for(int suffix = 2; fs::exists(candidate); ++suffix)
        candidate = fs::u8path(directory) /
            fs::u8path(name.stem().u8string() + "-" + std::to_string(suffix) + ReplayExtension);
    return candidate.u8string();
}

bool writeValidatedCopy(const std::string & destination, const std::string & contents,
                        bool overwrite, std::string* error)
{
    const fs::path destinationPath = fs::u8path(destination);
    const fs::path parent = destinationPath.parent_path();
    if(parent.empty())
    {
        if(error) *error = "the destination directory is missing";
        return false;
    }
    if(!Systems::isDirectory(parent.u8string()) &&
       !Systems::makeDirectory(parent.u8string()))
    {
        if(error) *error = "unable to create the destination directory";
        return false;
    }
    if(fs::exists(destinationPath) && !overwrite)
    {
        if(error) *error = "the destination replay already exists";
        return false;
    }

    const std::string temporary = destination + ".tmp";
    std::error_code ec;
    fs::remove(fs::u8path(temporary), ec);
    if(!Systems::saveString2File(contents, temporary))
    {
        if(error) *error = "unable to write the temporary replay";
        return false;
    }

    std::string writtenContents;
    Replay::JournalInfo writtenInfo;
    std::string validationError;
    const bool bytesMatch = Systems::readFile2String(temporary, writtenContents) &&
                            writtenContents == contents;
    const JsonObject written = bytesMatch ?
        JsonContentString(writtenContents).toObject() : JsonObject();
    if(!bytesMatch || !written.isValid() ||
       !Replay::inspectJournal(written, writtenInfo, &validationError) ||
       writtenInfo.actionCount <= 0 || !writtenInfo.contiguousToCheckpoint)
    {
        fs::remove(fs::u8path(temporary), ec);
        if(validationError.empty())
            validationError = bytesMatch ? "replay file is not valid JSON" :
                              "written bytes do not match the source replay";
        if(error) *error = std::string("temporary replay validation failed: ") +
                           validationError;
        return false;
    }

    if(!replaceFile(temporary, destination))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = "unable to replace the replay file";
        return false;
    }
    return true;
}

std::string automaticPath(const std::string & directory,
                          const Replay::JournalInfo & info)
{
    long long epoch = info.startedAtEpoch;
    if(epoch <= 0) epoch = info.savedAtEpoch;
    if(epoch <= 0) epoch = static_cast<long long>(std::time(nullptr));
    return Systems::concatePath(directory,
        std::string("session-") + std::to_string(epoch) + ReplayExtension);
}
}

std::string ReplayFiles::defaultDirectory(void)
{
    if(const char* overrideDirectory = Systems::environment("FOUR_WINDS_REPLAY_DIR"))
        return overrideDirectory;
    if(const char* saveOverride = Systems::environment("FOUR_WINDS_SAVE_DIR"))
        return Systems::concatePath(saveOverride, "replays");
    return Systems::concatePath(Settings::shareDir(), "replays");
}

std::vector<ReplayFiles::Info> ReplayFiles::inspect(const std::string & directory)
{
    std::vector<Info> result;
    if(!Systems::isDirectory(directory)) return result;

    for(const std::string & path : Systems::readDir(directory, true))
    {
        if(!Systems::isFile(path) || !endsWithReplay(path)) continue;

        Info info;
        info.path = path;
        const JsonObject journal = JsonContentFile(path).toObject();
        info.valid = journal.isValid() &&
                     Replay::inspectJournal(journal, info.journal, &info.error);
        if(!journal.isValid()) info.error = "replay file is not valid JSON";
        result.push_back(std::move(info));
    }

    std::stable_sort(result.begin(), result.end(), [](const Info & left, const Info & right)
    {
        if(left.valid != right.valid) return left.valid;
        if(left.journal.savedAtEpoch != right.journal.savedAtEpoch)
            return left.journal.savedAtEpoch > right.journal.savedAtEpoch;
        return String::toLower(left.path) < String::toLower(right.path);
    });
    return result;
}

std::vector<ReplayFiles::Info> ReplayFiles::inspect(void)
{
    return inspect(defaultDirectory());
}

bool ReplayFiles::hasReplays(void)
{
    return !inspect().empty();
}

bool ReplayFiles::writeAutomatic(const std::string & directory,
                                 const JsonObject & journal,
                                 std::string* savedFile, std::string* error)
{
    Replay::JournalInfo info;
    std::string validationError;
    if(!Replay::inspectJournal(journal, info, &validationError))
    {
        if(error) *error = validationError;
        return false;
    }

    if(info.actionCount <= 0 || !info.contiguousToCheckpoint)
    {
        if(error) *error = "replay journal is empty or not contiguous";
        return false;
    }

    if(!Systems::isDirectory(directory) && !Systems::makeDirectory(directory))
    {
        if(error) *error = "unable to create the replay directory";
        return false;
    }

    const std::string destination = automaticPath(directory, info);
    const std::string temporary = destination + ".tmp";
    std::error_code ec;
    fs::remove(fs::u8path(temporary), ec);
    if(!Systems::saveString2File(journal.toString(), temporary))
    {
        if(error) *error = "unable to write the temporary replay";
        return false;
    }

    const JsonObject written = JsonContentFile(temporary).toObject();
    Replay::JournalInfo writtenInfo;
    if(!written.isValid() || !Replay::inspectJournal(written, writtenInfo, &validationError))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = std::string("temporary replay validation failed: ") +
                           validationError;
        return false;
    }

    if(!replaceFile(temporary, destination))
    {
        fs::remove(fs::u8path(temporary), ec);
        if(error) *error = "unable to replace the replay file";
        return false;
    }

    if(savedFile) *savedFile = destination;
    if(error) error->clear();
    return true;
}

bool ReplayFiles::writeAutomatic(const JsonObject & journal,
                                 std::string* savedFile, std::string* error)
{
    return writeAutomatic(defaultDirectory(), journal, savedFile, error);
}

bool ReplayFiles::deleteReplay(const std::string & directory, const std::string & file,
                               std::string* error)
{
    if(!isDirectReplay(directory, file))
    {
        if(error) *error = "the selected file is outside the replay directory";
        return false;
    }

    const fs::path replayPath = fs::u8path(file);
    std::error_code ec;
    if(!fs::remove(replayPath, ec) || ec)
    {
        if(error) *error = "unable to remove the replay";
        return false;
    }

    if(error) error->clear();
    return true;
}

bool ReplayFiles::deleteReplay(const std::string & file, std::string* error)
{
    return deleteReplay(defaultDirectory(), file, error);
}

bool ReplayFiles::importReplay(const std::string & directory, const std::string & source,
                               std::string* importedFile, std::string* error)
{
    std::string contents;
    Replay::JournalInfo info;
    if(!readValidatedReplay(source, contents, info, error)) return false;

    if(isDirectReplay(directory, source))
    {
        if(error) *error = "the selected replay is already in the library";
        return false;
    }

    if(!Systems::isDirectory(directory) && !Systems::makeDirectory(directory))
    {
        if(error) *error = "unable to create the replay directory";
        return false;
    }

    const std::string destination = uniqueReplayPath(directory, source);
    if(!writeValidatedCopy(destination, contents, false, error)) return false;
    if(importedFile) *importedFile = destination;
    if(error) error->clear();
    return true;
}

bool ReplayFiles::importReplay(const std::string & source, std::string* importedFile,
                               std::string* error)
{
    return importReplay(defaultDirectory(), source, importedFile, error);
}

bool ReplayFiles::exportReplay(const std::string & directory, const std::string & file,
                               const std::string & destination, bool overwrite,
                               std::string* exportedFile, std::string* error)
{
    if(!isDirectReplay(directory, file))
    {
        if(error) *error = "the selected file is outside the replay directory";
        return false;
    }

    std::string contents;
    Replay::JournalInfo info;
    if(!readValidatedReplay(file, contents, info, error)) return false;

    std::string actualDestination = destination;
    if(!endsWithReplay(actualDestination)) actualDestination += ReplayExtension;
    std::error_code ec;
    const fs::path sourcePath = fs::weakly_canonical(fs::u8path(file), ec);
    const fs::path destinationPath = fs::absolute(fs::u8path(actualDestination)).lexically_normal();
    if(!ec && sourcePath == destinationPath)
    {
        if(error) *error = "the export destination is the library replay itself";
        return false;
    }

    if(!writeValidatedCopy(actualDestination, contents, overwrite, error)) return false;
    if(exportedFile) *exportedFile = actualDestination;
    if(error) error->clear();
    return true;
}

bool ReplayFiles::exportReplay(const std::string & file,
                               const std::string & destination, bool overwrite,
                               std::string* exportedFile, std::string* error)
{
    return exportReplay(defaultDirectory(), file, destination, overwrite,
                        exportedFile, error);
}
