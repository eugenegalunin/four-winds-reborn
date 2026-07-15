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

#ifndef _RWNA_SELECTPERSON_
#define _RWNA_SELECTPERSON_

#include "gamedata.h"
#include "aiprofile.h"

class AvatarItem : public ListWidgetItem
{
    Avatar		id;

public:
    AvatarItem(const Avatar &, ListWidget &);
    void		renderWindow(void) override;

    const Avatar & 	avatar(void) const { return id; }
    void		setDisabled(bool f);
};

class SelectPersonScreen : public JsonWindow
{
    JsonButton*		buttonOk;
    JsonButton*		buttonClose;

    Avatar		selectedAvatar;
    Clan		selectedClan;
    AI::Difficulty      difficulty;
    Rect                difficultyArea;
    JsonTextInfo        difficultyTitle;
    JsonTextInfo        difficultyValue;

    Texture		gray, marker;
    Point		personImagePos, creaturesPos, textPos;

    JsonTextInfo	personName1, personName2, personDescription, personClan, personClans, spellText;
    JsonTextInfo	supplementialSpells, specialAbilities;

    std::list<AvatarIcon> avatarsIcon;
    std::list<ClanIcon>	clansIcon;
    std::list<CreatureIcon> creaturesIcon;
    std::list<SpellIcon> spellsIcon;

    bool	actionButtonOk(void);
    bool	actionButtonClose(void);
    bool	actionClickPersons(const ButtonsEvent &);
    bool	actionClickClans(const ButtonsEvent &);

protected:
    bool	mouseClickEvent(const ButtonsEvent &) override;
    bool	userEvent(int, void*) override;
    bool	keyPressEvent(const KeySym &) override;

public:
    SelectPersonScreen();

    void	renderWindow(void) override;
    Person	selectedPerson(void) const;
    AI::Difficulty selectedDifficulty(void) const { return difficulty; }
};

#endif
