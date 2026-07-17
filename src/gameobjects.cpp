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

#include <set>
#include <cstring>
#include <numeric>
#include <sstream>
#include <forward_list>
#include <algorithm>
#include <utility>

#include "gameplayrng.h"
#include "gamedata.h"

std::initializer_list<Clan::clan_t> clans_all = { Clan::Red, Clan::Yellow, Clan::Aqua, Clan::Purple };
std::initializer_list<Wind::wind_t> winds_all = { Wind::East, Wind::South, Wind::West, Wind::North };
std::initializer_list<Land::land_t> lands_all = { Land::TowerOf4Winds, Land::Maithaius, Land::Baliphon, Land::Vermille, Land::Sulanthia,
		Land::Trojensek, Land::Talon, Land::Siramak, Land::Ronzinol, Land::Corzen, Land::Greenbaw, Land::Zubrus, Land::Corimar,
		Land::Inkartha, Land::Hexan, Land::Firland, Land::Vesna, Land::Kern, Land::RegencyPeaks, Land::Knighthaven, Land::Rikter, Land::Gorok,
		Land::Suntura, Land::Bertram, Land::Mastenbroek, Land::Reisse, Land::Cirrus, Land::Grosek, Land::Chahrr, Land::Saugrezz, Land::Mahnjeet,
		Land::Trassk, Land::Bechnarr, Land::Azuria, Land::PyramusReach, Land::SerenityPlains, Land::Sunspot, Land::CharmersExpanse, Land::GambitsRun,
		Land::AshPoint, Land::OrchidBay, Land::Mocklebury, Land::CelestialWood, Land::TortoiseIsle, Land::SiphonsChute };
std::initializer_list<Avatar::avatar_t> avatars_all = { Avatar::Orachi, Avatar::Lakkho, Avatar::Dayla, Avatar::Ziag, Avatar::Niana, Avatar::Kierac,
		Avatar::Logun, Avatar::Nucrus, Avatar::Javed };

namespace
{
    auto _ids = { "index:winds", "index:clans", "index:abilities", "index:specials",
			    "index:avatars", "index:spells", "index:creatures", "index:lands" };

    std::array<StringList, 8> idsList;

    const StringList & windsId = idsList[0];
    const StringList & clansId = idsList[1];
    const StringList & abilitiesId = idsList[2];
    const StringList & specialsId = idsList[3];
    const StringList & avatarsId = idsList[4];
    const StringList & spellsId = idsList[5];
    const StringList & creaturesId = idsList[6];
    const StringList & landsId = idsList[7];

    const char* legacyNamedClanId(const Clan & clan)
    {
        switch(clan())
        {
            case Clan::Red:    return "maitha";
            case Clan::Yellow: return "kartha";
            case Clan::Aqua:   return "iz";
            case Clan::Purple: return "marz";
            default: break;
        }
        return "none";
    }
}

namespace GameData
{
    extern int bonusStart;
    extern int bonusGame;
    extern int bonusKong;
    extern int bonusPung;
    extern int bonusChao;
    extern int bonusPass;

    bool loadIndexes(const JsonObject & jo)
    {
	int index = 0;

        // load game indexes
        for(auto & id : _ids)
        {
            if(jo.isArray(id))
                idsList[index] = jo.getStdList<std::string>(id);
            else
            {
                ERROR("index.json incorrect: " << id);
                return false;
            }

	    index++;
        }

	return true;
    }
}

Wind::Wind(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(windsId.begin(), windsId.end(), str);
	if(it != windsId.end()) val = std::distance(windsId.begin(), it);
	else ERROR("unknown wind id: " << str);
    }
}

Wind Wind::prev(void) const
{
    switch(id())
    {
	case East:	return North;
	case South:	return East;
	case West:	return South;
	case North:	return West;
        default: break;
    }
    return None;
}

Wind Wind::next(void) const
{
    switch(id())
    {
	case East:	return South;
	case South:	return West;
	case West:	return North;
	case North:	return East;
        default: break;
    }
    return None;
}

std::string Wind::toString(void) const
{
    auto it = windsId.begin();
    std::advance(it, index());
    return it != windsId.end() ? *it : "none";
}

Clan::Clan(const std::string & str) : Enum(None)
{
    if(str == "Red" || str == "red" || str == "Maitha" || str == "maitha") val = Red;
    else if(str == "Yellow" || str == "yellow" || str == "Kartha" || str == "kartha") val = Yellow;
    else if(str == "Aqua" || str == "aqua" || str == "Iz" || str == "iz") val = Aqua;
    else if(str == "Purple" || str == "purple" || str == "Marz" || str == "marz") val = Purple;
    else if(str != "none")
    {
	auto it = std::find(clansId.begin(), clansId.end(), str);
	if(it != clansId.end()) val = std::distance(clansId.begin(), it);
	else ERROR("unknown clan id: " << str);
    }
}

Clan Clan::prev(void) const
{
    switch(id())
    {
	case Red:	return Purple;
	case Yellow:	return Red;
	case Aqua:	return Yellow;
	case Purple:	return Aqua;
	default: break;
    }
    return None;
}

Clan Clan::next(void) const
{
    switch(id())
    {
	case Red:	return Yellow;
	case Yellow:	return Aqua;
	case Aqua:	return Purple;
	case Purple:	return Red;
	default: break;
    }
    return None;
}

std::string Clan::toString(void) const
{
    auto it = clansId.begin();
    std::advance(it, index());
    return it != clansId.end() ? *it : "none";
}

std::string Clan::canonicalName(void) const
{
    switch(id())
    {
        case Red:    return "Red";
        case Yellow: return "Yellow";
        case Aqua:   return "Aqua";
        case Purple: return "Purple";
        default: break;
    }
    return "None";
}

Clan Clan::random(void)
{
    auto res = GameplayRng::choose(clans_all.begin(), clans_all.end());
    return *res;
}

Speciality::Speciality(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(specialsId.begin(), specialsId.end(), str);
	if(it != specialsId.end()) val = std::distance(specialsId.begin(), it);
	else ERROR("unknown speciality id: " << str);
    }
}

int Speciality::index(void) const
{
    switch(id())
    {
	case Swarm:		return 1;
	case Merge:		return 2;
	case Invisibility:	return 3;
	case Regeneration:	return 4;
	case CastHellblast:	return 5;
	case MagicResistence:	return 6;
	case MightyBlow:	return 7;
	case Gate:		return 8;
	case FirstStrike:	return 9;
	case SeeInvisible:	return 10;
	case IgnoreMissiles:	return 11;
	case Devotion:		return 12;
        case FireShield:	return 13;
	case CastDrawNumber:	return 14;
	case CastDrawSword:	return 15;
	case CastDrawSkull:	return 16;
	case CastRandomDiscard:	return 17;
	case CastSilence:	return 18;
	case CastScryRunes:	return 19;
	case CastManaFog:	return 20;
	case RangerAttack:	return 21;
	default: break;
    }
    return 0;
}

Speciality Speciality::fromIndex(int val)
{
    switch(val)
    {
	case 1:		return Swarm;
	case 2:		return Merge;
	case 3:		return Invisibility;
	case 4:		return Regeneration;
	case 5:		return CastHellblast;
	case 6:		return MagicResistence;
	case 7:		return MightyBlow;
	case 8:		return Gate;
	case 9:		return FirstStrike;
	case 10:	return SeeInvisible;
	case 11:	return IgnoreMissiles;
	case 12:	return Devotion;
        case 13:	return FireShield;
	case 14:	return CastDrawNumber;
	case 15:	return CastDrawSword;
	case 16:	return CastDrawSkull;
	case 17:	return CastRandomDiscard;
	case 18:	return CastSilence;
	case 19:	return CastScryRunes;
	case 20:	return CastManaFog;
	case 21:	return RangerAttack;
	default: break;
    }
    return None;
}

Spell Speciality::toSpell(void) const
{
    switch(id())
    {
	case CastHellblast:	return Spell::HellBlast;
	case CastDrawNumber:	return Spell::DrawNumber;
	case CastDrawSword:	return Spell::DrawSword;
	case CastDrawSkull:	return Spell::DrawSkull;
	case CastRandomDiscard:	return Spell::RandomDiscard;
	case CastSilence:	return Spell::Silence;
	case CastScryRunes:	return Spell::ScryRunes;
	case CastManaFog:	return Spell::ManaFog;
	default: break;
    }

    return Spell::None;
}

std::string Speciality::toString(void) const
{
    auto it = specialsId.begin();
    std::advance(it, index());
    return it != specialsId.end() ? *it : "none";
}

int SpecialityDevotion::restore(void) const
{
    return 2;
}

int SpecialityMagicResistence::chance(int creatureId) const
{
    switch(creatureId)
    {
	case Creature::MazRa:	  return 25;
	case Creature::KingDrago: return 90;
	default: break;
    }

    return 0;
}

int SpecialityMightyBlow::chance(void) const
{
    return 25;
}

int SpecialityMightyBlow::strength(void) const
{
    return 3;
}

/* Specials */
Specials::Specials(const StringList & list)
{
    for(auto & str : list)
	set(String::trimmed(str));
}

Specials Specials::allCastSpells(void)
{
    return Specials() << Speciality::CastHellblast << Speciality::CastDrawNumber <<
                Speciality::CastDrawSword << Speciality::CastDrawSkull << Speciality::CastRandomDiscard <<
                Speciality::CastSilence << Speciality::CastScryRunes << Speciality::CastManaFog;
}

Specials & Specials::operator<< (const Speciality & sp)
{
    set(sp);
    return *this;
}

void Specials::set(const Speciality & sp)
{
    std::bitset<32>::set(sp.index(), true);
}

void Specials::reset(const Speciality & sp)
{
    std::bitset<32>::set(sp.index(), false);
}

bool Specials::check(const Speciality & sp) const
{
    return std::bitset<32>::test(sp.index());
}

/*
int Specials::counts(void) const
{
    return countBits();
}
*/

std::list<Speciality> Specials::toList(void) const
{
    std::list<Speciality> res;

    for(int pos = 0; pos < 31; ++pos)
	if(test(pos))
	res.push_back(Speciality::fromIndex(pos));

    return res;
}

Ability::Ability(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(abilitiesId.begin(), abilitiesId.end(), str);
	if(it != abilitiesId.end()) val = std::distance(abilitiesId.begin(), it);
	else ERROR("unknown ability id: " << str);
    }
}

std::string Ability::toString(void) const
{
    auto it = abilitiesId.begin();
    std::advance(it, index());
    return it != abilitiesId.end() ? *it : "none";
}

Avatar::Avatar(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(avatarsId.begin(), avatarsId.end(), str);
	if(it != avatarsId.end()) val = std::distance(avatarsId.begin(), it);
	else ERROR("unknown avatar id: " << str);
    }
}

std::string Avatar::toString(void) const
{
    auto it = avatarsId.begin();
    std::advance(it, index());
    return it != avatarsId.end() ? *it : "none";
}

Avatar Avatar::random(void)
{
    switch(GameplayRng::uniform(1, 9))
    {
	case 1: return Orachi;
	case 2:	return Lakkho;
	case 3: return Dayla;
	case 4: return Ziag;
	case 5: return Niana;
	case 6: return Kierac;
	case 7: return Logun;
	case 8: return Nucrus;
	case 9: return Javed;
	default: break;
    }

    return None;
}

Spell::Spell(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(spellsId.begin(), spellsId.end(), str);
	if(it != spellsId.end()) val = std::distance(spellsId.begin(), it);
	else ERROR("unknown spell id: " << str);
    }
}

std::string Spell::toString(void) const
{
    auto it = spellsId.begin();
    std::advance(it, index());
    return it != spellsId.end() ? *it : "none";
}

SpellTarget::SpellTarget(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	if(str == "friendly")		val = Friendly;
	else
	if(str == "enemy")		val = Enemy;
	else
	if(str == "any")		val = Any;
	else
	if(str == "friendly_party")	val = Friendly|Party;
	else
	if(str == "enemy_party")	val = Enemy|Party;
	else
	if(str == "land")		val = Land;
	else
	if(str == "my_player")		val = MyPlayer;
	else
	if(str == "other_player")	val = OtherPlayer;
	else
	if(str == "all_players")	val = AllPlayers;
	else ERROR("unknown target id: " << str);
    }
}


int SpellTarget::index(void) const
{
    switch(val)
    {
	case Friendly:	return 1;
	case Enemy:	return 2;
	case Any:	return 3;
	case Friendly|Party:	return 5;
	case Enemy|Party:	return 6;
	case Land:	return 7;
	case MyPlayer:	return 8;
	case OtherPlayer: return 9;
	case AllPlayers: return 10;
	default: break;
    }

    return 0;
}

std::string SpellTarget::toString(void) const
{
    switch(val)
    {
	case Friendly:	return "friendly";
	case Enemy:	return "enemy";
	case Any:	return "any";
	case Friendly|Party:	return "friendly_party";
	case Enemy|Party:	return "enemy_party";
	case Land:	return "land";
	case MyPlayer:	return "my_player";
	case OtherPlayer:	return "other_player";
	case AllPlayers: return "all_players";
	default: break;
    }

    return "none";
}

Creature::Creature(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(creaturesId.begin(), creaturesId.end(), str);
	if(it != creaturesId.end()) val = std::distance(creaturesId.begin(), it);
	else ERROR("unknown creature id: " << str);
    }
}

std::string Creature::toString(void) const
{
    auto it = creaturesId.begin();
    std::advance(it, index());
    return it != creaturesId.end() ? *it : "none";
}

Land::Land(const std::string & str) : Enum(None)
{
    if(str != "none")
    {
	auto it = std::find(landsId.begin(), landsId.end(), str);
	if(it != landsId.end()) val = std::distance(landsId.begin(), it);
	else ERROR("unknown land id: " << str);
    }
}

std::string Land::toString(void) const
{
    auto it = landsId.begin();
    std::advance(it, index());
    return it != landsId.end() ? *it : "none";
}

bool Land::isTowerWinds(void) const
{
    return id() == TowerOf4Winds;
}

bool Land::isPower(void) const
{
    if(isValid())
    {
	const LandInfo & info = GameData::landInfo(*this);
	return info.stat.power;
    }
    return false;
}

/* Lands */
Lands Lands::powerOnly(void) const
{
    Lands res;
    res.assign(begin(), end());

    auto itend = std::remove_if(res.begin(), res.end(),
			[](const Land & land){ return ! land.isPower(); });

    res.erase(itend, res.end());
    res.emplace_back(Land::TowerOf4Winds);

    return res;
}

Lands Lands::thisClan(const Clan & clan)
{
    Lands res;

    for(auto & id : lands_all)
    {
	const LandInfo & info = GameData::landInfo(id);
	if(info.clan == clan) res << info.id;
    }

    return res;
}

