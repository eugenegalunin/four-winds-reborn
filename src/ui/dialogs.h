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

#ifndef _RWNA_DIALOGS_
#define _RWNA_DIALOGS_

#include <functional>
#include <vector>

#include "gamedata.h"

struct CreatureInfo;

void MessageTop(const std::string & hdr, const std::string & msg, Window &);

class DialogWindow : public Window
{
protected:
    JsonObject		jobject;

    int			borderWidth;
    std::string		font;

    Color		backgroundColor;
    Color		borderColor;
    Color		headerColor;
    Color		textColor;

    void		actionDialogClose(void);

protected:
    bool                keyPressEvent(const KeySym &) override;
    bool		mouseClickEvent(const ButtonsEvent &) override;

public:
    DialogWindow(const char*, Window &);
    ~DialogWindow() {}
};

class TextAreaScroll : public Window
{
    struct TextLine
    {
	Texture		texture;
	int		offsetX;
	int		height;
    };

    std::vector<TextLine>	lines;
    int			contentWidth;
    int			firstLine;
    int			scrollWidth;
    int			minimumCursorHeight;

	Color			backgroundColor;
    Color		colorRect;
    Color		colorFill;
    Color		cursorRect;
    Color		cursorFill;

    int			lastFirstLine(void) const;
    int			totalTextHeight(void) const;
    void			appendLine(const FontRender &, const UCString &, int);
    void			setFirstLine(int);

protected:
    bool		scrollUpEvent(void) override;
    bool		scrollDownEvent(void) override;
    bool		mouseClickEvent(const ButtonsEvent &) override;

public:
    TextAreaScroll(const JsonObject &, Window &);

    void		setPositionSize(int, int, int, int);
    void		clear(void);

    TextAreaScroll &	appendString(const FontRender &, const UnicodeString &, const Color &,
				     int halign = AlignLeft, bool wrap = false);
    TextAreaScroll &	appendString(const FontRender &, const UCString &,
				     int halign = AlignLeft, bool wrap = false);

    void		renderWindow(void) override;
};

class CreatureInfoDialog : public DialogWindow
{
    const CreatureInfo& creatureInfo;
    TextAreaScroll	textArea;

    Texture		skill1;
    Texture		skill2;
    Texture		skill3;
    Texture		skill4;
    Texture		skill5;

public:
    CreatureInfoDialog(const Creature &, Window &);
    void		renderWindow(void);
};

class SpellInfoDialog : public DialogWindow
{
    const SpellInfo &	spellInfo;
    TextAreaScroll	textArea;

public:
    SpellInfoDialog(const Spell &, Window &);
    void		renderWindow(void) override;
};

class AvatarInfoDialog : public DialogWindow
{
    const AvatarInfo &	avatarInfo;
    TextAreaScroll	textArea;

public:
    AvatarInfoDialog(const AvatarInfo &, Window &);
    void		renderWindow(void) override;
};

struct RuneCastRow
{
    Rect		pos;
    Stones		stones;
    std::string		name;
    std::string		description;
    int			cost;
    bool		allowCast;
    bool		disabledUnique;
    std::unique_ptr<Enum> val;

    RuneCastRow() : cost(0), allowCast(false), disabledUnique(false) {}
};

struct RuneCastContent : std::vector<RuneCastRow>
{
    Rect		offsetColumn1;
    Rect		offsetColumn2;
    int			rowHeight;

    RuneCastContent();
    void		addRow(const Enum &, const Stones &, const std::string &, const std::string &, int, const LocalPlayer &, const Stone &);
};

class RuneCastDialog : public DialogWindow
{
    const LocalPlayer*	player;
    RuneCastContent	content;
    int			selected;

    Texture		background;
    Texture             skill1;
    Texture             skill2;
    Texture             skill3;
    Texture             skill4;
    Texture             skill5;

    Color		normalColor;
    Color		allowColor;
    Color		selectedColor;

    Point		offsetPlayerName;
    Point		offsetPlayerPoints;
    Point		offsetRuleCost;
    Point		offsetRuleName;
    Point		offsetRuleDescription;
    Rect		offsetCloseWindow;

    void		renderContent(void);

protected:
    bool                mouseClickEvent(const ButtonsEvent &) override;

public:
    RuneCastDialog(const LocalPlayer &, const Stone &, Window &);

    void		renderWindow(void) override;
    bool		resultIsCreature(void) const;
    Creature		resultCreature(void) const;
    Spell		resultSpell(void) const;
};

class ShowLogsDialog : public DialogWindow
{
    TextAreaScroll	textArea;
    Texture		background;

protected:
    bool		mouseClickEvent(const ButtonsEvent &) override;

public:
    ShowLogsDialog(const UnicodeList &, Window &);
    void		renderWindow(void) override;
};

class MessageBox : public DialogWindow
{
    TextAreaScroll	textArea;

    std::string		header;

    TextureButton	buttonOk;
    TextureButton	buttonCancel;

protected:
    bool                userEvent(int, void*) override;
    bool		mouseClickEvent(const ButtonsEvent & coords) override { return false; }

public:
    MessageBox(const std::string &, const std::string &, Window &, bool buttons = true);
    void		renderWindow(void) override;
};

namespace InGameMenuResult
{
    constexpr int Cancel = 0;
    constexpr int SaveGame = 1;
    constexpr int MainMenu = 2;
}

class InGameMenuDialog : public DialogWindow
{
    struct Entry
    {
        std::string label;
        std::string note;
        Rect        area;
        int         result;
    };

