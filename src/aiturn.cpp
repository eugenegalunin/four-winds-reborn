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

#include <set>
#include <limits>
#include <random>
#include <algorithm>

#include "aiturn.h"
#include "aiadventure.h"
#include "aispell.h"

namespace GameData
{
    extern LocalPlayers gamers;

    LocalPlayer &	playerOfAvatar(const Avatar &);
    LocalPlayer &	playerOfClan(const Clan &);
    LocalPlayer &	playerOfWind(const Wind &);
    bool		client2Mahjong(const Avatar &, const ClientMessage &, ActionList &);
}

bool AI::mahjongTurn(const Wind & currentWind, const Avatar & avatar, const VecStones & trash,
    const WinRules & left, const WinRules & right, const WinRules & top, bool showGame, bool showKong, ActionList & actions)
{
    // simple AI
    actions.push_back(MahjongTurn(currentWind, Stone(), false, false));

    if(showGame)
    {
	GameData::client2Mahjong(avatar, ClientSayGame(), actions);
	GameData::client2Mahjong(avatar, ClientButtonGame(), actions);
	return true;
    }
    else
    if(showKong)
    {
	GameData::client2Mahjong(avatar, ClientSayKong(2), actions);
	GameData::client2Mahjong(avatar, ClientButtonKong2(), actions);
	return true;
    }

    // AI select
    WinRules other; other.reserve(12);
    other.insert(other.end(), left.begin(), left.end());
    other.insert(other.end(), right.begin(), right.end());
    other.insert(other.end(), top.begin(), top.end());

    LocalPlayer & player = GameData::playerOfAvatar(avatar);

    if(player.newStone.isValid())
    {
	player.stones.add(player.newStone);
	player.newStone = GameStone(Stone::None, true);
    }

    // cast or summon
    const AvatarInfo & avatarInfo = GameData::avatarInfo(avatar);

    if(!player.isCasted() && !player.isAffectedSpell(Spell::Silence) &&
       !player.isAffectedSpell(Spell::ManaFog))
    {
	Creatures summons;
        Spells casts;

	// allow summon creatures
	for(auto & cr : avatarInfo.creatures)
	{
	    const CreatureInfo & creatureInfo = GameData::creatureInfo(cr);

	    // allow summon
	    if(creatureInfo.cost <= player.points &&
		player.stones.allowCast(creatureInfo.stones)) summons.push_back(creatureInfo.id);
	}

	casts = availableSpellCasts(player);

	if(summons.size() || casts.size())
	    mahjongSummonCast(avatar, summons, casts, actions);
    }

    // drop stone
    int dropIndex = mahjongSelect(player.stones, trash, other);
    return GameData::client2Mahjong(avatar, ClientDropIndex(dropIndex), actions);
}

namespace
{
    bool trySummon(const LocalPlayer & player, Creatures summons, ActionList & actions)
    {
        if(summons.empty() || player.army.isMaximumSummoning()) return false;

        const AI::BehaviorProfile profile = AI::behaviorProfile(player);

        summons.erase(std::remove_if(summons.begin(), summons.end(), [&](const Creature & creature)
        {
            const CreatureInfo & info = GameData::creatureInfo(creature);
            return info.unique && GameData::findCreatureUnique(creature);
        }), summons.end());
        std::sort(summons.begin(), summons.end(), [&](const Creature & left, const Creature & right)
        {
            const int leftValue = AI::creatureSummonScore(left, profile);
            const int rightValue = AI::creatureSummonScore(right, profile);
            return leftValue == rightValue ? left() < right() : leftValue > rightValue;
        });

        Lands lands = Lands::thisClan(player.clan).powerOnly();
        lands.erase(std::remove_if(lands.begin(), lands.end(), [&](const Land & land)
        {
            const BattleParty* party = player.army.findPartyConst(land);
            return party && !party->canJoin();
        }), lands.end());
        std::sort(lands.begin(), lands.end(), [&](const Land & left, const Land & right)
        {
            const BattleParty* leftParty = player.army.findPartyConst(left);
            const BattleParty* rightParty = player.army.findPartyConst(right);
            const int leftValue = (leftParty ? 100 : 0) + GameData::landInfo(left).stat.point;
            const int rightValue = (rightParty ? 100 : 0) + GameData::landInfo(right).stat.point;
            return leftValue == rightValue ? left() < right() : leftValue > rightValue;
        });

        for(const Creature & creature : summons)
            for(const Land & land : lands)
                if(GameData::client2Mahjong(player.avatar, ClientSummonCreature(creature, land), actions))
                    return true;

        return false;
    }
}