Lands Lands::enemyAroundOnly(const Clan & clan)
{
    Lands enemyLands;
    Lands clanLands = thisClan(clan);

    for(auto it1 = clanLands.begin(); it1 != clanLands.end(); ++it1)
    {
	const LandInfo & info1 = GameData::landInfo(*it1);
	const auto & aroundLands = info1.borders;

	for(auto it2 = aroundLands.begin(); it2 != aroundLands.end(); ++it2)
	{
	    const LandInfo & info2 = GameData::landInfo(*it2);
	    if(info2.clan != clan) enemyLands << info2.id;
	}
    }

    std::sort(enemyLands.begin(), enemyLands.end());
    enemyLands.resize(std::distance(enemyLands.begin(), std::unique(enemyLands.begin(), enemyLands.end())));

    return enemyLands;
}

struct LandCost
{
    Land	owner;
    Land	parent;
    int		cost;
    bool	open;

    LandCost(const Land & land, const Land & prnt, bool st, int val) : owner(land), parent(prnt), cost(val), open(st) {}
};

struct LandCostCompare
{
    bool operator() (const LandCost & lc1, const LandCost & lc2) const
    {
	if(lc1.open)
	{
	    if(! lc2.open)
		return true;

	    if(lc1.cost == lc2.cost)
		return lc1.owner < lc2.owner;

	    return lc1.cost < lc2.cost;
	}

	if(lc2.open)
	    return false;

	if(lc1.cost == lc2.cost)
	    return lc1.owner < lc2.owner;

	return lc1.cost < lc2.cost;
    }
};

Lands Lands::pathfind(const Land & from, const Land & to)
{
    std::set<LandCost, LandCostCompare> wave;

    auto pair = wave.emplace(from, Land::None, false, 0);
    auto cur = pair.first;

    while((*cur).owner != to)
    {
	for(auto & land : GameData::landInfo((*cur).owner).borders)
	{
	    auto tmp = std::find_if(wave.begin(), wave.end(),
				    [&](auto & landCost){ return landCost.owner == land; });
	    int cost = 10;

	    if(tmp == wave.end())
	    {
		wave.emplace(land, (*cur).owner, true, (*cur).cost + cost);
	    }
	    else
	    if((*cur).open &&
		(*tmp).cost > (*cur).cost + cost)
	    {
		wave.erase(tmp);
		wave.emplace(land, (*cur).owner, true, (*cur).cost + cost);
	    }
	}

	//cur close
        if((*cur).open)
        {
            pair = wave.emplace((*cur).owner, (*cur).parent, false, (*cur).cost);
            wave.erase(cur);
            cur = pair.first;
        }

        // std::set sorted
        // return first small cost
        cur = wave.begin();
        if(cur == wave.end() || ! (*cur).open)
        {
            // not found, and exception
            ERROR("not found");
            break;
        }
    }

    std::forward_list<Land> res;

    if((*cur).owner == to)
    {
	while(cur != wave.end() &&
	    (*cur).owner != from)
	{
	    res.push_front((*cur).owner);
	    cur = std::find_if(wave.begin(), wave.end(),
                                    [&](auto & landCost){ return landCost.owner == (*cur).parent; });
	}
    }

    return std::vector<Land>(res.begin(), res.end());
}

std::string Lands::toString(void) const
{
    std::ostringstream os;
    os << "[";
    for(auto it = begin(); it != end(); ++it)
	os << (*it).toString() << ",";
    os << "]";
    return os.str();
}

/* Stone */
Stone::Stone(stone_t v) : Enum(v)
{
    if(v && stoneType() == StoneType::None)
	ERROR("unknown stone id: " << v);
}

Stone::Stone(const std::string & str) : Enum(None)
{
    if(str.size() && str != "none")
    {
	int decimal = 0;
	int order = 0;

	if(0 == str.compare(0, str.size() - 1, "skull"))
	{
	    decimal = 1;
	    order = String::toInt(str.substr(str.size() - 1, 1));
	}
	else
	if(0 == str.compare(0, str.size() - 1, "sword"))
	{
	    decimal = 2;
	    order = String::toInt(str.substr(str.size() - 1, 1));
	}
	else
	if(0 == str.compare(0, str.size() - 1, "number"))
	{
	    decimal = 3;
	    order = String::toInt(str.substr(str.size() - 1, 1));
	}
	else
	if(0 == str.compare(0, str.size() - 1, "wind"))
	{
	    decimal = 4;
	    order = String::toInt(str.substr(str.size() - 1, 1));
	}
	else
	if(0 == str.compare(0, str.size() - 1, "dragon"))
	{
	    decimal = 5;
	    order = String::toInt(str.substr(str.size() - 1, 1));
	}

	val = decimal * 10 + order;

	if(stoneType() == StoneType::None)
	    ERROR("unknown stone: " << str);
    }
}

std::string Stone::toString(void) const
{
    std::ostringstream os;

    switch(0x1F & stoneType())
    {
	case StoneType::IsSkull:  os << "skull" << order(); break;
	case StoneType::IsSword:  os << "sword" << order(); break;
	case StoneType::IsNumber: os << "number" << order(); break;
	case StoneType::IsWind:   os << "wind" << order(); break;
	case StoneType::IsDragon: os << "dragon" << order(); break;
	default: os << "none"; break;
    }

    return os.str();
}

Stone Stone::prev(void) const
{
    switch(id())
    {
	case Skull1:	return None;
	case Skull2:	return Skull1;
	case Skull3:	return Skull2;
	case Skull4:	return Skull3;
	case Skull5:	return Skull4;
	case Skull6:	return Skull5;
	case Skull7:	return Skull6;
	case Skull8:	return Skull7;
	case Skull9:	return Skull8;

	case Sword1:	return None;
	case Sword2:	return Sword1;
	case Sword3:	return Sword2;
	case Sword4:	return Sword3;
	case Sword5:	return Sword4;
	case Sword6:	return Sword5;
	case Sword7:	return Sword6;
	case Sword8:	return Sword7;
	case Sword9:	return Sword8;

	case Number1:	return None;
	case Number2:	return Number1;
	case Number3:	return Number2;
	case Number4:	return Number3;
	case Number5:	return Number4;
	case Number6:	return Number5;
	case Number7:	return Number6;
	case Number8:	return Number7;
	case Number9:	return Number8;

        case Wind1:	return None;
	case Wind2:	return Wind1;
	case Wind3:	return Wind2;
	case Wind4:	return Wind3;

        case Dragon1:	return None;
	case Dragon2:	return Dragon1;
	case Dragon3:	return Dragon2;

	default: break;
    }

    return None;
}

Stone Stone::next(void) const
{
    switch(id())
    {
	case Skull1:	return Skull2;
	case Skull2:	return Skull3;
	case Skull3:	return Skull4;
	case Skull4:	return Skull5;
	case Skull5:	return Skull6;
	case Skull6:	return Skull7;
	case Skull7:	return Skull8;
	case Skull8:	return Skull9;
	case Skull9:	return None;

	case Sword1:	return Sword2;
	case Sword2:	return Sword3;
	case Sword3:	return Sword4;
	case Sword4:	return Sword5;
	case Sword5:	return Sword6;
	case Sword6:	return Sword7;
	case Sword7:	return Sword8;
	case Sword8:	return Sword9;
	case Sword9:	return None;

	case Number1:	return Number2;
	case Number2:	return Number3;
	case Number3:	return Number4;
	case Number4:	return Number5;
	case Number5:	return Number6;
	case Number6:	return Number7;
	case Number7:	return Number8;
	case Number8:	return Number9;
	case Number9:	return None;

        case Wind1:	return Wind2;
	case Wind2:	return Wind3;
	case Wind3:	return Wind4;
	case Wind4:	return None;

        case Dragon1:	return Dragon1;
	case Dragon2:	return Dragon2;
	case Dragon3:	return None;

	default: break;
    }

    return None;
}

int Stone::index(void) const
{
    if(isSkull())	return id() - 10;
    else
    if(isSword())	return id() - 11;
    else
    if(isNumber())	return id() - 12;
    else
    if(isWind())	return id() - 13;
    else
    if(isDragon())	return id() - 19;

    return 0;
}

int Stone::stoneType(void) const
{
    int v2 = StoneType::None;

    if(Skull1 <= id() && id() <= Skull9) v2 = StoneType::IsSkull;
    else
    if(Sword1 <= id() && id() <= Sword9) v2 = StoneType::IsSword;
    else
    if(Number1 <= id() && id() <= Number9) v2 = StoneType::IsNumber;
    else
    if(Wind1 <= id() && id() <= Wind4) v2 = StoneType::IsWind;
    else
    if(Dragon1 <= id() && id() <= Dragon3) v2 = StoneType::IsDragon;

    if(! (StoneType::IsSpecial & v2) && (order() == 1 || order() == 9))
	v2 |= StoneType::IsTerminal;

    return v2;
}

bool Stone::isWind(const Wind & wind) const
{
    switch(wind())
    {
	case Wind::East:	return WindEast == id();
	case Wind::West:	return WindWest == id();
	case Wind::South:	return WindSouth == id();
	case Wind::North:	return WindNorth == id();
	default: break;
    }
    return false;
}

/* VecStones */
std::string VecStones::toString(void) const
{
    std::ostringstream os;
    os << "[ ";

    for(auto it = begin(); it != end(); ++it)
    {
        os << (*it).id();
        if(std::next(it) != end()) os << ", ";
    }

    os << " ]";
    return os.str();
}

JsonArray VecStones::toJsonArray(void) const
{
    JsonArray ja;
    for(auto & st : *this)
        ja.addString(st.toString());
    return ja;
}

VecStones VecStones::fromJsonArray(const JsonArray & ja)
{
    VecStones res; res.reserve(ja.size());
    for(int it = 0; it < ja.size(); ++it)
    {
        const JsonValue* jv = ja.getValue(it);
        if(jv) res.push_back(Stone(jv->getString()));
    }
    return res;
}

/* Stones */
JsonArray Stones::toJsonArray(void) const
{
    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
	ja.addString((*it).toString());
    return ja;
}

Stones Stones::fromJsonArray(const JsonArray & ja)
{
    Stones res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonValue* jv = ja.getValue(it);
	if(jv) res.push_back(Stone(jv->getString()));
    }
    return res;
}

bool Stones::findStone(const Stone & stone) const
{
    return end() != std::find(begin(), end(), stone);
}

int Stones::countStone(const Stone & stone) const
{
    return std::count(begin(), end(), stone);
}

Stones Stones::toUnique(void) const
{
    Stones res = *this;
    std::sort(res.begin(), res.end());
    auto itend = std::unique(res.begin(), res.end());
    res.erase(itend, res.end());
    return res;
}

bool Stones::removeStone(const Stone & stone)
{
    auto it = std::find(begin(), end(), stone);
    if(it != end())
    {
	erase(it);
	return true;
    }
    return false;
}

Stones Stones::findPairs(void) const
{
    Stones res;

    for(auto & stone : toUnique())
    {
        int count = countStone(stone);

        if(2 == count || 3 == count)
            res << stone;
        else
	if(4 == count)
            res << stone << stone;
    }

    return res;
}

bool Stones::findWinRule(const WinRule & rule) const
{
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
		return findStone(rule.stone()) &&
			findStone(rule.stone().next()) && findStone(rule.stone().next().next());
	    break;

	case WinRule::Pung:
	    return 2 < countStone(rule.stone());

	case WinRule::Kong:
	    return 3 < countStone(rule.stone());

	default: break;
    }

    return false;
}

bool Stones::removeWinRule(const WinRule & rule)
{
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
	    {
		bool r1 = removeStone(rule.stone());
		bool r2 = removeStone(rule.stone().next());
		bool r3 = removeStone(rule.stone().next().next());
		if(r1 && r2 && r3) return true;
	    }
	    ERROR("chao not found: " << rule.stone().id());
	    break;

	case WinRule::Pung:
	{
	    bool r1 = removeStone(rule.stone());
	    bool r2 = removeStone(rule.stone());
	    bool r3 = removeStone(rule.stone());
	    if(r1 && r2 && r3) return true;
	    ERROR("pung not found: " << rule.stone().id());
	    break;
	}

	case WinRule::Kong:
	{
	    bool r1 = removeStone(rule.stone());
	    bool r2 = removeStone(rule.stone());
	    bool r3 = removeStone(rule.stone());
	    bool r4 = removeStone(rule.stone());
	    if(r1 && r2 & r3 && r4) return true;
	    ERROR("kong not found: " << rule.stone().id());
	    break;
	}

	default: break;
    }

    return false;
}

JsonObject GameStone::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("stone", Stone::toString());
    jo.addBoolean("casted", isCasted());
    jo.addBoolean("isnew", isNewStone());
    return jo;
}

GameStone GameStone::fromJsonObject(const JsonObject & jo)
{
    GameStone gs(jo.getString("stone"));
    if(jo.getBoolean("casted"))
	gs.setCasted(true);
    if(jo.getBoolean("isnew"))
	gs.setNewStone(true);
    return gs;
}

JsonArray GameStones::toJsonArray(void) const
{
    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
	ja.addObject(static_cast<const GameStone &>(*it).toJsonObject());
    return ja;
}

GameStones GameStones::fromJsonArray(const JsonArray & ja)
{
    GameStones res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonObject* jo =ja.getObject(it);
	if(jo) res.push_back(GameStone::fromJsonObject(*jo));
    }
    return res;
}

bool Stones::isChaoStone(const Stone & stone, bool firstOnly) const
{
    Stones unique = toUnique();
    auto it = std::find(unique.begin(), unique.end(), stone);

    if(it != unique.end())
    {
	int pos = std::distance(unique.begin(), it);

        const Stone & st = *it;
        if(st.isHonor()) return false;

        if(! firstOnly && st.order() == 9 && 1 < pos)
        {
            const Stone & st1 = unique[pos - 2];
            const Stone & st2 = unique[pos - 1];
            const Stone & st3 = unique[pos];

            return ! st2.isHonor() && ! st3.isHonor() &&
                st1.index() + 1 == st2.index() && st2.index() + 1 == st3.index();
        }
        else
	if(! firstOnly && st.order() == 8 && 0 < pos && pos + 1 < unique.size())
        {
            const Stone & st1 = unique[pos - 1];
            const Stone & st2 = unique[pos];
            const Stone & st3 = unique[pos + 1];

            return ! st2.isHonor() && ! st3.isHonor() &&
                st1.index() + 1 == st2.index() && st2.index() + 1 == st3.index();
        }
        else
	if(st.order() < 8 && pos + 2 < unique.size())
        {
            const Stone & st1 = unique[pos];
            const Stone & st2 = unique[pos + 1];
            const Stone & st3 = unique[pos + 2];

            return ! st2.isHonor() && ! st3.isHonor() &&
                st1.index() + 1 == st2.index() && st2.index() + 1 == st3.index();
        }
    }

    return false;
}

