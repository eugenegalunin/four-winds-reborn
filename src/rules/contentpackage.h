/***************************************************************************
 *   Copyright (C) 2026 by Four Winds Reborn contributors                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef FOUR_WINDS_CONTENT_PACKAGE_H
#define FOUR_WINDS_CONTENT_PACKAGE_H

#include <string>
#include <vector>

#include "swe/swe_json.h"

constexpr const char ContentPackageIdentityKey[] = "contentPackage";
constexpr const char ClassicContentPackageId[] = "classic-original";
constexpr int ClassicContentPackageVersion = 1;
constexpr int ContentPackageManifestSchema = 1;
constexpr const char ClassicEngineContentContract[] = "four-winds-classic-v1";

struct ContentPackageIdentity
{
    std::string id;
    int version = 0;

    bool isValid(void) const { return !id.empty() && 0 < version; }
};

struct ContentPackageManifest
{
    int schema = 0;
    ContentPackageIdentity identity;
    std::string engineContract;
    std::string gameData;
    std::vector<int> compatibleVersions;

    bool isValid(void) const;
    bool supports(const ContentPackageIdentity &) const;
};

const ContentPackageManifest & classicContentPackageManifest(void);
const ContentPackageManifest & activeContentPackageManifest(void);
ContentPackageIdentity contentPackageIdentity(const ContentPackageManifest &);
SWE::JsonObject contentPackageIdentityJson(const ContentPackageManifest &);

// Reads the mandatory contentPackage manifest embedded in a theme index and
// activates it only after the complete manifest has passed validation.
bool activateContentPackage(const SWE::JsonObject & themeIndex,
                            std::string* error = nullptr);

// Artifact identities live in saves, recovery metadata and replay journals.
// Missing identities are accepted only for pre-manifest Classic artifacts.
bool resolveContentPackageIdentity(const SWE::JsonObject & container,
                                   ContentPackageIdentity & identity,
                                   bool allowLegacyClassic,
                                   std::string* error = nullptr);
bool activeContentPackageSupports(const ContentPackageIdentity &,
                                  std::string* error = nullptr);
bool sameContentPackage(const ContentPackageIdentity &,
                        const ContentPackageIdentity &);

#endif
