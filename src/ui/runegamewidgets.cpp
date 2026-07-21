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
 ***************************************************************************/

#include "runegamewidgets.h"

StoneSprite::StoneSprite(const Stone & value, int size, const Point & pos) : Stone(value)
{
    set(value, size);
    setPosition(pos);
}

StoneSprite::StoneSprite(const Stone & value, int size) : Stone(value)
{
    set(value, size);
}

void StoneSprite::set(const Stone & value, int size)
{
    const StoneInfo & info = GameData::stoneInfo(value);
    Stone::set(value);

    switch(size)
    {
        case Small: setTexture(GameTheme::texture(info.small)); break;
        case Medium: setTexture(GameTheme::texture(info.medium)); break;
        case Large: setTexture(GameTheme::texture(info.large)); break;
        default: break;
    }
}

LuckDrawDialog::LuckDrawDialog(const VecStones & choices, Window & win)
    : DialogWindow("dialog_luckdraw.json", win),
      first(choices[0], StoneSprite::Large, GameTheme::jsonPoint(jobject, "offset:left")),
      second(choices[1], StoneSprite::Large, GameTheme::jsonPoint(jobject, "offset:right")),
      selectedColor(GameTheme::jsonColor(jobject, "color:selected")),
      hintPos(GameTheme::jsonPoint(jobject, "offset:hint")), selected(0)
{
    setResultCode(-1);
    setVisible(true);
}

void LuckDrawDialog::choose(int index)
{
    setResultCode(index);
    actionDialogClose();
}

bool LuckDrawDialog::mouseClickEvent(const ButtonsEvent & coords)
{
    if(!coords.isButtonLeft()) return true;

    if(coords.isClick(first.area()))
        choose(0);
    else if(coords.isClick(second.area()))
        choose(1);

    return true;
}

bool LuckDrawDialog::keyPressEvent(const KeySym & key)
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

void LuckDrawDialog::renderWindow(void)
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
