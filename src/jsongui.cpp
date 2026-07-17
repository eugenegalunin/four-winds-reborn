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

#include "gametheme.h"
#include "settings.h"
#include "dialogs.h"
#include "jsongui.h"

IconToolTip::IconToolTip(const Texture & tx, const std::string & str, Window & win)
    : WindowToolTipIcon(& win)
{
    setTexture(tx);
    setToolTip(str);
    setVisible(true);
}

void IconToolTip::setToolTip(const std::string & str, int width)
{
    const JsonToolTip & tti = GameTheme::jsonToolTipInfo();
    const FontRender & frs = GameTheme::fontRender(tti.font);
    setToolTip(frs.splitStringWidth(str, width));
}

void IconToolTip::setToolTip(const std::string & str)
{
    const JsonToolTip & tti = GameTheme::jsonToolTipInfo();
    const FontRender & frs = GameTheme::fontRender(tti.font);
    renderToolTip(str, frs, tti.fncolor, tti.bgcolor, tti.rtcolor);
}

void IconToolTip::setToolTip(const StringList & list)
{
    const JsonToolTip & tti = GameTheme::jsonToolTipInfo();
    const FontRender & frs = GameTheme::fontRender(tti.font);
    renderToolTip(list, frs, tti.fncolor, tti.bgcolor, tti.rtcolor);
}

void IconToolTip::setJsonObject(const JsonObject* jo)
{
    if(jo)
    {
	setTexture(GameTheme::sprite(jo->getString("sprite")));
	setPosition(JsonUnpack::point(*jo, "offset"));
	setToolTip(jo->getString(("info")));
	setVisible(true);
    }
}

/* CreatureIcon */
CreatureIcon::CreatureIcon(const CreatureInfo & cr, const Point & pos, Window & win)
    : IconToolTip(GameTheme::texture(cr.image2), StringFormat(_("Creature: %1, stats [%2, %3, %4, %5]")).
	arg(cr.name).arg(cr.stat.attack).arg(cr.stat.ranger).arg(cr.stat.defense).arg(cr.stat.loyalty), win), creature(cr.id)
{
    setPosition(pos);
}

bool CreatureIcon::mouseClickEvent(const ButtonsEvent & coords)
{
    if(parent())
	CreatureInfoDialog(creature, *parent()).exec();
    return true;
}

/* SpellIcon */
SpellIcon::SpellIcon(const SpellInfo & sp, const Point & pos, Window & win)
    : IconToolTip(GameTheme::texture(sp.image), StringFormat(_("Spell: %1, target: %2, %3")).
	arg(sp.name).arg(sp.target.toString()).arg(sp.effectDescription()), win), spell(sp.id)
{
    setPosition(pos);
}

bool SpellIcon::mouseClickEvent(const ButtonsEvent & coords)
{
    if(parent())
	SpellInfoDialog(spell, *parent()).exec();
    return true;
}

ExtIconToolTip::ExtIconToolTip(const Texture & tx1, const Texture & tx2, const Texture & tx3, const std::string & str, Window & win)
    : IconToolTip(tx1, str, win), selectedtx(tx2), disabledtx(tx3)
{
}

void ExtIconToolTip::setSelected(bool f)
{
    setState(FlagSelected, f);
}

void ExtIconToolTip::setDisabled(bool f)
{
    setState(FlagDisabled, f);
}

bool ExtIconToolTip::isSelected(void) const
{
    return checkState(FlagSelected);
}

bool ExtIconToolTip::isDisabled(void) const
{
    return checkState(FlagDisabled);
}

void ExtIconToolTip::renderWindow(void)
{
    renderTexture(icon, Point(0, 0));

    if(isDisabled())
	renderTexture(disabledtx, Point(0, 0));
    else
    if(isSelected())
	renderTexture(selectedtx, Point(0, 0));
}

ClanIcon::ClanIcon(const ClanInfo & cl, const Texture & tx1, const Texture & tx2, const Point & pos, Window & win)
    : ExtIconToolTip(GameTheme::texture(cl.image), tx1, tx2, StringFormat(_("Clan: %1")).arg(cl.name), win), clan(cl.id)
{
    setPosition(pos);
}

