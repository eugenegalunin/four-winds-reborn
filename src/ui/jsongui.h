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

#ifndef _RWNA_JSONGUI_H_
#define _RWNA_JSONGUI_H_

#include "gamedata.h"

class IconToolTip : public WindowToolTipIcon
{
public:
    IconToolTip(Window* win) : WindowToolTipIcon(win) {}
    IconToolTip(const Texture &, const std::string &, Window &);

    void		setToolTip(const std::string &) override;
    void		setToolTip(const std::string &, int) override;
    void		setToolTip(const StringList &) override;

    void		setJsonObject(const JsonObject*);
};

class CreatureIcon : public IconToolTip
{
    Creature            creature;

protected:
    bool	        mouseClickEvent(const ButtonsEvent &) override;

public:
    CreatureIcon(const CreatureInfo &, const Point &, Window &);

    bool                isCreature(const Creature & cr) const { return cr == creature; }
};

class SpellIcon : public IconToolTip
{
    Spell	        spell;

protected:
    bool	        mouseClickEvent(const ButtonsEvent &) override;

public:
    SpellIcon(const SpellInfo &, const Point &, Window &);

    bool                isSpell(const Spell & sp) const { return sp == spell; }
};

class ExtIconToolTip : public IconToolTip
{
    Texture selectedtx, disabledtx;

public:
    ExtIconToolTip(const Texture &, const Texture &, const Texture &, const std::string &, Window &);

    void                renderWindow(void) override;

    void                setSelected(bool f);
    void                setDisabled(bool f);

    bool                isSelected(void) const;
    bool                isDisabled(void) const;
};

class ClanIcon : public ExtIconToolTip
{
    Clan	        clan;

public:
    ClanIcon(const ClanInfo &, const Texture &, const Texture &, const Point &, Window &);

    bool                isClan(const Clan & cl) const { return cl == clan; }
    const Clan &        toClan(void) const { return clan; }
};

class AvatarIcon : public ExtIconToolTip
{
    Avatar	        avatar;

public:
    AvatarIcon(const AvatarInfo &, const Texture &, const Texture &, const Point &, Window &);

    bool                isAvatar(const Avatar & av) const { return av == avatar; }
    const Avatar &      toAvatar(void) const { return avatar; }
};

struct Sprite : public TexturePos
{
    Sprite() {}
    Sprite(const Texture & tx) : TexturePos(tx) {}
    Sprite(const Texture & tx, const Point & pos) : TexturePos(tx, pos) {}
};

struct Sprites : std::vector<Sprite>
{
    Sprites() {}
    Sprites(const Sprites & s) : std::vector<Sprite>(s) {}
    Sprites(Sprites && s) noexcept { swap(s); }
    Sprites(std::vector<Sprite> && v) noexcept { swap(v); }

    Sprites &           operator=(const Sprites & s) { assign(s.begin(), s.end()); return *this; }
    Sprites &           operator=(Sprites && s) noexcept { swap(s); return *this; }
    Sprites &           operator=(std::vector<Sprite> && v) noexcept { swap(v); return *this; }

    Sprites &           operator<< (const Sprite & sp) { push_back(sp); return *this; }
};

struct SpritesAnimation
{
    int                 index;
    bool		loop;
    u32                 delay;
    std::string         sound;
    Sprites             sprites;
    TickTrigger         tt;

    SpritesAnimation() : index(-1), loop(false), delay(250) {}
    SpritesAnimation(const JsonObject &, const std::string &);
    virtual ~SpritesAnimation() {}

    void                render(Window &) const;
    void                renderCentered(Window &, const Point &) const;

    Size		spriteSize(void) const;
    void		setPosition(const Point &);

    void                setEnabled(bool);
    virtual bool        isEnabled(void) const { return 0 < sprites.size() && 0 <= index; }

    bool                next(u32 ms);
    void                next(void);
};

struct AlignValue
{
    AlignType           value;

    AlignValue(const AlignType & v = AlignLeft) : value(v) {}
    AlignValue(const std::string &, const AlignType & def = AlignLeft);

