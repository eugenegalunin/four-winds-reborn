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

#include "battlesession.h"

GameData::PendingBattle GameData::pendingBattle;

bool GameData::PendingBattle::isValid(void) const
{
    return attacker.isValid() && defender.isValid() && legend.land().isValid() &&
           session.isValid();
}

BattleLegend GameData::PendingBattle::choiceLegend(void) const
{
    BattleLegend result = legend;
    result.attackers = session.attackers();
    result.defenders = session.defenders();
    result.town = session.town();
    return result;
}

JsonObject GameData::PendingBattle::toJsonObject(void) const
{
    JsonObject result;
    result.addString("attacker", attacker.toString());
    result.addString("defender", defender.toString());
    result.addObject("legend", legend.toJsonObject());
    result.addObject("session", session.toJsonObject());
    return result;
}

GameData::PendingBattle GameData::PendingBattle::fromJsonObject(const JsonObject & object)
{
    PendingBattle result;
    result.attacker = Avatar(object.getString("attacker"));
    result.defender = Avatar(object.getString("defender"));
    const JsonObject* nested = object.getObject("legend");
    if(nested) result.legend = BattleLegend::fromJsonObject(*nested);
    nested = object.getObject("session");
    if(nested) result.session = Battle::Session::fromJsonObject(*nested);
    return result;
}
