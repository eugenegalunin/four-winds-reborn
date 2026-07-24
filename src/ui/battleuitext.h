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

#ifndef _RWNA_BATTLE_UI_TEXT_
#define _RWNA_BATTLE_UI_TEXT_

#include <string>

struct BattleStrike;

namespace BattleUiText
{
    std::string strikePhase(int);
    std::string strikeSummary(const BattleStrike &);
    std::string choicePhase(const std::string &, int choiceNumber,
                            int choiceCount);
}

#endif