Stones Stones::findChaoVariants(const Stone & stone) const
{
    Stones res;
    res.reserve(5);

    switch(stone.order())
    {
        case 1: // 1 2 3
            if(findStone(stone.next()) &&
		findStone(stone.next().next())) res.push_back(stone);
            break;
        case 2: // 1 2 3, 2 3 4
            if(findStone(stone.prev()) &&
		findStone(stone.next())) res.push_back(stone.prev());
            if(findStone(stone.next()) &&
		findStone(stone.next().next())) res.push_back(stone);
            break;
        case 3: // 1 2 3, 2 3 4, 3 4 5
        case 4: // 2 3 4, 3 4 5, 4 5 6
        case 5: // 3 4 5, 4 5 6, 5 6 7
        case 6: // 4 5 6, 5 6 7, 6 7 8
        case 7: // 5 6 7, 6 7 8, 7 8 9
            if(findStone(stone.prev().prev()) &&
		findStone(stone.prev())) res.push_back(stone.prev().prev());
            if(findStone(stone.prev()) &&
		findStone(stone.next())) res.push_back(stone.prev());
            if(findStone(stone.next()) &&
		findStone(stone.next().next())) res.push_back(stone);
            break;
        case 8: // 6 7 8, 7 8 9
            if(findStone(stone.prev().prev()) &&
		findStone(stone.prev())) res.push_back(stone.prev().prev());
            if(findStone(stone.prev()) &&
		findStone(stone.next())) res.push_back(stone.prev());
            break;
        case 9: // 7 8 9
            if(findStone(stone.prev().prev()) &&
		findStone(stone.prev())) res.push_back(stone.prev().prev());
            break;
        default: break;
    }

    return res;
}

bool Stones::haveKong(void) const
{
    // kong: 4 equal
    for(auto & st : toUnique())
    {
        auto bound = std::equal_range(begin(), end(), st);
        if(4 == std::distance(bound.first, bound.second))
            return true;
    }

    return false;
}

void GameStones::add(const GameStone & stone)
{
    if(!stone.isValid())
	ERROR("invalid stone");
    push_back(stone);
    std::sort(begin(), end());
}

Stone GameStones::del(int index)
{
    Stone res;
    if(0 <= index && index < size())
    {
	res = at(index);
	erase(begin() + index);
	std::sort(begin(), end());
    }
    return res;
}

bool GameStones::find(const WinRule & rule) const
{
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
		return findStone(rule.stone()) && findStone(rule.stone().next()) && findStone(rule.stone().next().next());
	    break;

	case WinRule::Pung:	return 2 < countStone(rule.stone()); break;
	case WinRule::Kong:	return 3 < countStone(rule.stone()); break;

	default: break;
    }
    return false;
}

void GameStones::remove(const WinRule & rule)
{
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
	    {
		const bool removedFirst = removeStone(rule.stone());
		const bool removedSecond = removeStone(rule.stone().next());
		const bool removedThird = removeStone(rule.stone().next().next());
		if(!removedFirst || !removedSecond || !removedThird)
		    ERROR("chao not found: " << rule.stone().id());
	    }
	    break;
	case WinRule::Pung:
	    for(int ii = 1; ii <= 3; ++ii)
	    {
		auto it = std::find(begin(), end(), rule.stone());
		if(it != end())
		    erase(it);
		else
		    ERROR("pung not found: " << rule.stone().id());
	    }
	    break;
	case WinRule::Kong:
	    for(int ii = 1; ii <= 4; ++ii)
	    {
		auto it = std::find(begin(), end(), rule.stone());
		if(it != end())
		    erase(it);
		else
		    ERROR("kong not found: " << rule.stone().id());
	    }
	    break;
	default: break;
    }
}

std::string GameStones::toString(void) const
{
    std::ostringstream os;
    os << "[ ";

    for(auto it = begin(); it != end(); ++it)
	os << (*it).id() << (GameStone(*it).isCasted() ? "*" : "") << ", ";

    os << " ]";
    return os.str();
}

bool GameStones::allowCast(const Stones & rules) const
{
    if(! rules.size())
	return true;

    GameStones stones = *this;
    stones.resize(std::distance(stones.begin(), std::remove_if(stones.begin(), stones.end(), [](auto & st){ return GameStone::isCasted(st); })));

    Stones uniq = rules.toUnique();

    for(auto it = uniq.begin(); it != uniq.end(); ++it)
    {
	if(rules.countStone(*it) > stones.countStone(*it))
	    return false;
    }

    return true;
}

bool GameStones::allowCast(const Stones & rules, const GameStone & newStone) const
{
    // empty rules: allow cast for points only
    if(! rules.size())
	return true;

    if(newStone.isValid())
    {
	GameStones stones = *this;
	stones.add(newStone);
	return stones.allowCast(rules);
    }

    return allowCast(rules);
}

void GameStones::setCasted(const Stones & rules, GameStone & newStone)
{
    for(auto it1 = rules.begin(); it1 != rules.end(); ++it1)
    {
	auto it2 = std::find(begin(), end(), GameStone(*it1, false));
	if(it2 != end())
	    static_cast<GameStone &>(*it2).setCasted(true);
	else
	if(newStone.isValid() && ! newStone.isCasted() && newStone == *it1)
	    newStone.setCasted(true);
	else
	    ERROR("rune not found: " << (*it1).id());
    }
}

std::string Stones::toString(void) const
{
    std::ostringstream os;
    os << "[ ";

    for(auto it = begin(); it != end(); ++it)
	os << (*it).id() << ", ";

    os << " ]";
    return os.str();
}

/* BaseStat */
BaseStat & BaseStat::operator+= (const BaseStat & bs)
{
    attack += bs.attack;
    defense += bs.defense;
    ranger += bs.ranger;
    loyalty += bs.loyalty;

    return *this;
}

BaseStat & BaseStat::operator-= (const BaseStat & bs)
{
    attack -= bs.attack;
    defense -= bs.defense;
    ranger -= bs.ranger;
    loyalty -= bs.loyalty;

    return *this;
}

std::string BaseStat::toString(void) const
{
    return StringFormat("[%1, %2, %3, %4]").arg(attack).arg(ranger).arg(defense).arg(loyalty);
}

void CreatureSkill::initMahjongPart(const Ability & ability, const Specials & specials, const AffectedSpells & affected)
{
    // reset move
    stat5.reset();
}

void CreatureSkill::initAdventurePart(const Ability & ability, const Specials & specials, const AffectedSpells & affected)
{
    // bard ability: +2 loyalty restore
    const Ability abilityBart(Ability::Bard);
    if(ability == abilityBart && stat4.current() < stat4.base())
    {
	VERBOSE("Ability: " << abilityBart.toString());
	stat4 += 2;
    }

    // Regeneration restores the creature completely at the start of the map phase.
    const Speciality specialityRegeneration(Speciality::Regeneration);
    if(specials.check(specialityRegeneration) && stat4.current() < stat4.base())
    {
	VERBOSE("Speciality: " << specialityRegeneration.toString());
	stat4.reset();
    }

    // devotion speciality: +2 loyalty restore
    const Speciality specialityDevotion(Speciality::Devotion);
    if(specials.check(specialityDevotion) && stat4.current() < stat4.base())
    {
        VERBOSE("Speciality: " << specialityDevotion.toString());
	stat4 += SpecialityDevotion().restore();
    }

    // fixed overload loyalty
    if(stat4.current() > stat4.base())
	stat4.reset();
}

void CreatureSkill::moved(int steps)
{
    if(canMove(steps)) stat5 -= steps;
}

std::string BattleStat::toString(void) const
{
    std::ostringstream os;
    os << "[" << base() << ", " << current() << "]";
    return os.str();
}

JsonArray BattleStat::toJsonArray(void) const
{
    JsonArray ja;
    ja.addInteger(base());
    ja.addInteger(current());
    return ja;
}

BattleStat BattleStat::fromJsonArray(const JsonArray & ja)
{
    const JsonValue* jv = nullptr;

    jv = ja.getValue(0);
    int v1 = jv ? jv->getInteger() : 0;

    jv = ja.getValue(1);
    int v2 = jv ? jv->getInteger() : 0;

    return BattleStat(v1, v2);
}

JsonObject BattleUnit::toJsonObject(void) const
{
    JsonObject jo;
    jo.addArray("melee", stat1.toJsonArray());
    jo.addArray("range", stat2.toJsonArray());
    jo.addArray("defense", stat3.toJsonArray());
    jo.addArray("loyalty", stat4.toJsonArray());
    return jo;
}

BattleUnit BattleUnit::fromJsonObject(const JsonObject & jo)
{
    BattleUnit res;
    const JsonArray* ja = nullptr;

    ja = jo.getArray("melee");
    if(ja) res.stat1 = BattleStat::fromJsonArray(*ja);

    ja = jo.getArray("range");
    if(ja) res.stat2 = BattleStat::fromJsonArray(*ja);

    ja = jo.getArray("defense");
    if(ja) res.stat3 = BattleStat::fromJsonArray(*ja);

    ja = jo.getArray("loyalty");
    if(ja) res.stat4 = BattleStat::fromJsonArray(*ja);

    return res;
}

std::string BattleUnit::toString(void) const
{
    std::ostringstream os;
    os << "attack: " << stat1.toString() << ", " <<
	"ranged: " << stat2.toString() << ", " <<
	"defense: " << stat3.toString() << ", " <<
	"loyalty: " << stat4.toString();
    return os.str();
}

void BattleUnit::applyStats(const BaseStat & stat)
{
    stat1 += stat.attack;
    stat2 += stat.ranger;
    stat3 += stat.defense;
    stat4 += stat.loyalty;
}

JsonObject CreatureSkill::toJsonObject(void) const
{
    JsonObject jo = BattleUnit::toJsonObject();
    jo.addArray("move", stat5.toJsonArray());
    return jo;
}

CreatureSkill CreatureSkill::fromJsonObject(const JsonObject & jo)
{
    CreatureSkill res(BattleUnit::fromJsonObject(jo));

    const JsonArray* ja = jo.getArray("move");
    if(ja) res.stat5 = BattleStat::fromJsonArray(*ja);

    return res;
}

std::string CreatureSkill::toString(void) const
{
    std::ostringstream os;
    os << BattleUnit::toString() << ", " << "move: " << stat5.toString();
    return os.str();
}

AffectedSpell::AffectedSpell(const Spell & sp) : Spell(sp), duration(0)
{
    const SpellInfo & si = GameData::spellInfo(sp);
    duration = si.duration();
    if(si.persistent && 0 == duration) duration = Permanent;
}

AffectedSpell::AffectedSpell(const Spell & sp, int val) : Spell(sp), duration(val)
{
}

AffectedSpell::AffectedSpell(const Spell & sp, int val, const Avatar & caster)
    : Spell(sp), duration(val), source(caster)
{
}

void AffectedSpell::actionReduceDuration(void)
{
    if(0 < duration)
	duration -= 1;
}

JsonObject AffectedSpell::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("spell", Spell::toString());
    jo.addInteger("duration", duration);
    if(source.isValid()) jo.addString("source", source.toString());
    return jo;
}

AffectedSpell AffectedSpell::fromJsonObject(const JsonObject & jo)
{
    Spell spell(jo.getString("spell", "none"));
    int val = jo.getInteger("duration", 0);
    Avatar source(jo.getString("source", "none"));

    // Older saves encoded permanent enchantments as duration 0.
    if(0 == val)
    {
	const SpellInfo & info = GameData::spellInfo(spell);
	if(info.persistent && 0 == info.duration()) val = Permanent;
    }

    return AffectedSpell(spell, val, source);
}

int AffectedSpells::attack(void) const
{
    int cur = 0;

    for(auto it = begin(); it != end(); ++it)
	cur += GameData::spellInfo(*it).effect.attack;

    return cur;
}

int AffectedSpells::ranger(void) const
{
    int cur = 0;

    for(auto it = begin(); it != end(); ++it)
	cur += GameData::spellInfo(*it).effect.ranger;

    return cur;
}

int AffectedSpells::defense(void) const
{
    int cur = 0;

    for(auto it = begin(); it != end(); ++it)
	cur += GameData::spellInfo(*it).effect.defense;

    return cur;
}

int AffectedSpells::loyalty(void) const
{
    int cur = 0;

    for(auto it = begin(); it != end(); ++it)
	cur += GameData::spellInfo(*it).effect.loyalty;

    return cur;
}

bool AffectedSpells::isAffected(const Spell & spell) const
{
    return end() != std::find(begin(), end(), spell);
}

bool AffectedSpells::isAffected(const Spell & spell, const Avatar & source) const
{
    return end() != std::find_if(begin(), end(), [&](const AffectedSpell & affectedSpell)
    {
	return affectedSpell == spell && affectedSpell.source == source;
    });
}

void AffectedSpells::insert(const AffectedSpell & as)
{
    if(! as.isValid()) return;

    auto it = std::find(begin(), end(), as);
    if(it != end())
    {
	*it = as;
    }
    else
    {
	push_back(as);
    }
}

JsonArray AffectedSpells::toJsonArray(void) const
{
    JsonArray ja;

    for(auto it = begin(); it != end(); ++it)
	ja.addObject((*it).toJsonObject());

    return ja;
}

AffectedSpells AffectedSpells::fromJsonArray(const JsonArray & ja)
{
    AffectedSpells res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonObject* jj = ja.getObject(it);
	if(jj) res.insert(AffectedSpell::fromJsonObject(*jj));
    }
    return res;
}

void AffectedSpells::spellAffected(const Spell & spell)
{
    auto it = std::find(begin(), end(), spell);

    if(it != end())
    {
	(*it).actionReduceDuration();

	if(! (*it).isValid()) erase(it);
    }
}

void AffectedSpells::advanceAdventureRound(void)
{
    for(auto & as : *this)
	as.actionReduceDuration();

    auto itend = std::remove_if(begin(), end(), [](const AffectedSpell & as){ return ! as.isValid(); });
    erase(itend, end());
}

BattleStrike::BattleStrike(const BattleUnit & bs, int dmg, const BattleUnit & target, int val)
    : unit1(bs.battleUnit()), is_creature1(bs.isCreature()), unit2(target.battleUnit()), is_creature2(target.isCreature()), damage(dmg), type(val)
{
}

JsonObject BattleStrike::toJsonObject(void) const
{
    JsonObject jo;
    jo.addInteger("unit1", unit1);
    jo.addBoolean("is_creature1", is_creature1);
    jo.addInteger("unit2", unit2);
    jo.addBoolean("is_creature2", is_creature2);
    jo.addInteger("damage", damage);
    jo.addInteger("type", type);
    return jo;
}

BattleStrike BattleStrike::fromJsonObject(const JsonObject & jo)
{
    BattleStrike res;
    res.unit1 = jo.getInteger("unit1");
    res.is_creature1 = jo.getBoolean("is_creature1");
    res.unit2 = jo.getInteger("unit2");
    res.is_creature2 = jo.getBoolean("is_creature2");
    res.damage = jo.getInteger("damage");
    res.type = jo.getInteger("type");
    return res;
}

