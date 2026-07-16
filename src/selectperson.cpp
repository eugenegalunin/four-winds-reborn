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
#include "runewars.h"
#include "dialogs.h"
#include "actions.h"
#include "selectperson.h"

AvatarItem::AvatarItem(const Avatar & ava, ListWidget & win) : ListWidgetItem(win), id(ava)
{
    setVisible(true);
}

void AvatarItem::setDisabled(bool f)
{
    setState(FlagDisabled, f);
}

void AvatarItem::renderWindow(void)
{
}


Avatars RevertPersons(const Avatars & allowPersons)
{
    Avatars res;

    // set difference
    for(auto & id : avatars_all)
    {
	if(std::none_of(allowPersons.begin(), allowPersons.end(), [=](const Avatar & ava){ return ava.id() == id; }))
	    res.emplace_back(id);
    }

    return res;
}

Clans RevertClans(const Clans & allowClans)
{
    Clans res;

    // set difference
    for(auto & id : clans_all)
    {
	if(std::none_of(allowClans.begin(), allowClans.end(), [=](const Clan & clan){ return clan.id() == id; }))
	    res.emplace_back(id);
    }

    return res;
}

SelectPersonScreen::SelectPersonScreen() : JsonWindow("screen_selectperson.json", nullptr),
    buttonOk(nullptr), buttonClose(nullptr), selectedAvatar(Avatar::Random),
    difficulty(GameData::aiDifficulty())
{
    buttonOk = buttons.findIds("but_ok");
    buttonClose = buttons.findIds("but_close");

    if(buttonOk) buttonOk->setAction(Action::ButtonOk);
    if(buttonClose) buttonClose->setAction(Action::ButtonClose);

    marker = GameTheme::jsonSprite(jobject, "sprite:marker");
    const JsonObject* jo = nullptr;

    jo = jobject.getObject("icon:disabled");
    if(jo)
    {
	Color col  = jo->getString("color");
	Size sz = JsonUnpack::size(*jo, "size");
	int space = jo->getInteger("space");

	gray = Display::renderRect(col, Color::Transparent, sz);

	for(int xx = 0; xx < sz.w; xx += space)
	    Display::renderLine(col, gray, Point(xx, 0), Point(xx, gray.height() - 1));
	for(int yy = 0; yy < sz.h; yy += space)

	Display::renderLine(col, gray, Point(0, yy), Point(gray.width() - 1, yy));
    }

    jo = jobject.getObject("person:selected");
    if(jo)
    {
        textPos = GameTheme::jsonPoint(*jo, "offset:info");
        personImagePos = GameTheme::jsonPoint(*jo, "offset:image");
        creaturesPos = GameTheme::jsonPoint(*jo, "offset:creatures");

        personName1 = GameTheme::jsonTextInfo(*jo, "textinfo:name1");
        personName2 = GameTheme::jsonTextInfo(*jo, "textinfo:name2");
        personDescription = GameTheme::jsonTextInfo(*jo, "textinfo:description");
        personClan = GameTheme::jsonTextInfo(*jo, "textinfo:clan");
        personClans = GameTheme::jsonTextInfo(*jo, "textinfo:clans");
        spellText = GameTheme::jsonTextInfo(*jo, "textinfo:spell");

        supplementialSpells = personClans;
        specialAbilities = personClans;
    }

    jo = jobject.getObject("ai:difficulty");
    if(jo)
    {
        difficultyArea = JsonUnpack::rect(*jo, "area");
        difficultyValue = GameTheme::jsonTextInfo(*jo, "textinfo:value");
        difficultyBackground = GameTheme::jsonColor(*jo, "color:background");
        difficultyBorder = GameTheme::jsonColor(*jo, "color:border");
        difficultyInnerBorder = GameTheme::jsonColor(*jo, "color:inner_border");
    }

    const JsonArray* ja = nullptr;

    ja = jobject.getArray("area:clans");
    if(ja)
    {
	for(int ii = 0; ii < ja->size(); ++ii)
	{
	    const JsonObject* jo = ja->getObject(ii);
	    if(jo)
	    {
		const ClanInfo & ci = GameData::clanInfo(jo->getString("id"));
		const Rect & area = JsonUnpack::rect(*jo, "area");
		clansIcon.emplace_back(ci, marker, gray, area.toPoint(), *this);
	    }
	}
    }

    ja = jobject.getArray("area:avatars");
    if(ja)
    {
	for(int ii = 0; ii < ja->size(); ++ii)
	{
	    const JsonObject* jo = ja->getObject(ii);
	    if(jo)
	    {
		const AvatarInfo & av = GameData::avatarInfo(jo->getString("id"));
		const Rect & area = JsonUnpack::rect(*jo, "area");
		avatarsIcon.emplace_back(av, marker, gray, area.toPoint(), *this);
	    }
	}
    }

    setVisible(true);
}

void SelectPersonScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    if(difficultyArea.w && difficultyArea.h)
    {
        renderRect(difficultyBackground, difficultyArea);
        renderLine(difficultyBorder, difficultyArea.toPoint(),
                   Point(difficultyArea.x + difficultyArea.w - 1, difficultyArea.y));
        renderLine(difficultyBorder, Point(difficultyArea.x, difficultyArea.y + difficultyArea.h - 1),
                   Point(difficultyArea.x + difficultyArea.w - 1,
                         difficultyArea.y + difficultyArea.h - 1));
        renderLine(difficultyBorder, difficultyArea.toPoint(),
                   Point(difficultyArea.x, difficultyArea.y + difficultyArea.h - 1));
        renderLine(difficultyBorder,
                   Point(difficultyArea.x + difficultyArea.w - 1, difficultyArea.y),
                   Point(difficultyArea.x + difficultyArea.w - 1,
                         difficultyArea.y + difficultyArea.h - 1));

        const Rect inner(difficultyArea.x + 3, difficultyArea.y + 3,
                         difficultyArea.w - 6, difficultyArea.h - 6);
        renderLine(difficultyInnerBorder, inner.toPoint(),
                   Point(inner.x + inner.w - 1, inner.y));
        renderLine(difficultyInnerBorder, Point(inner.x, inner.y + inner.h - 1),
                   Point(inner.x + inner.w - 1, inner.y + inner.h - 1));
        renderLine(difficultyInnerBorder, inner.toPoint(),
                   Point(inner.x, inner.y + inner.h - 1));
        renderLine(difficultyInnerBorder, Point(inner.x + inner.w - 1, inner.y),
                   Point(inner.x + inner.w - 1, inner.y + inner.h - 1));

        const char* value = _("Normal");
        if(difficulty == AI::Difficulty::Easy) value = _("Easy");
        else if(difficulty == AI::Difficulty::Hard) value = _("Hard");
        renderTextInfo(difficultyValue, value);
    }

    if(selectedAvatar.isValid())
    {
	const AvatarInfo & avatarInfo = GameData::avatarInfo(selectedAvatar);

	renderTexture(GameTheme::texture(avatarInfo.image), personImagePos);

	renderTextInfo(personName1, avatarInfo.name);
	renderTextInfo(personName2, avatarInfo.name);
	renderTextInfo(personDescription, avatarInfo.dignity);
	renderTextInfo(personClans, avatarInfo.toStringClans());
	renderTextInfo(personClan, selectedClan.isValid() ? GameData::clanInfo(selectedClan).name : _("Random"));

	// creatures
	Point drawPos = creaturesPos;
	int icon_h = 0;
	bool creaturesUpdate = creaturesIcon.empty();

	for(auto it = avatarInfo.creatures.begin(); it != avatarInfo.creatures.end(); ++it)
        {
	    const CreatureInfo & creatureInfo = GameData::creatureInfo(*it);
            Texture icon = GameTheme::texture(creatureInfo.image2);

	    if(creaturesUpdate)
	    {
		creaturesIcon.emplace_back(creatureInfo, Rect(drawPos, icon.size()), *this);
	    }

            drawPos.x += icon.width();
            icon_h = icon.height();
            if(5 == std::distance(avatarInfo.creatures.begin(), it) && avatarInfo.creatures.size() > 6)
            {
                drawPos.x = creaturesPos.x;
                drawPos.y = creaturesPos.y + icon_h;
            }
            else
            if(11 == std::distance(avatarInfo.creatures.begin(), it) && avatarInfo.creatures.size() > 12)
            {
                drawPos.x = creaturesPos.x;
		drawPos.y = creaturesPos.y + 2 * icon_h;
	    }
	}

	// spells
	drawPos.x = textPos.x;
	drawPos.y = drawPos.y + icon_h + 5;
	renderTextInfo(supplementialSpells, _("Supplemential Spells:"), drawPos);

	drawPos.y = drawPos.y + 40;
	bool spellsUpdate = spellsIcon.empty();

	for(auto it = avatarInfo.spells.begin(); it != avatarInfo.spells.end(); ++it)
	{
	    const SpellInfo & spellInfo = GameData::spellInfo(*it);
	    drawPos.x = std::distance(avatarInfo.spells.begin(), it) % 2 ? textPos.x + 210 : textPos.x + 20;
	    Texture icon = GameTheme::texture(spellInfo.image);

	    Rect rtClick(drawPos.x, drawPos.y, icon.width() + 3, icon.height());
	    drawPos.x = drawPos.x + icon.width() + 3;
	    icon_h = icon.height();

	    const Rect & rt = renderTextInfo(spellText, spellInfo.name, drawPos);
	    rtClick.w += rt.w;

	    if(spellsUpdate)
		spellsIcon.emplace_back(spellInfo, rtClick, *this);

	    if(std::distance(avatarInfo.spells.begin(), it) % 2 &&
		std::distance(avatarInfo.spells.begin(), it) < avatarInfo.spells.size() - 1)
		drawPos.y = drawPos.y + icon.height() + 3;
	}

	// ability
	drawPos.x = textPos.x;
	drawPos.y = drawPos.y + icon_h + 5;
	renderTextInfo(specialAbilities, _("Special Abilities:"), drawPos);

	if(avatarInfo.ability())
	{
	    const AbilityInfo & ability = GameData::abilityInfo(avatarInfo.ability);
	    drawPos.x = textPos.x + 10;
	    drawPos.y = drawPos.y + 30;

	    const FontRender & fontRender = GameTheme::fontRender(personClans.font);
	    for(auto & ustr : fontRender.splitStringWidth(String::replace(ability.description, "%1", avatarInfo.name), 410))
	    {
		renderText(fontRender, ustr, personClans.color, drawPos);
		drawPos.y += fontRender.lineSkipHeight();
	    }
	}
    }
}

