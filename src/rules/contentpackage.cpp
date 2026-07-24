/***************************************************************************
 *   Copyright (C) 2026 by Four Winds Reborn contributors                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "contentpackage.h"

#include <algorithm>

namespace
{
std::string packageLabel(const ContentPackageIdentity & identity)
{
    return identity.id + "@" + std::to_string(identity.version);
}

ContentPackageManifest & selectedContentPackage(void)
{
    static ContentPackageManifest selected = classicContentPackageManifest();
    return selected;
}

}

bool parseContentPackageManifest(const SWE::JsonObject & themeIndex,
                                 ContentPackageManifest & manifest,
                                 std::string* error)
{
    const SWE::JsonObject* encoded = themeIndex.getObject(ContentPackageIdentityKey);
    if(!encoded)
    {
        if(error) *error = "Content package manifest is missing";
        return false;
    }
    if(!encoded->isInteger("schema") || !encoded->isString("id") ||
       !encoded->isInteger("version") || !encoded->isString("engineContract") ||
       !encoded->isString("gameData") ||
       !encoded->isArray("compatibleVersions"))
    {
        if(error) *error = "Content package manifest is invalid";
        return false;
    }

    ContentPackageManifest parsed;
    parsed.schema = encoded->getInteger("schema");
    parsed.identity.id = encoded->getString("id");
    parsed.identity.version = encoded->getInteger("version");
    parsed.engineContract = encoded->getString("engineContract");
    parsed.gameData = encoded->getString("gameData");

    const SWE::JsonArray* versions = encoded->getArray("compatibleVersions");
    for(std::size_t index = 0; versions && index < versions->size(); ++index)
    {
        const SWE::JsonValue* value = versions->getValue(index);
        if(!value || value->getType() != SWE::JsonType::Integer)
        {
            if(error) *error = "Content package manifest is invalid";
            return false;
        }
        parsed.compatibleVersions.push_back(value->getInteger());
    }

    std::sort(parsed.compatibleVersions.begin(), parsed.compatibleVersions.end());
    parsed.compatibleVersions.erase(
        std::unique(parsed.compatibleVersions.begin(), parsed.compatibleVersions.end()),
        parsed.compatibleVersions.end());
    if(!parsed.isValid())
    {
        if(error)
        {
            if(parsed.schema != ContentPackageManifestSchema)
                *error = "Content package manifest schema is unsupported";
            else if(parsed.engineContract != ClassicEngineContentContract)
                *error = "Content package engine contract is unsupported: " +
                    parsed.engineContract;
            else
                *error = "Content package manifest is invalid";
        }
        return false;
    }

    manifest = std::move(parsed);
    return true;
}

bool ContentPackageManifest::isValid(void) const
{
    return schema == ContentPackageManifestSchema && identity.isValid() &&
        engineContract == ClassicEngineContentContract &&
        !gameData.empty() && gameData != "." && gameData != ".." &&
        gameData.find('/') == std::string::npos &&
        gameData.find('\\') == std::string::npos &&
        !compatibleVersions.empty() &&
        std::all_of(compatibleVersions.begin(), compatibleVersions.end(),
                    [](int version){ return 0 < version; }) &&
        std::find(compatibleVersions.begin(), compatibleVersions.end(),
                  identity.version) != compatibleVersions.end();
}

bool ContentPackageManifest::supports(const ContentPackageIdentity & other) const
{
    return isValid() && other.isValid() && identity.id == other.id &&
        std::find(compatibleVersions.begin(), compatibleVersions.end(),
                  other.version) != compatibleVersions.end();
}

const ContentPackageManifest & classicContentPackageManifest(void)
{
    static const ContentPackageManifest manifest = {
        ContentPackageManifestSchema,
        { ClassicContentPackageId, ClassicContentPackageVersion },
        ClassicEngineContentContract,
        "content.json",
        { ClassicContentPackageVersion }
    };
    return manifest;
}

const ContentPackageManifest & activeContentPackageManifest(void)
{
    return selectedContentPackage();
}

ContentPackageIdentity contentPackageIdentity(const ContentPackageManifest & manifest)
{
    return manifest.identity;
}

SWE::JsonObject contentPackageIdentityJson(const ContentPackageManifest & manifest)
{
    SWE::JsonObject result;
    result.addString("id", manifest.identity.id);
    result.addInteger("version", manifest.identity.version);
    return result;
}

bool activateContentPackage(const SWE::JsonObject & themeIndex, std::string* error)
{
    ContentPackageManifest parsed;
    if(!parseContentPackageManifest(themeIndex, parsed, error)) return false;

    selectedContentPackage() = std::move(parsed);
    if(error) error->clear();
    return true;
}

bool activeContentPackageSupports(const ContentPackageIdentity & identity,
                                  std::string* error)
{
    const ContentPackageManifest & active = activeContentPackageManifest();
    if(!active.supports(identity))
    {
        if(error) *error = "Content package is unavailable or incompatible: " +
            packageLabel(identity) + " (active " + packageLabel(active.identity) + ")";
        return false;
    }
    if(error) error->clear();
    return true;
}

bool resolveContentPackageIdentity(const SWE::JsonObject & container,
                                   ContentPackageIdentity & identity,
                                   bool allowLegacyClassic,
                                   std::string* error)
{
    identity = ContentPackageIdentity();
    if(!container.hasKey(ContentPackageIdentityKey))
    {
        if(!allowLegacyClassic)
        {
            if(error) *error = "Content package metadata is missing";
            return false;
        }
        identity = classicContentPackageManifest().identity;
        return activeContentPackageSupports(identity, error);
    }

    const SWE::JsonObject* encoded = container.getObject(ContentPackageIdentityKey);
    if(!encoded || !encoded->isString("id") || !encoded->isInteger("version"))
    {
        if(error) *error = "Content package metadata is invalid";
        return false;
    }

    identity.id = encoded->getString("id");
    identity.version = encoded->getInteger("version");
    if(!identity.isValid())
    {
        if(error) *error = "Content package metadata is invalid";
        return false;
    }
    return activeContentPackageSupports(identity, error);
}

bool sameContentPackage(const ContentPackageIdentity & first,
                        const ContentPackageIdentity & second)
{
    return first.id == second.id && first.version == second.version;
}