AvatarIcon::AvatarIcon(const AvatarInfo & av, const Texture & tx1, const Texture & tx2, const Point & pos, Window & win)
    : ExtIconToolTip(GameTheme::texture(av.portrait), tx1, tx2, StringFormat(_("Avatar: %1")).arg(av.name), win), avatar(av.id)
{
    setPosition(pos);
}

SpritesAnimation::SpritesAnimation(const JsonObject & jobject, const std::string & ids) : index(-1), loop(false), delay(250)
{
    const JsonObject* jo = jobject.getObject(ids);

    if(jo)
    {
	loop = jo->getBoolean("loop", false);
        delay = jo->getInteger("delay", 250);
        sound = jo->getString("sound");
        sprites = GameTheme::jsonSprites(*jo, "sprites");

	Point offset = JsonUnpack::point(*jo, "offset", Point(0, 0));
	if(offset != Point(0, 0))
	    for(auto it = sprites.begin(); it != sprites.end(); ++it)
		(*it).setPosition(offset);
    }
}

void SpritesAnimation::setEnabled(bool f)
{
    if(sprites.size())
    {
	if(0 == tt.latest)
	    tt.latest = Tools::ticks() - Display::timeStart();

	index = f ? 0 : -1;
    }
}

void SpritesAnimation::setPosition(const Point & offset)
{
    for(auto it = sprites.begin(); it != sprites.end(); ++it)
	(*it).setPosition(offset);
}

void SpritesAnimation::render(Window & win) const
{
    if(sprites.size())
    {
	const Sprite & sp = sprites[index < 0 ? 0 : index % sprites.size()];
	win.renderTexture(sp, sp.position());
    }
}

void SpritesAnimation::renderCentered(Window & win, const Point & center) const
{
    if(sprites.size())
    {
	const Sprite & sp = sprites[index < 0 ? 0 : index % sprites.size()];
	win.renderTexture(sp, center - sp.size() / 2);
    }
}

Size SpritesAnimation::spriteSize(void) const
{
    return sprites.size() ? sprites[0].size() : Size(0, 0);
}

void SpritesAnimation::next(void)
{
    if(sprites.size())
    {
#ifndef SWE_DISABLE_AUDIO
	if(0 == index && Settings::sound() && sound.size())
	{
	    Sound::play(GameTheme::sound(sound));
	}
#endif

	index = index + 1 < sprites.size() ? index + 1 : -1;
	if(loop && 0 > index) index = 0;
    }
}

bool SpritesAnimation::next(u32 ms)
{
    if(tt.check(ms, delay))
    {
	next();
	return true;
    }

    return false;
}

void ButtonInfo::setJsonInfo(const JsonObject* jo)
{
    if(jo)
    {
	ids = jo->getString("id", ids);
	action2 = jo->getString("action2", action2);

        const JsonValue* js = jo->getValue("sprites");
        if(js)
        {
            Sprites sprites = GameTheme::jsonSprites(*js);

            if(0 < sprites.size() && sprites[0].isValid()) sprite1 = sprites[0];
            if(1 < sprites.size() && sprites[1].isValid()) sprite2 = sprites[1];
            if(2 < sprites.size() && sprites[2].isValid()) sprite3 = sprites[2];
            if(3 < sprites.size() && sprites[3].isValid()) sprite4 = sprites[3];
        }

        if(jo->hasKey("cursors"))
        {
            StringList cursors = jo->getStdList<std::string>("cursors");
            if(0 < cursors.size()) cursorFocusTheme = cursors.front();
            if(1 < cursors.size()) cursorPressTheme = cursors.back();
        }
    }
}

JsonObject ButtonInfo::toJsonInfo(void) const
{
    JsonObject jo;
    jo.addString("id", ids);
    jo.addString("action2", action2);
    jo.addArray("cursors", JsonArray({cursorFocusTheme, cursorPressTheme}));
    return jo;
}

