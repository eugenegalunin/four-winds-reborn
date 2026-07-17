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

#ifndef _RWNA_GAMETHEME_
#define _RWNA_GAMETHEME_

#include "jsongui.h"
#include "gamedata.h"

struct Application;

namespace GameTheme
{
    bool		init(const Application &);
    void		clear(void);

    const std::string & name(void);
    const std::string & author(void);
    const Size &	size(void);

    const BinaryBuf &   readResource(const std::string &, std::string* res = nullptr);
    bool                findResource(const std::string &, std::string* res = nullptr);
    JsonContent         jsonResource(const std::string &);

    const FontRender &	fontRender(const std::string &);

    const BinaryBuf &	sound(const std::string &);
    const BinaryBuf &	music(const std::string &);
    std::string         localizedSoundFile(const FileInfo &, const std::string &);
    Texture		texture(const std::string &);
    Sprite		sprite(const std::string &);

    SidesPositions	jsonSidesPositions(const JsonObject &, const std::string &);

    Point		jsonPoint(const JsonObject &, const std::string &);
    Size		jsonSize(const JsonObject &, const std::string &);
    Rect		jsonRect(const JsonObject &, const std::string &);
    Color		jsonColor(const JsonObject &, const std::string &);

    Sprite		jsonSprite(const JsonObject &, const std::string &);
    Sprites		jsonSprites(const JsonObject &, const std::string &);

    JsonButton          jsonButton(const JsonObject &, const std::string &);
    JsonButtons         jsonButtons(const JsonObject &, const std::string &);

    JsonTextColor       jsonTextColor(const JsonObject &, const std::string &);
    JsonAlignPosition	jsonAlignPosition(const JsonObject &, const std::string &);
    JsonTextInfo        jsonTextInfo(const JsonObject &, const std::string &);

    const JsonToolTip & jsonToolTipInfo(void);

    SidesPositions	jsonSidesPositions(const JsonValue &);

    Sprite		jsonSprite(const JsonValue &);
    Sprites		jsonSprites(const JsonValue &);
    JsonButton          jsonButton(const JsonValue &);
    JsonButtons         jsonButtons(const JsonValue &);

    JsonTextColor       jsonTextColor(const JsonValue &);
    JsonAlignPosition	jsonAlignPosition(const JsonValue &);
    JsonTextInfo        jsonTextInfo(const JsonValue &);
};

const JsonValue & operator>> (const JsonValue &, ImageInfo &);
const JsonValue & operator>> (const JsonValue &, FontInfo &);
const JsonValue & operator>> (const JsonValue &, FileInfo &);

const JsonValue & operator>> (const JsonValue &, StoneInfo &);
const JsonValue & operator>> (const JsonValue &, WindInfo &);
const JsonValue & operator>> (const JsonValue &, ClanInfo &);
const JsonValue & operator>> (const JsonValue &, CreatureInfo &);
const JsonValue & operator>> (const JsonValue &, SpellInfo &);
const JsonValue & operator>> (const JsonValue &, SpecialityInfo &);
const JsonValue & operator>> (const JsonValue &, AbilityInfo &);
const JsonValue & operator>> (const JsonValue &, AvatarInfo &);
const JsonValue & operator>> (const JsonValue &, LandInfo &);

const JsonValue & operator>> (const JsonValue &, Sprite &);
const JsonValue & operator>> (const JsonValue &, Sprites &);

const JsonValue & operator>> (const JsonValue &, JsonButton &);
const JsonValue & operator>> (const JsonValue &, JsonButtons &);

#endif
