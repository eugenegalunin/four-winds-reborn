/***************************************************************************
 *   Copyright (C) 2026 by Four Winds Reborn contributors                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef FOUR_WINDS_RUNE_GAME_RULESET_H
#define FOUR_WINDS_RUNE_GAME_RULESET_H

#include <string>

// Authoritative Rune Game rules are introduced one stable seam at a time.
// The first seam owns scoring constants and arithmetic. Further slices extend
// this contract with wall, hand, call, win-validation and round-flow policy.
class RuneGameRuleset
{
public:
    virtual ~RuneGameRuleset() = default;

    virtual const std::string & id(void) const = 0;
    virtual int version(void) const = 0;

    virtual int baseWinPoints(void) const = 0;
    virtual int scoreMultiplier(int doubles) const = 0;
    virtual int maximumWinScore(void) const = 0;
};

// Existing games use Classic implicitly until ruleset identity is added to
// the save/replay schemas in a later, separately gated slice.
const RuneGameRuleset & classicRuneGameRuleset(void);

#endif