JsonArray BattleStrikes::toJsonArray(void) const
{
    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
        ja.addObject((*it).toJsonObject());
    return ja;
}

BattleStrikes BattleStrikes::fromJsonArray(const JsonArray & ja)
{
    BattleStrikes res;
    for(int it = 0; it < ja.size(); ++it)
    {
        const JsonObject* jo = ja.getObject(it);
        if(jo) res.push_back(BattleStrike::fromJsonObject(*jo));
    }
    return res;
}

std::string BattleStrikes::toString(void) const
{
    std::ostringstream os;

    for(auto it = begin(); it != end(); ++it)
        os << "[" <<
	    ((*it).is_creature1 ? "bcr" : "town") << "(" << String::hex((*it).unit1, 8) << "), " <<
	    ((*it).is_creature2 ? "bcr" : "town") << "(" << String::hex((*it).unit2, 8) << "), " <<
	    (*it).damage << ", " <<
	    ((*it).type == BattleStrike::Melee ? "melee" : "ranger") << "], ";

    return os.str();
}

BattleCreature::BattleCreature(const Clan & clan, const Creature & cr, int uid)
    : Creature(cr), CreatureSkill(GameData::creatureInfo(cr).stat), buid(uid), selected(true), owner(clan)
{
}

BattleCreature::BattleCreature(const Clan & clan, const Creature & cr, const CreatureSkill & cs, int uid)
    : Creature(cr), CreatureSkill(cs), buid(uid), selected(false), owner(clan)
{
}

int BattleCreature::attack(void) const
{
    return BattleUnit::attack() + affected.attack();
}

int BattleCreature::ranger(void) const
{
    return BattleUnit::ranger() + affected.ranger();
}

int BattleCreature::defense(void) const
{
    return BattleUnit::defense() + affected.defense();
}

int BattleCreature::loyalty(void) const
{
    return BattleUnit::loyalty() + affected.loyalty();
}

int BattleCreature::freeMovePoint(void) const
{
    bool isParalyze = isAffectedSpell(Spell::Paralyze);
    return isParalyze ? 0 : CreatureSkill::freeMovePoint();
}

bool BattleCreature::haveSpeciality(const Speciality & spec) const
{
    const CreatureInfo & info = GameData::creatureInfo(*this);
    return info.specials.check(spec);
}

bool BattleCreature::isAffectedSpell(const Spell & spell) const
{
    return affected.isAffected(spell);
}

bool BattleCreature::canReceiveSpell(const Spell & spell) const
{
    // Guidance improves an existing ranged attack; it does not grant one.
    return spell() != Spell::Guidance || 0 < baseRanger();
}

bool BattleCreature::applySpell(const Spell & spell)
{
    if(haveSpeciality(Speciality::MagicResistence))
    {
	int chance = SpecialityMagicResistence().chance(Creature::id());

	if(chance >= GameplayRng::uniform(1, 100))
	{
	    VERBOSE("Speciality: " << "Magic Resistence!");
	    return false;
	}
    }

    switch(spell())
    {
	case Spell::Paralyze:
	    affected.insert(AffectedSpell(spell));
	    return true;

	case Spell::Smoke:
	case Spell::DemonicCompulsion:
	case Spell::MassPanic:
	case Spell::Reduction:
	case Spell::BattleFury:
	case Spell::Guidance:
	case Spell::ForceShield:
	case Spell::DustCloud:
	case Spell::Heroism:
	case Spell::BlindAmbition:
	case Spell::BrilliantLights:
	case Spell::MagicalAura:
	    affected.insert(spell);
	    return true;

	case Spell::MysticalFountain:
	{
	    auto rnd = static_cast<Spell::spell_t>(Spell::MysticalFountain + GameplayRng::uniform(1, 3));
	    affected.insert(Spell(rnd));
	}
	    return true;


	case Spell::Healing:
	    applyStats(GameData::spellInfo(spell).effect);
	    if(stat4.current() > stat4.base()) stat4.reset();
	    return true;

	case Spell::LightningBolt:
	case Spell::HellBlast:
	    applyStats(GameData::spellInfo(spell).effect);
	    return true;

	case Spell::DispelMagic:
	case Spell::MassDispel:
	    affected.clear();
	    return true;

	case Spell::Teleport:
	case Spell::DrawSkull:
	case Spell::DrawSword:
	case Spell::DrawNumber:
	case Spell::RandomDiscard:
	case Spell::ScryRunes:
	case Spell::Silence:
	case Spell::ManaFog:
	    break;

	default: ERROR("unknown spell: " << spell()); break;
    }

    return false;
}

void BattleCreature::initMahjongPart(const Ability & ability)
{
    // The previous Adventure phase consumes one round of timed enchantments.
    affected.advanceAdventureRound();

    // skills turn
    const CreatureInfo & info = GameData::creatureInfo(*this);
    CreatureSkill::initMahjongPart(ability, info.specials, affected);
    setSelected(true);
}

void BattleCreature::initAdventurePart(const Ability & ability)
{
    const CreatureInfo & info = GameData::creatureInfo(*this);
    CreatureSkill::initAdventurePart(ability, info.specials, affected);
}

bool BattleCreature::canMoveSelected(void) const
{
    bool isParalyze = isAffectedSpell(Spell::Paralyze);
    return 0 < freeMovePoint() && ! isParalyze && isSelected();
}

std::string BattleCreature::name(void) const
{
    std::ostringstream os;
    os << "creature" << "(" << Creature::toString() << ", " << "battleUnit: " << String::hex(battleUnit(), 8) << ")";
    return os.str();
}

std::string BattleCreature::toString(void) const
{
    std::ostringstream os;
    os << Creature::toString() << "(" << CreatureSkill::toString() << ", " << "clan: " << owner.toString() << ", " << "battleUnit: " << String::hex(battleUnit(), 8) << ")";
    return os.str();
}

JsonObject BattleCreature::toJsonObject(void) const
{
    JsonObject jo = CreatureSkill::toJsonObject();
    jo.addInteger("battleUnit", battleUnit());
    jo.addString("creature", Creature::toString());
    jo.addString("owner", owner.toString());

    jo.addArray("affected", affected.toJsonArray());

    return jo;
}

BattleCreature BattleCreature::fromJsonObject(const JsonObject & jo)
{
    Creature cr(jo.getString("creature", "none"));
    CreatureSkill bs = CreatureSkill::fromJsonObject(jo);
    BattleCreature res(Clan(jo.getString("owner", "none")), cr, bs, jo.getInteger("battleUnit", 0));
    res.selected = true;

    const JsonArray* ja = jo.getArray("affected");
    if(ja) res.affected = AffectedSpells::fromJsonArray(*ja);

    return res;
}

/* BattleCreatures */
BattleCreatures & BattleCreatures::operator<< (const BattleCreatures & bcrs)
{
    insert(end(), bcrs.begin(), bcrs.end());
    return *this;
}

int BattleCreatures::validSize(void) const
{
    int res = 0;
    for(auto it = begin(); it != end(); ++it)
	if(*it && (*it)->isValid()) ++res;
    return res;
}

/* BattleTargets */
BattleTargets::BattleTargets(const BattleCreatures & bcrs)
{
    for(auto it = bcrs.begin(); it != bcrs.end(); ++it)
	if(*it && (*it)->isValid()) push_back(*it);
}

BattleTargets & BattleTargets::operator<< (const BattleUnit* bu)
{
    if(bu && bu->isValid()) push_back(const_cast<BattleUnit*>(bu));
    return *this;
}

BattleTargets & BattleTargets::operator<< (const BattleTargets & btrs)
{
    insert(end(), btrs.begin(), btrs.end());
    return *this;
}

JsonArray BattleTargets::toJsonArray(void) const
{
    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
        if(*it) ja.addInteger((*it)->battleUnit());
    return ja;
}

BattleTargets BattleTargets::fromJsonArray(const JsonArray & ja)
{
    BattleTargets res;
    for(int it = 0; it < ja.size(); ++it)
    {
	int battleUnit = ja.getInteger(it);
	BattleCreature* creature = GameData::findBattleCreature(battleUnit);
	if(creature) res.push_back(creature);
    }
    return res;
}

/* BattleTown */
BattleTown::BattleTown(const Land & land) : BattleUnit(GameData::landInfo(land).stat), territory(land)
{
    previous = GameData::landInfo(land).clan;
}

JsonObject BattleTown::toJsonObject(void) const
{
    JsonObject jo = BattleUnit::toJsonObject();

    jo.addString("previous", previous.toString());
    jo.addString("territory", land().toString());

    return jo;
}

BattleTown BattleTown::fromJsonObject(const JsonObject & jo)
{
    const Clan & previous = Clan(jo.getString("previous", "none"));
    const Land & territory = Land(jo.getString("territory", "none"));

    return BattleTown(BattleUnit::fromJsonObject(jo), previous, territory);
}

const Clan & BattleTown::previousClan(void) const
{
    return previous;
}

const Clan & BattleTown::currentClan(void) const
{
    return GameData::landInfo(land()).clan;
}

std::string BattleTown::toString(void) const
{
    std::ostringstream os;
    os << land().toString() << "(" << BattleUnit::toString() << ", " << "battleUnit: " << String::hex(battleUnit(), 8) << ")";
    return os.str();
}

std::string BattleTown::name(void) const
{
    std::ostringstream os;
    os << "tower" << "(" << land().toString() << ")";
    return os.str();
}

/* BattleParty */
BattleParty::BattleParty(const Clan & clan, const Land & land) : position(land), target(Land::None), owner(clan)
{
    resize(3);
}

std::string BattleParty::toString(void) const
{
    std::ostringstream os;

    os << "party: " <<
	"clan(" << owner.toString() << ")" << ", " <<
	"land(" << position.toString() << ")" << ", ";

    if(isEmpty())
	os << "empty";
    else
    {
	for(auto it = begin(); it != end(); ++it)
	    if((*it).isValid()) os << (*it).toString() << ", ";
    }

    return os.str();
}

JsonObject BattleParty::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("position", position.toString());
    jo.addString("target", target.toString());
    jo.addString("owner", owner.toString());

    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
    {
	ja.addObject((*it).toJsonObject());
    }
    jo.addArray("creatures", ja);

    return jo;
}

BattleParty BattleParty::fromJsonObject(const JsonObject & jo)
{
    BattleParty res;
    res.position = Land(jo.getString("position", "none"));
    res.target = Land(jo.getString("target", "none"));
    res.owner = Clan(jo.getString("owner", "none"));

    const JsonArray* ja = jo.getArray("creatures");
    if(ja)
    {
	res.resize(ja->size());

	for(int it = 0; it < ja->size(); ++it)
	{
	    const JsonObject* jo = ja->getObject(it);
	    if(jo && jo->isValid())
	    {
		res[it] = BattleCreature::fromJsonObject(*jo);
	    }
	}
    }

    return res;
}

BattleCreatures BattleParty::toBattleCreatures(const Specials & specials, bool filter) const
{
    BattleCreatures res;

    for(auto & bcr : *this)
	if(bcr.isValid())
    {
	const CreatureInfo & info = GameData::creatureInfo(bcr);
	bool push = false;

	if(filter)
	{
	    if(specials.any())
	    {
		push = info.specials.to_ulong() & specials.to_ulong();
	    }
	    else
	    {
		push = info.specials.none();
	    }
	}
	else
	{
	    if(specials.any())
	    {
		push = info.specials.to_ulong() & ~specials.to_ulong();
	    }
	    else
	    {
		push = info.specials.any();
	    }
	}

	if(push)
	    res.push_back(const_cast<BattleCreature*>(& bcr));
    }

    return res;
}

BattleCreatures BattleParty::toBattleCreatures(void) const
{
    BattleCreatures res;
    for(auto it = begin(); it != end(); ++it)
	if((*it).isValid()) res.push_back(const_cast<BattleCreature*>(& (*it)));

    return res;
}

int BattleParty::count(void) const
{
    return size() - std::count_if(begin(), end(), [](const BattleCreature & bcr){ return ! bcr.isValid(); });
}

bool BattleParty::isEmpty(void) const
{
    return count() == 0;
}

void BattleParty::dismiss(void)
{
    clear();
}

bool BattleParty::canJoin(void) const
{
    return std::any_of(begin(), end(), [](const BattleCreature & bcr){ return ! bcr.isValid(); });
}

bool BattleParty::join(const Creature & cr)
{
    // first invalid
    auto it = std::find_if(begin(), end(), [](const BattleCreature & bcr){ return ! bcr.isValid(); });
    if(it == end()){ ERROR("party: is full"); return false; }

    *it = BattleCreature(owner, cr, GameData::nextBattleUnitId());
    DEBUG((*it).toString());

    return true;
}

bool BattleParty::join(const BattleCreature & bcr)
{
    if(! bcr.isValid() || bcr.clan() != owner)
    {
	ERROR("invalid creature for party");
	return false;
    }

    auto it = std::find_if(begin(), end(), [](const BattleCreature & current){ return ! current.isValid(); });
    if(it == end())
    {
	ERROR("party: is full");
	return false;
    }

    *it = bcr;
    return true;
}

bool BattleParty::remove(const BattleCreature & bcr)
{
    auto it = std::find(begin(), end(), bcr);
    if(it != end())
    {
	*it = BattleCreature();
	return true;
    }
    return false;
}

void BattleParty::removeUnloyalty(void)
{
     for(auto it = begin(); it != end(); ++it)
	if((*it).isValid() && ! (*it).isAlive())
	*it = BattleCreature();
}

const BattleCreature* BattleParty::index(int index) const
{
    return 0 <= index && index < size() ? & at(index) : nullptr;
}

BattleCreature* BattleParty::findBattleUnit(int uid)
{
    return const_cast<BattleCreature*>(findBattleUnitConst(uid));
}

const BattleCreature* BattleParty::findBattleUnitConst(int uid) const
{
    auto it = std::find_if(begin(), end(),[=](const BattleCreature & bcr){ return bcr.isBattleUnit(uid); });
    return it != end() ? & (*it) : nullptr;
}

bool BattleParty::findCreature(const Creature & cr) const
{
    return end() != std::find(begin(), end(), cr);
}

int BattleParty::movePoint(void) const
{
    int res = 0;

    for(auto it = begin(); it != end(); ++it)
    {
	if(! (*it).isValid()) continue;

	const int move = (*it).freeMovePoint();
	res = res ? std::min(res, move) : move;
	if(! res) break;
    }

    return res;
}

BaseStat BattleParty::toBaseStatSummary(void) const
{
    BaseStat res;

    for(auto it = begin(); it != end(); ++it)
    {
        res.attack += (*it).attack();
        res.defense += (*it).defense();
        res.ranger += (*it).ranger();
        res.loyalty += (*it).loyalty();
    }

    return res;
}

/*BattleArmy */
BattleParty* BattleArmy::findParty(const Land & land)
{
    return const_cast<BattleParty*>(findPartyConst(land));
}

