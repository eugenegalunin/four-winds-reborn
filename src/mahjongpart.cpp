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
#include <iterator>

#include "settings.h"
#include "runewars.h"
#include "gametheme.h"
#include "dialogs.h"
#include "adventurepart.h"
#include "actions.h"
#include "crashreport.h"
#include "mahjongpart.h"

StoneSprite::StoneSprite(const Stone & v, int size, const Point & pos) : Stone(v)
{
    set(v, size);
    setPosition(pos);
}

StoneSprite::StoneSprite(const Stone & v, int size) : Stone(v)
{
    set(v, size);
}

void StoneSprite::set(const Stone & v, int size)
{
    const StoneInfo & info = GameData::stoneInfo(v);
    Stone::set(v);

    switch(size)
    {
        case Small:     setTexture(GameTheme::texture(info.small)); break;
        case Medium:    setTexture(GameTheme::texture(info.medium)); break;
        case Large:     setTexture(GameTheme::texture(info.large)); break;
        default: break;
    }
}

namespace
{
class ScopedTickPause
{
    Window & window;

public:
    explicit ScopedTickPause(Window & win) : window(win)
    {
	window.disableTickEvent(true);
    }

    ~ScopedTickPause()
    {
	window.disableTickEvent(false);
    }

    ScopedTickPause(const ScopedTickPause &) = delete;
    ScopedTickPause & operator=(const ScopedTickPause &) = delete;
};

class ScopedFlag
{
    bool & flag;

public:
    explicit ScopedFlag(bool & value) : flag(value)
    {
	flag = true;
    }

    ~ScopedFlag()
    {
	flag = false;
    }

    ScopedFlag(const ScopedFlag &) = delete;
    ScopedFlag & operator=(const ScopedFlag &) = delete;
};

class LuckDrawDialog : public DialogWindow
{
    StoneSprite		first;
    StoneSprite		second;
    Color		selectedColor;
    Point		hintPos;
    int			selected;

    void choose(int index)
    {
	setResultCode(index);
	actionDialogClose();
    }

protected:
    bool mouseClickEvent(const ButtonsEvent & coords) override
    {
	if(!coords.isButtonLeft()) return true;

	if(coords.isClick(first.area()))
	    choose(0);
	else if(coords.isClick(second.area()))
	    choose(1);

	return true;
    }

    bool keyPressEvent(const KeySym & key) override
    {
	switch(key.keycode())
	{
	    case Key::LEFT:
		selected = 0;
		renderWindow();
		return true;
	    case Key::RIGHT:
		selected = 1;
		renderWindow();
		return true;
	    case Key::RETURN:
	    case Key::SPACE:
		choose(selected);
		return true;
	    case Key::ESCAPE:
		// Luck is a mandatory draw decision; closing would strand the turn.
		return true;
	    default: break;
	}

	return false;
    }

public:
    LuckDrawDialog(const VecStones & choices, Window & win)
	: DialogWindow("dialog_luckdraw.json", win),
	  first(choices[0], StoneSprite::Large, GameTheme::jsonPoint(jobject, "offset:left")),
	  second(choices[1], StoneSprite::Large, GameTheme::jsonPoint(jobject, "offset:right")),
	  selectedColor(GameTheme::jsonColor(jobject, "color:selected")),
	  hintPos(GameTheme::jsonPoint(jobject, "offset:hint")), selected(0)
    {
	setResultCode(-1);
	setVisible(true);
    }

    void renderWindow(void) override
    {
	renderColor(backgroundColor, rect());
	renderColor(borderColor, Rect(Point(0, 0), Size(width(), borderWidth)));
	renderColor(borderColor, Rect(Point(0, height() - borderWidth), Size(width(), borderWidth)));
	renderColor(borderColor, Rect(Point(0, 0), Size(borderWidth, height())));
	renderColor(borderColor, Rect(Point(width() - borderWidth, 0), Size(borderWidth, height())));

	const FontRender & defaultFont = GameTheme::fontRender(font);
	renderText(defaultFont, _("Luck: choose a rune"), headerColor,
	           Point(width() / 2, 18), AlignCenter);

	Rect highlight = selected == 0 ? first.area() : second.area();
	highlight.x -= 6;
	highlight.y -= 6;
	highlight.w += 12;
	highlight.h += 12;
	renderColor(selectedColor, highlight);
	renderTexture(first);
	renderTexture(second);

	renderText(defaultFont, _("The other rune returns to the wall."),
	           textColor, hintPos, AlignCenter);
    }
};
}

WindMarker::WindMarker(const Sprites & sp)
{
    if(1 < sp.size())
    {
	first = sp[0];
	second = sp[1];
    }
}

OrderTurn::OrderTurn(const JsonObject & jobject)
{
    const JsonObject* jo = jobject.getObject("windcompass:sprites");
    if(jo)
    {
	windsMarker[0] = WindMarker(GameTheme::jsonSprites(*jo, "east"));
	windsMarker[1] = WindMarker(GameTheme::jsonSprites(*jo, "south"));
	windsMarker[2] = WindMarker(GameTheme::jsonSprites(*jo, "west"));
	windsMarker[3] = WindMarker(GameTheme::jsonSprites(*jo, "north"));
    }

    windPositions = GameTheme::jsonSidesPositions(jobject, "windcompass:positions");
}

WindMarker OrderTurn::createMarker(const Wind & wind, bool local) const
{
    WindMarker res = windsMarker[wind() - 1];
    if(local) res.swap();
    return res;
}

void OrderTurn::renderCentered(Window & win, const Point & pos, const WindMarker & marker) const
{
    win.renderTexture(marker.tx(), Point(pos.x - marker.tx().width() / 2, pos.y - marker.tx().height() / 2));
}

void OrderTurn::render(Window & win, const Wind & localWind, const Wind & currentWind, const Wind & partWind) const
{
    const WindCompass winds(localWind);

    renderCentered(win, windPositions.center, windsMarker[partWind() - 1]);
    renderCentered(win, windPositions.left, createMarker(winds.left(), winds.left() == currentWind));
    renderCentered(win, windPositions.right, createMarker(winds.right(), winds.right() == currentWind));
    renderCentered(win, windPositions.top, createMarker(winds.top(), winds.top() == currentWind));
    renderCentered(win, windPositions.bottom, createMarker(winds.bottom(), winds.bottom() == currentWind));
}

Rect TurnAnimation::maxArea(void) const
{
    Rects rects; rects.reserve(sprites.size());
    for(auto & sprite : sprites)
	rects.push_back(sprite.area());
    return rects.around();
}

void TurnAnimation::renderTick(Window & win) const
{
    for(auto it = sprites.begin(); it != sprites.end(); ++it)
    {
        win.renderTexture(*it);

	if(std::distance(sprites.begin(), it) + 1 == sprites.size() - index)
	    break;
    }
}

void TurnAnimation::renderAll(Window & win) const
{
    for(auto & sprite: sprites)
        win.renderTexture(sprite);
}

bool TurnAnimation::isLastSprite(void) const
{
    return index + 1 == sprites.size();
}

