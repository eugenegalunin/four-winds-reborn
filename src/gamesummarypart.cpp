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

#include <algorithm>

#include "settings.h"
#include "actions.h"
#include "gametheme.h"
#include "gamesummarypart.h"
#include "matchscore.h"

GameSummaryScreen::GameSummaryScreen() : JsonWindow("screen_game_summary.json", nullptr)
{
    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:category"));
    labels.back().text = _("Category");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:score1"));
    labels.back().text = _("Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:score2"));
    labels.back().text = _("Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:score3"));
    labels.back().text = _("Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:score4"));
    labels.back().text = _("Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:rank1"));
    labels.back().text = _("Rank");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:rank2"));
    labels.back().text = _("Rank");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:rank3"));
    labels.back().text = _("Rank");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:rank4"));
    labels.back().text = _("Rank");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:territory"));
    labels.back().text = _("Territory Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:summon"));
    labels.back().text = _("Summon Circle Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:unit"));
    labels.back().text = _("Unit Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spell"));
    labels.back().text = _("Spell Point Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:land"));
    labels.back().text = _("Land Claim Score");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:total"));
    labels.back().text = _("Total Score");

    const std::array<const char*, 4> scoreKeys = {
        "textinfo:score1", "textinfo:score2", "textinfo:score3", "textinfo:score4"
    };
    const std::array<const char*, 4> rankKeys = {
        "textinfo:rank1", "textinfo:rank2", "textinfo:rank3", "textinfo:rank4"
    };
    const std::array<int, MatchScore::CategoryCount> rowY = { 286, 320, 374, 410, 446 };
    const MatchScore::Results scores = MatchScore::current();

    for(std::size_t playerIndex = 0;
        playerIndex < scores.size() && playerIndex < scoreKeys.size(); ++playerIndex)
    {
        const MatchScore::PlayerResult & player = scores[playerIndex];
        const JsonTextInfo scoreTemplate = GameTheme::jsonTextInfo(jobject, scoreKeys[playerIndex]);
        const JsonTextInfo rankTemplate = GameTheme::jsonTextInfo(jobject, rankKeys[playerIndex]);

        JsonTextInfo playerName = scoreTemplate;
        playerName.position.x = (scoreTemplate.position.x + rankTemplate.position.x) / 2;
        playerName.position.y = 215;
        playerName.font = "dejavus22";
        playerName.text = player.person.name();
        values.push_back(playerName);

        for(std::size_t category = 0; category < MatchScore::CategoryCount; ++category)
        {
            JsonTextInfo scoreValue = scoreTemplate;
            scoreValue.position.y = rowY[category];
            scoreValue.font = "dejavus22";
            scoreValue.text = String::number(player.categories[category].score);
            values.push_back(scoreValue);

            JsonTextInfo rankValue = rankTemplate;
            rankValue.position.y = rowY[category];
            rankValue.font = "dejavus22";
            rankValue.text = String::number(player.categories[category].rank);
            values.push_back(rankValue);
        }

        JsonTextInfo totalValue = scoreTemplate;
        totalValue.position.y = 638;
        totalValue.font = "dejavus26";
        totalValue.text = String::number(player.totalScore);
        values.push_back(totalValue);

        JsonTextInfo finalRank = rankTemplate;
        finalRank.position.y = 638;
        finalRank.font = "dejavus26";
        finalRank.text = String::number(player.finalRank);
        values.push_back(finalRank);
    }

    buttonNext = buttons.findIds("but_done");
    if(buttonNext)
        buttonNext->setAction(Action::ButtonDone);

    setVisible(true);
}

void GameSummaryScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    for(auto & label : labels)
	renderTextInfo(label);

    for(auto & value : values)
        renderTextInfo(value);
}

bool GameSummaryScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::SPACE:
            pushEventAction(Action::ButtonDone, this, nullptr);
            return true;

        default: break;
    }

    return false;
}

bool GameSummaryScreen::userEvent(int act, void* data)
{
    switch(act)
    {
        case Action::ButtonDone:
            setResultCode(Menu::GameExit);
            setVisible(false);
            break;

        default: break;
    }

    return true;
}