    std::vector<Entry> entries;
    int                selected;
    std::string        titleFont;
    std::string        entryFont;
    std::string        smallFont;
    Color              itemColor;
    Color              itemSelectedColor;
    Color              itemBorderColor;
    Color              selectedBorderColor;
    Color              mutedColor;

    bool                activateSelected(void);
    bool                selectNext(int);

protected:
    bool                keyPressEvent(const KeySym &) override;
    bool                mouseClickEvent(const ButtonsEvent &) override;
    bool                mouseMotionEvent(const Point &, u32) override;

public:
    explicit InGameMenuDialog(Window &);
    void                renderWindow(void) override;
};

class SaveNameDialog : public DialogWindow
{
    std::string        saveName;
    std::string        validationMessage;
    Rect               inputArea;
    Rect               saveArea;
    Rect               cancelArea;
    std::string        titleFont;
    std::string        inputFont;
    std::string        smallFont;
    Color              inputColor;
    Color              inputBorderColor;
    Color              buttonColor;
    Color              buttonBorderColor;
    Color              mutedColor;
    Color              errorColor;

    bool               accept(void);
    void               eraseLastCodepoint(void);

protected:
    bool               keyPressEvent(const KeySym &) override;
    bool               textInputEvent(const std::string &) override;
    bool               mouseClickEvent(const ButtonsEvent &) override;

public:
    explicit SaveNameDialog(Window &);
    ~SaveNameDialog();

    const std::string & name(void) const { return saveName; }
    void               renderWindow(void) override;
};

bool SaveGameAs(const JsonObject &, Window &, const std::function<bool()> & prepare = {});

class MapStatusDialog : public DialogWindow
{
    const LocalData &	localData;

    Texture		background;
    JsonButton		buttonClose;
    Sprites		iconSprites;

    Point		orderPlayPos;
    Point		youArePos;
    Point		offsetArmy;
    Point		offsetAvatarPort;
    Point		offsetClanIcon;
    Point		offsetTextAvatar;
    Point		offsetTextClan;
    Point		offsetTextPoint;
    Point		offsetTextLands;
    Point		offsetTextLandPoint;
    Point		offsetTextLandPower;

    Points		offsetClanIcons;
    Points		offsetClanValues;
    Point		offsetWinds[4];

    void		renderWindSection(const RemotePlayer &, const Point &);
    void 		renderBattleArmy(const BattleCreatures &, const Point &);

protected:
    bool                userEvent(int, void*) override;

public:
    MapStatusDialog(const LocalData &, Window &);

    void		renderWindow(void) override;
};

struct CombatUnit
{
    BattleUnit*	battle;
    Point       	pos;
    Texture     	back;
    Texture     	status;
    Texture     	image;

    CombatUnit() : battle(nullptr) {}
    CombatUnit(BattleUnit*, const Point &, const Texture &, const Texture &);

    void		render(Window &, const std::pair<Color, Color> &, bool showCorpse = true);
    bool		isUid(int uid) const { return battle ? battle->isBattleUnit(uid) : false; }
    int                 uid(void) const { return battle ? battle->battleUnit() : -1; }
    Rect                area(void) const { return Rect(pos, back.size()); }
};

struct CombatUnits : std::vector<CombatUnit>
{
    CombatUnits() { reserve(7); }

    using std::vector<CombatUnit>::push_back;
    void		push_back(const Point &, const Point &, const Point &, const Texture &, const Texture &, BattleCreatures);

    CombatUnit*		find(int uid);
};

class CombatScreenDialog : public DialogWindow
{
    BattleLegend	legend;
    BattleStrikes	strikes;
    std::size_t         totalStrikes;
    std::size_t         appliedStrikes;
    BattleStrike        lastStrike;
    bool                hasLastStrike;
    int			delayStrikeAnim;
    int			delayCombatScreen;
    bool                playbackPaused;
    int                 playbackSpeed;
    u32                 playbackClock;
    u32                 lastTickMs;

    Texture		background;
    Color		textColor;
    Color               controlColor;
    std::pair<Color, Color>
			damageColors;

    std::string		name1;
    std::string		name2;

    Sprite		spritePort1;
    Sprite		spritePort2;

    CombatUnits		units;
    SWE::TickTrigger	tt;

    SpritesAnimation	animationStrikeMelee;
    SpritesAnimation	animationStrikeRanger;
    SpritesAnimation	animationFireShield;

    Point		center1Pos;
    Point		center2Pos;
    Point               phasePos;
    Point               timelinePos;
    Point               pauseTextPos;
    Point               speedTextPos;
    Rect                pauseArea;
    Rect                speedArea;
    std::string         controlFont;

    bool		renderCorpse;
    bool		doDialogClose;

    void		renderSummary(const Avatar & winner);
    Rect		applyStrikeDamage(const BattleStrike &);

protected:
    bool                mouseClickEvent(const ButtonsEvent &) override;
    void		tickEvent(u32) override;

public:
    CombatScreenDialog(const BattleLegend &, const BattleStrikes &, Window &);

    void		renderWindow(void) override;
};

class TargetPlayerButton : public JsonButton
{
public:
    TargetPlayerButton(const RemotePlayer &, Window &);
};

class TargetPlayerDialog : public DialogWindow
{
    TargetPlayerButton  buttonTargetLeft;
    TargetPlayerButton  buttonTargetRight;
    TargetPlayerButton  buttonTargetTop;

protected:
    void		signalReceive(int, const SignalMember*) override;
    bool		keyPressEvent(const KeySym &) override { return true; }
    bool                userEvent(int, void*) override;

public:
    TargetPlayerDialog(const LocalData &, Window &);

    void		renderWindow(void) override;
};

#endif