void AI::mahjongSummonCast(const Avatar & avatar, const Creatures & summons, const Spells & casts, ActionList & actions)
{
    const LocalPlayer & player = GameData::playerOfAvatar(avatar);
    const SpellCastPlan spellPlan = chooseSpellCast(player, casts);

    // Each profile decides when a tactical plan is worth delaying a summon.
    if(shouldCastBeforeSummon(spellPlan) && executeSpellCast(player, spellPlan, actions)) return;
    if(trySummon(player, summons, actions)) return;

    executeSpellCast(player, spellPlan, actions);
}

void AI::mahjongOtherPass(const Wind & currentWind, ActionList & actions, const Wind & skip)
{
    for(auto & id : winds_all)
    {
	LocalPlayer & playerAI = GameData::playerOfWind(id);

	if(id == currentWind() || id == skip() || ! playerAI.isAI())
		continue;

	actions.push_back(MahjongPass(id));
    }
}

bool AI::mahjongGameKongPungChao(const Wind & currentWind, const Wind & roundWind, const Stone & dropStone, WinResults & winResult, ActionList & actions, bool sayOnly)
{
    // set game
    for(auto & id : winds_all)
    {
	if(id == currentWind())
		continue;

	LocalPlayer & playerAI = GameData::playerOfWind(id);

	if(playerAI.isAI() &&
	    playerAI.isWinMahjong(currentWind, roundWind, dropStone, & winResult))
	{
	    if(sayOnly)
		return GameData::client2Mahjong(playerAI.avatar, ClientSayGame(), actions);
	    else
		return GameData::client2Mahjong(playerAI.avatar, ClientButtonGame(), actions);
	}
    }

    // set kong, pung
    for(auto & id : winds_all)
    {
	if(id == currentWind())
		continue;

	LocalPlayer & playerAI = GameData::playerOfWind(id);

	if(playerAI.isAI())
	{
	    if(playerAI.isMahjongKong1(currentWind, dropStone))
	    {
		if(sayOnly)
		    return GameData::client2Mahjong(playerAI.avatar, ClientSayKong(2), actions);
		else
		    return GameData::client2Mahjong(playerAI.avatar, ClientButtonKong1(), actions);
	    }
	    else
	    if(playerAI.isMahjongPung(currentWind, dropStone))
	    {
		if(sayOnly)
		    return GameData::client2Mahjong(playerAI.avatar, ClientSayPung(), actions);
		else
		    return GameData::client2Mahjong(playerAI.avatar, ClientButtonPung(), actions);
	    }
	}
    }

    // set chao
    for(auto & id : winds_all)
    {
	if(id == currentWind())
	    continue;

	LocalPlayer & playerAI = GameData::playerOfWind(id);

	if(playerAI.isAI() &&
	    playerAI.isMahjongChao(currentWind, dropStone))
	{
	    if(sayOnly)
		return GameData::client2Mahjong(playerAI.avatar, ClientSayChao(), actions);
	    else
		return GameData::client2Mahjong(playerAI.avatar, ClientChaoVariant(255), actions);
	}
    }

    return false;
}

struct StoneCost
{
    GameStone		stone;
    int			index;
    int			cost;

    StoneCost() : index(-1), cost(0) {}
    StoneCost(const GameStone & gs, int pos, int val) : stone(gs), index(pos), cost(val) {}

    bool		operator< (const StoneCost & sc) const { return cost < sc.cost; }
};

int mahjongStoneValue(const GameStones & stones, const GameStone & stone,
                      const VecStones & trash, const WinRules & other)
{
    int cost = 100;

    const int count1 = std::count(stones.begin(), stones.end(), stone);
    const int count2 = std::count(trash.begin(), trash.end(), stone) +
                       std::count(other.begin(), other.end(), stone);

    if(stone.isSpecial())
    {
	if(1 < count1)
	    cost += 3 < count1 + count2 ? -20 * count2 : 20 * count1;
	return cost;
    }

    if(1 < count1)
	cost += 3 < count1 + count2 ? -10 * count2 : 10 * count1;

    switch(stone.order())
    {
	case 1:
	    cost += 10 * ((stones.findStone(stone.next()) ? 1 : 0) +
			 (stones.findStone(stone.next().next()) ? 1 : 0));
	    break;
	case 2:
	    cost += 10 * ((stones.findStone(stone.prev()) ? 1 : 0) +
			 (stones.findStone(stone.next()) ? 1 : 0) +
			 (stones.findStone(stone.next().next()) ? 1 : 0));
	    break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	    cost += 10 * ((stones.findStone(stone.prev().prev()) ? 1 : 0) +
			 (stones.findStone(stone.prev()) ? 1 : 0) +
			 (stones.findStone(stone.next()) ? 1 : 0) +
			 (stones.findStone(stone.next().next()) ? 1 : 0));
	    break;
	case 8:
	    cost += 10 * ((stones.findStone(stone.prev().prev()) ? 1 : 0) +
			 (stones.findStone(stone.prev()) ? 1 : 0) +
			 (stones.findStone(stone.next()) ? 1 : 0));
	    break;
	case 9:
	    cost += 10 * ((stones.findStone(stone.prev().prev()) ? 1 : 0) +
			 (stones.findStone(stone.prev()) ? 1 : 0));
	    break;
	default: break;
    }

    return cost;
}

