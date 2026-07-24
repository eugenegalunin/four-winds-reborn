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
 ***************************************************************************/

#include <sstream>

#include "gameobjects.h"

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
    const JsonValue* jv = ja.getValue(0);
    const int v1 = jv ? jv->getInteger() : 0;

    jv = ja.getValue(1);
    const int v2 = jv ? jv->getInteger() : 0;

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
    const JsonArray* ja = jo.getArray("melee");
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

BattleStrike::BattleStrike(const BattleUnit & source, int damageValue,
                           const BattleUnit & target, int strikeType)
    : unit1(source.battleUnit()), is_creature1(source.isCreature()),
      unit2(target.battleUnit()), is_creature2(target.isCreature()),
      damage(damageValue), type(strikeType)
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
    return StringFormat("attacker(%1) [%2], defender(%3) [%4], town(%5), result: %6")
        .arg(attacker.toString()).arg(attackers.toString())
        .arg(defender.toString()).arg(defenders.toString())
        .arg(town.toString()).arg(wins ? "wins" : "loss");
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