bool SelectPersonScreen::mouseClickEvent(const ButtonsEvent & coords)
{

    if(difficultyArea.w && coords.isClick(difficultyArea))
    {
        difficulty = AI::nextDifficulty(difficulty);
        playSound("button");
        renderWindow();
        return true;
    }

    if(actionClickPersons(coords))
    {
	renderWindow();
	return true;
    }

    if(actionClickClans(coords))
    {
	renderWindow();
	return true;
    }

    if(selectedAvatar.isValid() &&
	selectedAvatar() != Avatar::Random)
    {
	const AvatarInfo & avatarInfo = GameData::avatarInfo(selectedAvatar);

	if(coords.isClick(Rect(personImagePos, GameTheme::texture(avatarInfo.image).size())))
	{
	    AvatarInfoDialog(avatarInfo, *this).exec();
	    return true;
	}
    }

    return false;
}

bool SelectPersonScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
	case Key::ESCAPE: actionButtonClose(); return true;
	default: break;
    }

    return false;
}

bool SelectPersonScreen::userEvent(int act, void* data)
{
    switch(act)
    {
	case Action::ButtonOk:		return actionButtonOk();
	case Action::ButtonClose:	return actionButtonClose();

	default: break;
    }

    return false;
}

bool SelectPersonScreen::actionButtonOk(void)
{
    playSound("button");

    if(selectedAvatar() == Avatar::Random)
	selectedAvatar = Avatar();

    setResultCode(Menu::ShowPlayers);
    setVisible(false);

    return true;
}

Person SelectPersonScreen::selectedPerson(void) const
{
    return Person(selectedAvatar, selectedClan, Wind());
}

bool SelectPersonScreen::actionButtonClose(void)
{
    playSound("button");
    setResultCode(Menu::GameExit);
    setVisible(false);

    return true;
}

bool SelectPersonScreen::actionClickPersons(const ButtonsEvent & coords)
{
    auto ita = avatarsIcon.begin();

    for(; ita != avatarsIcon.end(); ++ita)
	if(coords.isClick((*ita).area())) break;

    if(ita == avatarsIcon.end() ||
	    (*ita).isDisabled()) return false;

    const Avatar & clickedAvatar = (*ita).toAvatar();

    if(selectedAvatar == clickedAvatar)
	return true;

    // reset selected
    for(auto & icon : avatarsIcon)
	icon.setSelected(false);

    (*ita).setSelected(true);

    selectedAvatar = clickedAvatar;
    const AvatarInfo & avatarInfo = GameData::avatarInfo(selectedAvatar);

    // reset clans
    for(auto & icon : clansIcon)
	icon.setDisabled(false);

    creaturesIcon.clear();
    spellsIcon.clear();

    if(selectedAvatar() == Avatar::Random)
    {
	playSound("button");
	return true;
    }
    else
    {
	// set disabled clans
	auto disabledClans = RevertClans(avatarInfo.clans);

	for(auto & icon : clansIcon)
	{
	    if(std::any_of(disabledClans.begin(), disabledClans.end(), [&](const Clan & clan){ return clan == icon.toClan(); }))
		icon.setDisabled(true);
	}

	//
	playSound(selectedAvatar.toString().append("_name"));
    }

    return true;
}

bool SelectPersonScreen::actionClickClans(const ButtonsEvent & coords)
{
    auto itc = clansIcon.begin();

    for(; itc != clansIcon.end(); ++itc)
	if(coords.isClick((*itc).area())) break;

    if(itc == clansIcon.end() ||
	(*itc).isDisabled()) return false;

    const Clan & clickedClan = (*itc).toClan();

    if(selectedClan == clickedClan)
	return true;

    // reset selected
    for(auto & icon : clansIcon)
	icon.setSelected(false);

    (*itc).setSelected(true);

    selectedClan = clickedClan;

    // set disabled avatars
    for(auto & icon : avatarsIcon)
	icon.setDisabled(false);

    auto disabledAvatars = RevertPersons(GameData::avatarsOfClan(selectedClan));

    for(auto & icon : avatarsIcon)
    {
        if(disabledAvatars.end() != std::find(disabledAvatars.begin(), disabledAvatars.end(), icon.toAvatar()))
	    icon.setDisabled(true);
    }

    //
    playSound(selectedClan.toString().append("_name"));

    return true;
}
