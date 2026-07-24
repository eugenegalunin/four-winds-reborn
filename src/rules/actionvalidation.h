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

#ifndef _RWNA_ACTION_VALIDATION_
#define _RWNA_ACTION_VALIDATION_

#include "gamedata.h"

namespace GameData
{
    class ScopedActionRejection
    {
        ActionRejection* previous;

    public:
        explicit ScopedActionRejection(ActionRejection* current);
        ~ScopedActionRejection();

        ScopedActionRejection(const ScopedActionRejection &) = delete;
        ScopedActionRejection & operator=(const ScopedActionRejection &) = delete;
    };

    bool rejectAction(ActionRejectReason, int available = -1, int required = -1);
}

#endif