const BattleParty* BattleArmy::findPartyConst(const Land & land) const
{
    auto it = std::find_if(begin(), end(), [&](const BattleParty & party){ return party.isPosition(land); });
    return it != end() ? & (*it) : nullptr;
}

BattleCreatures BattleArmy::partySelected(const Land & land) const
{
    BattleCreatures res;
    const BattleParty* party = findPartyConst(land);
    if(party)
    {
	res = party->toBattleCreatures();
	auto itend = std::remove_if(res.begin(), res.end(), [](auto & bcr){ return ! bcr->canMoveSelected(); });
	res.erase(itend, res.end());
    }
    return res;
}

void BattleArmy::partySetAllSelected(const Land & land)
{
    BattleParty* party = findParty(land);
    if(party)
    {
	for(auto & bcr : party->toBattleCreatures())
	    bcr->setSelected(true);
    }
}

BattleCreature* BattleArmy::findBattleUnit(int uid)
{
    return const_cast<BattleCreature*>(findBattleUnitConst(uid));
}

const BattleCreature* BattleArmy::findBattleUnitConst(int uid) const
{
    for(auto & party : *this)
    {
	const BattleCreature* res = party.findBattleUnitConst(uid);
	if(res) return res;
    }
    return nullptr;
}

bool BattleArmy::findCreature(const Creature & cr) const
{
    return std::any_of(begin(), end(), [&](const BattleParty & party){ return party.findCreature(cr); });
}

BattleTargets BattleArmy::toBattleTargets(const Clan & clan) const
{
    BattleTargets res;

    for(auto it = begin(); it != end(); ++it)
	res << BattleTargets((*it).toBattleCreatures());

    return res;
}

BattleCreatures BattleArmy::toBattleCreatures(const Specials & specials, bool filter) const
{
    BattleCreatures res;

    for(auto it = begin(); it != end(); ++it)
	res << (*it).toBattleCreatures(specials, filter);

    return res;
}

BattleCreatures BattleArmy::toBattleCreatures(void) const
{
    BattleCreatures res;

    for(auto it = begin(); it != end(); ++it)
	res << (*it).toBattleCreatures();

    return res;
}

JsonArray BattleArmy::toJsonArray(void) const
{
    JsonArray ja;
    for(auto it = begin(); it != end(); ++it)
	ja.addObject((*it).toJsonObject());
    return ja;
}

BattleArmy BattleArmy::fromJsonArray(const JsonArray & ja)
{
    BattleArmy res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonObject* jo = ja.getObject(it);
	if(jo) res.push_back(BattleParty::fromJsonObject(*jo));
    }
    return res;
}

void BattleArmy::setAllSelected(void)
{
    BattleCreatures creatures = toBattleCreatures();

    for(auto it = creatures.begin(); it != creatures.end(); ++it)
	if(*it) (*it)->setSelected(true);
}

bool BattleArmy::isFullHouse(void) const
{
    int count = 0;

    for(auto it = begin(); it != end(); ++it)
	count += (*it).count();

    return count >= 6;
}

Spells BattleArmy::allCastSpells(void) const
{
    Spells spells;
    Specials specialsCast = Specials::allCastSpells();
    BattleCreatures bcrs = toBattleCreatures(specialsCast, true);

    for(auto it = bcrs.begin(); it != bcrs.end(); ++it) if(*it && (*it)->isValid())
    {
        const CreatureInfo & info = GameData::creatureInfo(**it);
        for(auto & spec : Specials(info.specials.to_ulong() & specialsCast.to_ulong()).toList())
        {
            const Spell spell = spec.toSpell();
            if(spell.isValid())
		spells.push_back(spell);
        }
    }

    return spells;
}

bool BattleArmy::canJoin(const Land & land) const
{
    if(isFullHouse())
	return false;

    const BattleParty* party = findPartyConst(land);
    return party ? party->canJoin() : false;
}

bool BattleArmy::join(const Creature & creature, const Land & land)
{
    if(! isFullHouse())
    {
	BattleParty* party = findParty(land);

	if(! party)
	{
	    auto remote = GameData::getBattleArmyOwner(*this);
	    if(remote)
	    {
		push_back(BattleParty(remote->clan, land));
		party = & back();
	    }
	}

	return party->join(creature);
    }

    ERROR("army: is full");
    return false;
}

bool BattleArmy::isMaximumSummoning(void) const
{
    return 4 < toBattleCreatures().size();
}

void BattleArmy::remove(const BattleCreature & bcr)
{
    for(auto & party : *this)
	party.remove(bcr);
    shrinkEmpty();
}

void BattleArmy::removeUnloyalty(void)
{
    for(auto & party : *this)
	party.removeUnloyalty();

    shrinkEmpty();
}

void BattleArmy::applyInvisibility(const Clan & observer)
{
    const BattleArmy & observerArmy = GameData::getBattleArmy(observer);

    auto observerHasAdjacentDetector = [&observerArmy](const std::vector<Land> & lands) -> bool
    {
	for(auto & land : lands)
	{
	    const BattleParty* party = observerArmy.findPartyConst(land);
	    if(party && party->toBattleCreatures(Specials() << Speciality::SeeInvisible, true).size())
		return true;
	}
	return false;
    };

    // remove if Speciality::Invisibility
    for(auto & party : *this)
    {
	BattleCreatures bcrs = party.toBattleCreatures(Specials() << Speciality::Invisibility, true);
        bool remove = true;

        // check Speciality::SeeInvisible on borders
        const LandInfo & positionInfo = GameData::landInfo(party.land());

	// skip if position: TowerOf4Winds
	if(positionInfo.id.isTowerWinds())
	    remove = false;
	else
	{
	    if(observerHasAdjacentDetector(positionInfo.borders))
	    {
		DEBUG("found Speciality::SeeInvisible" << ", " << "observer: " << observer.toString() << ", " << "land: " << party.land().toString());
                remove = false;
            }
	}

        if(remove)
        {
	    for(auto & bcr : bcrs)
	    {
		DEBUG("remove Speciality::Invisibility" << ", " << "land: " << party.land().toString() << ", " << "creature: " << bcr->toString());
		party.remove(*bcr);
	    }
	}
    }

    shrinkEmpty();
}

void BattleArmy::shrinkEmpty(void)
{
    remove_if([](const BattleParty & party){ return party.isEmpty(); });
}

std::string BattleArmy::toString(void) const
{
    std::ostringstream os;

    for(auto it = begin(); it != end(); ++it)
	os << (*it).toString() << ", ";

    return os.str();
}


bool BattleArmy::canGateParty(const Land & fromLand, const Land & toLand) const
{
    if(!fromLand.isValid() || !toLand.isValid() || fromLand == toLand) return false;

    const BattleParty* source = findPartyConst(fromLand);
    if(!source || source->isEmpty()) return false;

    const LandInfo & destination = GameData::landInfo(toLand);
    if(!toLand.isTowerWinds() && (!destination.stat.power || destination.clan != source->clan()))
	return false;

    const BattleCreatures creatures = source->toBattleCreatures();
    return std::any_of(creatures.begin(), creatures.end(), [](const BattleCreature* creature)
    {
	return creature && creature->haveSpeciality(Speciality::Gate);
    });
}

bool BattleArmy::canMoveCreature(const BattleCreature & bcr, const Land & fromLand, const Lands & path) const
{
    if(path.empty())
    {
        DEBUG("creature can't move: empty path from " << fromLand.toString());
	return false;
    }

    const Land & toLand = path.back();

    const bool gate = canGateParty(fromLand, toLand);
    const int movementCost = gate ? 1 : static_cast<int>(path.size());

    // check move point
    if(! bcr.canMove(movementCost))
    {
	DEBUG("creature can't move: " << "move cost: " << movementCost << ", " << bcr.toString() << " " << "point: " << bcr.freeMovePoint() << ", " <<
	    "from: " << fromLand.toString() << ", " << "to: " << toLand.toString());
	return false;
    }

    // long jump
    if(!gate && 1 < path.size())
    {
	const CreatureInfo & info = GameData::creatureInfo(bcr);

	if(! info.fly)
	{
	    // check owner: skip dest
	    for(int it = 0; it < path.size() - 1; ++it)
	    {
		const LandInfo & land1 = GameData::landInfo(fromLand);
		const LandInfo & land2 = GameData::landInfo(path[it]);
		if(land1.clan != land2.clan) return false;
	    }
	}
    }

    return true;
}

std::vector<int> BattleArmy::moveSelectedCreatures(const Land & fromLand, const Land & toLand)
{
    std::vector<int> units;
    if(!fromLand.isValid() || !toLand.isValid() || fromLand == toLand) return units;

    const BattleCreatures selected = partySelected(fromLand);
    if(selected.empty()) return units;

    const BattleParty* destination = findPartyConst(toLand);
    const int occupied = destination ? destination->count() : 0;
    if(3 < occupied + static_cast<int>(selected.size())) return units;

    const bool gate = canGateParty(fromLand, toLand);
    Lands path;
    if(gate) path << toLand;
    else path = Lands::pathfind(fromLand, toLand);
    if(path.empty()) return units;

    BattleCreatures ordered = selected;
    if(gate)
    {
	// Keep the portal open until every selected companion has moved.
	std::stable_sort(ordered.begin(), ordered.end(), [](const BattleCreature* left, const BattleCreature* right)
	{
	    const bool leftGate = left && left->haveSpeciality(Speciality::Gate);
	    const bool rightGate = right && right->haveSpeciality(Speciality::Gate);
	    return leftGate < rightGate;
	});
    }

    units.reserve(selected.size());
    for(const BattleCreature* creature : ordered)
    {
	if(!creature || !canMoveCreature(*creature, fromLand, path))
	{
	    units.clear();
	    return units;
	}
	units.push_back(creature->battleUnit());
    }

    BattleArmy planned(*this);
    for(const int unit : units)
    {
	BattleCreature* creature = planned.findBattleUnit(unit);
	if(!creature || !planned.moveCreature(*creature, toLand))
	{
	    units.clear();
	    return units;
	}
    }

    *this = std::move(planned);
    return units;
}

bool BattleArmy::moveCreature(const BattleCreature & bcr, const Land & toLand)
{
    auto it = std::find_if(begin(), end(), [&](BattleParty & party){ return party.findBattleUnit(bcr.battleUnit()); });
    if(it == end())
    {
	ERROR("creature not found, unit: " << bcr.battleUnit());
	return false;
    }

    BattleParty & fromParty = *it;
    BattleParty* toParty = findParty(toLand);
    const Land & fromLand = fromParty.land();

    if(fromLand == toLand)
	return false;

    DEBUG(bcr.toString() << ", from land: " << fromParty.land().toString() << ", to land: " << toLand.toString());

    if(toParty && ! toParty->canJoin())
    {
	ERROR("land can't join: " << toLand.toString());
	return false;
    }

    const bool gate = canGateParty(fromLand, toLand);

    // Gate ignores map distance but still costs one movement point.
    Lands path;
    if(gate) path << toLand;
    else path = Lands::pathfind(fromLand, toLand);
    if(path.empty())
    {
	ERROR("path not found: " << fromLand.toString() << ", " << toLand.toString());
	return false;
    }

    if(! canMoveCreature(bcr, fromLand, path))
    {
        ERROR("creature can't move, path: " << path.toString());
        return false;
    }

    // move
    if(! toParty)
    {
	push_back(BattleParty(fromParty.clan(), toLand));
	toParty = & back();
    }

    BattleCreature* battle = findBattleUnit(bcr.battleUnit());

    if(battle)
    {
	const int battleUnit = battle->battleUnit();
	if(! toParty->join(*battle)) return false;

	BattleCreature* moved = toParty->findBattleUnit(battleUnit);
	if(moved) moved->moved(gate ? 1 : path.size());
    }

    if(! battle) return false;

    // remove
    fromParty.remove(bcr);
    remove_if([](const BattleParty & party){ return party.isEmpty(); });

    return true;
}

bool BattleArmy::teleportCreature(const BattleCreature & bcr, const Land & toLand)
{
    auto it = std::find_if(begin(), end(), [&](BattleParty & party){ return party.findBattleUnit(bcr.battleUnit()); });
    if(it == end() || it->land() == toLand)
	return false;

    BattleParty & fromParty = *it;
    BattleParty* toParty = findParty(toLand);

    if(toParty && ! toParty->canJoin())
	return false;

    if(! toParty)
    {
	push_back(BattleParty(fromParty.clan(), toLand));
	toParty = & back();
    }

    BattleCreature* battle = findBattleUnit(bcr.battleUnit());
    if(! battle || ! toParty->join(*battle))
    {
	shrinkEmpty();
	return false;
    }

    fromParty.remove(bcr);
    shrinkEmpty();
    return true;
}

/* BattleLegend */
JsonObject BattleLegend::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("attacker", attacker.toString());
    jo.addObject("attackers", attackers.toJsonObject());
    jo.addString("defender", defender.toString());
    jo.addObject("defenders", defenders.toJsonObject());
    jo.addObject("town", town.toJsonObject());
    jo.addBoolean("wins", wins);
    return jo;
}

std::string BattleLegend::toString(void) const
{
    return StringFormat("attacker(%1) [%2], defender(%3) [%4], town(%5), result: %6").arg(attacker.toString()).
	arg(attackers.toString()).arg(defender.toString()).arg(defenders.toString()).arg(town.toString()).arg(wins ? "wins" : "loss");
}


BattleLegend BattleLegend::fromJsonObject(const JsonObject & jo)
{
    BattleLegend res;
    res.attacker = Avatar(jo.getString("attacker", "none"));
    res.defender = Avatar(jo.getString("defender", "none"));
    res.wins = jo.getBoolean("wins");

    const JsonObject* bp1 = jo.getObject("attackers");
    if(bp1) res.attackers = BattleParty::fromJsonObject(*bp1);

    const JsonObject* bp2 = jo.getObject("defenders");
    if(bp2) res.defenders = BattleParty::fromJsonObject(*bp2);

    const JsonObject* bt = jo.getObject("town");
    if(bt) res.town = BattleTown::fromJsonObject(*bt);

    return res;
}

/* WinRule */
bool WinRule::operator== (const Stone & st) const
{
    switch(rule())
    {
	case Kong:
	case Pung:	return stone() == st;
	case Chao:	return stone().id() <= st.id() && st.id() <= stone().id() + 2;
	default: break;
    }
    return false;
}

int string2rule(const std::string & str)
{
    if(str == "game")
	return WinRule::Game;
    if(str == "kong")
	return WinRule::Kong;
    if(str == "pung")
	return WinRule::Pung;
    if(str == "chao")
	return WinRule::Chao;

    return WinRule::None;
}

const char* rule2string(int rule)
{
    switch(rule)
    {
	case WinRule::Game: return "game";
	case WinRule::Kong: return "kong";
	case WinRule::Pung: return "pung";
	case WinRule::Chao: return "chao";
	default: break;
    }

    return "none";
}

