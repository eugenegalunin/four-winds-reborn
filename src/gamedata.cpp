/***************************************************************************
 *   Copyright (C) 2020 by RuneWarsNA team <runewars.newage@gmail.com>     *
 *                                                                         *
 *   Part of the RuneWars: NewAge engine:                                  *
 *   https://github.com/AndreyBarmaley/runewars.newage                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <cstring>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <set>

#include "settings.h"
#include "actionvalidation.h"
#include "adventurecommands.h"
#include "aiprofile.h"
#include "aiturn.h"
#include "actions.h"
#include "battle.h"
#include "battlesession.h"
#include "crashreport.h"
#include "gamedata.h"
#include "gameplayrng.h"
#include "recovery.h"
#include "savegames.h"
#include "replay.h"

#ifndef FOUR_WINDS_VERSION
#define FOUR_WINDS_VERSION "unknown"
#endif
#ifndef FOUR_WINDS_GAME_REVISION
#define FOUR_WINDS_GAME_REVISION "unknown"
#endif
#ifndef FOUR_WINDS_ENGINE_REVISION
#define FOUR_WINDS_ENGINE_REVISION "unknown"
#endif

std::string SpellInfo::effectDescription(void) const
{
    switch(id())
    {
	case Spell::LightningBolt:
	case Spell::MassPanic:
	case Spell::HellBlast:		return StringFormat(_("-%1 Loyalty")).arg(std::abs(effect.loyalty));
	case Spell::DemonicCompulsion:	return StringFormat(_("+%1 Loyalty")).arg(std::abs(effect.loyalty));

	case Spell::BattleFury:		return StringFormat(_("+%1 Attack")).arg(std::abs(effect.attack));
	case Spell::DustCloud:		return StringFormat(_("-%1 Attack")).arg(std::abs(effect.attack));

	case Spell::Guidance:		return StringFormat(_("+%1 Missile")).arg(std::abs(effect.ranger));
	case Spell::Smoke:		return StringFormat(_("-%1 Missile")).arg(std::abs(effect.ranger));

	case Spell::MagicalAura:	return StringFormat(_("+%1 Defense")).arg(std::abs(effect.defense));
	case Spell::BrilliantLights:	return StringFormat(_("-%1 Defense")).arg(std::abs(effect.defense));

	case Spell::Heroism:		return StringFormat(_("+%1 Attack, +%2 Max Loyalty")).arg(std::abs(effect.attack)).arg(std::abs(effect.loyalty));
	case Spell::MysticalFountain:	return StringFormat(_("+%1 Attack or +%2 Missile or +%3 Max Loyalty")).arg(std::abs(effect.attack)).arg(std::abs(effect.ranger)).arg(std::abs(effect.loyalty));
	case Spell::BlindAmbition:	return StringFormat(_("+%1 Attack, +%2 Missile, -%3 Max Loyalty")).arg(std::abs(effect.attack)).arg(std::abs(effect.ranger)).arg(std::abs(effect.loyalty));
	case Spell::Healing:		return StringFormat(_("+%1 Loyalty, up to max")).arg(std::abs(effect.loyalty));
	case Spell::Reduction:		return StringFormat(_("-%1 Defense, -%2 Attack")).arg(std::abs(effect.defense)).arg(std::abs(effect.attack));
	case Spell::ForceShield:	return StringFormat(_("Missile attacks do %1 less damage")).arg(value);

	case Spell::Paralyze:		return _("Freeze creature");
	case Spell::Teleport:		return _("Move one unit to any friendly territory");
	case Spell::DispelMagic:
	case Spell::MassDispel:		return _("Remove enchantments");

	default:		break;
    }

    return name;
}

std::string AvatarInfo::toStringClans(void) const
{
    StringList res;

    for(auto & id : clans)
    {
	const ClanInfo & clan = GameData::clanInfo(id);
	res << clan.name;
    }

    return res.join(", ");
}

namespace GameData
{
    std::vector<StoneInfo>		stonesInfo;
    std::vector<WindInfo>		windsInfo;
    std::vector<ClanInfo>		clansInfo;
    std::vector<CreatureInfo>		creaturesInfo;
    std::vector<SpellInfo>		spellsInfo;
    std::vector<SpecialityInfo>		specialsInfo;
    std::vector<AbilityInfo>		abilitiesInfo;
    std::vector<AvatarInfo>		avatarsInfo;
    std::vector<LandInfo>		landsInfo;
    std::vector<Clan>			initialLandOwners;

    int					bonusStart;
    int					bonusGame;
    int					bonusKong;
    int					bonusPung;
    int					bonusChao;
    int					bonusPass;

    template<typename T>
    bool loadJson(const char* json, std::vector<T> & v)
    {
	JsonContent jc = GameTheme::jsonResource(json);
	if(jc.isArray())
	{
	    auto list = jc.toArray().toStdList<T>();
	    v.resize(list.size() + 1); // nul reserve

	    for(auto & val : list)
		v[val.id.index()] = val;

	    return true;
	}

	ERROR("incorrect json array: " << json);
	return false;
    }

    bool 		loadIndexes(const JsonObject &);

    LocalPlayer &	playerOfClan(const Clan &);
    LocalPlayer &	playerOfAvatar(const Avatar &);
    LocalPlayer &	playerOfWind(const Wind &);
    bool		clientLuckChoice(const Avatar &, const ClientMessage &, ActionList &);
}

bool GameData::init(const JsonObject & jo)
{
    if(! loadIndexes(jo))
	return false;

    bonusStart = jo.getInteger("bonus:start", 250);
    bonusGame = jo.getInteger("bonus:game", 50);
    bonusPass = jo.getInteger("bonus:pass", 10);
    bonusChao = jo.getInteger("bonus:chao", 20);
    bonusPung = jo.getInteger("bonus:pung", 30);
    bonusKong = jo.getInteger("bonus:kong", 40);

    // load game jsons
    if(! loadJson<StoneInfo>("stones.json", stonesInfo))
	return false;

    if(! loadJson<WindInfo>("winds.json", windsInfo))
	return false;

    if(! loadJson<ClanInfo>("clans.json", clansInfo))
	return false;

    if(! loadJson<CreatureInfo>("creatures.json", creaturesInfo))
	return false;

    if(! loadJson<SpellInfo>("spells.json", spellsInfo))
	return false;

    if(! loadJson<SpecialityInfo>("specials.json", specialsInfo))
	return false;

    if(! loadJson<AbilityInfo>("abilities.json", abilitiesInfo))
	return false;

    if(! loadJson<AvatarInfo>("avatars.json", avatarsInfo))
	return false;

    if(! loadJson<LandInfo>("lands.json", landsInfo))
	return false;

    retranslateThemeData();

    initialLandOwners.resize(landsInfo.size());
    for(std::size_t index = 0; index < landsInfo.size(); ++index)
        initialLandOwners[index] = landsInfo[index].clan;

    // check path valid
    for(auto & info1 : landsInfo)
    {
        const Land & land1 = info1.id;
        for(auto & land2 : info1.borders)
        {
            const LandInfo & info2 = landInfo(land2);

            if(std::none_of(info2.borders.begin(), info2.borders.end(), [&](const Land & land){ return land == land1; }))
                ERROR("land path error: " << land1.toString() << "<->" << land2.toString());
        }
    }

    return true;
}

const LandInfo & GameData::landInfo(const Land & landId)
{
    return landsInfo[landId()];
}

const AvatarInfo & GameData::avatarInfo(const Avatar & avatarId)
{
    return avatarsInfo[avatarId()];
}

const AbilityInfo & GameData::abilityInfo(const Ability & abilityId)
{
    return abilitiesInfo[abilityId()];
}

const SpecialityInfo & GameData::specialityInfo(const Speciality & speciality)
{
    return specialsInfo[speciality.index()];
}

const CreatureInfo & GameData::creatureInfo(const Creature & creatureId)
{
    return creaturesInfo[creatureId()];
}

const SpellInfo & GameData::spellInfo(const Spell & spellId)
{
    return spellsInfo[spellId()];
}

const StoneInfo & GameData::stoneInfo(const Stone & stone)
{
    return stonesInfo[stone.index()];
}

const WindInfo & GameData::windInfo(const Wind & windId)
{
    return windsInfo[windId()];
}

const ClanInfo & GameData::clanInfo(const Clan & clanId)
{
    return clansInfo[clanId()];
}

Avatars GameData::avatarsOfClan(const Clan & clanId)
{
    Avatars res;

    for(auto id : avatars_all)
    {
	const AvatarInfo & info = GameData::avatarInfo(id);
	if(std::any_of(info.clans.begin(), info.clans.end(), [&](const Clan & id){ return id == clanId; }))
	    res.emplace_back(id);
    }

    return res;
}

const RemotePlayer & LocalData::playerCurrentWind(void) const
{
    return playerOfWind(currentWind);
}

LocalPlayer & LocalData::myPlayer(void)
{
    return players[3];
}

const LocalPlayer & LocalData::myPlayer(void) const
{
    return players[3];
}

const RemotePlayer & LocalData::playerOfWind(const Wind & wind) const
{
    auto it = std::find_if(players.begin(), players.end(),
			    [&](const Person & pers){ return pers.isWind(wind); });

    if(it == players.end())
    {
	ERROR("player not found" << ", " << "wind: " << wind.toString());
	Engine::except(__FUNCTION__, "exit");
    }

    return *it;
}

const RemotePlayer & LocalData::playerOfClan(const Clan & clan) const
{
    auto it = std::find_if(players.begin(), players.end(),
			    [&](const Person & pers){ return pers.isClan(clan); });

    if(it == players.end())
    {
	ERROR("player not found" << ", " << "clan: " << clan.canonicalName());
	Engine::except(__FUNCTION__, "exit");
    }

    return *it;
}

const RemotePlayer & LocalData::playerOfAvatar(const Avatar & avatar) const
{
    auto it = std::find_if(players.begin(), players.end(),
			    [&](const Person & pers){ return pers.isAvatar(avatar); });

    if(it == players.end())
    {
	ERROR("player not found" << ", " << "avatar: " << avatar.toString());
	Engine::except(__FUNCTION__, "exit");
    }

    return *it;
}

const BattleCreature* LocalData::findBattleUnitConst(int unit) const
{
    if(0 < unit)
    {
	for(const auto & player : players)
	{
	    const BattleCreature* creature = player.army.findBattleUnitConst(unit);
	    if(creature) return creature;
	}
    }

    return nullptr;
}

Persons LocalData::toPersons(void) const
{
    Persons res;
    res.assign(players.begin(), players.end());
    return res;
}

namespace GameData
{
    // remote data
    Person				person;
    LocalPlayers			gamers;
    Wind				currentWind;
    Wind				roundWind;
    Wind				partWind;
    CroupierSet				croupier;
    int					stoneLastCount = 0;
    Stone				dropStone;
    WinResults				winResult;
    std::list<BattleLegend>		battleHistory;
    bool				skipRepeatSay = false;
    bool				skipNewStone = false;
    bool				skipNewTurn = false;
    int					gamePart = 0;
    int					battleUnitId = 1;
    AI::Difficulty                      difficulty = AI::Difficulty::Normal;
    bool                                assistedByDeveloper = false;
#ifdef BUILD_DEBUG
    Avatar                              developerAutoplayAvatar;
#endif
    JsonObject				stateGUI;

    void grantAiDifficultyIncome(bool initial)
    {
        const AI::DifficultyRules & rules = AI::difficultyRules(difficulty);
        const int spellPoints = initial ? rules.initialAiSpellPointBonus :
                                          rules.mahjongPartAiSpellPointBonus;

        for(LocalPlayer & player : gamers)
        {
            // Developer autoplay still represents the human seat. Only actual AI
            // opponents receive the deliberately asymmetric Unfair economy.
            if(!player.isAI()) continue;

            player.points += spellPoints;
            if(!initial && rules.mahjongPartAiLandClaimBonus > 0)
            {
                for(const auto clanId : clans_all)
                {
                    const Clan clan(clanId);
                    if(clan != player.clan)
                        player.addLandClaimPoints(clan, rules.mahjongPartAiLandClaimBonus);
                }
            }
        }
    }

    Wind                                prevWindCompass(const Wind &);
    Wind                                nextWindCompass(const Wind &);
    Wind                                oppositeWindCompass(const Wind &);

    void				dumpOrderPersons(void);
    void				validateMahjongSummary(void);

    bool				clientReady(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientSayGame(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientSayChao(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientSayPung(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientSayKong(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientButtonGame(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientButtonPass(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientButtonPung(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientButtonKong1(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientButtonKong2(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientChaoVariant(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientDropIndex(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientSummonCreature(const Avatar &, const ClientMessage &, ActionList &);
    bool				clientCastSpell(const Avatar &, const ClientMessage &, ActionList &);
    const char* recoveryPlatform(void)
    {
#if defined(_WIN32)
        return "windows";
#elif defined(__APPLE__)
        return "macos";
#elif defined(__linux__)
        return "linux";
#else
        return "unknown";
#endif
    }

    JsonObject                  	toJsonObject(const JsonObject &);
    bool				fromJsonObject(const JsonObject &);
}

int GameData::nextBattleUnitId(void)
{
    return battleUnitId++;
}

bool GameData::isGameOver(void)
{
    return roundWind() == Wind::North && partWind() == Wind::North;
}

std::list<BattleLegend> GameData::getBattleHistoryFor(const Avatar & avatar)
{
    std::list<BattleLegend> res;

    for(auto & legend : battleHistory)
	if(legend.attacker == avatar) res.push_back(legend);

    return res;
}

const JsonObject & GameData::jsonGUI(void)
{
    return stateGUI;
}

void GameData::setGamePart(int v)
{
    gamePart = v;

    if(v == Menu::AdventurePart)
    {
	for(auto & player : gamers)
            player.initAdventurePart();
    }
}

int GameData::loadedGamePart(void)
{
    return gamePart;
}

const Person & GameData::myPerson(void)
{
    return person;
}

const Person & GameData::currentPerson(void)
{
    return playerOfWind(currentWind);
}

const LocalPlayers & GameData::players(void)
{
    return gamers;
}

AI::Difficulty GameData::aiDifficulty(void)
{
    return difficulty;
}

void GameData::setAIDifficulty(AI::Difficulty value)
{
    difficulty = value;
}

bool GameData::usesAI(const Person & player)
{
#ifdef BUILD_DEBUG
    return player.isAI() || (developerAutoplayAvatar.isValid() &&
                             player.avatar == developerAutoplayAvatar);
#else
    return player.isAI();
#endif
}

bool GameData::developerAssisted(void)
{
    return assistedByDeveloper;
}

#ifdef BUILD_DEBUG
bool GameData::developerAutoplay(const Avatar & avatar)
{
    return developerAutoplayAvatar.isValid() && developerAutoplayAvatar == avatar;
}

void GameData::setDeveloperAutoplay(const Avatar & avatar, bool enabled)
{
    developerAutoplayAvatar = enabled ? avatar : Avatar();
    if(enabled) assistedByDeveloper = true;
    CrashReport::breadcrumb(std::string("Developer autoplay avatar=")
        .append(enabled ? avatar.toString() : "none"));
}
#endif

JsonObject GameData::toJsonObject(const JsonObject & gui)
{
    JsonObject jo;
    jo.addInteger("version", FORMAT_VERSION_CURRENT);
    jo.addString("wind:round", roundWind.toString());
    jo.addString("wind:part", partWind.toString());
    jo.addString("wind:current", currentWind.toString());
    jo.addInteger("lastcount", stoneLastCount);
    jo.addString("stone:drop", dropStone.toString());
    jo.addBoolean("skiprepeat", skipRepeatSay);
    jo.addBoolean("skipnew", skipNewStone);
    jo.addBoolean("skipturn", skipNewTurn);
    jo.addInteger("gamepart", gamePart);
    jo.addInteger("nextBattleUnitId", battleUnitId);
    jo.addString("ai:difficulty", AI::difficultyName(difficulty));
    jo.addBoolean("developerAssisted", assistedByDeveloper);
    jo.addObject("gameplayRng", GameplayRng::toJsonObject());

    jo.addObject("myperson", person.toJsonObject());
    jo.addObject("croupier", croupier.toJsonObject());
    jo.addObject("winresult", winResult.toJsonObject());
    jo.addArray("players", gamers.toJsonArray());

    JsonObject landOwners;
    for(auto landId : lands_all)
    {
        const Land land(landId);
        landOwners.addString(land.toString(), landInfo(land).clan.toString());
    }
    jo.addObject("landOwners", landOwners);

    JsonArray ja;
    for(auto & legend : battleHistory)
	ja.addObject(legend.toJsonObject());
    jo.addArray("history", ja);

    if(pendingBattle.isValid())
	jo.addObject("battleSession", pendingBattle.toJsonObject());

    if(gui.isValid()) jo.addObject("gui", gui);

    return jo;
}

JsonObject GameData::authoritativeState(void)
{
    return toJsonObject(JsonObject());
}

bool GameData::fromJsonObject(const JsonObject & jo)
{
    int version = jo.getInteger("version");

    if(version < FORMAT_VERSION_LAST || version > FORMAT_VERSION_CURRENT)
    {
	ERROR("unknown version: " << version << ", " <<
	    "supported release: " << FORMAT_VERSION_LAST);
	return false;
    }

    std::string validationError;
    if(!Recovery::validateSaveState(jo, &validationError))
    {
	ERROR("invalid saved game: " << validationError);
	return false;
    }

    const JsonObject* loadedPersonObject = jo.getObject("myperson");
    const JsonArray* loadedPlayersArray = jo.getArray("players");
    const Person loadedPerson = Person::fromJsonObject(*loadedPersonObject);
    LocalPlayers loadedGamers = LocalPlayers::fromJsonArray(*loadedPlayersArray);

    std::set<int> loadedAvatars;
    std::set<int> loadedClans;
    std::set<int> loadedWinds;
    bool rosterIsValid = loadedPerson.avatar.isValid() &&
        loadedPerson.avatar != Avatar(Avatar::Random) && loadedPerson.clan.isValid();
    bool matchingLocalPlayer = false;
    for(const LocalPlayer & player : loadedGamers)
    {
        const bool validIdentity = player.avatar.isValid() &&
            player.avatar != Avatar(Avatar::Random) && player.clan.isValid() &&
            player.wind.isValid();
        if(!validIdentity || !loadedAvatars.insert(player.avatar()).second ||
           !loadedClans.insert(player.clan()).second ||
           !loadedWinds.insert(player.wind()).second)
        {
            rosterIsValid = false;
            break;
        }

        if(player.avatar == loadedPerson.avatar && player.clan == loadedPerson.clan)
            matchingLocalPlayer = true;
    }

    if(loadedGamers.size() != 4 || !rosterIsValid || !matchingLocalPlayer)
    {
	ERROR("invalid saved game: player roster is incomplete or inconsistent");
	return false;
    }

    VERBOSE("load gamedata, version: " << version);

    stoneLastCount = jo.getInteger("lastcount");
    roundWind = Wind(jo.getString("wind:round"));
    partWind = Wind(jo.getString("wind:part"));
    currentWind = Wind(jo.getString("wind:current"));
    dropStone = Stone(jo.getString("stone:drop"));
    skipNewStone = jo.getBoolean("skipnew");
    skipNewTurn = jo.getBoolean("skipturn");
    skipRepeatSay = false; // initial say need! jo.getBoolean("skiprepeat");
    gamePart = jo.getInteger("gamepart");
    battleUnitId = jo.getInteger("nextBattleUnitId");
    difficulty = AI::difficultyFromString(jo.getString("ai:difficulty", "normal"));
    assistedByDeveloper = jo.getBoolean("developerAssisted", false);
#ifdef BUILD_DEBUG
    developerAutoplayAvatar = Avatar();
#endif

    const JsonObject* jo2 = nullptr;

    person = loadedPerson;

    jo2 = jo.getObject("croupier");
    if(! jo2)
    {
	ERROR("json parse: " << "croupier");
	return false;
    }
    croupier = CroupierSet::fromJsonObject(*jo2);

    jo2 = jo.getObject("winresult");
    if(! jo2)
    {
	ERROR("json parse: " << "winresult");
	return false;
    }
    winResult = WinResults::fromJsonObject(*jo2);

    const JsonArray* ja2 = nullptr;

    gamers = std::move(loadedGamers);
    resetAdventureCommandState();
    pendingBattle = PendingBattle();

    // Older saves did not persist captured territory owners and keep theme defaults.
    jo2 = jo.getObject("landOwners");
    if(jo2)
    {
        for(auto landId : lands_all)
        {
            const Land land(landId);
            const Clan owner(jo2->getString(land.toString(), landInfo(land).clan.toString()));
            if(!land.isTowerWinds() && owner.isValid()) landsInfo[land()].clan = owner;
        }
    }

    battleHistory.clear();

    ja2 = jo.getArray("history");
    if(! ja2)
    {
	ERROR("json parse: " << "history");
	return false;
    }

    for(int it = 0; it < ja2->size(); ++it)
    {
	jo2 = ja2->getObject(it);
	if(jo2) battleHistory.push_back(BattleLegend::fromJsonObject(*jo2));
    }

    jo2 = jo.getObject("battleSession");
    if(jo2)
    {
	pendingBattle = PendingBattle::fromJsonObject(*jo2);
	if(!pendingBattle.isValid())
	{
	    ERROR("invalid pending battle session");
	    return false;
	}
    }

    stateGUI.clear();

    jo2 = jo.getObject("gui");
    if(jo2) stateGUI = *jo2;

    const JsonObject* rng = jo.getObject("gameplayRng");
    if(rng)
    {
        if(!GameplayRng::fromJsonObject(*rng))
        {
            ERROR("unsupported or invalid gameplay RNG state");
            return false;
        }
    }
    else
    {
        // Compatibility for saves created before deterministic gameplay RNG.
        GameplayRng::seedFromEntropy();
    }

    return true;
}

bool GameData::restoreState(const JsonObject & state)
{
    return fromJsonObject(state);
}

bool GameData::saveGame(const JsonObject & gui)
{
    const std::string & share = Settings::shareDir();
    if(!Systems::isDirectory(share)) Systems::makeDirectory(share);
    Display::renderScreenshot(Settings::fileSave("game.png"));
    std::string error;
    const bool saved = SaveGames::writeAutosave(Settings::fileSaveGame(),
                                                GameData::toJsonObject(gui), &error);
    if(!saved) ERROR("autosave failed: " << error);
    return saved;
}

bool GameData::saveNamedGame(const JsonObject & gui, const std::string & name,
                             bool overwrite, std::string* error)
{
    std::string savedFile;
    if(!SaveGames::writeManual(GameData::toJsonObject(gui), name, overwrite, &savedFile, error))
        return false;

    if(4 < savedFile.size() && savedFile.substr(savedFile.size() - 4) == ".sav")
        Display::renderScreenshot(savedFile.substr(0, savedFile.size() - 4) + ".png");
    return true;
}

void GameData::retranslateThemeData(void)
{
    const auto translated = [](const std::string & source) {
        return source.empty() ? source : std::string(_(source));
    };

    for(auto & info : stonesInfo)
        info.name = translated(info.sourceName);
    for(auto & info : windsInfo)
        info.name = translated(info.sourceName);
    for(auto & info : clansInfo)
        info.name = translated(info.sourceName);
    for(auto & info : creaturesInfo)
    {
        info.name = translated(info.sourceName);
        info.description = translated(info.sourceDescription);
    }
    for(auto & info : spellsInfo)
    {
        info.name = translated(info.sourceName);
        info.description = translated(info.sourceDescription);
    }
    for(auto & info : specialsInfo)
    {
        info.name = translated(info.sourceName);
        info.description = translated(info.sourceDescription);
    }
    for(auto & info : abilitiesInfo)
    {
        info.name = translated(info.sourceName);
        info.description = translated(info.sourceDescription);
    }
    for(auto & info : avatarsInfo)
    {
        info.name = translated(info.sourceName);
        info.dignity = translated(info.sourceDignity);
        info.description = translated(info.sourceDescription);
    }
    for(auto & info : landsInfo)
        info.name = translated(info.sourceName);
}

bool GameData::saveRecovery(const JsonObject & gui, const std::string & reason)
{
    if(!Recovery::enabled()) return true;
    if(gamers.empty()) return false;

    CrashReport::breadcrumb(std::string("Recovery stage=begin reason=").append(reason));
    const JsonObject saveState = GameData::toJsonObject(gui);
    const std::string saveData = saveState.toString();

    JsonObject metadata;
    metadata.addInteger("schema", 1);
    metadata.addInteger("saveFormat", FORMAT_VERSION_CURRENT);
    metadata.addString("savedAtEpoch", std::to_string(static_cast<long long>(std::time(nullptr))));
    metadata.addString("reason", reason);
    metadata.addString("platform", recoveryPlatform());
    metadata.addString("gameVersion", FOUR_WINDS_VERSION);
    metadata.addString("gameRevision", FOUR_WINDS_GAME_REVISION);
    metadata.addString("engineRevision", FOUR_WINDS_ENGINE_REVISION);
    metadata.addString("fileHashFNV1a64", Recovery::stateHash(saveData));
    metadata.addString("stateHashFNV1a64", Recovery::stateHash(saveState));
    metadata.addObject("gameplayRng", GameplayRng::toJsonObject());
    metadata.addObject("replay", Replay::actionJournal(GameData::authoritativeState()));
    metadata.addInteger("stateBytes", static_cast<int>(saveData.size()));
    metadata.addInteger("gamePart", gamePart);
    metadata.addString("roundWind", roundWind.toString());
    metadata.addString("partWind", partWind.toString());
    metadata.addString("currentWind", currentWind.toString());
    metadata.addString("aiDifficulty", AI::difficultyName(difficulty));
    metadata.addString("breadcrumbSequence", std::to_string(CrashReport::breadcrumbSequence()));
    metadata.addString("crashReport", CrashReport::filePath());

    JsonArray breadcrumbs;
    for(const std::string & entry : CrashReport::recentBreadcrumbs())
        breadcrumbs.addString(entry);
    metadata.addArray("recentBreadcrumbs", breadcrumbs);

    const std::string directory = Recovery::defaultDirectory();
    const bool saved = Recovery::writeCheckpoint(directory, saveData, metadata);
    CrashReport::breadcrumb(std::string("Recovery stage=end reason=").append(reason)
        .append(" status=").append(saved ? "ok" : "failed")
        .append(" directory=").append(directory));
    return saved;
}

bool GameData::loadGame(void)
{
    return loadGame(Settings::fileSaveGame());
}

bool GameData::loadGame(const std::string & fn)
{
    std::string str;
    std::string validationError;

    if(Recovery::validateSaveFile(fn, &validationError, &str))
    {
	const bool loaded = fromJsonObject(JsonContentString(str).toObject());
	if(loaded) Replay::clearActionJournal();
	return loaded;
    }

    ERROR("saved game rejected: " << validationError);
    return false;
}

LocalData GameData::toLocalData(const Avatar & ava)
{
    LocalPlayer* lp = gamers.playerOfAvatar(ava);

    if(! lp)
    {
        ERROR("player not found" << ", " << "avatar: " << ava.toString());
        Engine::except(__FUNCTION__, "exit");
    }

    const Wind & wind = lp->wind;
    const AvatarInfo & avaInfo = avatarInfo(ava);

    LocalData ld;
    ld.trashSet = croupier.trash;

    ld.roundWind = roundWind;
    ld.partWind = partWind;
    ld.currentWind = currentWind;
    ld.compass = WindCompass(wind);
    ld.dropStone = dropStone;
    ld.stoneLastCount = stoneLastCount;
    ld.winResult = winResult;


    lp = gamers.playerOfWind(ld.compass.left());
    if(lp) ld.players[0] = *lp;
    else ERROR("player not found" << ", wind: " << ld.compass.left().toString());

    lp = gamers.playerOfWind(ld.compass.right());
    if(lp) ld.players[1] = *lp;
    else ERROR("player not found" << ", wind: " << ld.compass.right().toString());

    lp = gamers.playerOfWind(ld.compass.top());
    if(lp) ld.players[2] = *lp;
    else ERROR("player not found" << ", wind: " << ld.compass.top().toString());

    lp = gamers.playerOfWind(ld.compass.bottom());
    if(lp) ld.players[3] = *lp;
    else ERROR("player not found" << ", wind: " << ld.compass.bottom().toString());

    // Only the caster can see the hand marked by Scry Runes.
    for(int it = 0; it < 3; ++it)
    {
	if(! ld.players[it].isAffectedSpell(Spell::ScryRunes, ava))
	{
	    // hide: private game info
	    ld.players[it].stones.clear();
	    ld.players[it].newStone.reset();
	}
    }

    // check: Ability Monocle
    if(avaInfo.ability() != Ability::Monacle)
    {
	ld.players[0].army.applyInvisibility(ld.players[3].clan);
	ld.players[1].army.applyInvisibility(ld.players[3].clan);
	ld.players[2].army.applyInvisibility(ld.players[3].clan);
    }

    return ld;
}

BattleArmy & GameData::getBattleArmy(const Clan & clan)
{
    return playerOfClan(clan).army;
}

bool GameData::findCreatureUnique(const Creature & cr)
{
    for(auto & player : gamers)
	if(player.army.findCreature(cr)) return true;

    return false;
}

BattleParty* GameData::getBattleParty(int unit)
{
    if(0 < unit)
    {
	for(auto & player : gamers)
	{
	    for(auto & party : player.army)
		if(party.findBattleUnitConst(unit)) return & party;
	}
    }

    ERROR("battle unit not found" << ", " << "id: " << String::hex(unit));
    Engine::except(__FUNCTION__, "exit");

    return nullptr;
}

BattleCreature* GameData::findBattleCreature(int unit)
{
    if(0 < unit)
    {
	for(auto & player : gamers)
	{
	    auto bcr = player.army.findBattleUnit(unit);
	    if(bcr) return bcr;
	}
    }

    return nullptr;
}

BattleCreature* GameData::getBattleCreature(int unit)
{
    BattleCreature* creature = findBattleCreature(unit);
    if(creature) return creature;

    ERROR("battle unit not found" << ", " << "id: " << String::hex(unit));
    Engine::except(__FUNCTION__, "exit");

    return nullptr;
}

RemotePlayer* GameData::getBattleArmyOwner(const BattleArmy & army)
{
    for(auto & player : gamers)
	if(& player.army == & army) return & player;

    ERROR("battle army not found");
    Engine::except(__FUNCTION__, "exit");

    return nullptr;
}

Wind GameData::oppositeWindCompass(const Wind & wind)
{
    return wind.isValid() ? WindCompass(wind).top() : Wind(Wind::South);
}

Wind GameData::nextWindCompass(const Wind & wind)
{
    return wind.isValid() ? WindCompass(wind).right() : Wind(Wind::East);
}

Wind GameData::prevWindCompass(const Wind & wind)
{
    return wind.isValid() ? WindCompass(wind).left() : Wind(Wind::North);
}

void GameData::initPersons(const Person & cur)
{
    Replay::clearActionJournal();
    Persons persons(cur);
    gamers.setPersons(persons);
    grantAiDifficultyIncome(true);

    person = cur;
    roundWind = Wind(Wind::None);
    partWind = Wind(Wind::None);
    currentWind = Wind(Wind::None);
    pendingBattle = PendingBattle();
    assistedByDeveloper = false;
#ifdef BUILD_DEBUG
    developerAutoplayAvatar = Avatar();
#endif
}

bool GameData::initPersons(const Persons & configured)
{
    if(configured.size() != winds_all.size()) return false;

    std::set<int> avatars;
    std::set<int> clans;
    std::set<int> winds;
    Persons persons;

    for(const Person & configuredPerson : configured)
    {
        if(!configuredPerson.avatar.isValid() || configuredPerson.avatar == Avatar(Avatar::Random) ||
           !configuredPerson.clan.isValid() || !configuredPerson.wind.isValid() ||
           !avatars.insert(configuredPerson.avatar()).second ||
           !clans.insert(configuredPerson.clan()).second ||
           !winds.insert(configuredPerson.wind()).second)
            return false;

        const AvatarInfo & info = avatarInfo(configuredPerson.avatar);
        if(std::find(info.clans.begin(), info.clans.end(), configuredPerson.clan) == info.clans.end())
            return false;

        Person clean(configuredPerson.avatar, configuredPerson.clan, configuredPerson.wind);
        clean.setAI(configuredPerson.isAI());
        persons.push_back(clean);
    }

    Replay::clearActionJournal();
    gamers.setPersons(persons);
    grantAiDifficultyIncome(true);
    person = persons.front();
    roundWind = Wind(Wind::None);
    partWind = Wind(Wind::None);
    currentWind = Wind(Wind::None);
    stoneLastCount = 0;
    dropStone = Stone(Stone::None);
    winResult = WinResults();
    battleHistory.clear();
    skipRepeatSay = false;
    skipNewStone = false;
    skipNewTurn = false;
    gamePart = 0;
    battleUnitId = 1;
    assistedByDeveloper = false;
#ifdef BUILD_DEBUG
    developerAutoplayAvatar = Avatar();
#endif
    stateGUI.clear();
    resetAdventureCommandState();
    pendingBattle = PendingBattle();

    if(initialLandOwners.size() == landsInfo.size())
    {
        for(std::size_t index = 0; index < landsInfo.size(); ++index)
            landsInfo[index].clan = initialLandOwners[index];
    }

    return true;
}

bool GameData::initMahjong(void)
{
    pendingBattle = PendingBattle();
    do
    {
	for(auto & lp : gamers)
	    lp.initMahjongPart();

	croupier.reset();
	gamers.distributeStones(croupier);
    }
    // fix kong startup
    while(gamers.findKongs());

    stoneLastCount = GAME_STONE_MAX;
    skipRepeatSay = false;
    gamePart = Menu::MahjongPart;
    skipNewStone = false;
    skipNewTurn = false;
    stateGUI.clear();

    if(partWind() == Wind::North && roundWind() == Wind::North)
	return false;
    else
    if(! partWind.isValid() && ! roundWind.isValid())
    {
	// new round, new part
        partWind = Wind(Wind::East);
        roundWind = Wind(Wind::East);
    }
    else
    if(partWind() == Wind::North)
    {
	// new round, new part
	roundWind.shift();
	partWind = Wind(Wind::East);
    }
    else
    {
	// new part
	gamers.shiftWinds();
	partWind.shift();
    }

    currentWind = Wind(Wind::East);
    dropStone = Stone(Stone::None);
    winResult = WinResults();

    battleHistory.clear();

    grantAiDifficultyIncome(false);

    VERBOSE("wind round: " << roundWind.toString());
    VERBOSE("wind part: " << partWind.toString());

    dumpOrderPersons();
    return true;
}

void GameData::dumpOrderPersons(void)
{
    DEBUG("players: ");
    for(auto & id : winds_all)
    {
	LocalPlayer & player = playerOfWind(id);

	DEBUG("wind: " << player.wind.toString() << ", " << "player: " << player.name() << ", " <<
		(! player.isAI() ? " (*)" : "") << ", " << "clan: " << player.clan.canonicalName() << ", " <<
		"stones: " <<  player.stones.toString() << ", " << "new stone: " << player.newStone.toString());
    }
}

LocalPlayer & GameData::playerOfAvatar(const Avatar & avatar)
{
    LocalPlayer* res = gamers.playerOfAvatar(avatar);

    if(! res)
    {
	ERROR("unknown avatar id: " << avatar());
	Engine::except(__FUNCTION__, "exit");
    }

    return *res;
}

LocalPlayer & GameData::playerOfClan(const Clan & clan)
{
    LocalPlayer* res = gamers.playerOfClan(clan);

    if(! res)
    {
	ERROR("unknown clan id: " << clan());
	Engine::except(__FUNCTION__, "exit");
    }

    return *res;
}

LocalPlayer & GameData::playerOfWind(const Wind & wind)
{
    LocalPlayer* res = gamers.playerOfWind(wind);

    if(! res)
    {
	ERROR("unknown wind: " << wind());
	Engine::except(__FUNCTION__, "exit");
    }

    return *res;
}

bool GameData::mahjong2Client(const Avatar & avatar, ActionList & actions)
{
    LocalPlayer & current = playerOfWind(currentWind);
    const auto runAutomatedTurn = [&](bool showGame, bool showKong)
    {
	const WinRules & left = playerOfWind(prevWindCompass(currentWind)).rules;
	const WinRules & right = playerOfWind(nextWindCompass(currentWind)).rules;
	const WinRules & top = playerOfWind(oppositeWindCompass(currentWind)).rules;
	return AI::mahjongTurn(currentWind, current.avatar, croupier.trash,
	                       left, right, top, showGame, showKong, actions);
    };

    if(croupier.hasLuckDraw())
    {
	if(GameData::usesAI(current))
	{
	    WinRules other;
	    const WinRules & left = playerOfWind(prevWindCompass(currentWind)).rules;
	    const WinRules & right = playerOfWind(nextWindCompass(currentWind)).rules;
	    const WinRules & top = playerOfWind(oppositeWindCompass(currentWind)).rules;
	    other.reserve(left.size() + right.size() + top.size());
	    other.insert(other.end(), left.begin(), left.end());
	    other.insert(other.end(), right.begin(), right.end());
	    other.insert(other.end(), top.begin(), top.end());

	    const AI::StrategicIntent intent = AI::chooseStrategicIntent(
	        AI::observePlayer(current.avatar), AI::behaviorProfile(current), aiDifficulty());
	    const int selected = AI::mahjongLuckChoice(current.stones, croupier.luckChoices(),
	                                                  croupier.trash, other, intent);
	    current.newStone = GameStone(croupier.resolveLuckDraw(selected), false);
	    if(0 < stoneLastCount) --stoneLastCount;
	    return runAutomatedTurn(
	        current.isWinMahjong(currentWind, roundWind, dropStone, &winResult),
	        current.isMahjongKong2(currentWind));
	}

	if(current.avatar == avatar)
	{
	    actions.push_back(MahjongLuckChoice(currentWind, croupier.luckChoices()));
	    return true;
	}
	return false;
    }

    if(current.newStone.isValid() || skipNewTurn)
    {
	if(GameData::usesAI(current))
	    return runAutomatedTurn(
	        current.isWinMahjong(currentWind, roundWind, dropStone, &winResult),
	        current.isMahjongKong2(currentWind));

	//DEBUG("wind: " << currentWind.toString() << ", " << "person: " << current.name() << ", " <<
	//	"new stone: " << current.newStone() << ", " << "wait action");
	return false;
    }
    else
    if(dropStone.isValid())
    {
	if(skipRepeatSay)
	{
	    const bool allAI = std::all_of(gamers.begin(), gamers.end(),
	        [](const LocalPlayer & player){ return GameData::usesAI(player); });
	    return allAI ? client2Mahjong(current.avatar, ClientButtonPass(), actions) : false;
	}

	skipRepeatSay = true;

	if(AI::mahjongGameKongPungChao(currentWind, roundWind, dropStone, winResult, actions, true))
	    return true;

	DEBUG("wait player pass" << ", " << "current: " << current.toString());
	return false;
    }

    DEBUG("new turn: " << "last count: " << stoneLastCount);

    if(0 == stoneLastCount)
    {
	winResult = WinResults::drawn(currentWind, roundWind);
	actions.push_back(MahjongEnd(currentWind));
	validateMahjongSummary();
	gamePart = Menu::MahjongSummaryPart;
	return true;
    }

    bool showGame2 = false;
    bool showKong2 = false;

    const bool luckDraw = current.newTurnEvent(croupier, skipNewStone);

    if(! skipNewStone)
    {
	if(luckDraw)
	{
	    if(!GameData::usesAI(current))
	    {
		actions.push_back(MahjongLuckChoice(currentWind, croupier.luckChoices()));
		return true;
	    }

	    WinRules other;
	    const WinRules & left = playerOfWind(prevWindCompass(currentWind)).rules;
	    const WinRules & right = playerOfWind(nextWindCompass(currentWind)).rules;
	    const WinRules & top = playerOfWind(oppositeWindCompass(currentWind)).rules;
	    other.reserve(left.size() + right.size() + top.size());
	    other.insert(other.end(), left.begin(), left.end());
	    other.insert(other.end(), right.begin(), right.end());
	    other.insert(other.end(), top.begin(), top.end());

	    const AI::StrategicIntent intent = AI::chooseStrategicIntent(
	        AI::observePlayer(current.avatar), AI::behaviorProfile(current), aiDifficulty());
	    const int selected = AI::mahjongLuckChoice(current.stones, croupier.luckChoices(),
	                                                  croupier.trash, other, intent);
	    current.newStone = GameStone(croupier.resolveLuckDraw(selected), false);
	}

	stoneLastCount--;

	showGame2 = current.isWinMahjong(currentWind, roundWind, dropStone, & winResult);
	showKong2 = current.isMahjongKong2(currentWind);
    }
    else
    {
	skipNewTurn = true;
    }

    if(GameData::usesAI(current))
    {
	runAutomatedTurn(showGame2, showKong2);
    }
    else
    {
	actions.push_back(MahjongTurn(currentWind, current.newStone, showKong2, showGame2));
	actions.push_back(MahjongData(currentWind));
    }

    return true;
}

bool GameData::clientLuckChoice(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);
    if(client.wind != currentWind)
    {
	ERROR("invalid luck choice outside current turn: " << client.toString());
	return rejectAction(ActionRejectReason::WrongTurn);
    }

    if(GameData::usesAI(client) || client.newStone.isValid() || !croupier.hasLuckDraw())
    {
	ERROR("invalid luck choice: " << client.toString());
	return rejectAction(ActionRejectReason::StaleSelection);
    }

    const auto choice = static_cast<const ClientLuckChoice &>(act);
    const Stone selected = croupier.resolveLuckDraw(choice.index());
    if(!selected.isValid()) return rejectAction(ActionRejectReason::InvalidRuneChoice);

    client.newStone = GameStone(selected, false);
    if(0 < stoneLastCount) stoneLastCount--;

    const bool showGame = client.isWinMahjong(currentWind, roundWind, dropStone, &winResult);
    const bool showKong = client.isMahjongKong2(currentWind);
    actions.push_back(MahjongTurn(currentWind, client.newStone, showKong, showGame));
    actions.push_back(MahjongData(currentWind));
    return true;
}

bool GameData::clientReady(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);
    DEBUG(client.toString());

    actions.push_back(MahjongBegin(currentWind, roundWind, partWind == Wind(Wind::East)));

    return true;
}

bool GameData::clientSayGame(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    // need fill winResult
    if(client.isWinMahjong(currentWind, roundWind, dropStone, & winResult))
    {
	DEBUG(client.toString());

	actions.push_back(MahjongSayGame(client.wind));
	AI::mahjongOtherPass(currentWind, actions, client.wind);
	return true;
    }

    ERROR("isWinMahjong: false");
    return rejectAction(ActionRejectReason::IllegalMahjongCall);
}

bool GameData::clientSayChao(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    if(client.isMahjongChao(currentWind, dropStone))
    {
	actions.push_back(MahjongSayChao(client.wind));
	AI::mahjongOtherPass(currentWind, actions, client.wind);
	return true;
    }

    ERROR("isMahjongChao: false");
    return rejectAction(ActionRejectReason::IllegalMahjongCall);
}

bool GameData::clientSayPung(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    if(client.isMahjongPung(currentWind, dropStone))
    {
	actions.push_back(MahjongSayPung(client.wind));
	AI::mahjongOtherPass(currentWind, actions, client.wind);
	return true;
    }

    ERROR("isMahjongPung: false");
    return rejectAction(ActionRejectReason::IllegalMahjongCall);
}

bool GameData::clientSayKong(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);
    auto action = static_cast<const ClientSayKong &>(act);

    DEBUG(client.toString());

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    if(1 == action.kongType() && client.isMahjongKong1(currentWind, dropStone))
    {
	actions.push_back(MahjongSayKong(client.wind));
	AI::mahjongOtherPass(currentWind, actions, client.wind);
	return true;
    }
    else
    if(2 == action.kongType() && client.isMahjongKong2(currentWind))
    {
	actions.push_back(MahjongSayKong(client.wind));
	return true;
    }

    ERROR("isMahjongKong: false");
    return rejectAction(ActionRejectReason::IllegalMahjongCall);
}

bool GameData::clientButtonGame(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    WinResults validatedResult;
    if(!client.isWinMahjong(currentWind, roundWind, dropStone, &validatedResult))
    {
	ERROR("invalid Mahjong Game claim: " << client.toString());
	return rejectAction(client.isSilenced() ? ActionRejectReason::Silenced :
	                    ActionRejectReason::IllegalMahjongCall);
    }

    winResult = validatedResult;

    client.setMahjongGame(winResult);

    const int score = winResult.totalScore();
    if(0 < score)
    {
        for(const auto & fine : winResult.opponentFines())
        {
            const LocalPlayer & opponent = playerOfWind(fine.wind());
            client.addLandClaimPoints(opponent.clan, score * fine.value());
        }
    }

    actions.push_back(MahjongGame(client.wind));
    AI::mahjongOtherPass(currentWind, actions, client.wind);
    actions.push_back(MahjongData(currentWind));

    actions.push_back(MahjongEnd(currentWind));
    validateMahjongSummary();
    gamePart = Menu::MahjongSummaryPart;

    return true;
}

bool GameData::clientButtonPass(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(AI::mahjongGameKongPungChao(currentWind, roundWind, dropStone, winResult, actions, false))
	return true;

	const bool allAI = std::all_of(gamers.begin(), gamers.end(),
	    [](const LocalPlayer & player){ return GameData::usesAI(player); });
	if(!GameData::usesAI(client) || allAI)
	{
	    currentWind.shift();
	    croupier.put(dropStone);
	    dropStone = Stone(Stone::None);
    }

    actions.push_back(MahjongData(currentWind));
    skipRepeatSay = false;
    return true;
}

bool GameData::clientButtonPung(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(client.isSilenced()) return rejectAction(ActionRejectReason::Silenced);
    if(!client.isMahjongPung(currentWind, dropStone))
	return rejectAction(ActionRejectReason::IllegalMahjongCall);

    actions.push_back(MahjongPung(client.wind, dropStone));
    client.setMahjongPung(dropStone);
    dropStone.reset();
    currentWind = client.wind;
    actions.push_back(MahjongData(currentWind));
    skipNewStone = true;

    return true;
}

bool GameData::clientButtonKong1(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(client.isSilenced()) return rejectAction(ActionRejectReason::Silenced);
    if(!client.isMahjongKong1(currentWind, dropStone))
	return rejectAction(ActionRejectReason::IllegalMahjongCall);

    actions.push_back(MahjongKong1(client.wind, dropStone));
    client.setMahjongKong1(dropStone);
    dropStone.reset();
    currentWind = client.wind;
    actions.push_back(MahjongData(currentWind));
    // An exposed Kong, unlike Pung or Chao, receives a replacement draw.
    // Skipping it leaves the caller one rune short and can eventually empty
    // the concealed hand before the part is over.
    skipNewStone = false;

    return true;
}

bool GameData::clientButtonKong2(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    DEBUG(client.toString());

    if(client.isSilenced()) return rejectAction(ActionRejectReason::Silenced);
    if(!client.isMahjongKong2(currentWind))
	return rejectAction(ActionRejectReason::IllegalMahjongCall);

    actions.push_back(MahjongKong2(client.wind));
    client.setMahjongKong2();
    actions.push_back(MahjongData(currentWind));

    return true;
}

bool GameData::clientChaoVariant(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    auto ca = static_cast<const ClientChaoVariant &>(act);

    DEBUG(client.toString() << ", " << "variant: " << ca.chaoVariant());

    if(client.isSilenced()) return rejectAction(ActionRejectReason::Silenced);
    const Stones variants = client.stones.findChaoVariants(dropStone);
    if(!client.isMahjongChao(currentWind, dropStone) ||
       ca.chaoVariant() < 0 || variants.size() <= ca.chaoVariant())
	return rejectAction(ActionRejectReason::IllegalMahjongCall);

    actions.push_back(MahjongChao(client.wind, dropStone));
    client.setMahjongChao(dropStone, ca.chaoVariant());
    dropStone.reset();
    currentWind = client.wind;
    actions.push_back(MahjongData(currentWind));
    skipNewStone = true;

    return true;
}

bool GameData::clientDropIndex(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    auto ca = static_cast<const ClientDropIndex &>(act);

    DEBUG(client.toString() << ", " << "index: " << ca.dropIndex() << ", " << "stones: " << client.stones.toString());

    if(currentWind != client.wind) return rejectAction(ActionRejectReason::WrongTurn);

    if(dropStone.isValid())
    {
	ERROR("drop stone: " << dropStone() << ", " << "(" << dropStone.toString() << ")");
	return rejectAction(ActionRejectReason::StaleSelection);
    }

    if(ca.dropIndex() < 0 || client.stones.size() < ca.dropIndex() ||
       (client.stones.size() == ca.dropIndex() && !client.newStone.isValid()))
	return rejectAction(ActionRejectReason::InvalidRuneChoice);

    dropStone = client.setMahjongDrop(ca.dropIndex());
    actions.push_back(MahjongDrop(currentWind, dropStone));
    actions.push_back(MahjongData(currentWind));

    DEBUG("drop stone: " << dropStone() << ", " << "(" << dropStone.toString() << ")");

    AI::mahjongGameKongPungChao(currentWind, roundWind, dropStone, winResult, actions, true);
    skipRepeatSay = true;
    skipNewStone = false;
    skipNewTurn = false;

    return true;
}

bool GameData::clientSummonCreature(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    if(currentWind != client.wind)
    {
	ERROR("wind not current: " << client.wind.toString());
	return rejectAction(ActionRejectReason::WrongTurn);
    }

    auto ca = static_cast<const ClientSummonCreature &>(act);

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    if(client.isAffectedSpell(Spell::ManaFog) && !ca.isForce())
    {
	ERROR("player mana fog mode: " << client.name());
	return rejectAction(ActionRejectReason::ManaFog);
    }

    if(client.isCasted() && !ca.isForce())
    {
	ERROR("player casted: " << client.name());
	return rejectAction(ActionRejectReason::AlreadyCast);
    }

    if(client.army.isMaximumSummoning())
    {
	ERROR("player summmoning maximum: " << client.name());
	return rejectAction(ActionRejectReason::ArmyFull);
    }

    Creature creature = ca.creature();
    Land land = ca.land();

    if(! land.isValid())
    {
	ERROR("land invalid: " << land.toString());
	return rejectAction(ActionRejectReason::InvalidLand);
    }

    const LandInfo & landInfo = GameData::landInfo(land);

    if(! landInfo.stat.power || (! land.isTowerWinds() && landInfo.clan != client.clan))
    {
	ERROR("land incorrect: " << land.toString());
	return rejectAction(ActionRejectReason::InvalidLand);
    }

    const CreatureInfo & creatureInfo = GameData::creatureInfo(creature);
    if(! client.stones.allowCast(creatureInfo.stones, client.newStone) && !ca.isForce())
    {
	ERROR("player can not cast rule: " << creatureInfo.stones.toString());
	return rejectAction(ActionRejectReason::MissingRunes);
    }

    if(creatureInfo.unique && GameData::findCreatureUnique(creature))
    {
	ERROR("unique creature found on map, there can be only one!");
	return rejectAction(ActionRejectReason::UniqueCreatureExists);
    }

    if(client.points < creatureInfo.cost && !ca.isForce())
    {
	ERROR("points error: " << client.points << ", " << creatureInfo.cost);
	return rejectAction(ActionRejectReason::InsufficientPoints, client.points, creatureInfo.cost);
    }

    if(! client.army.join(creature, land))
	return rejectAction(ActionRejectReason::PartyFull);

    if(!ca.isForce())
    {
	client.points -= creatureInfo.cost;
	client.stones.setCasted(creatureInfo.stones, client.newStone);
	client.setCasted(true);
    }

    actions.push_back(MahjongSummon(currentWind, creature, land));
    actions.push_back(MahjongData(currentWind));

    DEBUG(client.toString() << ", " << "creature: " << creature.toString() << ", " << "land: " << land.toString());
    return true;
}

bool GameData::clientCastSpell(const Avatar & avatar, const ClientMessage & act, ActionList & actions)
{
    LocalPlayer & client = playerOfAvatar(avatar);

    if(currentWind != client.wind)
    {
	ERROR("wind not current: " << client.wind.toString());
	return rejectAction(ActionRejectReason::WrongTurn);
    }

    auto ca = static_cast<const ClientCastSpell &>(act);

    if(client.isSilenced())
    {
	ERROR("player silence mode: " << client.name());
	return rejectAction(ActionRejectReason::Silenced);
    }

    if(client.isAffectedSpell(Spell::ManaFog) && !ca.isForce())
    {
	ERROR("player mana fog mode: " << client.name());
	return rejectAction(ActionRejectReason::ManaFog);
    }

    if(client.isCasted() && !ca.isForce())
    {
	ERROR("player already casted: " << client.name());
	return rejectAction(ActionRejectReason::AlreadyCast);
    }

    Spell spell = ca.spell();

    const SpellInfo & spellInfo = GameData::spellInfo(spell);

    if(! client.allowCastSpell(spell) && ! ca.isForce())
    {
	if(spellInfo.stones.size())
	{
	    ERROR("player can not cast rule: " << spellInfo.stones.toString());
	}

	ERROR("player can not cast spell: " << spell.toString());
	return rejectAction(spellInfo.stones.size() ? ActionRejectReason::MissingRunes :
	                    ActionRejectReason::InvalidTarget);
    }

    if(client.points < spellInfo.cost)
    {
	ERROR("points error: " << client.points << ", " << spellInfo.cost);
	return rejectAction(ActionRejectReason::InsufficientPoints, client.points, spellInfo.cost);
    }

    if(spellInfo.target() == SpellTarget::AllPlayers)
    {
	DEBUG(client.toString() << ", " << "spell: " << spell.toString());

	actions.push_back(MahjongCast(currentWind, spell));
	for(auto & player : gamers)
	    player.mahjongApplySpell(spell, client.avatar);
    }
    else
    if(spellInfo.target() == SpellTarget::MyPlayer)
    {
	DEBUG(client.toString() << ", " << "spell: " << spell.toString());

	actions.push_back(MahjongCast(currentWind, spell));
	client.mahjongApplySpell(spell, client.avatar);
    }
    else
    if(spellInfo.target() == SpellTarget::OtherPlayer)
    {
	Avatar target = ca.target();
	DEBUG(client.toString() << ", " << "spell: " << spell.toString() << ", " << "target: " << target.toString());
	LocalPlayer* targetPlayer = gamers.playerOfAvatar(target);

	if(! targetPlayer || target == client.avatar)
	{
	    ERROR("other player target invalid: " << target.toString());
	    return rejectAction(ActionRejectReason::InvalidTarget);
	}

	actions.push_back(MahjongCast(currentWind, spell, target));
	targetPlayer->mahjongApplySpell(spell, client.avatar);
    }
    else
    {
	Land land = ca.land();
	int unit = ca.unit();

	DEBUG(client.toString() << ", " << "spell: " << spell.toString() << ", " <<
	    "land: " << land.toString() << ", " << "unit: " << String::hex(unit, 8) << ", " << "spell effect: " << spellInfo.effect.toString());

	BattleTargets targets;
	targets.reserve(10);

	if(spell() == Spell::Teleport)
	{
	    BattleCreature* target = client.army.findBattleUnit(unit);
	    const bool friendlyLand = land.isValid() &&
		(land.isTowerWinds() || GameData::landInfo(land).clan == client.clan);

	    if(! target || ! friendlyLand || ! client.army.teleportCreature(*target, land))
	    {
		ERROR("teleport target error" << ", " << "unit: " << String::hex(unit, 8) << ", " << "land: " << land.toString());
		return rejectAction(!target ? ActionRejectReason::UnitNotFound :
		                    (!friendlyLand ? ActionRejectReason::InvalidLand :
		                                     ActionRejectReason::PartyFull));
	    }

	    targets << client.army.findBattleUnit(unit);
	}
	else
	if(spellInfo.target() == SpellTarget::Land)
	{
	    for(auto & lp : gamers)
	    {
		auto party = lp.army.findPartyConst(land);
		if(party) targets << BattleTargets(party->toBattleCreatures());
	    }
	}
	else
	if(spellInfo.target() & SpellTarget::Party)
	{
	    BattleParty* party = 0 < unit ? getBattleParty(unit) : nullptr;
	    const bool friendly = party && party->clan() == client.clan;
	    const bool allowed = party && party->land() == land &&
		(((spellInfo.target() & SpellTarget::Friendly) && friendly) ||
		 ((spellInfo.target() & SpellTarget::Enemy) && !friendly));

	    if(! allowed)
	    {
		ERROR("party spell target invalid" << ", " << "unit: " << String::hex(unit, 8) << ", " << "land: " << land.toString());
		return rejectAction(ActionRejectReason::InvalidTarget);
	    }

	    targets << BattleTargets(party->toBattleCreatures());
	}
	else
	if(unit)
	{
	    BattleCreature* bcr = getBattleCreature(unit);
	    BattleParty* party = getBattleParty(unit);
	    const bool friendly = bcr && bcr->clan() == client.clan;
	    const bool allowed = bcr && party && party->land() == land && bcr->canReceiveSpell(spell) &&
		(((spellInfo.target() & SpellTarget::Friendly) && friendly) ||
		 ((spellInfo.target() & SpellTarget::Enemy) && !friendly));

	    if(! allowed)
	    {
		ERROR("unit spell target invalid" << ", " << "unit: " << String::hex(unit, 8) << ", " << "land: " << land.toString());
		return rejectAction(bcr ? ActionRejectReason::InvalidTarget : ActionRejectReason::UnitNotFound);
	    }

	    targets.push_back(bcr);
	}

	if(targets.empty())
	{
	    ERROR("spell has no valid targets: " << spell.toString());
	    return rejectAction(ActionRejectReason::InvalidTarget);
	}

	std::vector<int> resistence;
	resistence.reserve(5);

	if(spell() != Spell::Teleport)
	{
	    for(auto & tgt : targets)
	    {
		if(! tgt->applySpell(spell))
		    resistence.push_back(tgt->battleUnit());
	    }

	    // Serialize target ids before dead creatures are removed from their parties.
	    actions.push_back(MahjongCast(currentWind, spell, land, targets, resistence));

	    std::set<Clan> checkArmy;

	    // check dead creatures
	    for(auto & tgt : targets)
	    {
		if(0 >= tgt->loyalty())
		{
		    const LocalPlayer & other = playerOfClan(tgt->clan());
		    const std::string info = StringFormat(_("%1's %2 was vanquished"))
		        .arg(other.name())
		        .arg(GameData::creatureInfo(static_cast<const BattleCreature &>(*tgt)).name);
		    actions.push_back(MahjongInfo(currentWind, info));
		    checkArmy.insert(tgt->clan());
		}
	    }

	    for(auto & clan : checkArmy)
	    {
		LocalPlayer & other = playerOfClan(clan);
		other.army.removeUnloyalty();
	    }
	}
	else
	    actions.push_back(MahjongCast(currentWind, spell, land, targets, resistence));
    }

    // client apply
    client.points -= spellInfo.cost;
    client.stones.setCasted(spellInfo.stones, client.newStone);
    client.setCasted(true);

    // copy all data
    actions.push_back(MahjongData(currentWind));
    return true;
}

bool GameData::client2Mahjong(const Avatar & avatar, const ClientMessage & act, ActionList & actions,
                              ActionRejection* rejection)
{
    ScopedActionRejection rejectionScope(rejection);
    const std::size_t actionsBefore = actions.size();
    const bool recordAction = Replay::actionRecordingEnabled();
    const JsonObject beforeState = recordAction ? authoritativeState() : JsonObject();
    CrashReport::breadcrumb(std::string("GameData phase=Mahjong stage=before avatar=")
        .append(avatar.toString()).append(" type=").append(String::number(act.type()))
        .append(" payload=").append(act.toString()));

    bool accepted = false;
    bool recognized = true;
    switch(act.type())
    {
	case Action::ClientReady:	accepted = clientReady(avatar, act, actions); break;
	case Action::ClientSayGame:	accepted = clientSayGame(avatar, act, actions); break;
	case Action::ClientSayChao:	accepted = clientSayChao(avatar, act, actions); break;
	case Action::ClientSayPung:	accepted = clientSayPung(avatar, act, actions); break;
	case Action::ClientSayKong:	accepted = clientSayKong(avatar, act, actions); break;
	case Action::ClientButtonGame:	accepted = clientButtonGame(avatar, act, actions); break;
	case Action::ClientButtonPass:	accepted = clientButtonPass(avatar, act, actions); break;
	case Action::ClientButtonPung:	accepted = clientButtonPung(avatar, act, actions); break;
	case Action::ClientButtonKong1:	accepted = clientButtonKong1(avatar, act, actions); break;
	case Action::ClientButtonKong2:	accepted = clientButtonKong2(avatar, act, actions); break;
	case Action::ClientChaoVariant:	accepted = clientChaoVariant(avatar, act, actions); break;
	case Action::ClientDropIndex:	accepted = clientDropIndex(avatar, act, actions); break;
	case Action::ClientLuckChoice:	accepted = clientLuckChoice(avatar, act, actions); break;
	case Action::ClientSummonCreature: accepted = clientSummonCreature(avatar, act, actions); break;
	case Action::ClientCastSpell:	accepted = clientCastSpell(avatar, act, actions); break;

	default: recognized = false; rejectAction(ActionRejectReason::UnknownAction); break;
    }

    if(recognized && !accepted && (!rejection || !rejection->isValid()))
        rejectAction(ActionRejectReason::UnknownAction);

    if(recognized && accepted && recordAction)
        Replay::recordAcceptedAction(beforeState, avatar, act, authoritativeState());

    CrashReport::breadcrumb(std::string("GameData phase=Mahjong stage=after avatar=")
        .append(avatar.toString()).append(" type=").append(String::number(act.type()))
        .append(" recognized=").append(recognized ? "true" : "false")
        .append(" accepted=").append(accepted ? "true" : "false")
        .append(" rejection=").append(rejection ? actionRejectReasonName(rejection->reason) : "not_requested")
        .append(" emitted=").append(String::number(static_cast<int>(actions.size() - actionsBefore))));
    return accepted;
}

void GameData::validateMahjongSummary(void)
{
    int total = 0;

    for(auto & id : winds_all)
    {
	const LocalPlayer & player = GameData::playerOfWind(id);

        DEBUG(player.toString() << ", " <<
                "stones: " <<  player.stones.toString() << ", " << "rules: " << player.rules.toString());

	total += player.stones.size();
	total += player.rules.count();

	if(player.newStone.isValid())
	{
	    DEBUG("new stone: " << player.newStone());
	    total += 1;
	}
    }

    if(dropStone.isValid())
    {
	DEBUG("drop stone: " << dropStone());
	total += 1;
    }

    DEBUG("croupier trash: " << croupier.trash.toString());
    DEBUG("croupier bank: " << croupier.bank.toString());
    total += croupier.trash.size() + croupier.bank.size();
    total += croupier.luckDraw.size();

    DEBUG("game total: " << total << ", " << "(" << (total != 136 ? "FALSE" : "TRUE") << ")");
}
