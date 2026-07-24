/***************************************************************************
 *   Copyright (C) 2020 by RuneWarsNA team <runewars.newage@gmail.com>     *
 *                                                                         *
 *   Part of the RuneWars: NewAge engine.                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "actionvalidation.h"

namespace
{
    thread_local ActionRejection* activeActionRejection = nullptr;
}

GameData::ScopedActionRejection::ScopedActionRejection(ActionRejection* current)
    : previous(activeActionRejection)
{
    activeActionRejection = current;
    if(current) *current = ActionRejection();
}

GameData::ScopedActionRejection::~ScopedActionRejection()
{
    activeActionRejection = previous;
}

bool GameData::rejectAction(ActionRejectReason reason, int available, int required)
{
    if(activeActionRejection && !activeActionRejection->isValid())
    {
        activeActionRejection->reason = reason;
        activeActionRejection->available = available;
        activeActionRejection->required = required;
    }
    return false;
}

const char* GameData::actionRejectReasonName(ActionRejectReason reason)
{
    switch(reason)
    {
        case ActionRejectReason::None: return "none";
        case ActionRejectReason::UnknownAction: return "unknown_action";
        case ActionRejectReason::WrongPhase: return "wrong_phase";
        case ActionRejectReason::WrongTurn: return "wrong_turn";
        case ActionRejectReason::StaleSelection: return "stale_selection";
        case ActionRejectReason::Silenced: return "silenced";
        case ActionRejectReason::ManaFog: return "mana_fog";
        case ActionRejectReason::AlreadyCast: return "already_cast";
        case ActionRejectReason::IllegalMahjongCall: return "illegal_mahjong_call";
        case ActionRejectReason::InvalidRuneChoice: return "invalid_rune_choice";
        case ActionRejectReason::InvalidLand: return "invalid_land";
        case ActionRejectReason::InvalidTarget: return "invalid_target";
        case ActionRejectReason::MissingRunes: return "missing_runes";
        case ActionRejectReason::InsufficientPoints: return "insufficient_points";
        case ActionRejectReason::InsufficientClaimPoints: return "insufficient_claim_points";
        case ActionRejectReason::ArmyFull: return "army_full";
        case ActionRejectReason::UniqueCreatureExists: return "unique_creature_exists";
        case ActionRejectReason::PartyFull: return "party_full";
        case ActionRejectReason::UnitNotFound: return "unit_not_found";
        case ActionRejectReason::IllegalMovement: return "illegal_movement";
        case ActionRejectReason::IllegalLandClaim: return "illegal_land_claim";
        case ActionRejectReason::NothingToUndo: return "nothing_to_undo";
        case ActionRejectReason::NoPendingBattle: return "no_pending_battle";
        case ActionRejectReason::InvalidBattleChoice: return "invalid_battle_choice";
    }
    return "unknown_action";
}

std::string GameData::actionRejectionMessage(const ActionRejection & rejection)
{
    switch(rejection.reason)
    {
        case ActionRejectReason::WrongPhase:
            return _("That action is not available in the current phase.");
        case ActionRejectReason::WrongTurn:
            return _("It is not your turn.");
        case ActionRejectReason::StaleSelection:
            return _("That choice is no longer available.");
        case ActionRejectReason::Silenced:
            return _("Silence prevents this action.");
        case ActionRejectReason::ManaFog:
            return _("Mana Fog prevents casting this turn.");
        case ActionRejectReason::AlreadyCast:
            return _("You have already summoned or cast a spell this turn.");
        case ActionRejectReason::IllegalMahjongCall:
            return _("That Mahjong call is not legal for the current rune.");
        case ActionRejectReason::InvalidRuneChoice:
            return _("That rune choice is no longer available.");
        case ActionRejectReason::InvalidLand:
            return _("Select a valid territory.");
        case ActionRejectReason::InvalidTarget:
            return _("Select a valid target.");
        case ActionRejectReason::MissingRunes:
            return _("The required runes are not available.");
        case ActionRejectReason::InsufficientPoints:
            if(0 <= rejection.available && 0 <= rejection.required)
                return StringFormat(_("This action requires %2 spell points; only %1 are available."))
                    .arg(rejection.available).arg(rejection.required);
            return _("There are not enough spell points for that action.");
        case ActionRejectReason::InsufficientClaimPoints:
            if(0 <= rejection.available && 0 <= rejection.required)
                return StringFormat(_("This claim requires %2 claim points; only %1 are available."))
                    .arg(rejection.available).arg(rejection.required);
            return _("There are not enough claim points for that territory.");
        case ActionRejectReason::ArmyFull:
            return _("Your army has no free summoning slot.");
        case ActionRejectReason::UniqueCreatureExists:
            return _("That unique creature is already on the map.");
        case ActionRejectReason::PartyFull:
            return _("The destination party is full.");
        case ActionRejectReason::UnitNotFound:
            return _("The selected creature is no longer available.");
        case ActionRejectReason::IllegalMovement:
            return _("The selected creature cannot move to that territory.");
        case ActionRejectReason::IllegalLandClaim:
            return _("That territory cannot be claimed now.");
        case ActionRejectReason::NothingToUndo:
            return _("There are no Adventure orders to undo.");
        case ActionRejectReason::NoPendingBattle:
            return _("There is no battle choice awaiting input.");
        case ActionRejectReason::InvalidBattleChoice:
            return _("That battle choice is no longer legal.");
        case ActionRejectReason::None:
        case ActionRejectReason::UnknownAction:
            break;
    }
    return _("The game rejected that action.");
}