std::string WinRule::toString(void) const
{
    std::ostringstream os;
    os << "(" << rule2string(rule()) <<
	", " << stone().id() << ")";
    return os.str();
}

JsonObject WinRule::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("rule", rule2string(rule()));
    jo.addString("stone", stone().toString());
    jo.addInteger("flags", flags());
    return jo;
}

WinRule WinRule::fromJsonObject(const JsonObject & jo)
{
    WinRule res(string2rule(jo.getString("rule")), Stone(jo.getString("stone", "none")), false);
    res.flags = jo.getInteger("flags", 0);
    return res;
}

/* WinRules */
WinRules WinRules::fromStones(const Stones & stones2)
{
    WinRules res;

    Stones stones = stones2;
    Stones unique = stones.toUnique();

    for(auto it = unique.begin(); it != unique.end(); ++it)
    {
	int count = stones.countStone(*it);

	// stone not found, because maybe removed through the chao rule.
        if(0 == count) continue;

	// find pung, kong
        if(2 < count)
        {
            int type = WinRule::Pung;

            if(3 < count)
            {
                type = WinRule::Kong;

                // also, may be kong = pung + chao
                if(stones.isChaoStone(*it, 1))
                {
                    // check kong safe
                    type = stones.isChaoStone((*it).next(), 1) ? WinRule::Kong : WinRule::Pung;
                }
            }

	    const WinRule rule(type, *it, false);
            if(! stones.removeWinRule(rule))
		ERROR("stones: " << stones2.toString());
            res << rule;
        }
        else
	if(stones.isChaoStone(*it, 1))
        {
	    const WinRule rule(WinRule::Chao, *it, false);
            if(! stones.removeWinRule(rule))
		ERROR("stones: " << stones2.toString());
            res << rule;
            ++it; ++it; // skip chao
        }
    }

    return res;
}

std::string WinRules::toString(void) const
{
    std::ostringstream os;
    os << "[ ";

    for(auto it = begin(); it != end(); ++it)
	os << (*it).toString() << ", ";

    os << " ]";
    return os.str();
}

WinRules WinRules::copy(int type) const
{
    WinRules res;

    for(auto it = begin(); it != end(); ++it)
	if((*it).rule() == type) res << *it;

    return res;
}

int WinRules::countStoneType(int type) const
{
    return std::count_if(begin(), end(), [=](const WinRule & rule){ return rule.stoneType(type); });
}

int WinRules::count(void) const
{
    return std::accumulate(begin(), end(), 0,
		[](int v, const WinRule & rule){ return v + rule.count(); });
}

JsonArray WinRules::toJsonArray(void) const
{
    JsonArray ja;
    for(auto & rule : *this)
	ja.addObject(rule.toJsonObject());
    return ja;
}

WinRules WinRules::fromJsonArray(const JsonArray & ja)
{
    WinRules res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonObject* jo = ja.getObject(it);
	if(jo) res << WinRule::fromJsonObject(*jo);
    }
    return res;
}

/* CroupierSet */
CroupierSet::CroupierSet() : last(0)
{
    bank.reserve(136); // stones(skull9+sword9+number9+wind4+dragon3) * 4
    trash.reserve(136);
    reset();
}

void CroupierSet::reset(void)
{
    auto stones = { Stone::Skull1, Stone::Skull2, Stone::Skull3, Stone::Skull4, Stone::Skull5, Stone::Skull6, Stone::Skull7, Stone::Skull8, Stone::Skull9,
		    Stone::Sword1, Stone::Sword2, Stone::Sword3, Stone::Sword4, Stone::Sword5, Stone::Sword6, Stone::Sword7, Stone::Sword8, Stone::Sword9,
		    Stone::Number1, Stone::Number2, Stone::Number3, Stone::Number4, Stone::Number5, Stone::Number6, Stone::Number7, Stone::Number8, Stone::Number9,
		    Stone::Wind1, Stone::Wind2, Stone::Wind3, Stone::Wind4,
		    Stone::Dragon1, Stone::Dragon2, Stone::Dragon3 };

    bank.clear();

    bank.insert(bank.end(), stones.begin(), stones.end());
    GameplayRng::shuffle(bank.begin(), bank.end());

    bank.insert(bank.end(), stones.begin(), stones.end());
    GameplayRng::shuffle(bank.begin(), bank.end());

    bank.insert(bank.end(), stones.begin(), stones.end());
    GameplayRng::shuffle(bank.begin(), bank.end());

    bank.insert(bank.end(), stones.begin(), stones.end());
    GameplayRng::shuffle(bank.begin(), bank.end());

    trash.clear();
    luckDraw.clear();
    last = 0;
}

Stone CroupierSet::get(RemotePlayer & client)
{
    Stone res;
    auto it = bank.end();

    if(bank.empty())
    {
	ERROR("cannot draw from an empty rune wall");
	return res;
    }

    if(client.isAffectedSpell(Spell::DrawNumber))
    {
	DEBUG("affected spell over: " << "draw number");
	it = std::find_if(bank.begin(), bank.end(), [](const Stone & st){ return st.isNumber(); });
	client.affectedSpellActivate(Spell::DrawNumber);
    }
    else
    if(client.isAffectedSpell(Spell::DrawSword))
    {
	DEBUG("affected spell over: " << "draw sword");
	it = std::find_if(bank.begin(), bank.end(), [](const Stone & st){ return st.isSword(); });
	client.affectedSpellActivate(Spell::DrawSword);
    }
    else
    if(client.isAffectedSpell(Spell::DrawSkull))
    {
	DEBUG("affected spell over: " << "draw skull");
	it = std::find_if(bank.begin(), bank.end(), [](const Stone & st){ return st.isSkull(); });
	client.affectedSpellActivate(Spell::DrawSkull);
    }

    if(it != bank.end())
    {
	res = *it;
	bank.erase(it);
	DEBUG("new bank status:  " << bank.toString());
    }
    else
    {
	res = bank.back();
	bank.pop_back();
    }

    return res;
}

bool CroupierSet::beginLuckDraw(void)
{
    if(!luckDraw.empty() || bank.size() < 2) return false;

    luckDraw.push_back(bank.back());
    bank.pop_back();
    luckDraw.push_back(bank.back());
    bank.pop_back();
    return true;
}

Stone CroupierSet::resolveLuckDraw(int selected)
{
    if(!hasLuckDraw() || selected < 0 || static_cast<int>(luckDraw.size()) <= selected)
	return Stone();

    const Stone chosen = luckDraw[selected];
    const Stone returned = luckDraw[1 - selected];

    // Put the rejected rune at the bottom so replays and saves remain deterministic.
    bank.insert(bank.begin(), returned);
    luckDraw.clear();
    return chosen;
}

bool CroupierSet::valid(void) const
{
    return bank.size();
}

void CroupierSet::put(const Stone & stone)
{
    if(stone() != Stone::None) trash.push_back(stone);
}

JsonObject CroupierSet::toJsonObject(void) const
{
    JsonObject jo;
    jo.addInteger("last", last);
    jo.addArray("bank", bank.toJsonArray());
    jo.addArray("trash", trash.toJsonArray());
    if(!luckDraw.empty()) jo.addArray("luckDraw", luckDraw.toJsonArray());
    return jo;
}

CroupierSet CroupierSet::fromJsonObject(const JsonObject & jo)
{
    CroupierSet res;
    const JsonArray* ja = nullptr;

    ja = jo.getArray("bank");
    if(ja) res.bank = VecStones::fromJsonArray(*ja);

    ja = jo.getArray("trash");
    if(ja) res.trash = VecStones::fromJsonArray(*ja);

    ja = jo.getArray("luckDraw");
    if(ja)
    {
	VecStones pending = VecStones::fromJsonArray(*ja);
	if(pending.size() == 2)
	    res.luckDraw = pending;
	else
	    res.bank.insert(res.bank.begin(), pending.begin(), pending.end());
    }

    res.last = jo.getInteger("last", 0);
    return res;
}

/* Person */
JsonObject Person::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("avatar", avatar.toString());
    jo.addString("clan", clan.toString());
    jo.addString("wind", wind.toString());
    jo.addInteger("flags", flags());
    return jo;
}

Person Person::fromJsonObject(const JsonObject & jo)
{
    Person res;
    res.avatar = Avatar(jo.getString("avatar", "none"));
    res.clan = Clan(jo.getString("clan", "none"));
    res.wind = Wind(jo.getString("wind", "none"));
    res.flags = jo.getInteger("flags", 0);
    return res;
}

std::string Person::toString(void) const
{
    std::ostringstream os;
    os << "wind: " << wind.toString() << ", " << "avatar: " << name();
    return os.str();
}

std::string Person::name(void) const
{
    const AvatarInfo & info = GameData::avatarInfo(avatar);
    return info.name;
}

/* Persons */
Persons::Persons(const Person & person)
{
    reserve(4);
    push_back(person);

    // add others persons
    std::vector<Clan::clan_t> clans(clans_all);
    clans.erase(std::find(clans.begin(), clans.end(), person.clan.id()));

    while(! clans.empty())
    {
	Avatars avatars = GameData::avatarsOfClan(clans.back());
	auto it = std::remove_if(avatars.begin(), avatars.end(), [&](const Avatar & ava)
	{
	    return std::any_of(begin(), end(),
			[&](const Person & pers){ return pers.avatar == ava; });
	});
	avatars.erase(it, avatars.end());
        GameplayRng::shuffle(avatars.begin(), avatars.end());

	push_back(Person(avatars.front(), clans.back(), Wind()));
	clans.pop_back();
    }

    if(size() == 4)
    {
	// all ai
	for(auto & pers : *this)
	    pers.setAI(true);

	GameplayRng::shuffle(begin(), end());

	at(0).wind = Wind(Wind::East);
	at(1).wind = Wind(Wind::South);
	at(2).wind = Wind(Wind::West);
	at(3).wind = Wind(Wind::North);

	auto it = std::find_if(begin(), end(),
			[&](const Person & pers){ return pers.isAvatar(person.avatar); });

	if(it != end())
	    (*it).setAI(false);
	else
	    ERROR("local person not found: " << person.avatar.toString());
    }
    else
	ERROR("incorrect size");
}

/* LandClaims */
int LandClaims::points(const Clan & clan) const
{
    return clan.isValid() && clan() < static_cast<int>(values.size()) ? values[clan()] : 0;
}

void LandClaims::add(const Clan & clan, int value)
{
    if(clan.isValid() && clan() < static_cast<int>(values.size()) && 0 < value)
        values[clan()] += value;
}

bool LandClaims::spend(const Clan & clan, int value)
{
    if(!clan.isValid() || clan() >= static_cast<int>(values.size()) || value < 0 || values[clan()] < value)
        return false;

    values[clan()] -= value;
    return true;
}

JsonObject LandClaims::toJsonObject(void) const
{
    JsonObject jo;
    for(auto clanId : clans_all)
    {
        const Clan clan(clanId);
        jo.addInteger(clan.toString(), points(clan));
    }
    return jo;
}

LandClaims LandClaims::fromJsonObject(const JsonObject & jo)
{
    LandClaims res;
    for(auto clanId : clans_all)
    {
        const Clan clan(clanId);
        int points = jo.getInteger(clan.toString(), -1);
        if(points < 0)
            points = jo.getInteger(legacyNamedClanId(clan), 0);
        res.values[clan()] = std::max(0, points);
    }
    return res;
}

/* RemotePlayer */
RemotePlayer::RemotePlayer()
{
    points = GameData::bonusStart;
}

RemotePlayer::RemotePlayer(const Person & pers) : Person(pers)
{
    points = GameData::bonusStart;
}

bool RemotePlayer::adventurePartDone(void) const
{
    return flags.check(Person::AdventurePartDone);
}

void RemotePlayer::setAdventurePartDone(void)
{
    flags.set(Person::AdventurePartDone);
}

void RemotePlayer::initAdventurePart(void)
{
    flags.reset(Person::AdventurePartDone);

    const AvatarInfo & avaInfo = GameData::avatarInfo(avatar);
    for(auto & bcr : army.toBattleCreatures())
	bcr->initAdventurePart(avaInfo.ability);
}

Lands RemotePlayer::lands(void) const
{
    return Lands::thisClan(clan);
}

BattleTargets RemotePlayer::toBattleTargets(void) const
{
    return army.toBattleTargets(clan);
}

bool RemotePlayer::mahjongApplySpell(const Spell & spell, const Avatar & source)
{
    switch(spell())
    {
        case Spell::DrawSkull:
        case Spell::DrawSword:
        case Spell::DrawNumber:
	//
        case Spell::RandomDiscard:
	//
        case Spell::ManaFog:
	    affected.insert(AffectedSpell(spell));
            return true;

        case Spell::ScryRunes:
	    affected.insert(AffectedSpell(spell, GameData::spellInfo(spell).duration(), source));
            return true;

        case Spell::Silence:
        {
            // check telepath
            const AvatarInfo & avaInfo = GameData::avatarInfo(avatar);
            if(avaInfo.ability() == Ability::Telepath)
            {
                DEBUG("ability Telepath found, silence skipping...");
                return false;
            }

	    affected.insert(AffectedSpell(spell));
            return true;
        }

        default: ERROR("unknown action" << ", " << "spell: " << spell.toString()); break;
    }

    return false;
}

bool RemotePlayer::isAffectedSpell(const Spell & spell) const
{
    return affected.isAffected(spell);
}

bool RemotePlayer::isAffectedSpell(const Spell & spell, const Avatar & source) const
{
    return affected.isAffected(spell, source);
}

bool RemotePlayer::isSilenced(void) const
{
    return isAffectedSpell(Spell::Silence) &&
           GameData::avatarInfo(avatar).ability() != Ability::Telepath;
}

void RemotePlayer::affectedSpellActivate(const Spell & spell)
{
    affected.spellAffected(spell);
}

JsonObject RemotePlayer::toJsonObject(void) const
{
    JsonObject jo = Person::toJsonObject();
    jo.addArray("rules", rules.toJsonArray());
    jo.addArray("army", army.toJsonArray());
    jo.addInteger("points", points);
    jo.addArray("affected", affected.toJsonArray());
    jo.addObject("landClaims", landClaims.toJsonObject());

    return jo;
}

RemotePlayer RemotePlayer::fromJsonObject(const JsonObject & jo)
{
    RemotePlayer res(Person::fromJsonObject(jo));
    const JsonArray* ja = nullptr;

    ja = jo.getArray("rules");
    if(ja) res.rules = WinRules::fromJsonArray(*ja);

    ja = jo.getArray("army");
    if(ja) res.army = BattleArmy::fromJsonArray(*ja);

    res.points = jo.getInteger("points", 0);

    ja = jo.getArray("affected");
    if(ja) res.affected = AffectedSpells::fromJsonArray(*ja);

    const JsonObject* claims = jo.getObject("landClaims");
    if(claims) res.landClaims = LandClaims::fromJsonObject(*claims);

    return res;
}

