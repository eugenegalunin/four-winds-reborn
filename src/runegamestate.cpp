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

#include <numeric>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "gameplayrng.h"
#include "gamedata.h"

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