int AI::mahjongSelect(const GameStones & stones, const VecStones & trash, const WinRules & other)
{
    std::multiset<StoneCost> result;

    for(auto it = stones.begin(); it != stones.end(); ++it)
    {
	const GameStone & stone = *it;
	result.emplace(stone, std::distance(stones.begin(), it),
	               mahjongStoneValue(stones, stone, trash, other));
    }

    if(result.size())
    {
	auto itbeg = result.begin();
	auto itend = result.upper_bound(*itbeg);

	std::vector<StoneCost> rnd; rnd.reserve(8);
	std::copy(itbeg, itend, std::back_inserter(rnd));

        std::random_device rd;
        std::mt19937 mtg(rd());
	std::shuffle(rnd.begin(), rnd.end(), mtg);

	// first find casted
	auto itres = std::find_if(rnd.begin(), rnd.end(), [](const StoneCost & sc) { return sc.stone.isCasted(); });
	if(itres != rnd.end()) return (*itres).index;

	// after return normal
	return rnd.front().index;
    }

    // impossible ;)
    return Tools::rand(0, stones.size());
}

int AI::mahjongLuckChoice(const GameStones & stones, const VecStones & choices,
                          const VecStones & trash, const WinRules & other)
{
    int selected = 0;
    int bestValue = std::numeric_limits<int>::min();

    for(int index = 0; index < static_cast<int>(choices.size()); ++index)
    {
	GameStones hand = stones;
	const GameStone candidate(choices[index], false);
	hand.add(candidate);
	const int value = mahjongStoneValue(hand, candidate, trash, other);
	if(bestValue < value)
	{
	    bestValue = value;
	    selected = index;
	}
    }

    return selected;
}

void AI::adventureMove(const RemotePlayer & player, ActionList & actions)
{
    const BehaviorProfile profile = behaviorProfile(player);

    // Claims can open another border, so recalculate after each successful deed.
    while(true)
    {
        const AdventureClaimPlan claim = chooseAdventureClaim(player, profile);
        if(!claim.isValid()) break;
        DEBUG("AI profile: " << behaviorProfileName(profile) << ", claim: " << claim.land.toString() <<
              ", score: " << claim.score);
        if(!GameData::client2Adventure(player.avatar, ClientLandClaim(claim.land), actions)) break;
    }

    const AdventureTurnPlan turnPlan = chooseAdventureTurn(player, profile);
    for(const AdventureThreat & threat : turnPlan.threats)
    {
        DEBUG("AI profile: " << behaviorProfileName(profile) << ", avatar: " << player.avatar.toString() <<
              ", predicted threat: " << threat.land.toString() << ", enemy origin: " <<
              threat.enemyOrigin.toString() << ", score: " << threat.score << ", strength: " <<
              threat.enemyStrength << "/" << threat.defenseStrength << ", capture: " <<
              threat.captureChance << "%" << ", survival: " << threat.attackerSurvival << "%");
    }

    for(const AdventurePartyOrder & order : turnPlan.orders)
    {
        if(order.hold)
        {
            DEBUG("AI profile: " << behaviorProfileName(profile) << ", avatar: " << player.avatar.toString() <<
                  ", hold: " << order.origin.toString() <<
                  (order.defend.isValid() ? ", reserve for: " + order.defend.toString() : ""));
            continue;
        }

        if(!order.move.isValid()) continue;
        DEBUG("AI profile: " << behaviorProfileName(profile) << ", avatar: " << player.avatar.toString() <<
              ", target: " << order.move.target.toString() << ", destination: " <<
              order.move.destination.toString() << ", score: " << order.score << ", strength: " <<
              order.move.attackStrength << "/" << order.move.defenseStrength << ", capture: " <<
              order.move.captureChance << "%" << ", survival: " << order.move.attackerSurvival << "%" <<
              (order.defend.isValid() ? ", reinforce: " + order.defend.toString() : ""));

        for(int unit : order.units)
        {
            if(player.army.findBattleUnitConst(unit))
                GameData::client2Adventure(player.avatar, ClientUnitMoved(unit, order.move.destination), actions);
        }
    }
}
