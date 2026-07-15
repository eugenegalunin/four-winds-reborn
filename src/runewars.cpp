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

#include <random>
#include <clocale>
#include <algorithm>

#include "gametheme.h"
#include "settings.h"
#include "selectperson.h"
#include "showplayers.h"
#include "mahjongpart.h"
#include "mahjongsummarypart.h"
#include "adventurepart.h"
#include "battlesummarypart.h"
#include "gamesummarypart.h"

#include "runewars.h"

class GameLoadScreen : public DisplayWindow
{
    const std::string & savefile;

protected:
    void windowCreateEvent(void) override
    {
        if(MessageBox(Application::name(), _("Continue with the last saved game?"), *this).exec() &&
            GameData::loadGame(savefile))
        {
            MessageTop(_("Info"), _("Game data rendering..."), *this);
	    setResultCode(1);
        }
        else
        {
            Systems::remove(savefile);

            std::string screenshot = Systems::concatePath(Systems::dirname(savefile), "game.png");
            if(Systems::isFile(screenshot)) Systems::remove(screenshot);
	    setResultCode(0);
        }

	// close screen
	setVisible(false);
    }

public:
    GameLoadScreen(const std::string & file) : DisplayWindow(Color::Black), savefile(file) {}
};

std::string Application::domain(void)
{
    return "four-winds-reborn";
}

std::string Application::name(void)
{
    return "Four Winds Reborn";
}

std::string Application::version(void)
{
    std::string version;
#ifdef BUILDDATE
    version.append(BUILDDATE);
 #ifdef SRCREVISION
    version.append(".").append(SRCREVISION);
 #endif
#endif
    return version;
}

RuneWarsClient::RuneWarsClient(int argc, char** argv) : Application(argv[0], false, Size(0, 0), "default"), part(0)
{
    LogWrapper::init(domain(), argv[0]);

    // make params theme
#ifdef RUNEWARS_THEME
    theme = RUNEWARS_THEME;
#endif
    if(Systems::environment("RUNEWARS_THEME"))
    	theme = Systems::environment("RUNEWARS_THEME");

    savefile = Settings::fileSaveGame();
    parseCommandOptions(argc, argv);
}

void RuneWarsClient::parseCommandOptions(int argc, char** argv)
{
    int opt;

    while((opt = Systems::GetCommandOptions(argc, argv, "hft:g:p:s:")) != -1)
    switch(opt)
    {
	case 'f':
	    fullscreen = true;
	    break;

        case 't':
	    if(Systems::GetOptionsArgument())
        	theme = Systems::GetOptionsArgument();
            break;
#ifdef BUILD_DEBUG
        case 'p':
	    if(Systems::GetOptionsArgument())
		part = String::toInt(Systems::GetOptionsArgument());
	    break;
#endif

        case 's':
	    if(Systems::GetOptionsArgument())
		savefile = Systems::GetOptionsArgument();
	    break;

        case 'g':
	    if(Systems::GetOptionsArgument())
	    {
		StringList list = String::split(Systems::GetOptionsArgument(), 'x');

		if(2 == list.size())
		    geometry = Size(String::toInt(list.front()), String::toInt(list.back()));
		else
		    ERROR("unknown geometry format");
	    }
            break;

        case '?':
        case 'h':
	    COUT("Usage: " << argv[0] << " [OPTIONS]\n" <<
		"\t-f\tfullscreen\n" <<
		"\t-g\twindow geometry (1024x768 is default)\n" <<
		"\t-t\ttheme name\n" <<
		"\t-s\tload savefile\n" <<
		"\t-h\tprint this help and exit\n");
	    exit(0);

        default:  break;
    }
}

bool RuneWarsClient::translationInit(void)
{
    Translation::setStripContext('|');

    std::string lang = StringFormat("%1.mo").arg(Settings::language());
    std::string path;

    if(Translation::bindDomain(domain(), GameTheme::readResource(lang, &path)))
    {
        VERBOSE("loaded from: " << path);
        Translation::setDomain(domain());
        return true;
    }

    return false;
}

Person fixedEmptyPerson(const Person & cur)
{
    // fix random clan or avatar
    if(! cur.clan.isValid() && ! cur.avatar.isValid())
    {
        Avatar avatar = Avatar::random();
        const AvatarInfo & hi = GameData::avatarInfo(avatar);
        auto clan = Tools::random_n(hi.clans.begin(), hi.clans.end());
        return Person(avatar, *clan, cur.wind);
    }
    else
    if(! cur.clan.isValid())
    {
        const AvatarInfo & hi = GameData::avatarInfo(cur.avatar);
        auto clan = Tools::random_n(hi.clans.begin(), hi.clans.end());
        return Person(cur.avatar, *clan, cur.wind);
    }
    else
    if(! cur.avatar.isValid())
    {
        Avatars avatars = GameData::avatarsOfClan(cur.clan);
        std::random_device rd;
        std::mt19937 mtg(rd());
        std::shuffle(avatars.begin(), avatars.end(), mtg);
        return Person(avatars.front(), cur.clan, cur.wind);
    }

    return cur;
}

