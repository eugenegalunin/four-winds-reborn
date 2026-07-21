/***************************************************************************
 *   Four Winds Reborn: observer-safe Adventure map hints                 *
 ***************************************************************************/

#ifndef _FOUR_WINDS_ADVENTURE_HINTS_
#define _FOUR_WINDS_ADVENTURE_HINTS_

#include "gamedata.h"

namespace AdventureHints
{
    enum class DestinationCue
    {
        None,
        Move,
        Attack,
        Claim
    };

    struct BattlePreview
    {
        bool available = false;
        bool showPercentages = false;
        int samples = 0;
        int captureChance = 0;
        int attackerSurvival = 0;
        int attackerCount = 0;
        int visibleDefenderCount = 0;
        int townLoyalty = 0;
    };

    // These helpers accept LocalData deliberately. Hidden opponents are absent
    // from that observer view and therefore cannot influence passive UI hints.
    bool canClaimObserved(const LocalData &, const Land &);
    DestinationCue destinationCue(const LocalData &, const Land &, const Land &);
    BattlePreview battlePreview(const LocalData &, const Land &, const Land &,
                                AI::Difficulty);
}

#endif
