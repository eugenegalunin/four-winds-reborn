#ifndef FOUR_WINDS_DEVELOPER_TOOLS_H
#define FOUR_WINDS_DEVELOPER_TOOLS_H

#ifdef BUILD_DEBUG

#include <cstddef>
#include <string>

#include "gamedata.h"

namespace DeveloperTools
{
    enum class Command
    {
        Cancel,
        ToggleAutoplay,
        FinishPhase,
        NextAdventure,
        BattleFixture,
        FinishRound,
        FinishGame
    };

    struct FastForwardResult
    {
        bool success = false;
        int menu = Menu::MainMenu;
        std::size_t ticks = 0;
        std::string error;
    };

    Command showPanel(SWE::Window &, const Avatar &);
    FastForwardResult fastForward(Command, const Avatar &);
}

#endif

#endif
