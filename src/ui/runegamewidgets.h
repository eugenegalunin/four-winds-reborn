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

#ifndef _RWNA_RUNEGAMEWIDGETS_
#define _RWNA_RUNEGAMEWIDGETS_

#include "gametheme.h"
#include "dialogs.h"

class StoneSprite : public Stone, public Sprite
{
public:
    enum { Small = 1, Medium = 2, Large = 3 };

    StoneSprite() {}
    StoneSprite(const Stone &, int);
    StoneSprite(const Stone &, int, const Point &);

    void set(const Stone &, int);
};

class LuckDrawDialog : public DialogWindow
{
    StoneSprite first;
    StoneSprite second;
    Color selectedColor;
    Point hintPos;
    int selected;

    void choose(int);

protected:
    bool mouseClickEvent(const ButtonsEvent &) override;
    bool keyPressEvent(const KeySym &) override;

public:
    LuckDrawDialog(const VecStones &, Window &);

    void renderWindow(void) override;
};

#endif