MahjongPartScreen::MahjongPartScreen() : JsonWindow("screen_mahjongpart.json", nullptr),
    myAvatar(GameData::myPerson().avatar), orderTurn(jobject), animationTurn(jobject, "animation:turn"),
    animationChao(jobject, "animation:chao"), animationPung(jobject, "animation:pung"),
    animationKong(jobject, "animation:kong"), animationGame(jobject, "animation:game"),
    stoneSelected(-1), variantSelected(-1), playersMarker(0), animationDropStep(40),
    animationDropDelay(12), animationDealDelay(55), dealingInitialHand(false), dealtStoneCount(0),
    animatingDrawStone(false), iconAffectedSkull(this), iconAffectedSword(this), iconAffectedNumber(this),
    iconAffectedDiscard(this), iconAffectedSilence(this), iconAffectedScry(this), playerReady(false),
    resolvingLuckChoice(false)
{
    ld = GameData::toLocalData(myAvatar);
    playMusic(GameData::myPerson().clan.toString());

    stonesPos.reserve(GAME_SET_COUNT);
    stoneActiveSprite = GameTheme::jsonSprite(jobject, "stone:active");
    stoneSelectedSprite = GameTheme::jsonSprite(jobject, "stone:selected");

    stoneVariantSprite = Display::createTexture(Size(stoneSelectedSprite.width(), stoneSelectedSprite.height() / 2));
    Display::renderTexture(stoneSelectedSprite, Rect(0, stoneSelectedSprite.height() / 2, stoneSelectedSprite.width(), stoneSelectedSprite.height() / 2), stoneVariantSprite, Rect(Point(0, 0), stoneVariantSprite.size()));

    defaultColor = GameTheme::jsonColor(jobject, "default:color");
    defaultFont = jobject.getString("default:font");
    namesFont = jobject.getString("names:font");

    animationDropDelay = Settings::presentationDelay(jobject.getInteger("mahjongdrop:delay", 12));
    animationDropStep = jobject.getInteger("mahjongdrop:step", 40);
    animationDealDelay = Settings::presentationDelay(jobject.getInteger("mahjongdeal:delay", 55));

    animationChao.delay = Settings::presentationDelay(animationChao.delay);
    animationPung.delay = Settings::presentationDelay(animationPung.delay);
    animationKong.delay = Settings::presentationDelay(animationKong.delay);
    animationGame.delay = Settings::presentationDelay(animationGame.delay);

    iconAffectedSkull.setJsonObject(jobject.getObject("affected:skull"));
    iconAffectedSkull.setVisible(false);
    iconAffectedSword.setJsonObject(jobject.getObject("affected:sword"));
    iconAffectedSword.setVisible(false);
    iconAffectedNumber.setJsonObject(jobject.getObject("affected:number"));
    iconAffectedNumber.setVisible(false);
    iconAffectedDiscard.setJsonObject(jobject.getObject("affected:discard"));
    iconAffectedDiscard.setVisible(false);
    iconAffectedSilence.setJsonObject(jobject.getObject("affected:silence"));
    iconAffectedSilence.setVisible(false);
    iconAffectedScry.setJsonObject(jobject.getObject("affected:scry"));
    iconAffectedScry.setVisible(false);

    remainsPos = GameTheme::jsonPoint(jobject, "offset:remains");
    localSetPos = GameTheme::jsonPoint(jobject, "offset:localset");
    dropStonePos = GameTheme::jsonPoint(jobject, "offset:dropstone");
    croupierPos = GameTheme::jsonPoint(jobject, "offset:trash");

    winSetPos = GameTheme::jsonSidesPositions(jobject, "winrules:positions");
    namesPos = GameTheme::jsonSidesPositions(jobject, "names:positions");
    fastLogText = GameTheme::jsonTextInfo(jobject, "textinfo:fastlog");

    buttonPass = buttons.findIds("but_pass");
    buttonChao = buttons.findIds("but_chow");
    buttonPung = buttons.findIds("but_pung");
    buttonKong = buttons.findIds("but_kong");
    buttonGame = buttons.findIds("but_game");
    buttonCast = buttons.findIds("but_cast");

    if(!buttonPass)
    {
	ERROR("button id not found: " << "but_pass");
	Engine::except(__FUNCTION__, "exit");
    }

    if(!buttonChao)
    {
	ERROR("button id not found: " << "but_chow");
	Engine::except(__FUNCTION__, "exit");
    }

    if(!buttonPung)
    {
	ERROR("button id not found: " << "but_pung");
	Engine::except(__FUNCTION__, "exit");
    }

    if(!buttonKong)
    {
	ERROR("button id not found: " << "but_kong");
	Engine::except(__FUNCTION__, "exit");
    }

    if(!buttonGame)
    {
	ERROR("button id not found: " << "but_game");
	Engine::except(__FUNCTION__, "exit");
    }

    buttonLocalReady = *buttonPass;
    //buttonLocalReady.setParent(this);
    buttonLocalReady.setAction(Action::ButtonLocalReady);
    buttonLocalReady.setPosition(localReadyPos());

    buttonLocalKong = *buttonKong;
    //buttonLocalKong.setParent(this);
    buttonLocalKong.setAction(Action::ButtonLocalKong);
    buttonLocalKong.setPosition(localKongPos());

    buttonLocalGame = *buttonGame;
    //buttonLocalGame.setParent(this);
    buttonLocalGame.setAction(Action::ButtonLocalGame);
    buttonLocalGame.setPosition(localGamePos());

    setVisible(true);

    buttonPass->setVisible(false);
    buttonPass->setAction(Action::ButtonPass);

    buttonChao->setVisible(false);
    buttonChao->setAction(Action::ButtonChao);

    buttonPung->setVisible(false);
    buttonPung->setAction(Action::ButtonPung);

    buttonKong->setVisible(false);
    buttonKong->setAction(Action::ButtonKong);

    buttonGame->setVisible(false);
    buttonGame->setAction(Action::ButtonGame);

    buttonLocalReady.setVisible(true);
    buttonLocalKong.setVisible(false);
    buttonLocalGame.setVisible(false);

    JsonButton* button = nullptr;

    button = buttons.findIds("but_cast");
    if(button) button->setAction(Action::ButtonCast);

    button = buttons.findIds("but_logs");
    if(button) button->setAction(Action::ButtonChat);

    button = buttons.findIds("but_syst");
    if(button) button->setAction(Action::ButtonSystem);

    button = buttons.findIds("but_qest");
    if(button) button->setAction(Action::ButtonSummary);

    button = buttons.findIds("but_maps");
    if(button) button->setAction(Action::ButtonMap);

    // disable guardian rules sound
    if(! Settings::soundGuardianRules())
    {
	animationChao.sound.clear();
	animationPung.sound.clear();
	animationKong.sound.clear();
	// animationGame.sound.clear();
    }

    const JsonObject & savedState = GameData::jsonGUI();
    if(savedState.isValid()) fromJsonObject(savedState);

    DEBUG("start");
}

Point MahjongPartScreen::localReadyPos(void) const
{
    const Rect & pos = newStonePos();
    return pos.toPoint() + Point(10, pos.w - buttonLocalReady.height());
}

Point MahjongPartScreen::localKongPos(void) const
{
    const Rect & pos = newStonePos();
    return pos.toPoint() + Point(pos.w + 10, 0);
}

Point MahjongPartScreen::localGamePos(void) const
{
    const Rect & pos = newStonePos();
    return pos.toPoint() + pos.toSize() + Point(10, -buttonLocalGame.height());
}

Rect MahjongPartScreen::newStonePos(void) const
{
    const GameStones & stones = ld.myPlayer().stones;
    Rect res;

    if(stones.size())
    {
	const StoneSprite sprite(stones[0], StoneSprite::Large);
	res = Rect(Point(localSetPos.x + (sprite.width() * stones.size()) / 2 + 10, localSetPos.y), sprite.size());
    }

    return res;
}

void MahjongPartScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    renderCroupier();
    renderWinRules();
    renderNames();
    renderScryRunes();
    renderGameStoneRemains();

    if(0 <= variantSelected)
    {
	GameStones stones = ld.myPlayer().stones;
	stones.add(ld.dropStone);
	renderLocalSet(stones);
    }
    else
	renderLocalSet(ld.myPlayer().stones);

    orderTurn.render(*this, ld.myPlayer().wind, ld.currentWind, ld.partWind);

    if(animationTurn.isEnabled())
    {
	animationTurn.renderTick(*this);

	if(animationTurn.isLastSprite())
	{
	    animationTurn.setEnabled(false);
	    pushEventAction(Action::MahjongOutOfTime, this, nullptr);
	}
    }
    else
	animationTurn.renderAll(*this);

    animationChao.render(*this);
    animationPung.render(*this);
    animationKong.render(*this);
    animationGame.render(*this);

    if(0 > variantSelected &&
	ld.dropStone.isValid() && 0 > stoneSelected)
    {
	const StoneSprite sprite(ld.dropStone, StoneSprite::Large);
	renderTexture(sprite, dropStonePos - sprite.size() / 2);
    }

    if(buttonCast)
	buttonCast->setInformed(checkCastInformer());

    if(fastLogText.text.size()) renderTextInfo(fastLogText);
}

bool MahjongPartScreen::checkCastInformer(void) const
{
    if(ld.myPlayer().isCasted() ||
       ld.myPlayer().isSilenced() ||
       ld.myPlayer().isAffectedSpell(Spell::ManaFog))
	return false;

    const AvatarInfo & avatarInfo = GameData::avatarInfo(ld.myPlayer().avatar);
    const Spells & spells = avatarInfo.spells;
    const Creatures & creatures = avatarInfo.creatures;

    for(auto it = spells.begin(); it != spells.end(); ++it)
    {
        const SpellInfo & spellInfo = GameData::spellInfo(*it);
        if(spellInfo.stones.size() && ld.myPlayer().points >= spellInfo.cost &&
	    ld.myPlayer().stones.allowCast(spellInfo.stones, ld.myPlayer().newStone))
	    return true;
    }

    for(auto it = creatures.begin(); it != creatures.end(); ++it)
    {
        const CreatureInfo & creatureInfo = GameData::creatureInfo(*it);
        if(creatureInfo.stones.size() && ld.myPlayer().points >= creatureInfo.cost &&
	    ld.myPlayer().stones.allowCast(creatureInfo.stones, ld.myPlayer().newStone))
	    return true;
    }

    return false;
}

