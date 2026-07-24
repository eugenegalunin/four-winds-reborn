/***************************************************************************
 *   Four Winds Reborn installed content-package catalog                  *
 ***************************************************************************/

#include "contentcatalog.h"

#include <algorithm>
#include <map>
#include <set>

#include "runewars.h"

namespace
{
using ResourceMap = std::map<std::string, std::string>;

std::vector<std::string> themeRoots(const std::string & program,
                                    const std::string & domain)
{
    StringList roots = Systems::shareDirectories(domain);
    if(!program.empty()) roots.push_front(Systems::dirname(program));

    std::vector<std::string> result;
    for(const std::string & root : roots)
    {
        const std::string themes = Systems::concatePath(root, "themes");
        if(Systems::isDirectory(themes) &&
           std::find(result.begin(), result.end(), themes) == result.end())
            result.push_back(themes);
    }
    return result;
}

bool safeThemeName(const std::string & name)
{
    return !name.empty() && name != "." && name != ".." &&
        name.find('/') == std::string::npos &&
        name.find('\\') == std::string::npos;
}

ResourceMap themeResources(const std::vector<std::string> & roots,
                           const std::string & theme,
                           std::string* primaryPath)
{
    ResourceMap resources;
    // GameTheme scans the user root before the executable root and resolves
    // the first matching basename. Preserve that overlay order here.
    for(auto root = roots.rbegin(); root != roots.rend(); ++root)
    {
        const std::string directory = Systems::concatePath(*root, theme);
        if(!Systems::isDirectory(directory)) continue;
        if(primaryPath && primaryPath->empty()) *primaryPath = directory;

        for(const std::string & file : Systems::findFiles(directory))
        {
            const std::string key = String::toLower(Systems::basename(file));
            if(resources.find(key) == resources.end()) resources.emplace(key, file);
        }
    }
    return resources;
}

bool jsonObjectResource(const ResourceMap & resources, const std::string & name,
                        JsonObject* result, std::string* error)
{
    const auto found = resources.find(String::toLower(name));
    if(found == resources.end())
    {
        if(error) *error = "Required resource is missing: " + name;
        return false;
    }
    JsonObject object = JsonContentFile(found->second).toObject();
    if(!object.isValid())
    {
        if(error) *error = "Required JSON object is invalid: " + name;
        return false;
    }
    if(result) *result = std::move(object);
    return true;
}

bool jsonArrayResource(const ResourceMap & resources, const std::string & name,
                       std::string* error, std::size_t expectedSize = 0)
{
    const auto found = resources.find(String::toLower(name));
    if(found == resources.end())
    {
        if(error) *error = "Required resource is missing: " + name;
        return false;
    }
    const JsonContentFile content(found->second);
    if(!content.isValid() || !content.isArray())
    {
        if(error) *error = "Required JSON array is invalid: " + name;
        return false;
    }
    if(expectedSize && content.toArray().size() != expectedSize)
    {
        if(error) *error = "Fixed content table has an incompatible size: " + name;
        return false;
    }
    return true;
}

bool fixedContentIndexes(const JsonObject & gameData, std::string* error)
{
    static const std::map<std::string, std::vector<std::string>> required = {
        { "index:winds", { "none", "east", "south", "west", "north" } },
        { "index:clans", { "none", "red", "yellow", "aqua", "purple" } },
        { "index:abilities", { "none", "monacle", "catasrophic", "bard",
                                "luck", "telepath" } },
        { "index:specials", { "none", "swarm", "merge", "invisibility",
                               "regeneration", "cast_hellblast",
                               "magic_resistance", "mighty_blow", "gate",
                               "first_strike", "see_invisible", "ignore_missiles",
                               "devotion", "fire_shield", "cast_draw_number",
                               "cast_draw_sword", "cast_draw_skull",
                               "cast_random_discard", "cast_silence",
                               "cast_scry_runes", "cast_mana_fog",
                               "ranger_attack" } },
        { "index:avatars", { "none", "orachi", "lakkho", "dayla", "ziag",
                              "niana", "kierac", "logun", "nucrus", "javed",
                              "random" } },
        { "index:spells", { "none", "smoke", "demonic_compulsion", "mass_panic",
                             "paralyze", "reduction", "battle_fury", "guidance",
                             "force_shield", "dust_cloud", "heroism",
                             "mystical_fountain", "mystical_fountain_attack",
                             "mystical_fountain_range", "mystical_fountain_loyalty",
                             "blind_ambition", "brilliant_lights", "magical_aura",
                             "teleport", "dispel_magic", "lightning_bolt", "healing",
                             "hell_blast", "draw_skull", "draw_sword", "draw_number",
                             "random_discard", "scry_runes", "silence", "mana_fog",
                             "mass_dispel" } },
        { "index:creatures", { "none", "skeleton_horde", "shadow", "durlock",
                                "kilor_celsbane", "wraith", "maz_ra", "knight_templar",
                                "great_carol", "minotaur", "adventure_party", "fire_giant",
                                "king_drago", "sand_wraith", "stone_golem", "learra_siren",
                                "thunder_bird", "ki_lin", "shanahan", "white_dragon",
                                "green_dragon", "red_dragon", "fire_elemental",
                                "earth_elemental", "air_elemental", "water_elemental",
                                "juggernaut", "tornado", "griffon", "chameleon" } },
        { "index:lands", { "none", "tower_of4_winds", "maithaius", "baliphon",
                            "vermille", "sulanthia", "trojensek", "talon", "siramak",
                            "ronzinol", "corzen", "greenbaw", "zubrus", "corimar",
                            "inkartha", "hexan", "firland", "vesna", "kern",
                            "regency_peaks", "knighthaven", "rikter", "gorok", "suntura",
                            "bertram", "mastenbroek", "reisse", "cirrus", "grosek",
                            "chahrr", "saugrezz", "mahnjeet", "trassk", "bechnarr",
                            "azuria", "pyramus_reach", "serenity_plains", "sunspot",
                            "charmers_expanse", "gambits_run", "ash_point", "orchid_bay",
                            "mocklebury", "celestial_wood", "tortoise_isle",
                            "siphons_chute" } }
    };

    for(const auto & entry : required)
    {
        if(!gameData.isArray(entry.first) ||
           gameData.getStdList<std::string>(entry.first) !=
               std::list<std::string>(entry.second.begin(), entry.second.end()))
        {
            if(error) *error = "Fixed content index is incompatible: " + entry.first;
            return false;
        }
    }

    for(const char* bonus : { "bonus:start", "bonus:game", "bonus:kong",
                              "bonus:pung", "bonus:chao", "bonus:pass" })
    {
        if(!gameData.isInteger(bonus))
        {
            if(error) *error = "Content bonus is missing or invalid: " +
                std::string(bonus);
            return false;
        }
    }
    return true;
}

InstalledContentPackage inspectTheme(const std::vector<std::string> & roots,
                                     const std::string & theme)
{
    InstalledContentPackage package;
    package.theme = theme;
    const ResourceMap resources = themeResources(roots, theme, &package.path);

    JsonObject index;
    if(!jsonObjectResource(resources, "index.json", &index, &package.error))
        return package;

    package.name = index.getString("name", theme);
    package.description = index.getString("description", package.name);
    package.author = index.getString("author");
    if(!parseContentPackageManifest(index, package.manifest, &package.error))
        return package;

    JsonObject gameData;
    if(!jsonObjectResource(resources, package.manifest.gameData, &gameData,
                           &package.error))
        return package;
    if(!fixedContentIndexes(gameData, &package.error)) return package;

    for(const char* required : { "images.json", "musics.json", "sounds.json",
                                 "fonts.json" })
        if(!jsonArrayResource(resources, required, &package.error)) return package;

    static const std::map<std::string, std::size_t> fixedTables = {
        { "stones.json", 35 }, { "winds.json", 4 }, { "clans.json", 4 },
        { "creatures.json", 29 }, { "spells.json", 30 }, { "specials.json", 21 },
        { "abilities.json", 5 }, { "avatars.json", 10 }, { "lands.json", 45 }
    };
    for(const auto & table : fixedTables)
        if(!jsonArrayResource(resources, table.first, &package.error, table.second))
            return package;

    const Size logicalSize = JsonUnpack::size(index, "size");
    if(logicalSize.isEmpty())
    {
        package.error = "Theme logical size is invalid";
        return package;
    }

    package.compatible = true;
    return package;
}
}

