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

#ifndef _RWNA_RUNEGAMETARGETDIALOGS_
#define _RWNA_RUNEGAMETARGETDIALOGS_

#include "adventurepart.h"

class ShowMapDialog : public MapScreenBase
{
    void renderLabel(void) override;

public:
    ShowMapDialog(const LocalData &, Window &);
};

class ShowSummonCreatureDialog : public MapScreenBase
{
    void renderLabel(void) override;
    bool landAllowJoin(const LandInfo &, const LocalPlayer &);

protected:
    bool userEvent(int, void*) override;

public:
    ShowSummonCreatureDialog(const LocalData &, const Creature &, Window &);
    const Land & land(void) const;
};

class ShowCastSpellDialog : public MapScreenBase
{
    Spell spell;
    int targetUnit;
    Land targetSource;
    void renderLabel(void) override;

protected:
    bool userEvent(int, void*) override;

public:
    ShowCastSpellDialog(const LocalData &, const Spell &, Window &);
    const Land & land(void) const;
    int unit(void) const;
};

#endif