/* LocalPlayer */
JsonObject LocalPlayer::toJsonObject(void) const
{
    JsonObject jo = RemotePlayer::toJsonObject();
    jo.addArray("stones", stones.toJsonArray());
    jo.addObject("stone:new", newStone.toJsonObject());

    return jo;
}

LocalPlayer LocalPlayer::fromJsonObject(const JsonObject & jo)
{
    LocalPlayer res(RemotePlayer::fromJsonObject(jo));

    const JsonArray* ja = jo.getArray("stones");
    if(ja) res.stones = GameStones::fromJsonArray(*ja);

    const JsonObject* jj = jo.getObject("stone:new");
    if(jj) res.newStone = GameStone::fromJsonObject(*jj);

    return res;
}

bool LocalPlayer::newTurnEvent(CroupierSet & croupier, bool skipNewStone /* pung, kong, chao */)
{
    DEBUG(toString() << ", " << "stones: " <<  stones.toString() <<  ", " << "rules: " <<  rules.toString());

    setCasted(false);

    if(skipNewStone)
    {
	newStone = GameStone(Stone::None, true);
        DEBUG("new stone: " << "skipped");
    }
    else
    {
	const AvatarInfo & info = GameData::avatarInfo(avatar);
	const bool directedDraw = isAffectedSpell(Spell::DrawNumber) ||
	    isAffectedSpell(Spell::DrawSword) || isAffectedSpell(Spell::DrawSkull);
	if(info.ability() == Ability::Luck && !directedDraw && croupier.beginLuckDraw())
	{
	    newStone = GameStone(Stone::None, true);
	    DEBUG("luck draw: " << croupier.luckChoices().toString());
	    return true;
	}

	newStone = GameStone(croupier.get(*this), false);
	DEBUG("new stone: " << newStone() << ", " << "(" << newStone.toString() << ")");
    }

    return false;
}

void LocalPlayer::initMahjongPart(void)
{
    stones.clear();
    rules.clear();

    newStone.reset();
    flags.reset(Person::Casted);

    const AvatarInfo & avaInfo = GameData::avatarInfo(avatar);
    for(auto & bcr : army.toBattleCreatures())
	bcr->initMahjongPart(avaInfo.ability);
}

bool LocalPlayer::haveKong(void) const
{
    return stones.haveKong();
}

bool LocalPlayer::allowCastSpell(const Spell & spell) const
{
    if(isSilenced() || isAffectedSpell(Spell::ManaFog))
        return false;

    // check avatar info spells
    const AvatarInfo & avaInfo = GameData::avatarInfo(avatar);
    if(avaInfo.spells.end() == std::find(avaInfo.spells.begin(), avaInfo.spells.end(), spell))
    {
	const Spells & armySpells = army.allCastSpells();
	if(armySpells.end() == std::find(armySpells.begin(), armySpells.end(), spell))
	    return false;
    }

    const SpellInfo & spellInfo = GameData::spellInfo(spell);
    return stones.allowCast(spellInfo.stones, newStone);
}

bool LocalPlayer::isMahjongChao(const Wind & currentWind, const Stone & dropStone) const
{
    if(isSilenced())
        return false;

    return dropStone.isValid() && ! dropStone.isSpecial() &&
        wind == currentWind.next() &&
        stones.findChaoVariants(dropStone).size();
}

bool LocalPlayer::isMahjongPung(const Wind & currentWind, const Stone & dropStone) const
{
    if(isSilenced())
        return false;

    if(dropStone.isValid() && currentWind != wind)
        return 1 < stones.countStone(dropStone);

    return false;
}

bool LocalPlayer::isMahjongKong1(const Wind & currentWind, const Stone & dropStone) const
{
    if(isSilenced())
        return false;

    if(dropStone.isValid() && currentWind != wind)
        return 2 < stones.countStone(dropStone);

    return false;
}

bool LocalPlayer::isMahjongKong2(const Wind & currentWind) const
{
    if(isSilenced())
        return false;

    if(newStone.isValid() && currentWind == wind)
    {
        return std::any_of(rules.begin(), rules.end(),
		[&](const WinRule & rule){ return rule.isPungStone(newStone); }) || 3 == stones.countStone(newStone);
    }

    return false;
}

bool LocalPlayer::isWinMahjong(const Wind & currentWind, const Wind & roundWind, const Stone & dropStone, WinResults* winResult) const
{
    if(isSilenced())
        return false;

    Stone winStone;

    if(newStone.isValid())
        winStone = newStone;
    else
    if(dropStone.isValid() && currentWind != wind)
        winStone = dropStone;
    else
        return false;

    Stones stones2 = stones;
    stones2.push_back(winStone);

    Stones pairs = stones2.findPairs();
    if(pairs.empty()) return false;

    // wins: 4 rules and pair
    if(3 < rules.size())
    {
	DEBUG(toString() << ", " << "win stone: " << winStone() <<
		", " << "stones: " << stones.toString() << ", " << "rules: " << rules.toString());
        if(winResult) *winResult = WinResults(currentWind, wind, roundWind, rules, WinRules(), pairs[0], winStone);
        return true;
    }

    for(auto it1 = pairs.begin(); it1 != pairs.end(); ++it1)
    {
        Stones stones3 = stones2;

	bool r1 = stones3.removeStone(*it1);
	bool r2 = stones3.removeStone(*it1);

	if(!r1 || !r2)
	{
	    ERROR("pairs not found: " << (*it1).id() << ", " << "new stone: " << newStone() << ", " <<
		    "drop stone: " << dropStone() << ", " << "stones: " << stones3.toString());
	    return false;
	}

        WinRules rules2 = WinRules::fromStones(stones3);

        if(3 < rules.size() + rules2.size())
        {
	    DEBUG("wind: " << currentWind.toString() << ", " << "win stone: " << winStone() <<
		", " << "stones: " << stones.toString() << ", " << "rules: " << rules.toString() <<
		", " << "rules: " << rules2.toString());
	    if(winResult) *winResult = WinResults(currentWind, wind, roundWind, rules, rules2, *it1, winStone);
            return true;
        }
    }

    return false;
}

Stone LocalPlayer::setMahjongDrop(int indexDrop)
{

    if(stones.size() < indexDrop)
    {
	ERROR("index out of range");
	indexDrop = GameplayRng::uniform(0, stones.size() - 1);
    }

    Stone dropStone;

    if(indexDrop < stones.size())
    {
	dropStone = stones[indexDrop];
	stones.del(indexDrop);
	if(! isAI() && newStone.isValid()) stones.add(newStone);
    }
    else
    {
	dropStone = newStone;
    }

    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " << "drop stone: " << dropStone.id());

    if(isAffectedSpell(Spell::RandomDiscard))
    {
	affectedSpellActivate(Spell::RandomDiscard);
	stones.add(dropStone);
	indexDrop = GameplayRng::uniform(0, stones.size() - 1);
	dropStone = stones[indexDrop];
	stones.del(indexDrop);
	DEBUG("random discard affected" << ", " << "drop stone: " << dropStone.id());
    }

    newStone = GameStone(Stone::None, true);
    points += GameData::bonusPass;

    if(isAffectedSpell(Spell::Silence))
    {
	DEBUG("affected spell turn: " << "silence");
	affectedSpellActivate(Spell::Silence);
    }

    if(isAffectedSpell(Spell::ScryRunes))
    {
	DEBUG("affected spell turn: " << "scry runes");
	affectedSpellActivate(Spell::ScryRunes);
    }

    // The casting turn must not consume the first blocked turn.
    if(isAffectedSpell(Spell::ManaFog) && !isCasted())
    {
	DEBUG("affected spell turn: " << "mana fog");
	affectedSpellActivate(Spell::ManaFog);
    }

    return dropStone;
}

void LocalPlayer::setMahjongChao(const Stone & dropStone, int index)
{
    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " << "drop stone: " << dropStone.id() <<
	    ", " << "variant: " << index);

    Stones variants = stones.findChaoVariants(dropStone);

    // AI select variants
    if(variants.size() < index)
    {
        if(1 < variants.size())
            index = GameplayRng::uniform(0, variants.size() - 1);
        else
        if(variants.size())
            index = 0;
    }

    if(0 <= index && index < variants.size())
    {
	WinRule winRule(WinRule::Chao, variants[index], false);
	stones.add(dropStone);
	stones.remove(winRule);
	rules.push_back(winRule);
	points += GameData::bonusChao;
    }
    else
        ERROR("index out of range");

}

void LocalPlayer::setMahjongPung(const Stone & dropStone)
{
    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " << "drop stone: " << dropStone.id());

    auto it = std::find(stones.begin(), stones.end(), dropStone);

    if(it != stones.end())
    {
        WinRule winRule(WinRule::Pung, dropStone, false);
        stones.add(dropStone);
        stones.remove(winRule);
        rules.push_back(winRule);
        points += GameData::bonusPung;
    }
    else
        ERROR("stone not found: " << dropStone.id());
}

void LocalPlayer::setMahjongKong1(const Stone & dropStone)
{
    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " << "drop stone: " << dropStone.id());

    auto it = std::find(stones.begin(), stones.end(), dropStone);

    if(it != stones.end())
    {
        WinRule winRule(WinRule::Kong, dropStone, false);
        stones.add(dropStone);
        stones.remove(winRule);
        rules.push_back(winRule);
        points += GameData::bonusKong;
    }
    else
        ERROR("stone not found: " << dropStone.id());
}

void LocalPlayer::setMahjongGame(const WinResults & winResult)
{
    WinRules rules2 = winResult.winRulesConcealed();

    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " <<
	    ", " << "rules: " << rules.toString() << ", " << "win stone: " << winResult.lastStone.toString() <<
	    ", " << "win rules: " << rules2.toString());

    for(auto it = rules2.begin(); it != rules2.end(); ++it)
    {
	switch((*it).rule())
	{
	    case WinRule::Chao: points += GameData::bonusChao; break;
	    case WinRule::Pung: points += GameData::bonusPung; break;
	    case WinRule::Kong: points += GameData::bonusKong; break;
	    default: break;
	}
    }

    points += GameData::bonusGame;
}

void LocalPlayer::setMahjongKong2(void)
{
    DEBUG(toString() << ", " << "stones: " << stones.toString() << ", " << "new stone: " << newStone.id());

    if(3 == stones.countStone(newStone))
    {
        WinRule winRule(WinRule::Kong, newStone, true);
        stones.add(newStone);
        newStone = GameStone(Stone::None, true);
        stones.remove(winRule);
        rules.push_back(winRule);
        points += GameData::bonusKong;
    }
    else
    {
	auto it = std::find_if(rules.begin(), rules.end(),
		[&](const WinRule & rule){ return rule.isPungStone(newStone); });

	if(it != rules.end())
	{
	    (*it).upgradeKong();
	    newStone = GameStone(Stone::None, true);
	    points += GameData::bonusKong;
	}
	else
	{
	    ERROR("stone not found: " << newStone.id());
	}
    }
}

/* LocalPlayers */
void LocalPlayers::setPersons(const Persons & persons)
{
    clear();

    for(auto & pers : persons)
	emplace_back(pers);
}

void LocalPlayers::distributeStones(CroupierSet & croupier)
{
    for(int ii = 0; ii < GAME_SET_COUNT; ++ii)
    {
        for(auto & player : *this)
	    player.stones.add(GameStone(croupier.get(player), false));
    }
}

LocalPlayer* LocalPlayers::playerOfClan(const Clan & clan)
{
    auto it = std::find_if(begin(), end(), [&](const LocalPlayer & lp){ return lp.isClan(clan); });
    return it != end() ? & (*it) : nullptr;
}

LocalPlayer* LocalPlayers::playerOfWind(const Wind & wind)
{
    auto it = std::find_if(begin(), end(), [&](const LocalPlayer & lp){ return lp.isWind(wind); });
    return it != end() ? & (*it) : nullptr;
}

LocalPlayer* LocalPlayers::playerOfAvatar(const Avatar & ava)
{
    auto it = std::find_if(begin(), end(), [&](const LocalPlayer & lp){ return lp.isAvatar(ava); });
    return it != end() ? & (*it) : nullptr;
}

void LocalPlayers::shiftWinds(void)
{
    for(auto & lp : *this)
	lp.shiftWind();
}

bool LocalPlayers::findKongs(void) const
{
    return std::any_of(begin(), end(), [](const LocalPlayer & lp){ return lp.haveKong(); });
}

JsonArray LocalPlayers::toJsonArray(void) const
{
    JsonArray ja;
    for(auto & lp : *this)
	ja.addObject(lp.toJsonObject());
    return ja;
}

LocalPlayers LocalPlayers::fromJsonArray(const JsonArray & ja)
{
    LocalPlayers res;
    for(int it = 0; it < ja.size(); ++it)
    {
	const JsonObject* jo = ja.getObject(it);
	if(jo) res.push_back(LocalPlayer::fromJsonObject(*jo));
    }
    return res;
}

/* WinResults */
enum { AllConcealedWithDiscard = 0x80000000 };

WinResults::WinResults(const Wind & wind1, const Wind & wind2, const Wind & wind3, const WinRules & winRules1, const WinRules & winRules2, const Stone & winPair, const Stone & winStone)
{
    dealWind = wind1;
    winWind = wind2;
    roundWind = wind3;

    if(winRules1.empty())
	flags.set(AllConcealedWithDiscard);

    for(auto & rule : winRules1)
	if(rules.size() < 4) rules.push_back(rule);

    for(auto & rule : winRules2)
	if(rules.size() < 4) rules.emplace_back(rule.rule(), rule.stone(), true);

    pairStone = winPair;
    lastStone = winStone;
}

WinResults WinResults::drawn(const Wind & deal, const Wind & round)
{
    WinResults res;
    res.dealWind = deal;
    res.roundWind = round;
    return res;
}

WinRules WinResults::winRulesConcealed(void) const
{
    WinRules res;
    for(auto & rule : rules)
	if(rule.isConcealed()) res.push_back(rule);
    return res;
}

std::string WinResults::toString(void) const
{
    std::ostringstream os;

    os <<"deal" << "(" << dealWind.toString() << ")" << ", " <<
	"win" << "(" << winWind.toString() << ")" << ", " <<
	"round" << "(" << roundWind.toString() << ")" << ", ";

    os << "rules: " << "[ ";
    for(auto it = rules.begin(); it != rules.end(); ++it)
	os << (*it).toString() << ", ";
    os << " ]" << ", ";

    os << "pair" << "(" << pairStone() << ")" << ", " << "last" << "(" << lastStone() << ")";

    return os.str();
}

JsonObject WinResults::toJsonObject(void) const
{
    JsonObject jo;
    jo.addString("wind:deal", dealWind.toString());
    jo.addString("wind:win", winWind.toString());
    jo.addString("wind:round", roundWind.toString());
    jo.addString("stone:pair", pairStone.toString());
    jo.addString("stone:last", lastStone.toString());
    jo.addInteger("flags", flags());
    jo.addArray("rules", rules.toJsonArray());
    return jo;
}

