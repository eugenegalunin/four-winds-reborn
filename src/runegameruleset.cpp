/***************************************************************************
 *   Copyright (C) 2026 by Four Winds Reborn contributors                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <algorithm>

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
