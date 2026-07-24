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

#ifndef _RWNA_SHOWPLAYERS_
#define _RWNA_SHOWPLAYERS_

#include "gamedata.h"

class ShowPlayersScreen : public JsonWindow
{
    Persons		persons;

    std::string		font;
    Color		selectedColor, otherColor;

    JsonButton*         buttonStart;
    JsonButton*         buttonCancel;

    bool		actionButtonStart(void);
    bool 		actionButtonCancel(void);
    Size		renderPerson(const Person &, bool selected, const Point &);

protected:
    bool		userEvent(int, void*) override;

public:
    ShowPlayersScreen();

    void		renderWindow(void) override;
};

#endif
