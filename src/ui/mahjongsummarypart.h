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

#ifndef _RWNA_MAHJONGSUMMARYPART_
#define _RWNA_MAHJONGSUMMARYPART_

#include "gamedata.h"

class MahjongSummaryPartScreen : public JsonWindow
{
    LocalData		ld;

    Color               defaultColor;
    std::string         defaultFont;
    JsonButton*		buttonNext;

    int			multiplier;
    int			totalScore;

    RuneBonusList	runeBonusList;
    DoubleBonusList	doubleBonusList;
    HandBonusList	handBonusList;
    OpponentFinesList	opponentFinesList;

    Texture		markLeftSprite;
    Texture		markRightSprite;
    Texture		luck1Sprite;
    Texture		luck2Sprite;

    Point		winRunePos;
    Point		winRunesPos;
    Point		doublesOffset;

    JsonTextInfo	pointsText;
    JsonTextInfo	doublesText;
    JsonTextInfo	baseScoreText;
    JsonTextInfo	totalPointsText;
    JsonTextInfo	totalScoreText;
    JsonTextInfo	multiplierText;
    JsonTextInfo	finesText;

    std::list<JsonTextInfo> labels;

    void		renderWinStones(void);

protected:
    bool		keyPressEvent(const KeySym &) override;
    bool		userEvent(int, void*) override;

public:
    MahjongSummaryPartScreen();

    void		renderWindow(void) override;
};

#endif