bool RuneWarsClient::exec(void)
{
    if(! Engine::init())
        return false;

    if(Systems::isEmbeded())
	Display::hardwareCursorHide();

    if(! GameTheme::init(*this))
    {
	Size wsz(640, 480);

        if(Display::init("Error", wsz, wsz, Settings::fullscreen(), Settings::accel(), false))
        {
            DisplayWindow win1(Color::DarkSlateGray);
            TermGUI::MessageBox(_("Error"), _("load theme error, see log for details"),
                                TermGUI::ButtonOk, systemFont(), & win1).exec();
        }
        return false;
    }

    if(! translationInit())
	ERROR("translation not loaded");

    int menu = Systems::isFile(savefile) ? Menu::GameLoadPart : Menu::SelectPerson;
    Person selectedPerson;
    AI::Difficulty selectedDifficulty = AI::Difficulty::Normal;

#ifdef BUILD_DEBUG
    if(part)
    {
	if(! Systems::isFile(savefile))
	{
	    ERROR("savefile not found: " << savefile);
	}
	else
	if(GameData::loadGame(savefile))
	{
            selectedPerson = GameData::myPerson();

	    switch(part)
	    {
		case 1:  GameData::setGamePart(Menu::MahjongPart); break;
		case 2:  GameData::setGamePart(Menu::MahjongSummaryPart); break;
		case 3:  GameData::setGamePart(Menu::AdventurePart); break;
		case 4:  GameData::setGamePart(Menu::BattleSummaryPart); break;
		case 5:  GameData::setGamePart(Menu::GameSummaryPart); break;
		default: break;
	    }

	    switch(part)
	    {
		case 1:  menu = MahjongPartScreen().exec(); break;
		case 2:  menu = MahjongSummaryPartScreen().exec(); break;
		case 3:  menu = AdventurePartScreen(selectedPerson.avatar).exec(); break;
		case 4:  menu = BattleSummaryScreen().exec(); break;
		case 5:  menu = GameSummaryScreen().exec(); break;
		default: break;
	    }
	}
    }
#endif

    while(menu != Menu::GameExit)
    {
	switch(menu)
	{
	    case Menu::SelectPerson:
	    {
		SelectPersonScreen scr;
		menu = scr.exec();
		selectedPerson = fixedEmptyPerson(scr.selectedPerson());
		selectedDifficulty = scr.selectedDifficulty();
	    }
	    break;

	    case Menu::ShowPlayers:
		GameData::setAIDifficulty(selectedDifficulty);
		GameData::initPersons(selectedPerson);
		menu = ShowPlayersScreen().exec();
	    break;

	    case Menu::MahjongInitPart:
		menu = GameData::initMahjong() ? Menu::MahjongPart : Menu::GameSummaryPart;
	    break;

	    case Menu::MahjongPart:
		menu = MahjongPartScreen().exec();
	    break;

	    case Menu::MahjongSummaryPart:
		//GameData::saveGame();
		menu = MahjongSummaryPartScreen().exec();
	    break;

	    case Menu::AdventurePart:
		menu = GameData::initAdventure() ?
			AdventurePartScreen(selectedPerson.avatar).exec() : Menu::GameSummaryPart;
	    break;

	    case Menu::BattleSummaryPart:
//		GameData::saveGame();
		menu = BattleSummaryScreen().exec();
	    break;

	    case Menu::GameSummaryPart:
		menu = GameSummaryScreen().exec();
	    break;

	    case Menu::GameLoadPart:
		if(GameLoadScreen(savefile).exec())
		{
        	    selectedPerson = GameData::myPerson();
        	    menu = GameData::loadedGamePart();
		}
		else
		{
		    menu = Menu::SelectPerson;
		}
	    break;

	    default:
		menu = Menu::GameExit;
	    break;
	}

	Tools::delay(100);
    }

    GameTheme::clear();
    Engine::quit();

    return true;
}

#ifndef RUNEWARS_NO_MAIN
int main(int argc, char **argv)
{
    Systems::setLocale(LC_ALL, "");
    Systems::setLocale(LC_NUMERIC, "C");

    try
    {
	RuneWarsClient client(argc, argv);

	if(! client.exec())
	    return EXIT_FAILURE;
    }
    catch(Engine::exception &)
    {
    }

    return EXIT_SUCCESS;
}
#endif
