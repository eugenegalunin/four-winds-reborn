/***************************************************************************
 *   Four Winds Reborn player replay storage                              *
 ***************************************************************************/

#ifndef _FWR_REPLAYFILES_
#define _FWR_REPLAYFILES_

#include <string>
#include <vector>

#include "replay.h"

namespace ReplayFiles
{
    struct Info
    {
        std::string         path;
        std::string         error;
        Replay::JournalInfo journal;
        bool                valid = false;
    };

    std::string       defaultDirectory(void);
    std::vector<Info> inspect(const std::string & directory);
    std::vector<Info> inspect(void);
    bool              hasReplays(void);
    bool              writeAutomatic(const std::string & directory,
                                     const SWE::JsonObject & journal,
                                     std::string* savedFile = nullptr,
                                     std::string* error = nullptr);
    bool              writeAutomatic(const SWE::JsonObject & journal,
                                     std::string* savedFile = nullptr,
                                     std::string* error = nullptr);
    bool              deleteReplay(const std::string & directory,
                                   const std::string & file,
                                   std::string* error = nullptr);
    bool              deleteReplay(const std::string & file,
                                   std::string* error = nullptr);
    bool              importReplay(const std::string & directory,
                                   const std::string & source,
                                   std::string* importedFile = nullptr,
                                   std::string* error = nullptr);
    bool              importReplay(const std::string & source,
                                   std::string* importedFile = nullptr,
                                   std::string* error = nullptr);
    bool              exportReplay(const std::string & directory,
                                   const std::string & file,
                                   const std::string & destination,
                                   bool overwrite,
                                   std::string* exportedFile = nullptr,
                                   std::string* error = nullptr);
    bool              exportReplay(const std::string & file,
                                   const std::string & destination,
                                   bool overwrite,
                                   std::string* exportedFile = nullptr,
                                   std::string* error = nullptr);
}

#endif