bool MahjongPartScreen::userEvent(int act, void* data)
{
    switch(act)
    {
        case Action::MahjongGameQuit:	actionQuit(); break;
        case Action::MahjongOutOfTime:	actionOutOfTime(); break;
	case Action::MahjongDropSelected: actionDropSelected(); break;

	case Action::ButtonCast:	actionButtonShowCast(); break;

	case Action::ButtonLocalReady:	if(buttonLocalReady.isVisible()) actionButtonLocalReady(); break;
	case Action::ButtonLocalKong:	if(buttonLocalKong.isVisible()) actionButtonLocalKong(); break;
	case Action::ButtonLocalGame:	if(buttonLocalGame.isVisible()) actionButtonLocalGame(); break;

	case Action::ButtonPass:	if(buttonPass->isVisible()) actionButtonPass(WinRule::None); break;
	case Action::ButtonChao:	if(buttonChao->isVisible()) actionButtonPass(WinRule::Chao); break;
	case Action::ButtonPung:	if(buttonPung->isVisible()) actionButtonPass(WinRule::Pung); break;
	case Action::ButtonKong:	if(buttonKong->isVisible()) actionButtonPass(WinRule::Kong); break;
	case Action::ButtonGame:	if(buttonGame->isVisible()) actionButtonPass(WinRule::Game); break;

	case Action::ButtonChat:	actionButtonChat(); break;
	case Action::ButtonSystem:	actionButtonSystem(); break;
	case Action::ButtonSummary:	actionButtonSummary(); break;
	case Action::ButtonMap:		actionButtonMap(); break;

        default: break;
    }

    return true;
}

bool MahjongPartScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
	case Key::LEFT:		return actionSelectedShiftLeft();
	case Key::RIGHT:	return actionSelectedShiftRight();
	case Key::SPACE:	return actionPressSpace();
	case Key::F4:		return actionCreateScreenshot();

	case Key::c:		return userEvent(Action::ButtonChao, nullptr);
	case Key::p:		return userEvent(Action::ButtonPung, nullptr);
	case Key::k:		return userEvent(Action::ButtonKong, nullptr);
	case Key::g:		return userEvent(Action::ButtonGame, nullptr);

#ifdef BUILD_DEBUG
	case Key::F5:		return actionEventDebug1();
	case Key::F6:		return actionEventDebug2();
#endif

#ifdef ANDROID
	case SDLK_AC_BACK:
#endif
	case Key::ESCAPE:	return actionButtonSystem();
	default: break;
    }

    return false;
}

bool MahjongPartScreen::mouseClickEvent(const ButtonsEvent & coords)
{
    if(! coords.isButtonLeft())
	return false;

    if(ld.yourTurn())
    {
	if(0 <= stoneSelected)
	{
	    int index = -1;
	    auto it = std::find_if(stonesPos.begin(), stonesPos.end(),
			[&](const Rect & rt){ return coords.isClick(rt); });

	    if(it != stonesPos.end())
		index = std::distance(stonesPos.begin(), it);
	    else
	    if(coords.isClick(newStonePos()))
		index = stonesPos.size();

	    if(0 <= index)
	    {
		if(stoneSelected == index)
		    pushEventAction(Action::MahjongDropSelected, this, nullptr);
		else
		{
		    playSound("select");
		    stoneSelected = index;
		    renderWindow();
		}

		return true;
	    }
	}
    }

    if(0 <= variantSelected)
    {
	auto it = std::find_if(variantsPos.begin(), variantsPos.end(),
			[&](const Rect & rt){ return coords.isClick(rt); });

	int index = it != variantsPos.end() ?
		std::distance(variantsPos.begin(), it) : -1;

	if(0 <= index)
	{
	    if(variantSelected == index)
		pushEventAction(Action::MahjongDropSelected, this, nullptr);
	    else
	    {
		playSound("select");
		variantSelected = index;
		renderWindow();
	    }

	    return true;
	}
    }

    if(coords.isClick(animationTurn.maxArea()))
	return actionPressSpace();

    return false;
}

void MahjongPartScreen::actionOutOfTime(void)
{
    if(buttonLocalReady.isVisible())
	buttonLocalReady.setClicked();
    else
    if(buttonGame->isVisible())
	buttonGame->setClicked();
    else
    if(buttonKong->isVisible())
	buttonKong->setClicked();
    else
    if(buttonPung->isVisible())
	buttonPung->setClicked();
    else
    if(buttonChao->isVisible())
	buttonChao->setClicked();
    else
    if(buttonPass->isVisible())
	buttonPass->setClicked();
    else
    if(0 <= variantSelected)
	pushEventAction(Action::MahjongDropSelected, this, nullptr);
    else
    if(0 <= stoneSelected || ld.myPlayer().newStone.isValid())
	pushEventAction(Action::MahjongDropSelected, this, nullptr);
    else
    {
	ERROR("unknown reason");
	VERBOSE("json: " << toJsonObject().toString());
    }
}

void MahjongPartScreen::actionButtonLocalReady(void)
{
    buttonLocalReady.setVisible(false);

    GameData::client2Mahjong(myAvatar, ClientReady(), actions);
    playerReady = true;
}

void MahjongPartScreen::actionButtonLocalKong(void)
{
    buttonLocalKong.setVisible(false);
    actionButtonPass(WinRule::Kong);
}

void MahjongPartScreen::actionButtonLocalGame(void)
{
    buttonLocalGame.setVisible(false);
    actionButtonPass(WinRule::Game);
}

bool MahjongPartScreen::actionPressSpace(void)
{
    if(buttonPass->isVisible() || buttonLocalReady.isVisible() || 0 <= stoneSelected || 0 <= variantSelected)
    {
	if(buttonChao->isVisible() || buttonPung->isVisible() ||
	    buttonKong->isVisible() || buttonGame->isVisible() || buttonLocalGame.isVisible())
	{
	    VERBOSE("skip auto pass");
	}
	else
	    actionOutOfTime();

	return true;
    }

    return false;
}

void MahjongPartScreen::actionButtonPass(int rule)
{
    playSound("button");
    buttonPass->setVisible(false);
    buttonChao->setVisible(false);
    buttonPung->setVisible(false);
    buttonKong->setVisible(false);
    buttonGame->setVisible(false);

    buttonLocalReady.setVisible(false);
    bool sendPass = false;

    if(fastLogText.text.size()) fastLogText.text.clear();

    if(rule == WinRule::Game)
    {
	GameData::client2Mahjong(myAvatar, ClientSayGame(), actions);
	GameData::client2Mahjong(myAvatar, ClientButtonGame(), actions);
    }
    else
    if(rule == WinRule::Kong)
    {

	if(ld.yourTurn())
	{
	    GameData::client2Mahjong(myAvatar, ClientSayKong(2), actions);
	    GameData::client2Mahjong(myAvatar, ClientButtonKong2(), actions);
	}
	else
	{
	    GameData::client2Mahjong(myAvatar, ClientSayKong(1), actions);
	    GameData::client2Mahjong(myAvatar, ClientButtonKong1(), actions);
	}
    }
    else
    if(rule == WinRule::Pung)
    {
	GameData::client2Mahjong(myAvatar, ClientSayPung(), actions);
	GameData::client2Mahjong(myAvatar, ClientButtonPung(), actions);
    }
    else
    if(rule == WinRule::Chao)
    {
	GameData::client2Mahjong(myAvatar, ClientSayChao(), actions);
	const Stones & chaoVariants = ld.myPlayer().stones.findChaoVariants(ld.dropStone);

	if(chaoVariants.size())
	{
	    // user must choose variant
	    if(1 < chaoVariants.size())
	    {
		variantSelected = 0;
		DEBUG("chao multiple variant");
	    }
	    else
	    {
		GameData::client2Mahjong(myAvatar, ClientChaoVariant(0), actions);
	    }

	    renderWindow();
	}
	else
	    // SetPass
	    sendPass = true;
    }
    else
	// SetPass
	sendPass = true;

    if(sendPass)
    {
	GameData::client2Mahjong(myAvatar, ClientButtonPass(), actions);
    }
}