    const AlignType &   operator() (void) const { return value; }

    bool                isLeft(void) const { return value == AlignLeft; }
    bool                isRight(void) const { return value == AlignRight; }
    bool                isTop(void) const { return value == AlignTop; }
    bool                isBottom(void) const { return value == AlignBottom; }
    bool                isCenter(void) const { return value == AlignCenter; }
};

struct SidesPositions
{
    Point               left;
    Point               right;
    Point               top;
    Point               bottom;
    Point               center;
};

struct JsonToolTip
{
    Color	        bgcolor;
    Color	        rtcolor;
    Color	        fncolor;
    std::string	        font;
};

struct JsonTextColor
{
    std::string         font;
    std::string         text;
    Color               color;
};

struct JsonAlignPosition
{
    Point               position;
    Size                size;
    AlignValue          align;
};

struct JsonTextInfo : JsonTextColor, JsonAlignPosition
{
    JsonTextInfo() {}
    JsonTextInfo(const JsonTextColor & tc, const JsonAlignPosition & ap) : JsonTextColor(tc), JsonAlignPosition(ap) {}
};

struct ButtonInfo
{
    std::string         ids;
    std::string         action2;
    Sprite              sprite1;        // sprite release
    Sprite              sprite2;        // sprite press
    Sprite              sprite3;        // sprite mouse focus
    Sprite              sprite4;        // sprite informer
    std::string         cursorFocusTheme;
    std::string         cursorPressTheme;

    ButtonInfo() {}

    void                setJsonInfo(const JsonObject*);
    JsonObject          toJsonInfo(void) const;
};

class JsonButton : public WindowButton, public ButtonInfo
{
protected:
    const Texture*      textureFocused(void) const override { return &sprite3; }
    const Texture*      textureDisabled(void) const override { return texturePressed(); }
    const Texture*      texturePressed(void) const override { return &sprite2; }
    const Texture*      textureReleased(void) const override { return &sprite1; }
    const Texture*      textureInformed(void) const override { return &sprite4; }

    void                mouseFocusEvent(void) override;
    void                mouseLeaveEvent(void) override;

public:
    JsonButton(Window* win = nullptr) : WindowButton(win) {}
    JsonButton(const ButtonInfo & info) : WindowButton(nullptr), ButtonInfo(info) {}
    JsonButton(const JsonObject &, const char*, int, Window*);

    const std::string & themePress(void) const { return cursorPressTheme; }
    const std::string & themeFocus(void) const { return cursorFocusTheme; }
    const std::string & resultAction(void) const { return action2; }

    bool                isIds(const std::string & str) const { return str == ids; }
    bool                isAction(const std::string & str) const { return str == action2; }

    void                setJsonInfo(const JsonObject*);
    JsonObject          toJsonInfo(void) const;
};

struct JsonButtons : std::list<JsonButton>
{
    Rect                area;

    JsonButtons() {}
    JsonButtons(std::list<JsonButton> && list) noexcept { swap(list); }

    void                setVisible(bool);
    JsonButton*         findIds(const std::string &);
    JsonButton*         findAction(const std::string &);
};

class JsonWindow : public Window
{
protected:
    JsonObject		jobject;
    std::string		defaultCursor;
    Color		backColor;
    Sprites		sprites;
    JsonButtons		buttons;

    void		signalReceive(int, const SignalMember*) override;
    static void		playRandomMusic(void);

public:
    JsonWindow(const std::string &, Window*);

    void		renderWindow(void) override;

    Rect                renderTextInfo(const JsonTextInfo &);
    Rect                renderTextInfo(const JsonTextInfo &, const std::string &);
    Rect                renderTextInfo(const JsonTextInfo &, const std::string &, const Point &, const AlignType & = AlignLeft);

    void		cursorSet(const std::string &);
    void		cursorReset(void);

    static void		playSound(const std::string &);
    static void		playSoundWait(void);
    static void		playMusic(const std::string &);
};

#endif
