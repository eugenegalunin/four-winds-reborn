/***************************************************************************
 *   Four Winds Reborn player save storage                                 *
 ***************************************************************************/

#ifndef _FWR_SAVEGAMES_
#define _FWR_SAVEGAMES_

#include <string>
#include <vector>

#include "swe/swe_json.h"

namespace SaveGames
{
    struct Info
    {
        std::string name;
        std::string path;
        std::string error;
        std::string difficulty;
        long long   savedAtEpoch = 0;
        int         gamePart = 0;
        bool        valid = false;
    };

    std::string         defaultDirectory(void);
    std::string         normalizedName(const std::string &);
    std::vector<Info>   inspect(const std::string &);
    std::vector<Info>   inspect(void);
    std::string         existingPath(const std::string &, const std::string &);
    std::string         existingPath(const std::string &);
    bool                hasManualSaves(void);

    bool                writeAutosave(const std::string &, const SWE::JsonObject &,
                                      std::string* error = nullptr);
    bool                writeManual(const std::string &, const SWE::JsonObject &,
                                    const std::string &, bool overwrite,
                                    std::string* savedFile = nullptr,
                                    std::string* error = nullptr);
    bool                writeManual(const SWE::JsonObject &, const std::string &,
                                    bool overwrite, std::string* savedFile = nullptr,
                                    std::string* error = nullptr);
    bool                deleteManual(const std::string &, const std::string &,
                                     std::string* error = nullptr);
    bool                deleteManual(const std::string &, std::string* error = nullptr);
}

#endif