void MahjongPartScreen::actionDropSelected(void)
{
    buttonLocalGame.setVisible(false);
    buttonLocalKong.setVisible(false);

    // select chao mode
    if(0 <= variantSelected)
	GameData::client2Mahjong(myAvatar, ClientChaoVariant(variantSelected), actions);
    else
    if(0 <= stoneSelected)
    {
	GameData::client2Mahjong(myAvatar, ClientDropIndex(stoneSelected), actions);
	buttonPass->setClicked();
	stoneSelected = -1;
    }

    renderWindow();
}

bool MahjongPartScreen::actionSelectedShiftLeft(void)
{
    if(0 < variantSelected)
    {
	playSound("select");
	variantSelected--;
	renderWindow();
	return true;
    }
    else
    if(0 <= stoneSelected)
    {
	if(0 < stoneSelected)
	    stoneSelected--;
	else
	    stoneSelected = ld.myPlayer().newStone.isValid() ? stonesPos.size() : stonesPos.size() - 1;
	playSound("select");
	renderWindow();
	return true;
    }

    return false;
}

bool MahjongPartScreen::actionSelectedShiftRight(void)
{
    if(0 <= variantSelected && variantSelected < variantsPos.size() - 1)
    {
	playSound("select");
	variantSelected++;
	renderWindow();
	return true;
    }
    else
    if(0 <= stoneSelected)
    {
	if(stoneSelected < (ld.myPlayer().newStone.isValid() ? stonesPos.size() : stonesPos.size() - 1))
	    stoneSelected++;
	else
	    stoneSelected = 0;
	playSound("select");
	renderWindow();
	return true;
    }

    return false;
}

void MahjongPartScreen::actionQuit(void)
{
#ifndef SWE_DISABLE_AUDIO
    if(Settings::music()) Music::reset();
#endif

    setResultCode(Menu::MahjongSummaryPart);
    setVisible(false);
}

void MahjongPartScreen::restoreMahjongMusic(void)
{
#ifndef SWE_DISABLE_AUDIO
    if(Settings::music()) playMusic(GameData::myPerson().clan.toString());
#endif
}

void MahjongPartScreen::actionButtonShowCast(void)
{
    const LocalPlayer & player = ld.myPlayer();

    RuneCastDialog castDialog(player, player.newStone, *this);
    if(0 < castDialog.exec())
    {
        if(ld.yourTurn())
        {
	    if(castDialog.resultIsCreature())
	    {
		const Creature cr = castDialog.resultCreature();
		ShowSummonCreatureDialog summonDialog(ld, cr, *this);
		const Land & dst = summonDialog.exec() ? summonDialog.land() : Land(Land::None);
		GameData::client2Mahjong(myAvatar, ClientSummonCreature(cr, dst), actions);
	    }
	    else
	    {
		const Spell sp = castDialog.resultSpell();
		const SpellInfo & spellInfo = GameData::spellInfo(sp);

		if((spellInfo.target() == SpellTarget::AllPlayers) ||
		   (spellInfo.target() == SpellTarget::MyPlayer))
		{
		    GameData::client2Mahjong(myAvatar, ClientCastSpell(sp), actions);
		}
		else
		if(spellInfo.target() == SpellTarget::OtherPlayer)
		{
		    animationTurn.setPause(true);

		    TargetPlayerDialog targetDialog(ld, *this);
		    if(targetDialog.exec())
		    {
			auto avaid = static_cast<Avatar::avatar_t>(targetDialog.resultCode());
			GameData::client2Mahjong(myAvatar, ClientCastSpell(sp, avaid), actions);
		    }

		    animationTurn.setPause(false);
		}
		else
		{
		    ShowCastSpellDialog castDialog(ld, sp, *this);
		    int result = castDialog.exec();
		    if(0 < result)
		    {
			// Result 1 targets a land; creature and teleport targets carry a unit id.
			int battleUnit = 1 == result ? -1 : castDialog.unit();

			GameData::client2Mahjong(myAvatar, ClientCastSpell(sp, castDialog.land(), battleUnit), actions);
		    }
		}
	    }
	}
	else
	{
	    MessageBox(Application::name(), _("Now it is not your turn. Sorry..."), *this, false).exec();
	}
    }

    // Map-based spell and summon dialogs use the adventure screen theme.
    // Restore the local clan track before returning to the rune table.
    restoreMahjongMusic();
    renderWindow();
}

JsonObject MahjongPartScreen::toJsonObject(void)
{
    JsonObject jo;

    jo.addString("type", "MahjongPartScreen");

    jo.addInteger("stoneSelected", stoneSelected);
    jo.addInteger("variantSelected", variantSelected);
    jo.addInteger("playersMarker", playersMarker);
    jo.addString("fastLogText", fastLogText.text);
    jo.addString("fastLogOwner", fastLogOwner.toString());

    if(buttonPass) jo.addObject("buttonPass", buttonPass->toJsonInfo());
    if(buttonChao) jo.addObject("buttonChao", buttonChao->toJsonInfo());
    if(buttonPung) jo.addObject("buttonPung", buttonPung->toJsonInfo());
    if(buttonKong) jo.addObject("buttonKong", buttonKong->toJsonInfo());
    if(buttonGame) jo.addObject("buttonGame", buttonGame->toJsonInfo());

    jo.addObject("buttonLocalReady", buttonLocalReady.toJsonInfo());
    jo.addObject("buttonLocalKong", buttonLocalKong.toJsonInfo());
    jo.addObject("buttonLocalGame", buttonLocalGame.toJsonInfo());

    jo.addBoolean("playerReady", playerReady);
    jo.addArray("gameLogs", JsonPack::stringList(gameLogs.toStringList()));

    jo.addBoolean("affected:skull", iconAffectedSkull.isVisible());
    jo.addBoolean("affected:sword", iconAffectedSword.isVisible());
    jo.addBoolean("affected:number", iconAffectedNumber.isVisible());
    jo.addBoolean("affected:discard", iconAffectedDiscard.isVisible());
    jo.addBoolean("affected:silence", iconAffectedSilence.isVisible());
    jo.addBoolean("affected:scry", iconAffectedScry.isVisible());

    return jo;
}

bool MahjongPartScreen::fromJsonObject(const JsonObject & jo)
{
    const std::string & type = jo.getString("type");

    if(type == "MahjongPartScreen")
    {
	stoneSelected = jo.getInteger("stoneSelected");
	variantSelected = jo.getInteger("variantSelected");
	playersMarker = jo.getInteger("playersMarker");
	fastLogText.text = jo.getString("fastLogText");
	fastLogOwner = Wind(jo.getString("wind", "none"));

	if(buttonPass) buttonPass->setJsonInfo(jo.getObject("buttonPass"));
	if(buttonChao) buttonChao->setJsonInfo(jo.getObject("buttonChao"));
	if(buttonPung) buttonPung->setJsonInfo(jo.getObject("buttonPung"));
	if(buttonKong) buttonKong->setJsonInfo(jo.getObject("buttonKong"));
	if(buttonGame) buttonGame->setJsonInfo(jo.getObject("buttonGame"));

	buttonLocalReady.setJsonInfo(jo.getObject("buttonLocalReady"));
	buttonLocalKong.setJsonInfo(jo.getObject("buttonLocalKong"));
	buttonLocalGame.setJsonInfo(jo.getObject("buttonLocalGame"));

	iconAffectedSkull.setVisible(jo.getBoolean("affected:skull", false));
	iconAffectedSword.setVisible(jo.getBoolean("affected:sword", false));
	iconAffectedNumber.setVisible(jo.getBoolean("affected:number", false));
	iconAffectedDiscard.setVisible(jo.getBoolean("affected:discard", false));
	iconAffectedSilence.setVisible(jo.getBoolean("affected:silence", false));
	iconAffectedScry.setVisible(jo.getBoolean("affected:scry", false));

	playerReady = jo.getBoolean("playerReady", false);
	gameLogs = jo.getStdList<std::string>("gameLogs");
	return true;
    }
    else
    {
	ERROR("unknown type: " << (type.size() ? type : "null"));
    }

    return false;
}

void MahjongPartScreen::actionButtonChat(void)
{
    ShowLogsDialog(gameLogs, *this).exec();
}

bool MahjongPartScreen::actionCreateScreenshot(void)
{
    std::string file = Settings::fileSave("screenshot.png");

    if(Display::renderScreenshot(file))
    {
	VERBOSE(file);
	return true;
    }

    return false;
}

