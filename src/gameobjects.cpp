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
