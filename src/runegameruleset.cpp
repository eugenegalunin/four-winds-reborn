/***************************************************************************
 *   Copyright (C) 2026 by Four Winds Reborn contributors                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <algorithm>

#include "gameobjects.h"
#include "runegameruleset.h"

namespace
{
    class ClassicRuneGameRuleset final : public RuneGameRuleset
    {
    public:
        const std::string & id(void) const override
        {
            static const std::string value("classic");
            return value;
        }

        int version(void) const override
        {
            return 1;
        }

        const std::vector<int> & wallStoneIds(void) const override
        {
            static const std::vector<int> stones = {
                Stone::Skull1, Stone::Skull2, Stone::Skull3,
                Stone::Skull4, Stone::Skull5, Stone::Skull6,
                Stone::Skull7, Stone::Skull8, Stone::Skull9,
                Stone::Sword1, Stone::Sword2, Stone::Sword3,
                Stone::Sword4, Stone::Sword5, Stone::Sword6,
                Stone::Sword7, Stone::Sword8, Stone::Sword9,
                Stone::Number1, Stone::Number2, Stone::Number3,
                Stone::Number4, Stone::Number5, Stone::Number6,
                Stone::Number7, Stone::Number8, Stone::Number9,
                Stone::Wind1, Stone::Wind2, Stone::Wind3, Stone::Wind4,
                Stone::Dragon1, Stone::Dragon2, Stone::Dragon3
            };
            return stones;
        }

        int wallCopies(void) const override
        {
            return 4;
        }

        int initialHandSize(void) const override
        {
            return 13;
        }

        int baseWinPoints(void) const override
        {
            return 20;
        }

        int scoreMultiplier(int doubles) const override
        {
            if(doubles <= 0) return 1;
            return 1 << std::min(doubles, 5);
        }

        int maximumWinScore(void) const override
        {
            return 500;
        }
    };
}

const RuneGameRuleset & classicRuneGameRuleset(void)
{
    static const ClassicRuneGameRuleset ruleset;
    return ruleset;
}