bool MahjongPartScreen::actionButtonSystem(void)
{
    ScopedTickPause pause(*this);
    const int choice = InGameMenuDialog(*this).exec();

    if(choice == InGameMenuResult::SaveGame)
    {
        SaveGameAs(toJsonObject(), *this);
        renderWindow();
        return true;
    }

    if(choice == InGameMenuResult::MainMenu)
    {
        if(!GameData::saveGame(toJsonObject()))
        {
            MessageBox(_("Save Game"), _("Unable to update the autosave."), *this, false).exec();
            renderWindow();
            return true;
        }
        setResultCode(Menu::MainMenu);
        setVisible(false);
	return true;
    }

    renderWindow();
    return true;
}

void MahjongPartScreen::actionButtonSummary(void)
{
    MapStatusDialog(ld, *this).exec();
}

void MahjongPartScreen::actionButtonMap(void)
{
    ShowMapDialog(ld, *this).exec();
    restoreMahjongMusic();
}

void MahjongPartScreen::renderDropStone(void)
{
    playSound("stone2");

    const WindCompass compass(ld.myPlayer().wind);
    const Point* pointFrom = nullptr;

    if(compass.left() == ld.currentWind)
	pointFrom = & winSetPos.left;
    else
    if(compass.right() == ld.currentWind)
	pointFrom = & winSetPos.right;
    else
    if(compass.top() == ld.currentWind)
	pointFrom = & winSetPos.top;
    else
    if(compass.bottom() == ld.currentWind)
	pointFrom = & winSetPos.bottom;

    if(pointFrom && 0 < animationDropStep)
    {
	const Point dropPosBack = dropStonePos;
	const Points points = Tools::renderLine(*pointFrom, dropStonePos, animationDropStep);

	animationTurn.setPause(true);

	for(auto & pt : points)
	{
	    dropStonePos = pt;

	    setDirty(true);
	    DisplayScene::sceneRedraw();

            Tools::delay(1 < animationDropDelay ? animationDropDelay : 1);
	}

	dropStonePos = dropPosBack;
	animationTurn.setPause(false);
	renderWindow();
    }
}

void MahjongPartScreen::renderDrawStone(void)
{
    const LocalPlayer & player = ld.myPlayer();
    if(!player.newStone.isValid() || animationDropStep <= 0) return;

    const StoneSprite sprite(player.newStone, StoneSprite::Large);
    const Point source = remainsPos - sprite.size() / 2;
    const Point destination = newStonePos().toPoint();
    const Points points = Tools::renderLine(source, destination, animationDropStep);

    animationTurn.setPause(true);
    animatingDrawStone = true;
    for(const Point & point : points)
    {
        drawStoneAnimationPos = point;
        setDirty(true);
        DisplayScene::sceneRedraw();
        Tools::delay(1 < animationDropDelay ? animationDropDelay : 1);
    }
    animatingDrawStone = false;
    animationTurn.setPause(false);
    renderWindow();
}

void MahjongPartScreen::renderInitialDeal(void)
{
    const std::size_t stoneCount = ld.myPlayer().stones.size();
    if(!stoneCount) return;

    dealingInitialHand = true;
    dealtStoneCount = 0;
    renderWindow();
    playSound("stone2");

    while(dealtStoneCount < stoneCount)
    {
        ++dealtStoneCount;
        renderWindow();
        DisplayScene::sceneRedraw();
        Tools::delay(1 < animationDealDelay ? animationDealDelay : 1);
    }

    dealingInitialHand = false;
    renderWindow();
}

void MahjongPartScreen::renderNames(void)
{
    renderNamesHorizontal(ld.remoteTop(), namesPos.top);
    renderNamesHorizontal(ld.remoteBottom(), namesPos.bottom);
    renderNamesVertical(ld.remoteLeft(), namesPos.left);
    renderNamesVertical(ld.remoteRight(), namesPos.right);
}

std::string MahjongPartScreen::playerPrettyName(const RemotePlayer & player) const
{
    std::string name = String::ucFirst(player.name());

    if(player.isAI())
	name += " - AI";
    else
    if(! (playersMarker & (static_cast<int>(1) << player.wind())))
    {
	// wait marker
	name += " *";
    }

    return name;
}

void MahjongPartScreen::renderNamesHorizontal(const RemotePlayer & player, const Point & center)
{
    Color color = fastLogText.text.size() && fastLogOwner == player.wind ? fastLogText.color : defaultColor;
    std::string name = playerPrettyName(player);
    const FontRender & frs = GameTheme::fontRender(namesFont);

    Rect pos = renderText(frs, name, color, center, AlignCenter, AlignCenter, true);
    Texture flag = GameTheme::texture(GameData::clanInfo(player.clan).flag1);
    renderTexture(flag, Point(pos.x - flag.width() - 5, pos.y + (pos.h - flag.height()) / 2));
    renderTexture(flag, Point(pos.x + pos.w + 5, pos.y + (pos.h - flag.height()) / 2));
}

void MahjongPartScreen::renderNamesVertical(const RemotePlayer & player, const Point & center)
{
    Color color = fastLogText.text.size() && fastLogOwner == player.wind ? fastLogText.color : defaultColor;
    std::string name = playerPrettyName(player);
    const FontRender & frs = GameTheme::fontRender(namesFont);

    Rect pos = renderText(frs, name, color, center, AlignCenter, AlignCenter, false);
    Texture flag = GameTheme::texture(GameData::clanInfo(player.clan).flag1);
    renderTexture(flag, Point(pos.x + (pos.w - flag.width()) / 2, pos.y - flag.height() - 5));
    renderTexture(flag, Point(pos.x + (pos.w - flag.width()) / 2, pos.y + pos.h + 5));
}

int MahjongPartScreen::renderWinRuleHorizontal(const WinRule & rule, const Point & pos)
{
    int res = 0;
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
	    {
		StoneSprite sprite1(rule.stone(), StoneSprite::Medium);
		StoneSprite sprite2(rule.stone().next(), StoneSprite::Medium);
		StoneSprite sprite3(rule.stone().next().next(), StoneSprite::Medium);
		renderTexture(sprite1, pos);
		res += sprite1.width();
		renderTexture(sprite2, pos + Point(res, 0));
		res += sprite2.width();
		renderTexture(sprite3, pos + Point(res, 0));
		res += sprite3.width();
	    }
	    break;
	case WinRule::Pung:
	    for(int ii = 1; ii <= 3; ++ii)
	    {
		StoneSprite sprite(rule.stone(), StoneSprite::Medium);
		renderTexture(sprite, pos + Point(res, 0));
		res += sprite.width();
	    }
	    break;
	case WinRule::Kong:
	    for(int ii = 1; ii <= 4; ++ii)
	    {
		StoneSprite sprite(rule.stone(), StoneSprite::Medium);
		renderTexture(sprite, pos + Point(res, 0));
		res += sprite.width();
	    }
	    break;
	default: break;
    }
    return res;
}

int MahjongPartScreen::renderWinRuleVertical(const WinRule & rule, const Point & pos)
{
    int res = 0;
    switch(rule.rule())
    {
	case WinRule::Chao:
	    if(! rule.stone().isSpecial())
	    {
		StoneSprite sprite1(rule.stone(), StoneSprite::Medium);
		StoneSprite sprite2(rule.stone().next(), StoneSprite::Medium);
		StoneSprite sprite3(rule.stone().next().next(), StoneSprite::Medium);
		renderTexture(sprite1, pos);
		res += sprite1.height();
		renderTexture(sprite2, pos + Point(0, res));
		res += sprite2.height();
		renderTexture(sprite3, pos + Point(0, res));
		res += sprite3.height();
	    }
	    break;
	case WinRule::Pung:
	    for(int ii = 1; ii <= 3; ++ii)
	    {
		StoneSprite sprite(rule.stone(), StoneSprite::Medium);
		renderTexture(sprite, pos + Point(0, res));
		res += sprite.height();
	    }
	    break;
	case WinRule::Kong:
	    for(int ii = 1; ii <= 4; ++ii)
	    {
		StoneSprite sprite(rule.stone(), StoneSprite::Medium);
		renderTexture(sprite, pos + Point(0, res));
		res += sprite.height();
	    }
	    break;
	default: break;
    }
    return res;
}

void MahjongPartScreen::renderWinRules(void)
{
    renderWinRulesHorizontal(ld.remoteTop().rules, winSetPos.top);
    renderWinRulesHorizontal(ld.remoteBottom().rules, winSetPos.bottom);
    renderWinRulesVertical(ld.remoteLeft().rules, winSetPos.left);
    renderWinRulesVertical(ld.remoteRight().rules, winSetPos.right);
}

