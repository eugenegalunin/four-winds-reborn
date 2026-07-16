#ifndef _RWNA_AISPELL_
#define _RWNA_AISPELL_

#include <vector>

#include "aiprofile.h"

namespace AI
{
    struct SpellCastStep
    {
        Spell spell;
        Avatar target;
        Land land;
        int unit;
        int score;

        SpellCastStep() : unit(-1), score(0) {}
        bool isValid(void) const { return spell.isValid() && 0 < score; }
    };

    struct SpellCastPlan : SpellCastStep
    {
        BehaviorProfile profile;
        int immediateScore;
        int futureScore;
        std::vector<SpellCastStep> followUps;

        SpellCastPlan();
        explicit SpellCastPlan(const SpellCastStep &);
    };

    Spells              knownSpellCasts(const LocalPlayer &);
    Spells              availableSpellCasts(const LocalPlayer &);

    SpellCastPlan       chooseSpellCast(const LocalPlayer &, const Spells &);
    SpellCastPlan       chooseSpellCast(const LocalPlayer &, const Spells &, BehaviorProfile);
    SpellCastPlan       chooseSpellCast(const LocalPlayer &, const Spells &, BehaviorProfile,
                                        const Spells & futureSpells);
    SpellCastPlan       chooseSpellCast(const LocalData &, const Spells &, BehaviorProfile);
    SpellCastPlan       chooseSpellCast(const LocalData &, const Spells &, BehaviorProfile,
                                        const Spells & futureSpells);
    bool                shouldCastBeforeSummon(const SpellCastPlan &);

    ClientCastSpell     spellCastCommand(const SpellCastStep &);
    bool                executeSpellCast(const LocalPlayer &, const SpellCastPlan &, ActionList &);
}

#endif
