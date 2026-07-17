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

#include "gametheme.h"
#include "runewars.h"
#include "settings.h"

namespace
{
    bool gameMusic = true;
    bool gameSound = true;
    bool gameAccel = true;
    bool gameFullscreen = false;
    bool guardianRulesSound = true;
    std::string lang;
    std::string speed = "classic";

    std::string normalizedLanguage(const std::string & value)
    {
	const std::string lower = String::toLower(value);
	if(lower == "ru" || lower == "russian" || lower.rfind("ru_", 0) == 0 ||
	   lower.rfind("ru-", 0) == 0 || lower.rfind("russian_", 0) == 0)
	    return "ru";
	return "en";
    }

    std::string normalizedGameSpeed(const std::string & value)
    {
	const std::string lower = String::toLower(value);
	if(lower == "fast") return "fast";
	if(lower == "normal") return "normal";
	return "classic";
    }

    std::string userSettingsFile(void)
    {
	if(const char* overrideFile = Systems::environment("FOUR_WINDS_SETTINGS_FILE"))
	    return overrideFile;
	return Settings::fileSave("settings.json");
    }
}

bool Settings::read(void)
{
    JsonObject jo = GameTheme::jsonResource("config.json").toObject();

    if(jo.isValid())
    {
	gameMusic = jo.getBoolean("music", true);
	gameSound = jo.getBoolean("sound", true);
	gameFullscreen = jo.getBoolean("display:fullscreen", false);
	gameAccel = jo.getBoolean("display:accel", true);

	guardianRulesSound = jo.getBoolean("sound:guardianrules", true);
	lang = normalizedLanguage(jo.getString("language", Systems::messageLocale(1)));
	speed = normalizedGameSpeed(jo.getString("game:speed", "classic"));
    }

    JsonObject user = JsonContentFile(userSettingsFile()).toObject();
    if(user.isValid())
    {
	gameMusic = user.getBoolean("music", gameMusic);
	gameSound = user.getBoolean("sound", gameSound);
	gameFullscreen = user.getBoolean("display:fullscreen", gameFullscreen);
	guardianRulesSound = user.getBoolean("sound:guardianrules", guardianRulesSound);
	lang = normalizedLanguage(user.getString("language", lang));
	speed = normalizedGameSpeed(user.getString("game:speed", speed));
    }

    return true;
}

bool Settings::write(std::string* error)
{
    const std::string file = userSettingsFile();
    const std::string directory = Systems::dirname(file);
    if(!directory.empty() && !Systems::isDirectory(directory) && !Systems::makeDirectory(directory))
    {
	if(error) *error = "unable to create the settings directory";
	return false;
    }

    JsonObject jo;
    jo.addString("language", lang);
    jo.addBoolean("music", gameMusic);
    jo.addBoolean("sound", gameSound);
    jo.addBoolean("display:fullscreen", gameFullscreen);
    jo.addBoolean("sound:guardianrules", guardianRulesSound);
    jo.addString("game:speed", speed);

    if(!Systems::saveString2File(jo.toString(), file))
    {
	if(error) *error = "unable to write settings.json";
	return false;
    }

    if(error) error->clear();
    return true;
}

std::string Settings::language(void)
{
    return lang;
}

void Settings::setLanguage(const std::string & value)
{
    lang = normalizedLanguage(value);
}

std::string Settings::gameSpeed(void)
{
    return speed;
}

void Settings::setGameSpeed(const std::string & value)
{
    speed = normalizedGameSpeed(value);
}

int Settings::presentationDelay(int milliseconds)
{
    if(milliseconds <= 0) return milliseconds;

    // Classic deliberately restores the more deliberate 1990s presentation.
    // Normal preserves the New Age timing, while Fast is intended for repeat play.
    const int scaled = speed == "classic" ? (milliseconds * 7 + 2) / 4 :
                       speed == "fast" ? (milliseconds * 11 + 10) / 20 : milliseconds;
    return 0 < scaled ? scaled : 1;
}

bool Settings::fullscreen(void)
{
    return gameFullscreen;
}

void Settings::setFullscreen(bool value)
{
    gameFullscreen = value;
}

bool Settings::accel(void)
{
    return gameAccel;
}

bool Settings::music(void)
{
    return gameMusic;
}

void Settings::setMusic(bool value)
{
    gameMusic = value;
}

bool Settings::sound(void)
{
    return gameSound;
}

void Settings::setSound(bool value)
{
    gameSound = value;
}

bool Settings::soundGuardianRules(void)
{
    return guardianRulesSound;
}

void Settings::setSoundGuardianRules(bool value)
{
    guardianRulesSound = value;
}

//////////////////////////////////////////////////////
std::string Settings::fileSaveGame(void)
{
    return fileSave("game.sav");
}

std::string Settings::shareDir(void)
{
    return Systems::homeDirectory(Application::domain());
}

std::string Settings::fileSave(const std::string & file)
{
    return Systems::concatePath(Systems::homeDirectory(Application::domain()), file);
}

bool Settings::storeCache(void)
{
    return Systems::environment("RUNEWARS_STORE_CACHE");
}