JsonButton::JsonButton(const JsonObject & jobject, const char* id, int action, Window* win) : WindowButton(win)
{
    const JsonObject* jo = id ? jobject.getObject(id) : &jobject;
    if(jo)
    {
	ButtonInfo::setJsonInfo(jo);
	setSize(sprite1.size() < sprite2.size() ? sprite2.size() : sprite1.size());

        if(jo->hasKey("size"))
            setSize(JsonUnpack::size(*jo, "size"));

        if(jo->hasKey("position"))
            setPosition(JsonUnpack::point(*jo, "position"));

        if(jo->hasKey("hotkey"))
            setHotKey(Key::toKey(jo->getString("hotkey")));

        if(jo->hasKey("visible"))
            setVisible(jo->getBoolean("visible"));

        if(jo->hasKey("tooltip"))
	    setToolTip(jo->getString("tooltip"));

	if(0 < action)
	    setAction(action);
    }
}

void JsonButton::mouseFocusEvent(void)
{
    WindowButton::mouseFocusEvent();

    if(! cursorFocusTheme.empty())
    {
        JsonWindow* jbs = static_cast<JsonWindow*>(parent());
        if(jbs) jbs->cursorSet(cursorFocusTheme);
    }
}

void JsonButton::mouseLeaveEvent(void)
{
    WindowButton::mouseLeaveEvent();

    if(! cursorFocusTheme.empty() || !cursorPressTheme.empty())
    {
        JsonWindow* jbs = static_cast<JsonWindow*>(parent());
        if(jbs) jbs->cursorReset();
    }
}

JsonObject JsonButton::toJsonInfo(void) const
{
    JsonObject jo = ButtonInfo::toJsonInfo();

    jo.addBoolean("visible", isVisible());
    jo.addBoolean("disabled", isDisabled());
    jo.addBoolean("pressed", isPressed());

    return jo;
}

void JsonButton::setJsonInfo(const JsonObject* jo)
{
    if(jo)
    {
	ButtonInfo::setJsonInfo(jo);

	bool pressed = jo->getBoolean("pressed", isPressed());
	bool disabled = jo->getBoolean("disabled", isDisabled());
	bool visible = jo->getBoolean("visible", isVisible());

        if(pressed != isPressed()) setPressed(pressed);
	if(disabled != isDisabled()) setDisabled(disabled);
	if(visible != isVisible()) setVisible(visible);
    }
}

void JsonButtons::setVisible(bool f)
{
    std::for_each(begin(), end(), [=](JsonButton & btn){ btn.setVisible(f); });
}

/*
void JsonButtons::setCentered(const Rect & pos)
{
    if(size())
    {
        float space = (pos.w - front().width() * size()) / static_cast<float>(size());
        float offsetx = pos.x + space / 2;

        for(auto it = begin(); it != end(); ++it)
        {
            (*it).setPosition(Point(offsetx, pos.y));
            offsetx += (*it).width() + space;
        }
    }
}
*/

JsonButton* JsonButtons::findIds(const std::string & str)
{
    auto it = std::find_if(begin(), end(),
	    [&](const JsonButton & btn){ return btn.isIds(str); });
    return it != end() ? & (*it) : nullptr;
}

JsonButton* JsonButtons::findAction(const std::string & str)
{
    auto it = std::find_if(begin(), end(),
	    [&](const JsonButton & btn){ return btn.isAction(str); });
    return it != end() ? & (*it) : nullptr;
}

namespace
{
#ifndef SWE_DISABLE_AUDIO
    StringList  tracks;
#endif
}