std::string InstalledContentPackage::displayName(void) const
{
    if(!description.empty()) return description;
    if(!name.empty()) return name;
    return theme;
}

std::vector<InstalledContentPackage> ContentCatalog::discover(
    const std::string & program, const std::string & domain)
{
    const std::vector<std::string> roots = themeRoots(program, domain);
    std::set<std::string> names;
    for(const std::string & root : roots)
    {
        for(const std::string & entry : Systems::readDir(root, false))
        {
            if(!safeThemeName(entry)) continue;
            if(Systems::isDirectory(Systems::concatePath(root, entry))) names.insert(entry);
        }
    }

    std::vector<InstalledContentPackage> result;
    result.reserve(names.size());
    for(const std::string & name : names) result.push_back(inspectTheme(roots, name));
    std::stable_sort(result.begin(), result.end(),
        [](const InstalledContentPackage & first,
           const InstalledContentPackage & second)
        {
            if(first.theme == "classic") return second.theme != "classic";
            if(second.theme == "classic") return false;
            return String::toLower(first.displayName()) <
                   String::toLower(second.displayName());
        });
    return result;
}

bool ContentCatalog::isAvailable(const std::string & program,
                                 const std::string & domain,
                                 const std::string & theme,
                                 std::string* error)
{
    for(const InstalledContentPackage & package : discover(program, domain))
    {
        if(package.theme != theme) continue;
        if(package.compatible)
        {
            if(error) error->clear();
            return true;
        }
        if(error) *error = package.error;
        return false;
    }
    if(error) *error = "Theme is not installed: " + theme;
    return false;
}
