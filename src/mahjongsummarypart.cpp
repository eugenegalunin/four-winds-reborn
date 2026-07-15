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

#include <numeric>
#include <algorithm>

#include "settings.h"
#include "gametheme.h"
#include "actions.h"
#include "mahjongsummarypart.h"

MahjongSummaryPartScreen::MahjongSummaryPartScreen() : JsonWindow("screen_mahjong_summary.json", nullptr),
    multiplier(1), totalScore(0)
{
    const Person & pers = GameData::myPerson();
    ld = GameData::toLocalData(pers.avatar);

    defaultColor = GameTheme::jsonColor(jobject, "default:color");
    defaultFont = jobject.getString("default:font");

    doublesOffset = GameTheme::jsonPoint(jobject, "offset:doubles");
    winRunePos = GameTheme::jsonPoint(jobject, "offset:winrune");
    winRunesPos = GameTheme::jsonPoint(jobject, "offset:winrunes");

    pointsText = GameTheme::jsonTextInfo(jobject, "textinfo:points");
    doublesText = GameTheme::jsonTextInfo(jobject, "textinfo:doubles");
    baseScoreText = GameTheme::jsonTextInfo(jobject, "textinfo:basescore");
    totalPointsText = GameTheme::jsonTextInfo(jobject, "textinfo:totalpoints");
    totalScoreText = GameTheme::jsonTextInfo(jobject, "textinfo:totalscore");
    multiplierText = GameTheme::jsonTextInfo(jobject, "textinfo:multiplier");
    finesText = GameTheme::jsonTextInfo(jobject, "textinfo:fines");

    markLeftSprite = GameTheme::jsonSprite(jobject, "sprite:selected2");
    markRightSprite = GameTheme::jsonSprite(jobject, "sprite:selected1");
    luck1Sprite = GameTheme::jsonSprite(jobject, "sprite:luck1");
    luck2Sprite = GameTheme::jsonSprite(jobject, "sprite:luck2");

    runeBonusList = ld.winResult.bonusRunes();
    doubleBonusList = ld.winResult.bonusDoubles();
    handBonusList = ld.winResult.bonusHands();
    opponentFinesList = ld.winResult.opponentFines();

    const Wind & winWind = ld.winResult.winWind;
    const Wind & dealWind = ld.winResult.dealWind;
    const Wind & roundWind = ld.winResult.roundWind;

    DEBUG("wind win: " << winWind.toString());
    DEBUG("wind deal: " << dealWind.toString());
    DEBUG("wind round: " << roundWind.toString());

    const Avatar & winAvatar = ld.playerOfWind(winWind).avatar;
    const Avatar & dealAvatar = ld.playerOfWind(dealWind).avatar;

    const std::string & roundWindName = GameData::windInfo(roundWind).name;
    const std::string & dealAvatarName = GameData::avatarInfo(dealAvatar).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:winrune"));
    labels.back().text = _("Win Rune");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:sets"));
    labels.back().text = _("Sets:");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:landclaims"));
    labels.back().text = _("Land Claims");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spellpoints"));
    labels.back().text = _("Spell Points");

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:deal"));
    labels.back().text = StringFormat(_("%1 Round: %2 Deal")).arg(roundWindName).arg(dealAvatarName);

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:drawn"));

    bool selfDrawnHand = std::any_of(handBonusList.begin(), handBonusList.end(),
				[](const HandBonus & bonus){ return bonus.isType(HandBonus::SelfDrawn); });

    if(ld.winResult.isDrawn())
	labels.back().text = _("Game Drawn");
    else
    {
	const std::string & winAvatarName = GameData::avatarInfo(winAvatar).name;
	const std::string & winWindName = GameData::windInfo(winWind).name;
	const std::string drawnName = _("Drawn");

	labels.back().text = StringFormat(_("%1 (%2) wins from %3")).arg(winAvatarName).arg(winWindName).arg(selfDrawnHand ? drawnName : dealAvatarName);
    }

    const RemotePlayer & remoteEast = ld.playerOfWind(Wind::East);
    const RemotePlayer & remoteSouth = ld.playerOfWind(Wind::South);
    const RemotePlayer & remoteWest = ld.playerOfWind(Wind::West);
    const RemotePlayer & remoteNorth = ld.playerOfWind(Wind::North);

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:avatar1"));
    labels.back().text = GameData::avatarInfo(remoteEast.avatar).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:avatar2"));
    labels.back().text = GameData::avatarInfo(remoteSouth.avatar).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:avatar3"));
    labels.back().text = GameData::avatarInfo(remoteWest.avatar).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:avatar4"));
    labels.back().text = GameData::avatarInfo(remoteNorth.avatar).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:clan1"));
    labels.back().text = GameData::clanInfo(remoteEast.clan).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:clan2"));
    labels.back().text = GameData::clanInfo(remoteSouth.clan).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:clan3"));
    labels.back().text = GameData::clanInfo(remoteWest.clan).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:clan4"));
    labels.back().text = GameData::clanInfo(remoteNorth.clan).name;

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spell1"));
    labels.back().text = String::number(remoteEast.points);

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spell2"));
    labels.back().text = String::number(remoteSouth.points);

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spell3"));
    labels.back().text = String::number(remoteWest.points);

    labels.push_back(GameTheme::jsonTextInfo(jobject, "textinfo:spell4"));
    labels.back().text = String::number(remoteNorth.points);

    buttonNext = buttons.findIds("but_done");
    if(buttonNext)
	buttonNext->setAction(Action::ButtonDone);

    int doubles = std::accumulate(doubleBonusList.begin(), doubleBonusList.end(), 0,
				    [](int v, const DoubleBonus & bonus){ return v + bonus.value(); });
    multiplier = WinResults::scoreMultiplier(doubles);
    totalScore = ld.winResult.totalScore();

    setVisible(true);
}

void MahjongSummaryPartScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    renderWinStones();

   for(auto & label : labels)
        renderTextInfo(label);
}

void MahjongSummaryPartScreen::renderWinStones(void)
{
    if(! ld.winResult.isDrawn())
    {
	// win stone
	const Texture & last = GameTheme::texture(GameData::stoneInfo(ld.winResult.lastStone).large);
	renderTexture(last, winRunePos);

	renderTextInfo(pointsText, _("Points:"));

	Point pos = winRunesPos;
	JsonTextInfo bonusText = pointsText;

	// rules
	for(auto & runeBonus : runeBonusList)
	{
	    const Stones & stones = runeBonus.stones();
	    int sph = 0;

	    for(auto & st : stones)
	    {
		const Texture & sprite = GameTheme::texture(GameData::stoneInfo(st).large);
		renderTexture(sprite, pos);
		pos.x += sprite.width();
		sph = sprite.height();
	    }

	    if(! runeBonus.isConcealed())
		renderTexture(markRightSprite, pos - Point(6, 0));

	    if(runeBonus.isLucky())
		renderTexture(luck1Sprite, pos + Point(30, 0));

	    bonusText.position.y = pos.y + 10;
	    renderTextInfo(bonusText, String::number(runeBonus.value()));

	    pos.x = winRunesPos.x;

	    if(! runeBonus.isConcealed())
		renderTexture(markLeftSprite, pos - Point(markLeftSprite.width() - 6, 0));

	    pos.y += sph;
	}

	// base score
	Rect rt = renderTextInfo(baseScoreText, _("Base Score:"));
	renderTextInfo(baseScoreText, String::number(ld.winResult.baseScore()), Point(pointsText.position.x, baseScoreText.position.y), AlignRight);

	pos = baseScoreText.position + Point(0, rt.h + 10);

	for(auto & handBonus : handBonusList)
	{
	    DEBUG("hand: " << handBonus.name() << ", " << handBonus.value());
	    renderTextInfo(baseScoreText, handBonus.name(), pos, AlignLeft);
	    renderTextInfo(baseScoreText, String::number(handBonus.value()), Point(pointsText.position.x, pos.y), AlignRight);
	    pos.y += rt.h + 10;
	}

	// total points
	renderTextInfo(totalPointsText, _("Total Points:"));
	renderTextInfo(totalPointsText, String::number(ld.winResult.totalPoints()), Point(pointsText.position.x, totalPointsText.position.y), AlignRight);

	// double score
	rt = renderTextInfo(doublesText, _("Doubles:"));
	pos = doublesText.position + Point(0, rt.h + 10);

	for(auto & doubleBonus : doubleBonusList)
	{
	    Rect textpos = renderTextInfo(doublesText, doubleBonus.name(), pos, AlignLeft);
	    renderTextInfo(doublesText, String::number(doubleBonus.value()), Point(width() - doublesOffset.x, pos.y), AlignRight);

	    DEBUG("double: " << doubleBonus.name() << ", " << doubleBonus.value());
	    pos.y += textpos.h;
	}

	const Avatar & winAvatar = ld.playerOfWind(ld.winResult.winWind).avatar;
	const std::string & winAvatarName = GameData::avatarInfo(winAvatar).name;

	pos = finesText.position;

	if(0 < totalScore)
	for(auto & opponentFine : opponentFinesList)
	{
	    const std::string & loseAvatarName = GameData::avatarInfo(ld.playerOfWind(opponentFine.wind()).avatar).name;
	    int winsValue = totalScore * opponentFine.value();

	    const std::string fine = StringFormat(_("%1 wins %2 x%3 from %4 = %5")).arg(winAvatarName).arg(totalScore).arg(opponentFine.value()).arg(loseAvatarName).arg(winsValue);
	    Rect textpos = renderTextInfo(finesText, fine, pos);

	    DEBUG("fine: " << fine);
	    pos.y += textpos.h;
	}

	if(0 < multiplier)
	{
	    renderTextInfo(multiplierText, _("Multiplier:"));
	    renderTextInfo(multiplierText, StringFormat("x%1").arg(multiplier), Point(width() - doublesOffset.x, multiplierText.position.y), AlignRight);
	}

	if(0 < totalScore)
	{
	    renderTextInfo(totalScoreText, _("Total Score:"));
	    renderTextInfo(totalScoreText, String::number(totalScore), Point(width() - doublesOffset.x, totalScoreText.position.y), AlignRight);
	}
    }
}

bool MahjongSummaryPartScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::ESCAPE:
        case Key::RETURN:
            pushEventAction(Action::ButtonDone, this, nullptr);
            return true;

        default: break;
    }

    return false;
}

bool MahjongSummaryPartScreen::userEvent(int act, void* data)
{
    switch(act)
    {
        case Action::ButtonDone:
	    setResultCode(Menu::AdventurePart);
	    setVisible(false);
	    return true;

        default: break;
    }

    return false;
}