JsonWindow::JsonWindow(const std::string & res, Window* win) : Window(win), defaultCursor("default"), backColor(Color::transparent())
{
    jobject = GameTheme::jsonResource(res).toObject();

    setState(FlagModality);
    setPosition(JsonUnpack::point(jobject, "position", Point(0, 0)));
    setSize(JsonUnpack::size(jobject, "size", Display::size()));
    const JsonValue* jv = nullptr;

    if(jobject.isValid())
    {
	Sprite spBack;

	jv = jobject.getValue("background");
	if(jv)
	{
	    if(jv->isObject())
		spBack = GameTheme::jsonSprite(*jv);
	    else
	    if(jv->isString())
	    {
		std::string val = jv->getString();

		if(val.substr(0, 4) == "res:")
		    spBack = GameTheme::jsonSprite(*jv);
		else
		    backColor = jv->getString();
	    }
	}

	if(size().isEmpty())
	    ERROR("window size: empty!");

	if(spBack.isValid())
	{
	    setSize(spBack.size());
	    sprites.push_back(spBack);
	}

	jv = jobject.getValue("sprites");
	if(jv) sprites = GameTheme::jsonSprites(*jv);

	jv = jobject.getValue("buttons");
	if(jv)
	{
	    const JsonArray* ja = static_cast<const JsonArray*>(jv);
	    for(int it = 0; it < ja->size(); ++it)
	    {
		const JsonObject* jobut = ja->getObject(it);
		if(jobut)
		    buttons.emplace_back(*jobut, nullptr, 0, this);
	    }

	    if(jobject.getBoolean("buttons_visible", true))
		buttons.setVisible(true);
	}

	if(jobject.hasKey("cursor"))
	    defaultCursor = jobject.getString("cursor");

	if(Settings::music() && jobject.hasKey("music"))
	{
#ifndef SWE_DISABLE_AUDIO
	    tracks = jobject.getStdList<std::string>("music");
            auto mus = Tools::random_n(tracks.begin(), tracks.end());

	    if(mus != tracks.end())
	    {
		playMusic(*mus);
		Music::setHookFinished(playRandomMusic);
	    }
#endif
	}
    }
    else
    {
	renderColor(Color::Silver, area());
	renderText(systemFont(), StringFormat("check resource: %1").arg(res), Color::Black, size() / 2, AlignCenter, AlignCenter, true);
    }

    for(auto & btn : buttons)
    {
        signalSubscribe(btn, Signal::ButtonPressed);
        signalSubscribe(btn, Signal::ButtonClicked);
    }
}

void JsonWindow::renderWindow(void)
{
    if(! backColor.isTransparent())
	renderClear(backColor);

    for(auto & sprite : sprites)
	renderTexture(sprite);
}

void JsonWindow::signalReceive(int sig, const SignalMember* sm)
{
    if(sig == Signal::ButtonPressed)
    {
	auto btn = reinterpret_cast<const JsonButton*>(sm);

        if(btn && ! btn->themePress().empty())
	    cursorSet(btn->themePress());
    }
}

void JsonWindow::cursorSet(const std::string & theme)
{
    //GameTheme::setCursor(theme);
}

void JsonWindow::cursorReset(void)
{
    //GameTheme::setCursor(defaultCursor);
}

void JsonWindow::playSound(const std::string & name)
{
#ifndef SWE_DISABLE_AUDIO
    if(Settings::sound() && name.size())
    {
        Sound::play(GameTheme::sound(name));
    }
#endif
}

void JsonWindow::playSoundWait(void)
{
#ifndef SWE_DISABLE_AUDIO
    while(Sound::isPlaying()) Tools::delay(50);
#endif
}

void JsonWindow::playMusic(const std::string & name)
{
#ifndef SWE_DISABLE_AUDIO
    if(Settings::music() && name.size())
    {
        auto & buf = GameTheme::music(name);
        if(! Music::isPlaying(buf.crc32b()))
            Music::play(buf);
    }
#endif
}

void JsonWindow::playRandomMusic(void)
{
    auto mus = Tools::random_n(tracks.begin(), tracks.end());
    if(mus != tracks.end()) playMusic(*mus);
}

Rect JsonWindow::renderTextInfo(const JsonTextInfo & info)
{
    return renderTextInfo(info, info.text, info.position, info.align());
}

Rect JsonWindow::renderTextInfo(const JsonTextInfo & info, const std::string & text)
{
    return renderTextInfo(info, text, info.position, info.align());
}

Rect JsonWindow::renderTextInfo(const JsonTextInfo & info, const std::string & text, const Point & pos, const AlignType & align)
{
    return renderText(GameTheme::fontRender(info.font), text, info.color, pos, align);
}

AlignValue::AlignValue(const std::string & str, const AlignType & def) : value(def)
{
    std::string lower = String::toLower(str);

    if(lower == "left") value = AlignLeft;
    else
    if(lower == "right") value = AlignRight;
    else
    if(lower == "top") value = AlignTop;
    else
    if(lower == "bottom") value = AlignBottom;
    else
    if(lower == "center") value = AlignCenter;
}