void MahjongPartScreen::renderScryRunes(void)
{
    if(ld.remoteTop().isAffectedSpell(Spell::ScryRunes))
    {
	Point pos;
	for(auto & stone : ld.remoteTop().stones)
	{
	    const StoneInfo & info = GameData::stoneInfo(stone);
	    const Texture & tx = GameTheme::texture(info.small);

	    if(pos.isNull())
	    {
		pos.x = namesPos.top.x - (ld.remoteTop().stones.size() * tx.width()) / 2;
		pos.y = namesPos.top.y - tx.height() / 2;
	    }

	    renderTexture(tx, pos);
	    pos.x += tx.width();
	}
    }

    if(ld.remoteLeft().isAffectedSpell(Spell::ScryRunes))
	renderScryVertical(ld.remoteLeft().stones, namesPos.left);

    if(ld.remoteRight().isAffectedSpell(Spell::ScryRunes))
	renderScryVertical(ld.remoteRight().stones, namesPos.right);
}

void MahjongPartScreen::renderScryVertical(const Stones & stones, const Point & center)
{
    Point pos;

    for(auto & stone : stones)
    {
	const StoneInfo & info = GameData::stoneInfo(stone);
	const Texture & tx = GameTheme::texture(info.small);

	if(pos.isNull())
	{
	    pos.x = center.x - tx.width() / 2;
	    pos.y = center.y - (stones.size() * tx.height()) / 2;
	}

	renderTexture(tx, pos);
	pos.y += tx.height();
    }
}

void MahjongPartScreen::renderWinRulesHorizontal(const WinRules & rules, const Point & center)
{
    const int countStones = rules.size() * 3 +
	    std::count_if(rules.begin(), rules.end(), [](const WinRule & rule){ return rule.isKong(); });

    if(countStones)
    {
	const StoneSprite sprite(rules[0].stone(), StoneSprite::Medium);
	Point pos = Point(center.x - (sprite.width() * countStones) / 2, center.y);

	for(auto & rule : rules)
	{
	    int offx = renderWinRuleHorizontal(rule, pos);
	    pos.x += offx + 1;
	}
    }
}

void MahjongPartScreen::renderWinRulesVertical(const WinRules & rules, const Point & center)
{
    const int countStones = rules.size() * 3 +
	    std::count_if(rules.begin(), rules.end(), [](const WinRule & rule){ return rule.isKong(); });

    if(countStones)
    {
	const StoneSprite sprite(rules[0].stone(), StoneSprite::Medium);
	Point pos = Point(center.x, center.y - (sprite.height() * countStones) / 2);

	for(auto & rule : rules)
	{
	    int offy = renderWinRuleVertical(rule, pos);
	    pos.y += offy + 1;
	}
    }
}

void MahjongPartScreen::renderCroupier(void)
{
    Point pos = croupierPos;
    const VecStones & trash = ld.trashSet;

    for(int index = 0; index < trash.size(); ++index)
    {
	const StoneInfo & info = GameData::stoneInfo(trash[index]);
	const Texture & sprite = GameTheme::texture(info.medium);

	if(index && 0 == (index % 9))
	{
	    pos.x = croupierPos.x;
	    pos.y += sprite.height();
	}

	renderTexture(sprite, pos);
	pos.x += sprite.width();
    }
}

void MahjongPartScreen::renderLocalSet(const GameStones & stones)
{
    stonesPos.clear();
    variantsPos.clear();

    if(stones.size())
    {
	const StoneSprite sprite(stones[0], StoneSprite::Large);
        const std::size_t visibleCount = dealingInitialHand ?
            std::min(dealtStoneCount, stones.size()) : stones.size();
	Point pos = Point(localSetPos.x - (sprite.width() * visibleCount) / 2, localSetPos.y);

	std::size_t visibleIndex = 0;
	for(auto it = stones.begin(); it != stones.end() && visibleIndex < visibleCount; ++it, ++visibleIndex)
	{
	    const StoneSprite sprite(*it, StoneSprite::Large);

	    stonesPos.push_back(Rect(pos, sprite.size()));
	    renderTexture(sprite, stonesPos.back());

	    if(! static_cast<const GameStone &>(*it).isCasted())
		renderTexture(stoneActiveSprite, stonesPos.back());

	    pos.x += sprite.width();
	}
    }

    // selected
    if(0 <= variantSelected)
    {
	for(auto & stone : ld.myPlayer().stones.findChaoVariants(ld.dropStone))
	{
	    auto it = std::find(stones.begin(), stones.end(), stone);
	    if(it != stones.end())
		variantsPos.push_back(stonesPos[std::distance(stones.begin(), it)]);
	}

	// mark variant
	for(auto & rt : variantsPos)
	    renderTexture(stoneVariantSprite, rt.toPoint() - Point(4, 6) + Point(0, stoneVariantSprite.height()));

	if(0 <= variantSelected && variantSelected < variantsPos.size())
	    renderTexture(stoneSelectedSprite, variantsPos[variantSelected].toPoint() - Point(4, 6));
    }
    else
    {
	if(0 <= stoneSelected && stoneSelected < stonesPos.size())
	    renderTexture(stoneSelectedSprite, stonesPos[stoneSelected].toPoint() - Point(4, 6));

	const LocalPlayer & player = ld.myPlayer();

	if(player.newStone.isValid())
	{
	    const StoneSprite sprite(player.newStone, StoneSprite::Large);
            const Rect stonePosition = animatingDrawStone ? Rect(drawStoneAnimationPos, sprite.size()) :
                                      newStonePos();
	    renderTexture(sprite, stonePosition);

	    if(! player.newStone.isCasted())
		renderTexture(stoneActiveSprite, stonePosition);

	    if(!animatingDrawStone && stones.size() == stoneSelected)
		renderTexture(stoneSelectedSprite, stonePosition.toPoint() - Point(4, 6));
	}
    }
}

void MahjongPartScreen::renderGameStoneRemains(void)
{
    const FontRender & frs = GameTheme::fontRender(defaultFont);
    renderText(frs, String::number(ld.stoneLastCount), defaultColor, remainsPos, AlignCenter, AlignCenter);
}

void MahjongPartScreen::renderWaitPlayers(const Wind & wind)
{
    playersMarker |= static_cast<int>(1) << wind();
}

void MahjongPartScreen::tickEvent(u32 ms)
{
    // Modal dialogs run a nested event loop. Keep the Mahjong client from polling
    // and handling the same server prompt again while a Luck choice is open.
    if(resolvingLuckChoice) return;

    if(animationGame.isEnabled())
    {
	if(animationGame.next(ms))
	    renderWindow();

	// need wait end animation
	return;
    }
    else
    if(animationKong.isEnabled())
    {
	if(animationKong.next(ms))
	    renderWindow();

	// need wait end animation
	return;
    }
    else
    if(animationPung.isEnabled())
    {
	if(animationPung.next(ms))
	    renderWindow();

	// need wait end animation
	return;
    }
    else
    if(animationChao.isEnabled())
    {
	if(animationChao.next(ms))
	    renderWindow();

	// need wait end animation
	return;
    }

    if(animationTurn.isEnabled())
    {
	if(animationTurn.next(ms))
	    renderWindow();
    }

    if(playerReady && tt.check(ms, Settings::presentationDelay(100)))
    {
	// Presentation actions can deliberately remain queued while an animation runs.
	// Consume them before asking the authoritative state for another action.
	if(actions.empty()) GameData::mahjong2Client(myAvatar, actions);
	bool redraw = false;

	while(actions.size())
	{
	    auto action = actions.front();
	    actions.pop_front();
	    CrashReport::breadcrumb(std::string("Mahjong action type=")
	        .append(String::number(action.type())).append(" payload=").append(action.toString()));

	    switch(action.type())
	    {
		case Action::MahjongBegin:
		    redraw = actionMahjongBegin(action);
		    break;

		case Action::MahjongEnd:
		    redraw = actionMahjongEnd(action);
		    break;

	        case Action::MahjongTurn:
		    redraw = actionMahjongTurn(action);
		    break;

	        case Action::MahjongLuckChoice:
		    redraw = actionMahjongLuckChoice(action);
		    break;

	        case Action::MahjongPass:
		    redraw = actionMahjongPass(action);
		    break;

	        case Action::MahjongGame:
		    actionMahjongGame(action);
		    // for start animationGame
		    return;

	        case Action::MahjongKong1:
		    actionMahjongKong1(action);
		    // for start animationKong
		    return;

	        case Action::MahjongKong2:
		    actionMahjongKong2(action);
		    // for start animationKong
		    return;

	        case Action::MahjongPung:
		    actionMahjongPung(action);
		    // for start animationPung
		    return;

	        case Action::MahjongChao:
		    actionMahjongChao(action);
		    // for start animationChao
		    return;

	        case Action::MahjongDrop:
		    redraw = actionMahjongDrop(action);
		    break;

	        case Action::MahjongSummon:
		    redraw = actionMahjongSummon(action);
		    break;

	        case Action::MahjongCast:
		    redraw = actionMahjongCast(action);
		    break;

	        case Action::MahjongInfo:
		    redraw = actionMahjongInfo(action);
		    break;

	        case Action::MahjongData:
		    redraw = actionMahjongLoadData();
		    break;

		default:
		    ERROR("unknown action: " << action.type());
		    break;
	    }

	    if(action.type() == Action::MahjongBegin ||
	       action.type() == Action::MahjongData ||
	       action.type() == Action::MahjongEnd)
	    {
		const std::string reason = std::string("mahjong-action-") + String::number(action.type());
		if(!GameData::saveRecovery(toJsonObject(), reason))
		    ERROR("recovery checkpoint failed: " << reason);
	    }
	}

	if(redraw) renderWindow();
    }
}