WinResults WinResults::fromJsonObject(const JsonObject & jo)
{
    WinResults res;
    res.dealWind = Wind(jo.getString("wind:deal", "none"));
    res.winWind = Wind(jo.getString("wind:win", "none"));
    res.roundWind = Wind(jo.getString("wind:round", "none"));
    res.pairStone = Stone(jo.getString("stone:pair", "none"));
    res.lastStone = Stone(jo.getString("stone:last", "none"));
    res.flags = jo.getInteger("flags", 0);

    const JsonArray* ja = nullptr;

    ja = jo.getArray("rules");
    if(ja) res.rules = WinRules::fromJsonArray(*ja);

    return res;
}

/* HandBonus */
std::string HandBonus::name(void) const
{
    switch(type())
    {
	case OneChance:			return _("One Chance");
	case SelfDrawn:			return _("Self Drawn");
	case AllConcealedWithDiscard:	return _("All Concealed, with Discard");
	default: break;
    }

    return "None";
}

/* DoubleBonus */
std::string DoubleBonus::name(void) const
{
    switch(type())
    {
	case LuckySets:			return _("Lucky Sets");
	case OneSuitHonors:		return _("One Suit plus Honors");
	case NoPoints:			return _("No Points");
	case FourTriplets:		return _("Four Triplets");
	case TerminalsHonors:		return _("Terminals and Honors");
	case TerminalHonorEachSet:	return _("Terminal, Honor in each set");
	case AllSimples:		return _("All Simples");
	case ThreeLittleDragons:	return _("Three Little Dragons");
	case OneSuitOnly:		return _("One Suit Only");
	case ThreeConsecutiveSequences:	return _("Three Consecutive Sequences");
	case AllConcealedSelfDrawn:	return _("All Concealed, Self Drawn");
	case LastTileWall:		return _("Last Tile");
	case ThreeConcealedTriplets:	return _("Three Concealed Triplets");
	case RobbedKong:		return _("Robbed a Kong");
	case SupplementalTile:		return _("Supplemental Tile");
	case ThreeBigDragons:		return _("Three Big Dragons");
	case BigFourWinds:		return _("Big Four Winds");
	case AllTerminals:		return _("All Terminals");
	case AllHonors:			return _("All Honors");
	case NineGates:			return _("Nine Gates");
	case HeavenlyHand:		return _("Heavenly Hand");
	case FourConcealedTriplets:	return _("Four Concealed Triplets");
	case ThirteenOrphans:		return _("Thirteen Orphans");
	case LittleFourWinds:		return _("Little Four Winds");
	case EarthlyHand:		return _("Earthly Hand");
	default: break;
    }

    return "None";
}

/* RuneBonus */
Stones RuneBonus::stones(void) const
{
    Stones res; res.reserve(5);
    if(isKong())
	res.assign(4, stone());
    else
    if(isPung())
	res.assign(3, stone());
    else
    if(isPair())
	res.assign(2, stone());
    else
    if(isChao())
	res << stone() << stone().next() << stone().next().next();
    return res;
}

RuneBonusList WinResults::bonusRunes(void) const
{
    RuneBonusList res;

    for(auto & winRule : rules)
    {
	const Stone & stone = winRule.stone();
	const int & rule = winRule.rule();
	const bool & concealed = winRule.isConcealed();
	int type = 0; int value = 0;

        switch(rule)
        {
	    case WinRule::Pung:
		if(stone.isHonor() || stone.isTerminal())
		{
		    type |= RuneBonus::RunePung | (stone.isHonor() ? RuneBonus::RuneHonor : RuneBonus::RuneTerminal);
		    value = concealed ? 8 : 4;
		}
		else
		{
		    type |= RuneBonus::RunePung | RuneBonus::RuneSimple;
		    value = concealed ? 4 : 2;
		}
		break;

	    case WinRule::Kong:
		if(stone.isHonor() || stone.isTerminal())
		{
		    type |= RuneBonus::RuneKong | (stone.isHonor() ? RuneBonus::RuneHonor : RuneBonus::RuneTerminal);
		    value = concealed ? 32 : 16;
		}
		else
		{
		    type |= RuneBonus::RuneKong | RuneBonus::RuneSimple;
		    value = concealed ? 16 : 8;
		}
		break;

	    case WinRule::Chao:
		    type |= RuneBonus::RuneChao | RuneBonus::RuneSimple;
		    value = 0;
		break;

	    default:
		break;
        }

	if(stone.isWind())
	{
	    if(stone.isWind(roundWind)) type |= RuneBonus::RuneLucky;
	    if(stone.isWind(winWind)) type |= RuneBonus::RuneLucky;
	}
	else
	if(stone.isDragon())
	{
	    type |= RuneBonus::RuneLucky;
	}

	if(concealed) type |= RuneBonus::RuneConcealed;
	res << RuneBonus(stone, type, value);
    }

    // add pair
    int type = RuneBonus::RunePair;
    if(lastStone != pairStone) type |= RuneBonus::RuneConcealed;

    res << RuneBonus(pairStone, type, pairBonus());

    return res;
}

Wind OpponentFine::wind(void) const
{
    switch(type())
    {
	case Wind::East:	return Wind::East;
	case Wind::West:	return Wind::West;
	case Wind::South:	return Wind::South;
	case Wind::North:	return Wind::North;
	default: break;
    }

    return Wind::None;
}

int WinResults::scoreRules(void) const
{
    int res = 0;
    RuneBonusList list = bonusRunes();
    for(auto it = list.begin(); it != list.end(); ++it) res += (*it).value();
    return res;
}

bool WinResults::noPoints(void) const
{
    return 0 == scoreRules();
}

int WinResults::baseScore(void) const
{
    return 20;
}

int WinResults::pairBonus(void) const
{
    // pair bonus: double wind
    if(pairStone.isWind(roundWind) && pairStone.isWind(winWind))
        return 4;
    else
    // pair bonus: lucky runes
    if(pairStone.isDragon() || pairStone.isWind(roundWind) || pairStone.isWind(winWind))
        return 2;

    return 0;
}

HandBonusList WinResults::bonusHands(void) const
{
    HandBonusList res;

    // 2 points for "One Chance" and "Self Drawn" are not counted if the hand would otherwise be "No Points".
    if(! noPoints())
    {
	// other bonus: one chance hand
	if(pairStone == lastStone || pairStone.isLucky())
	    res << HandBonus(HandBonus::OneChance, 2);
	else
	{
	    if(std::any_of(rules.begin(), rules.end(),
		[&](const WinRule & rule){ return rule.isOneChance(pairStone); }))
		res << HandBonus(HandBonus::OneChance, 2);
	}

	// other bonus: self-drawn hand
	if(isSelfDrawn())
	    res << HandBonus(HandBonus::SelfDrawn, 2);
    }

    // other bonus: Concealed hand with	discarded tile
    if(flags.check(AllConcealedWithDiscard))
	res << HandBonus(HandBonus::AllConcealedWithDiscard, 10);

    return res;
}

int WinResults::totalPoints(void) const
{
    int res = baseScore();

    res += scoreRules();
    res += pairBonus();

    for(auto & bonus : bonusHands())
	res += bonus.value();

    return res;
}

DoubleBonusList WinResults::bonusDoubles(void) const
{
    DoubleBonusList res;

    // No Points: 1
    if(noPoints())
	res << DoubleBonus(DoubleBonus::NoPoints, 1);

    // Little three dragons: 1
    // (3 dragon sets, one being a pair)
    if(pairStone.isDragon())
    {
	if(2 == rules.countStoneType(StoneType::IsDragon))
	    res << DoubleBonus(DoubleBonus::ThreeLittleDragons, 1);
    }
    else
    // little four winds: 1
    if(pairStone.isWind())
    {
	if(3 == rules.countStoneType(StoneType::IsWind))
	    res << DoubleBonus(DoubleBonus::LittleFourWinds, 1);
    }

    // three big dragons
    if(3 == rules.countStoneType(StoneType::IsDragon))
    {
	res << DoubleBonus(DoubleBonus::ThreeBigDragons, 5);
    }
    // big four winds
    else
    if(4 == rules.countStoneType(StoneType::IsWind))
    {
	res << DoubleBonus(DoubleBonus::BigFourWinds, 5);
    }

    // Lucky Sets:
    {
	int value = 0;

	for(auto & rule : rules)
	{
	    // pung, kong only
	    if(! rule.isChao())
	    {
		// Double Wind triplet or foursome: 2
		if(rule.stone().isWind(roundWind) && rule.stone().isWind(winWind))
		    value += 2;
		else
		// Lucky triplet or foursome: 1
		if((rule.stone().isDragon() || rule.stone().isWind(roundWind) || rule.stone().isWind(winWind)))
		    value += 1;
	    }
	}

	if(value) res << DoubleBonus(DoubleBonus::LuckySets, value);
    }

    // Four triplets or foursomes:
    // 0, 1, or 2 concealed: 1
    // 3 concealed: 2
    // 4 concealed: Limit (5 - 500pt)
    int chaoCount = std::count_if(rules.begin(), rules.end(), [](const WinRule & rule){ return rule.isChao(); });

    if(0 == chaoCount)
    {
	int concealed = std::count_if(rules.begin(), rules.end(),
			[](const WinRule & rule){ return rule.isConcealed(); });

	if(3 < concealed)
	    res << DoubleBonus(DoubleBonus::FourConcealedTriplets, 5);
	else
	    res << DoubleBonus(DoubleBonus::FourTriplets, 2 < concealed ? 2 : 1);
    }

    // Three concealed triplets, one sequence: 1
    if(1 == chaoCount)
    {
	int concealed = std::count_if(rules.begin(), rules.end(),
			[](const WinRule & rule){ return ! rule.isChao() && rule.isConcealed(); });

	if(3 == concealed)
	    res << DoubleBonus(DoubleBonus::ThreeConcealedTriplets, 1);
    }

    // Three consecutive sequences: 1
    // (1,2,3 4,5,6 7,8,9 of same suit)
    {
	WinRules tmp = rules;
	auto itend = std::remove_if(tmp.begin(), tmp.end(),
		    [](const WinRule & rule){ return rule.stoneOrder(1) || rule.stoneOrder(4) || rule.stoneOrder(7); });

	if(3 <= std::distance(itend, tmp.end()))
	{
	    int type = 0;

	    if(3 == std::count_if(itend, tmp.end(), [](const WinRule & rule) { return rule.stoneType(StoneType::IsSkull); }))
		type = StoneType::IsSkull;
	    else
	    if(3 == std::count_if(itend, tmp.end(), [](const WinRule & rule) { return rule.stoneType(StoneType::IsNumber); }))
		type = StoneType::IsNumber;
	    else
	    if(3 == std::count_if(itend, tmp.end(), [](const WinRule & rule) { return rule.stoneType(StoneType::IsSword); }))
		type = StoneType::IsSword;

	    if(type)
		res << DoubleBonus(DoubleBonus::ThreeConsecutiveSequences, 1);
	}
    }

    // All Terminals and Honors: 1
    if(pairStone.isTerminal() || pairStone.isHonor())
    {
	int terminals = std::count_if(rules.begin(), rules.end(), [](const WinRule & rule) { return rule.stoneTerminal(); });
	int honors = std::count_if(rules.begin(), rules.end(), [](const WinRule & rule) { return rule.stoneHonor(); });

	if(4 == terminals && pairStone.isTerminal())
	    res << DoubleBonus(DoubleBonus::AllTerminals, 5);
	else
	if(4 == honors && pairStone.isHonor())
	    res << DoubleBonus(DoubleBonus::AllHonors, 5);
	else
	if(4 == terminals + honors)
	    res << DoubleBonus(DoubleBonus::TerminalsHonors, 1);
    }
    else
    // All Simples: 1
    {
	int terminals = std::count_if(rules.begin(), rules.end(), [](const WinRule & rule) { return rule.stoneTerminal(); });
	int honors = std::count_if(rules.begin(), rules.end(), [](const WinRule & rule) { return rule.stoneHonor(); });

	if(0 == terminals + honors)
	    res << DoubleBonus(DoubleBonus::AllSimples, 1);

    }

    // One suit only: 4
    if(std::all_of(rules.begin(), rules.end(), [&](const WinRule & rule) { return rule.stoneType(pairStone.stoneType()); }))
	res << DoubleBonus(DoubleBonus::OneSuitOnly, 4);

    // One suit with honors: 1
    {
	WinRules tmp = rules;
	auto itend = std::remove_if(tmp.begin(), tmp.end(), [](const WinRule & rule){ return rule.stoneHonor(); });
	int type = 0;

	if(! pairStone.isHonor())
	    type = pairStone.stoneType();
	else
	if(tmp.begin() != itend)
	    type = tmp.front().stone().stoneType();

	if(type && std::all_of(tmp.begin(), itend, [=](const WinRule & rule){ return rule.stoneType(type); }))
	    res << DoubleBonus(DoubleBonus::OneSuitHonors, 1);
    }

//    last:
//    TerminalHonorEachSet,
//    SupplementalTile, LastTileWall,

//    AllConcealedSelfDrawn, RobbedKong, HeavenlyHand, EarthlyHand

//Terminal or honor in each set           1 (only applies if the hand contains at least one sequence)
//
//Ways of Going Out
//     Supplemental Tile                  1
//     Last Tile of Wall                  1

    return res;
}

int WinResults::totalScore(void) const
{
    int doubles = 0;

    DoubleBonusList list = bonusDoubles();
    for(auto it = list.begin(); it != list.end(); ++it) doubles += (*it).value();

    int res = baseScore() * scoreMultiplier(doubles);
    if(res > 500) res = 500;

    return res;
}

int WinResults::scoreMultiplier(int doubles)
{
    if(doubles <= 0) return 1;
    if(5 < doubles) doubles = 5;
    return 1 << doubles;
}

OpponentFinesList WinResults::opponentFines(void) const
{
    OpponentFinesList res;

    // If dealer wins self-drawn:            Receives x2 from each player.
    // If dealer wins from discard:          Receives x6 from discarder.
    // If non-dealer wins self drawn:        Receives x2 from dealer, Receives x1 from other players.
    // If non-dealer wins from discard:      Receives x4 from discarder.

    Wind windWin = winWind;
    Wind windDeal(Wind::East);
    Wind windCurrent = dealWind;

    if(windWin == windDeal)
    {
        if(windWin == windCurrent)
        {
	    for(auto & id : winds_all)
	    {
                if(windWin.id() != id)
                    res << OpponentFine(id, 2);
	    }
        }
        else
        {
            res << OpponentFine(windCurrent, 6);
        }
    }
    else
    {
        if(windWin == windCurrent)
        {
	    for(auto & id : winds_all)
	    {
                if(windWin.id() != id)
                    res << OpponentFine(id, (windCurrent.id() == id ? 2 : 1));
	    }
        }
        else
        {
            res << OpponentFine(windCurrent, 4);
        }
    }

    return res;
}
