/***************************************************************************
 *   Four Winds Reborn installed content-package catalog                  *
 ***************************************************************************/

#ifndef FOUR_WINDS_CONTENT_CATALOG_H
#define FOUR_WINDS_CONTENT_CATALOG_H

#include <string>
#include <vector>

#include "contentpackage.h"

struct InstalledContentPackage
{
    std::string theme;
    std::string name;
    std::string description;
    std::string author;
    std::string path;
    ContentPackageManifest manifest;
    bool compatible = false;
    std::string error;

    std::string displayName(void) const;
};

namespace ContentCatalog
{
    // Discovers side-by-side theme directories using the same executable and
    // per-user search roots as GameTheme. Invalid entries are retained with a
    // diagnostic so callers can report them, but must never activate them.
    std::vector<InstalledContentPackage> discover(const std::string & program,
                                                   const std::string & domain);

    bool isAvailable(const std::string & program, const std::string & domain,
                     const std::string & theme, std::string* error = nullptr);
}

#endif
