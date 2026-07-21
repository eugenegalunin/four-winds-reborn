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

#ifndef _RWNA_ADVENTUREUIEVENTS_
#define _RWNA_ADVENTUREUIEVENTS_

enum AdventureUiEvent
{
    LandPolygonClickLeft = 1111,
    LandPolygonClickRight,
    LandPolygonFocus,
    LandPolygonFlagAnimationReInit,
    LandPolygonCombatStatus,
    LandPolygonCombatStatusReset,
    ClanIconClickLeft,
    ClanIconClickRight,
    CreatureIconClickLeft,
    CreatureIconClickRight,
    MapScreenClose,
    MapScreenSelectLand,
    AdventureTurnPlayer,
    AdventureTurnMoveStart,
    AdventureTurnMoveStop,
    AdventureTurnCreatureSelect,
    AdventureTurnShowConsole
};

#endif
