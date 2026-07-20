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

}

namespace GameData
{
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
