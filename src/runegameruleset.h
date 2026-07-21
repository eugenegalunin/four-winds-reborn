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
#include <vector>

enum class RuneGameCall
{
    None,
    Chao,
    Pung,
    Kong,
    Game
};

// Authoritative Rune Game rules are introduced one stable seam at a time.
// The contract now owns scoring, wall/hand construction, legal calls and win
// validation. Further slices extend it with round/dealer flow policy.
class RuneGameRuleset
{
public:
    virtual ~RuneGameRuleset() = default;

    virtual const std::string & id(void) const = 0;
    virtual int version(void) const = 0;

    // Stone identifiers are returned as data only. CroupierSet remains the
    // sole owner of gameplay RNG and preserves the established shuffle order.
    virtual const std::vector<int> & wallStoneIds(void) const = 0;
    virtual int wallCopies(void) const = 0;
    virtual int initialHandSize(void) const = 0;

    virtual bool allowsChao(bool nextPlayer, bool sequenceRune, int variantCount) const = 0;
    virtual bool allowsPung(bool otherPlayer, int matchingRuneCount) const = 0;
    virtual bool allowsExposedKong(bool otherPlayer, int matchingRuneCount) const = 0;
    virtual bool allowsSelfKong(bool currentPlayer, bool hasDrawnRune,
                                bool upgradesPung, int matchingRuneCount) const = 0;
    virtual bool allowsWinStone(bool hasDrawnRune, bool hasDiscard,
                                bool discardFromAnotherPlayer) const = 0;
    virtual bool isWinningStructure(int groupCount, int pairCount) const = 0;

    // Choice priority breaks ties between calls available to one player.
    // Claim priority resolves competing players while preserving wind order.
    virtual int callChoicePriority(RuneGameCall) const = 0;
    virtual int discardClaimPriority(RuneGameCall) const = 0;

    virtual int baseWinPoints(void) const = 0;
    virtual int scoreMultiplier(int doubles) const = 0;
    virtual int maximumWinScore(void) const = 0;
};

// Existing games use Classic implicitly until ruleset identity is added to
// the save/replay schemas in a later, separately gated slice.
const RuneGameRuleset & classicRuneGameRuleset(void);

#endif
