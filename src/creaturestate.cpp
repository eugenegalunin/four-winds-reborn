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

#include <algorithm>
#include <sstream>

#include "gamedata.h"

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