bool MahjongPartScreen::actionMahjongLoadData(void)
{
    ld = GameData::toLocalData(myAvatar);
    const LocalPlayer & player = ld.myPlayer();

    iconAffectedSkull.setVisible(player.isAffectedSpell(Spell::DrawSkull));
    iconAffectedSword.setVisible(player.isAffectedSpell(Spell::DrawSword));
    iconAffectedNumber.setVisible(player.isAffectedSpell(Spell::DrawNumber));
    iconAffectedDiscard.setVisible(player.isAffectedSpell(Spell::RandomDiscard));
    iconAffectedSilence.setVisible(player.isAffectedSpell(Spell::Silence));
    iconAffectedScry.setVisible(player.isAffectedSpell(Spell::ScryRunes));

    return true;
}

bool MahjongPartScreen::actionMahjongLuckChoice(const ActionMessage & v)
{
    const auto action = static_cast<const MahjongLuckChoice &>(v);
    const VecStones choices = action.choices();
    ld.currentWind = action.currentWind();

    if(!ld.yourTurn() || choices.size() != 2)
    {
	ERROR("invalid Luck draw action");
	return false;
    }

    ScopedFlag resolving(resolvingLuckChoice);
    LuckDrawDialog dialog(choices, *this);
    {
	// A nested dialog loop still ticks its parent unless explicitly paused.
	ScopedTickPause pause(*this);
	dialog.exec();
    }

    // Older/re-entrant polling may already have queued copies of this prompt.
    // They can never be a new valid choice before MahjongTurn/MahjongData.
    actions.remove_if([](const ActionMessage & queued) {
	return queued.type() == Action::MahjongLuckChoice;
    });

    const int selected = dialog.resultCode();
    if(selected < 0 || 1 < selected)
    {
	ERROR("invalid Luck draw selection: " << selected);
	return false;
    }

    CrashReport::breadcrumb(std::string("Luck selection=").append(String::number(selected)));
    if(!GameData::client2Mahjong(myAvatar, ClientLuckChoice(selected), actions))
	ERROR("Luck draw selection was rejected: " << selected);
    return false;
}

bool MahjongPartScreen::actionMahjongEnd(const ActionMessage & v)
{
    auto action = static_cast<const MahjongEnd &>(v);
    ld.currentWind = action.currentWind();

    // The authoritative round end invalidates any countdown that belonged to
    // the last local turn.  Leaving it active can emit a stale timeout while
    // the end-of-round presentation is running.
    animationTurn.setEnabled(false);

#ifndef SWE_DISABLE_AUDIO
    playSoundWait();
    Music::reset();
#endif

    pushEventAction(Action::MahjongGameQuit, this, nullptr);
    DEBUG("current wind: " << ld.currentWind.toString());

    return false;
}

bool MahjongPartScreen::actionMahjongBegin(const ActionMessage & v)
{
    auto action = static_cast<const MahjongBegin &>(v);
    ld.currentWind = action.currentWind();
    ld.roundWind = action.roundWind();

    // A new part must never inherit the final countdown from the previous one.
    animationTurn.setEnabled(false);

    playSound("begin");
    playSoundWait();

    if(action.newRound())
    {
	playSound(std::string("round").append("_").append(ld.roundWind.toString()));
	playSoundWait();
    }

    gameLogs << StringFormat(_("Game Begins With %1")).arg(GameData::windInfo(ld.roundWind).name);
    DEBUG("new round: " << (action.newRound() ? "true" : "false") << ", " << "wind round: " << ld.roundWind.toString());

    renderInitialDeal();

    return false;
}

bool MahjongPartScreen::actionMahjongTurn(const ActionMessage & v)
{
    auto action = static_cast<const MahjongTurn &>(v);
    ld.currentWind = action.currentWind();

    // Every authoritative turn change retires the previous local countdown.
    // Previously a submitted move left the animation ticking through later AI
    // turns until it raised MahjongOutOfTime with no remaining choice to make.
    animationTurn.setEnabled(false);

    stoneSelected = -1;
    variantSelected = -1;
    playersMarker = 0;

    if(! ld.yourTurn())
    {
	const RemotePlayer & owner = ld.playerOfWind(ld.currentWind);
	DEBUG(owner.toString());
	return false;
    }

    animationTurn.setEnabled(true);

    LocalPlayer & player = ld.myPlayer();

    player.newStone = GameStone(action.newStone(), false);
    player.setCasted(false);

    if(action.showGame())
    {
	buttonLocalGame.setPosition(localGamePos());
	buttonLocalGame.setVisible(true);
    }

    if(action.showKong())
    {
	buttonLocalKong.setPosition(localKongPos());
	buttonLocalKong.setVisible(true);
    }

    if(player.newStone.isValid())
    {
	playSound("stone2");
	stoneSelected = player.stones.size();
	renderDrawStone();

	DEBUG("current wind: " << ld.currentWind.toString() << ", " << "new stone: " << player.newStone());
	return true;
    }

    stoneSelected = player.stones.size() - 1;

    DEBUG("current wind: " << ld.currentWind.toString() << ", " << "new stone: " << "skipped");
    return true;
}

bool MahjongPartScreen::actionMahjongGame(const ActionMessage & v)
{
    auto action = static_cast<const MahjongGame &>(v);
    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    if(action.sayOnly())
    {
	buttonPass->setVisible(true);
	buttonChao->setVisible(false);
	buttonPung->setVisible(false);
	buttonKong->setVisible(false);

	playSound(owner.avatar.toString().append("_game"));
	renderWaitPlayers(ownerWind);

	fastLogText.text = StringFormat(_("%1 Call Game")).arg(owner.name());
	fastLogOwner = ownerWind;

	gameLogs << fastLogText.text;
	return true;
    }
    else
    {
	animationGame.setEnabled(true);

	DEBUG(owner.toString() << ", " << "drop stone: " << ld.dropStone());
    }

    //animationTurn.stop();

    return false;
}

u32 startAnimationBackgroundGuard(u32 ms, void* ptr)
{
    if(ptr)
    {
	SpritesAnimation* animation = reinterpret_cast<SpritesAnimation*>(ptr);
#ifndef SWE_DISABLE_AUDIO
	if(Settings::soundGuardianRules())
	    while(Sound::isPlaying()) Tools::delay(50);
#endif
	animation->setEnabled(true);
    }

    return 0;
}

bool MahjongPartScreen::actionMahjongKong1(const ActionMessage & v)
{
    auto action = static_cast<const MahjongKong1 &>(v);
    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    if(action.sayOnly())
    {
	buttonChao->setVisible(false);

	playSound(owner.avatar.toString().append("_kong"));
	timerVoiceAnimation = Timer::create(100, startAnimationBackgroundGuard, & animationKong);
	renderWaitPlayers(ownerWind);

	fastLogText.text = StringFormat(_("%1 Kongs The Last Discard")).arg(owner.name());
	fastLogOwner = ownerWind;

	gameLogs << fastLogText.text;
	return true;
    }
    else
    {
	ld.dropStone = action.dropStone();
	DEBUG(owner.toString() << ", " << "drop stone: " << ld.dropStone());
    }

    return false;
}

bool MahjongPartScreen::actionMahjongKong2(const ActionMessage & v)
{
    auto action = static_cast<const MahjongKong2 &>(v);
    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    DEBUG(owner.toString());

    playSound(owner.avatar.toString().append("_kong"));
    timerVoiceAnimation = Timer::create(100, startAnimationBackgroundGuard, & animationKong);

    fastLogText.text = StringFormat(_("%1 Kongs The Last Drawn Rule")).arg(owner.name());
    fastLogOwner = ld.currentWind;

    gameLogs << fastLogText.text;
    return true;
}

