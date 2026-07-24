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

#ifndef _RWNA_MAIN_
#define _RWNA_MAIN_

#include "gamedata.h"

struct Application
{
    const char*     program;
    bool            fullscreen;
    Size            geometry;
    std::string     theme;

    Application() : program(nullptr), fullscreen(false), theme("classic") {}
    Application(const char* p, bool f, const Size & g, const char* t): program(p), fullscreen(f), geometry(g), theme(t) {}

    static std::string domain(void);
    static std::string name(void);
    static std::string version(void);
    static std::string buildDate(void);
    static std::string revision(void);
    static bool        isReleaseBuild(void);
};

class RuneWarsClient : public Application
{
    int			part;
    std::string		savefile;
    bool                themeCommandOverride;

    void		parseCommandOptions(int argc, char** argv);
    void		loadResources(void);

    bool		translationInit(void);

public:
    RuneWarsClient(int, char**);

    bool		exec(void);
};

#endif
