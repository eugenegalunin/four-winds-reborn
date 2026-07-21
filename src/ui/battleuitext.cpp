/***************************************************************************
 *   Copyright (C) 2020 by RuneWarsNA team <runewars.newage@gmail.com>     *
 *                                                                         *
 *   Part of the RuneWars: NewAge engine.                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "gametheme.h"
#include "gamedata.h"
#include "battleuitext.h"

namespace BattleUiText
{
std::string strikePhase(int type)
{
    if(type == BattleStrike::Ranger) return _("Missile volley");
    if(type == BattleStrike::FireShield) return _("Fire Shield");
    return _("Melee");
}

std::string strikeSummary(const BattleStrike & strike)
{
    return std::string("#").append(std::to_string(strike.unit1))
        .append(" -> #").append(std::to_string(strike.unit2))
        .append(": ").append(std::to_string(strike.damage))
        .append(" ").append(_("damage"));
}

std::string choicePhase(const std::string & phase, int choiceNumber,
                        int choiceCount)
{
    if(phase == "opening_leader") return _("Opening leader");
    if(phase == "attacker_ranged")
    {
        if(0 < choiceNumber && 0 < choiceCount)
            return StringFormat(_("Missile assignment %1/%2"))
                .arg(choiceNumber).arg(choiceCount);
        return _("Missile assignment");
    }
    if(0 < choiceNumber)
        return StringFormat(_("Melee action %1")).arg(choiceNumber);
    return _("Melee action");
}
}