bool MahjongPartScreen::actionMahjongPung(const ActionMessage & v)
{
    auto action = static_cast<const MahjongPung &>(v);
    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    if(action.sayOnly())
    {
	buttonChao->setVisible(false);

	playSound(owner.avatar.toString().append("_pung"));
	timerVoiceAnimation = Timer::create(100, startAnimationBackgroundGuard, & animationPung);
	renderWaitPlayers(ownerWind);

	fastLogText.text = StringFormat(_("%1 Pungs The Last Discard")).arg(owner.name());
	fastLogOwner = ownerWind;
	gameLogs << fastLogText.text;
	return true;
    }
    else
    {
	ld.dropStone = action.dropStone();
	DEBUG(owner.toString() << ", " << "drop stone: " << ld.dropStone());
    }

    return false;
}

bool MahjongPartScreen::actionMahjongChao(const ActionMessage & v)
{
    auto action = static_cast<const MahjongChao &>(v);
    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    if(action.sayOnly())
    {
	playSound(owner.avatar.toString().append("_chao"));
	timerVoiceAnimation = Timer::create(100, startAnimationBackgroundGuard, & animationChao);
	renderWaitPlayers(ownerWind);

	fastLogText.text = StringFormat(_("%1 Chows The Last Discard")).arg(owner.name());
	fastLogOwner = ownerWind;
	gameLogs << fastLogText.text;

	return true;
    }
    else
    {
	ld.dropStone = action.dropStone();
	DEBUG(owner.toString() << ", " << "drop stone: " << ld.dropStone());
    }

    return false;
}

bool MahjongPartScreen::actionMahjongPass(const ActionMessage & v)
{
#ifdef BUILD_DEBUG
    auto action = static_cast<const MahjongPass &>(v);

    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    DEBUG(owner.toString());
#endif
    return false;
}

bool MahjongPartScreen::actionMahjongDrop(const ActionMessage & v)
{
    auto action = static_cast<const MahjongDrop &>(v);

    ld.currentWind = action.currentWind();
    ld.dropStone = action.dropStone();

    LocalPlayer & player = ld.myPlayer();

    bool showChao = player.isMahjongChao(ld.currentWind, ld.dropStone);
    bool showPung = player.isMahjongPung(ld.currentWind, ld.dropStone);
    bool showKong = player.isMahjongKong1(ld.currentWind, ld.dropStone);
    bool showGame = player.isWinMahjong(ld.currentWind, ld.roundWind, ld.dropStone, & ld.winResult);

    buttonPass->setVisible(true);
    buttonGame->setVisible(showGame);
    buttonKong->setVisible(showKong);
    buttonPung->setVisible(showPung);
    buttonChao->setVisible(showChao);

    player.newStone.reset();
    stoneSelected = -1;

    renderWaitPlayers(ld.currentWind);
    renderDropStone();

    DEBUG("current wind: " << ld.currentWind.toString() << ", " << "drop stone: " << ld.dropStone());

    return true;
}

bool MahjongPartScreen::actionMahjongSummon(const ActionMessage & v)
{
    auto action = static_cast<const MahjongSummon &>(v);

    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    Creature creature = action.creature();
    Land land = action.land();

    const CreatureInfo & creatureInfo = GameData::creatureInfo(creature);
    const LandInfo & landInfo = GameData::landInfo(land);

    playSound(creatureInfo.sound1);

    fastLogText.text = StringFormat(_("%1 summons %2 in %3")).
			arg(GameData::avatarInfo(owner.avatar).name).
			arg(creatureInfo.name).
			arg(landInfo.name);
    fastLogOwner = ownerWind;
    gameLogs << fastLogText.text;

    DEBUG(owner.toString() << ", " << "creature: " << creature.toString() << ", " << "land: " << land.toString());
    return true;
}

bool MahjongPartScreen::actionMahjongCast(const ActionMessage & v)
{
    auto action = static_cast<const MahjongCast &>(v);

    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    Spell spell = action.spell();
    const std::vector<int> targetUnits = action.targetUnits();
    std::vector<int> resists = action.resists();

    const SpellInfo & spellInfo = GameData::spellInfo(spell);

    playSound(spellInfo.sound);

    fastLogText.text.clear();
    fastLogOwner = ownerWind;

    if(spellInfo.target() == SpellTarget::Land)
    {
	Land land = action.land();
	const LandInfo & landInfo = GameData::landInfo(land);
	fastLogText.text = StringFormat(_("%1 casts %2 over %3")).
			    arg(owner.name()).arg(spellInfo.name).arg(landInfo.name);
	gameLogs << fastLogText.text;
    }
    else
    if(spellInfo.target() == SpellTarget::MyPlayer ||
	spellInfo.target() == SpellTarget::AllPlayers)
    {
	fastLogText.text = StringFormat(_("%1 casts %2")).
			    arg(owner.name()).arg(spellInfo.name);
	gameLogs << fastLogText.text;

    }
    else
    if(spellInfo.target() == SpellTarget::OtherPlayer)
    {
	Avatar target = action.target();
	const RemotePlayer & remote = ld.playerOfAvatar(target);

	fastLogText.text = StringFormat(_("%1 casts %2 to %3")).
			    arg(owner.name()).arg(spellInfo.name).arg(remote.name());
	gameLogs << fastLogText.text;
    }

    for(int unit : targetUnits)
    {
	const BattleCreature* target = ld.findBattleUnitConst(unit);

	if(target)
	{
	    bool resist = resists.end() != std::find(resists.begin(), resists.end(), target->battleUnit());

	    DEBUG(target->toString());
	    if(resist) DEBUG("Magic Resistance!");

	    const CreatureInfo & creatureInfo = GameData::creatureInfo(*target);
	    std::string tmp;

	    if(owner.clan != target->clan())
		tmp = StringFormat(_("%1 casts %2 at %3's %4")).
		    arg(owner.name()).arg(spellInfo.name).arg(ld.playerOfClan(target->clan()).name()).arg(creatureInfo.name);
	    else
		tmp = StringFormat(_("%1 casts %2 at friendly %3")).
		    arg(owner.name()).arg(spellInfo.name).arg(creatureInfo.name);

	    if(resist) tmp.append(", ").append(_("Magic Resistance!"));

	    if(fastLogText.text.empty()) fastLogText.text = tmp;
	    gameLogs << tmp;
	}
    }

    if(fastLogText.text.empty() && !targetUnits.empty())
    {
	fastLogText.text = StringFormat(_("%1 casts %2")).arg(owner.name()).arg(spellInfo.name);
	gameLogs << fastLogText.text;
    }

    DEBUG(owner.toString() << ", " << "spell: " << spell.toString() << ", " << "target: " << spellInfo.target.toString());
    return true;
}

bool MahjongPartScreen::actionMahjongInfo(const ActionMessage & v)
{
    auto action = static_cast<const MahjongInfo &>(v);

    Wind ownerWind = action.currentWind();
    const RemotePlayer & owner = ld.playerOfWind(ownerWind);

    const std::string info = action.info();

    fastLogText.text = info;
    fastLogOwner = ownerWind;

    gameLogs << info;
    DEBUG(owner.toString() << ", " << "info: " << info);

    return true;
}

bool MahjongPartScreen::actionEventDebug1(void)
{
    VERBOSE("test1");

    auto creature = Creature::RedDragon;
    ShowSummonCreatureDialog summonDialog(ld, creature, *this);
    if(0 < summonDialog.exec())
	GameData::client2Mahjong(myAvatar, ClientSummonCreature(creature, summonDialog.land(), true), actions);

    return true;
}

bool MahjongPartScreen::actionEventDebug2(void)
{
    VERBOSE("test2");

    auto creature = Creature::SkeletonHorde;
    ShowSummonCreatureDialog summonDialog(ld, creature, *this);
    if(0 < summonDialog.exec())
	GameData::client2Mahjong(myAvatar, ClientSummonCreature(creature, summonDialog.land(), true), actions);

/*
    int spell = Spell::Heroism;
    ShowCastSpellDialog spellDialog(ld, spell, *this);

    if(spellDialog.exec())
    {
	BattleTarget target = spellDialog.targetUnit();
	VERBOSE(target.toString());
	GameData::client2Mahjong(myAvatar, ClientCastSpell(spell, target.land, (target.bcr ? target.bcr->battleUnit() : 0), true), actions);
    }
*/

    return true;
}
