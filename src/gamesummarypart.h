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

#ifndef _RWNA_GAMESUMMARYPART_
#define _RWNA_GAMESUMMARYPART_

#include <cstdint>

#include "gamedata.h"
#include "matchscore.h"

namespace GameSummaryInput
{
    constexpr std::uint64_t ScorePageGuardMilliseconds = 350;

    inline bool blocksScorePageDone(std::uint64_t openedAt, std::uint64_t now)
    {
        return openedAt != 0 && openedAt <= now &&
               now - openedAt < ScorePageGuardMilliseconds;
    }
}

class GameSummaryScreen : public JsonWindow
{
    enum class Page
    {
        Victory,
        Scores
    };

    std::list<JsonTextInfo> labels;
    std::list<JsonTextInfo> values;
    JsonButton*         buttonNext;
    MatchScore::Results scores;
    std::vector<std::size_t> winners;
    std::size_t         winnerPage;
    Page                page;
    std::uint64_t       scoresOpenedAtMilliseconds;
    Texture             victoryArt;
    Rect                victoryPanel;
    Color               victoryPanelColor;
    Color               victoryBorderColor;
    JsonTextInfo        victoryTitle;
    JsonTextInfo        victoryName;
    JsonTextInfo        victoryDetails;
    JsonTextInfo        victoryWinners;
    JsonTextInfo        victoryHint;
    Rect                summaryPortraitArea;
    Rect                summaryWinnerArea;
    std::vector<Texture> summaryPortraits;
    JsonTextInfo        summaryTitle;
    JsonTextInfo        summaryWinners;
    JsonTextInfo        summaryDetails;

    void                updateVictoryPage(void);
    void                advanceVictoryPage(void);
    std::string         winnerNames(void) const;

protected:
    bool                keyPressEvent(const KeySym &) override;
    bool                mouseClickEvent(const ButtonsEvent &) override;
    bool		userEvent(int, void*) override;

public:
    GameSummaryScreen();

    void		renderWindow(void) override;
};

#endif
