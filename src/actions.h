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

#ifndef _RWNA_ACTIONS_
#define _RWNA_ACTIONS_

#include <list>
#include "gameobjects.h"

namespace Action
{
    enum { None,

	    ButtonOk, ButtonClose, ButtonStart, ButtonCancel,
            ButtonLocalReady, ButtonLocalKong, ButtonLocalGame,
            ButtonChao, ButtonPung, ButtonKong, ButtonGame, ButtonPass,
            ButtonCast, ButtonChat, ButtonSystem, ButtonSummary, ButtonMap,
	    ButtonDone, ButtonDismiss, ButtonInfo, ButtonUndo, ButtonMenu,

	    ClientReady, ClientButtonGame, ClientButtonPass, ClientChaoVariant, ClientButtonPung, ClientButtonKong1,
	    ClientButtonKong2, ClientDropIndex, ClientSayGame, ClientSayChao, ClientSayPung, ClientSayKong,
	    ClientSummonCreature, ClientCastSpell, ClientUnitMoved, ClientLandClaim, ClientAdventureUndo, ClientBattleReady,

	    MahjongBegin, MahjongTurn, MahjongDrop, MahjongGame, MahjongPass, MahjongChao, MahjongPung,
	    MahjongKong1, MahjongKong2, MahjongSummon, MahjongCast, MahjongData, MahjongInfo, MahjongEnd,

            AdventureBegin, AdventureTurn, AdventureMoves, AdventureClaim, AdventureCombat, AdventureEnd,

	    MahjongDropSelected, MahjongOutOfTime, MahjongGameQuit, ButtonOrder,
	    ClientLuckChoice, MahjongLuckChoice,
            Last };
}

struct ActionMessage : public JsonObject
{
    ActionMessage(int type)
    {
        addInteger("type", type);
    }

    int type(void) const
    {
        return getInteger("type", 0);
    }
};

struct MahjongMessage : ActionMessage
{
    MahjongMessage(int type, const Wind & curWind)
        : ActionMessage(type)
    {
        addString("currentWind", curWind.toString());
    }

    Wind currentWind(void) const
    {
        return Wind(getString("currentWind"));
    }
};

struct MahjongBegin : MahjongMessage
{
    MahjongBegin(const Wind & curWind, const Wind & roundWind, bool newRound)
        : MahjongMessage(Action::MahjongBegin, curWind)
    {
        addString("roundWind", roundWind.toString());
        addBoolean("newRound", newRound);
    }

    Wind roundWind(void) const
    {
        return Wind(getString("roundWind"));
    }

    bool newRound(void) const
    {
        return getBoolean("newRound");
    }
};

struct MahjongEnd : MahjongMessage
{
    MahjongEnd(const Wind & curWind) : MahjongMessage(Action::MahjongEnd, curWind) {}
};

struct MahjongTurn : MahjongMessage
{
    MahjongTurn(const Wind & curWind, const Stone & newStone, bool kong, bool game)
	: MahjongMessage(Action::MahjongTurn, curWind)
    {
	addString("newStone", newStone.toString());
	addBoolean("showKong", kong);
	addBoolean("showGame", game);
    }

    Stone newStone(void) const
    {
	return Stone(getString("newStone"));
    }

    bool showKong(void) const
    {
	return getBoolean("showKong");
    }

    bool showGame(void) const
    {
	return getBoolean("showGame");
    }
};

struct MahjongLuckChoice : MahjongMessage
{
    MahjongLuckChoice(const Wind & curWind, const VecStones & choices)
	: MahjongMessage(Action::MahjongLuckChoice, curWind)
    {
	addArray("choices", choices.toJsonArray());
    }

    VecStones choices(void) const
    {
	const JsonArray* values = getArray("choices");
	return values ? VecStones::fromJsonArray(*values) : VecStones();
    }
};

struct MahjongSayType : MahjongMessage
{
    MahjongSayType(int type, const Wind & curWind, bool say)
	: MahjongMessage(type, curWind)
    {
	addBoolean("sayOnly", say);
    }

    MahjongSayType(int type, const Wind & curWind, const Stone & dropStone)
	: MahjongMessage(type, curWind)
    {
	addBoolean("sayOnly", false);
	addString("dropStone", dropStone.toString());
    }

    bool sayOnly(void) const
    {
	return getBoolean("sayOnly");
    }

    Stone dropStone(void) const
    {
	return hasKey("dropStone") ? Stone(getString("dropStone")) : Stone();
    }
};

struct MahjongGame : MahjongSayType
{
    MahjongGame(const Wind & curWind) : MahjongSayType(Action::MahjongGame, curWind, false) {}
};

struct MahjongSayGame : MahjongSayType
{
    MahjongSayGame(const Wind & curWind) : MahjongSayType(Action::MahjongGame, curWind, true) {}
};

struct MahjongKong1 : MahjongSayType
{
    MahjongKong1(const Wind & curWind, bool say = false)
	: MahjongSayType(Action::MahjongKong1, curWind, say) {}

    MahjongKong1(const Wind & curWind, const Stone & dropStone)
	: MahjongSayType(Action::MahjongKong1, curWind, dropStone) {}
};

struct MahjongSayKong : MahjongKong1
{
    MahjongSayKong(const Wind & curWind) : MahjongKong1(curWind, true) {}
};

struct MahjongKong2 : MahjongMessage
{
    MahjongKong2(const Wind & curWind) : MahjongMessage(Action::MahjongKong2, curWind) {}
};

struct MahjongPung : MahjongSayType
{
    MahjongPung(const Wind & curWind, bool say = false)
	: MahjongSayType(Action::MahjongPung, curWind, say) {}

    MahjongPung(const Wind & curWind, const Stone & dropStone)
	: MahjongSayType(Action::MahjongPung, curWind, dropStone) {}
};

struct MahjongSayPung : MahjongPung
{
    MahjongSayPung(const Wind & curWind) : MahjongPung(curWind, true) {}
};

struct MahjongChao : MahjongSayType
{
    MahjongChao(const Wind & curWind, bool say = false)
	: MahjongSayType(Action::MahjongChao, curWind, say) {}

    MahjongChao(const Wind & curWind, const Stone & dropStone)
	: MahjongSayType(Action::MahjongChao, curWind, dropStone) {}
};

struct MahjongSayChao : MahjongChao
{
    MahjongSayChao(const Wind & curWind) : MahjongChao(curWind, true) {}
};

struct MahjongPass : MahjongMessage
{
    MahjongPass(const Wind & curWind) : MahjongMessage(Action::MahjongPass, curWind) {}
};

struct MahjongDrop : MahjongMessage
{
    MahjongDrop(const Wind & curWind, const Stone & dropStone)
	: MahjongMessage(Action::MahjongDrop, curWind)
    {
	addString("dropStone", dropStone.toString());
    }

    Stone dropStone(void) const
    {
	return Stone(getString("dropStone"));
    }
};

struct MahjongSummon : MahjongMessage
{
    MahjongSummon(const Wind & curWind, const Creature & creature, const Land & land)
	: MahjongMessage(Action::MahjongSummon, curWind)
    {
	addString("creature", creature.toString());
	addString("land", land.toString());
    }

    Creature creature(void) const { return Creature(getString("creature")); }
    Land land(void) const { return Land(getString("land")); }
};

struct MahjongCast : MahjongMessage
{
    MahjongCast(const Wind & curWind, const Spell & spell, const Land & land, const BattleTargets & targets, const std::vector<int> & resists)
	: MahjongMessage(Action::MahjongCast, curWind)
    {
	addString("spell", spell.toString());
	addString("land", land.toString());
	addArray("targets", targets.toJsonArray());
	addArray("resists", JsonPack::stdVector<int>(resists));
    }

    MahjongCast(const Wind & curWind, const Spell & spell, const Avatar & target)
	: MahjongMessage(Action::MahjongCast, curWind)
    {
	addString("target", target.toString());
	addString("spell", spell.toString());
    }

    MahjongCast(const Wind & curWind, const Spell & spell)
	: MahjongMessage(Action::MahjongCast, curWind)
    {
	addString("spell", spell.toString());
    }

    Avatar target(void) const { return Avatar(getString("target")); }
    Spell spell(void) const { return Spell(getString("spell")); }
    Land land(void) const { return Land(getString("land")); }
    std::vector<int> targetUnits(void) const { return getStdVector<int>("targets"); }

    BattleTargets targets(void) const
    {
	const JsonArray* ja = getArray("targets");
	return ja ? BattleTargets::fromJsonArray(*ja) : BattleTargets();
    }

    std::vector<int> resists(void) const
    {
	return getStdVector<int>("resists");
    }
};

struct MahjongInfo : MahjongMessage
{
    MahjongInfo(const Wind & curWind, const std::string & str)
        : MahjongMessage(Action::MahjongInfo, curWind)
    {
	addString("info", str);
    }

    std::string info(void) const { return getString("info"); }
};

// send all data, localdata
struct MahjongData : MahjongMessage
{
    MahjongData(const Wind & currentWind) : MahjongMessage(Action::MahjongData, currentWind) {}
};

struct AdventureMessage : ActionMessage
{
    AdventureMessage(int type, const Wind & curWind)
        : ActionMessage(type)
    {
        addString("currentWind", curWind.toString());
    }

    Wind currentWind(void) const
    {
        return Wind(getString("currentWind"));
    }
};

struct AdventureTurn : AdventureMessage
{
    AdventureTurn(const Wind & currentWind) : AdventureMessage(Action::AdventureTurn, currentWind) {}
};

struct AdventureMoves : AdventureMessage
{
    AdventureMoves(const Wind & currentWind, int unit, const Land & land)
	: AdventureMessage(Action::AdventureMoves, currentWind)
    {
	addInteger("unit", unit);
	addString("land", land.toString());
    }

    int unit(void) const { return getInteger("unit"); }
    Land land(void) const { return Land(getString("land")); }
};

struct AdventureClaim : AdventureMessage
{
    AdventureClaim(const Wind & currentWind, const Land & land, const Clan & previousOwner, const Clan & owner, int cost, bool reverted = false)
	: AdventureMessage(Action::AdventureClaim, currentWind)
    {
	addString("land", land.toString());
	addString("previousOwner", previousOwner.toString());
	addString("owner", owner.toString());
	addInteger("cost", cost);
	addBoolean("reverted", reverted);
    }

    Land land(void) const { return Land(getString("land")); }
    Clan previousOwner(void) const { return Clan(getString("previousOwner")); }
    Clan owner(void) const { return Clan(getString("owner")); }
    int cost(void) const { return getInteger("cost"); }
    bool reverted(void) const { return getBoolean("reverted"); }
};

struct AdventureCombat : AdventureMessage
{
    AdventureCombat(const Wind & currentWind, const BattleLegend & legend, const BattleStrikes & strikes)
	: AdventureMessage(Action::AdventureCombat, currentWind)
    {
	addObject("legend", legend.toJsonObject());
	addArray("strikes", strikes.toJsonArray());
    }

    BattleLegend legend(void) const
    {
	const JsonObject* jo = getObject("legend");
	return jo ? BattleLegend::fromJsonObject(*jo) : BattleLegend();
    }

    BattleStrikes strikes(void) const
    {
	const JsonArray* ja = getArray("strikes");
	return ja ? BattleStrikes::fromJsonArray(*ja) : BattleStrikes();
    }
};

struct AdventureEnd : AdventureMessage
{
    AdventureEnd(const Wind & currentWind) : AdventureMessage(Action::AdventureEnd, currentWind) {}
};


struct ClientMessage : ActionMessage
{
    ClientMessage(int type) : ActionMessage(type)
    {
    }
};

struct ClientReady : ClientMessage
{
    ClientReady() : ClientMessage(Action::ClientReady) {}
};

struct ClientSayGame : ClientMessage
{
    ClientSayGame() : ClientMessage(Action::ClientSayGame) {}
};

struct ClientButtonGame : ClientMessage
{
    ClientButtonGame() : ClientMessage(Action::ClientButtonGame) {}
};

struct ClientButtonPass : ClientMessage
{
    ClientButtonPass() : ClientMessage(Action::ClientButtonPass) {}
};

struct ClientSayChao : ClientMessage
{
    ClientSayChao() : ClientMessage(Action::ClientSayChao) {}
};

struct ClientChaoVariant : ClientMessage
{
    ClientChaoVariant(int variant) : ClientMessage(Action::ClientChaoVariant)
    {
	addInteger("variant", variant);
    }

    int chaoVariant(void) const { return getInteger("variant"); }
};

struct ClientSayPung : ClientMessage
{
    ClientSayPung() : ClientMessage(Action::ClientSayPung) {}
};

struct ClientButtonPung : ClientMessage
{
    ClientButtonPung() : ClientMessage(Action::ClientButtonPung) {}
};

struct ClientSayKong : ClientMessage
{
    ClientSayKong(int type) : ClientMessage(Action::ClientSayKong)
    {
	addInteger("kongType", type);
    }

    int kongType(void) const { return getInteger("kongType"); }
};

struct ClientButtonKong1 : ClientMessage
{
    ClientButtonKong1() : ClientMessage(Action::ClientButtonKong1) {}
};

struct ClientButtonKong2 : ClientMessage
{
    ClientButtonKong2() : ClientMessage(Action::ClientButtonKong2) {}
};

struct ClientDropIndex : ClientMessage
{
    ClientDropIndex(int index) : ClientMessage(Action::ClientDropIndex)
    {
	addInteger("dropIndex", index);
    }

    int dropIndex(void) const { return getInteger("dropIndex"); }
};

struct ClientLuckChoice : ClientMessage
{
    ClientLuckChoice(int index) : ClientMessage(Action::ClientLuckChoice)
    {
	addInteger("index", index);
    }

    int index(void) const { return getInteger("index", -1); }
};

struct ClientSummonCreature : ClientMessage
{
    ClientSummonCreature(const Creature & creature, const Land & land, bool force = false)
	: ClientMessage(Action::ClientSummonCreature)
    {
	addString("creature", creature.toString());
	addString("land", land.toString());
	addBoolean("force", force);
    }

    Creature creature(void) const { return Creature(getString("creature")); }
    Land land(void) const { return Land(getString("land")); }
    bool isForce(void) const { return getBoolean("force"); }
};

struct ClientCastSpell : ClientMessage
{
    ClientCastSpell(const Spell & spell, const Land & land, int unit, bool force = false)
	: ClientMessage(Action::ClientCastSpell)
    {
	addString("spell", spell.toString());
	addString("land", land.toString());
	addInteger("unit", unit);
	addBoolean("force", force);
    }

    ClientCastSpell(const Spell & spell)
	: ClientMessage(Action::ClientCastSpell)
    {
	addString("spell", spell.toString());
    }

    ClientCastSpell(const Spell & spell, const Avatar & target)
	: ClientMessage(Action::ClientCastSpell)
    {
	addString("spell", spell.toString());
	addString("target", target.toString());
    }

    Avatar target(void) const { return Avatar(getString("target")); }
    Spell spell(void) const { return Spell(getString("spell")); }
    Land land(void) const { return Land(getString("land")); }
    int unit(void) const { return getInteger("unit"); }
    bool isForce(void) const { return getBoolean("force"); }
};

struct ClientUnitMoved : ClientMessage
{
    ClientUnitMoved(int unit, const Land & land)
	: ClientMessage(Action::ClientUnitMoved)
    {
	addString("land", land.toString());
	addInteger("unit", unit);
    }

    Land land(void) const { return Land(getString("land")); }
    int unit(void) const { return getInteger("unit"); }
};

struct ClientLandClaim : ClientMessage
{
    ClientLandClaim(const Land & land)
	: ClientMessage(Action::ClientLandClaim)
    {
	addString("land", land.toString());
    }

    Land land(void) const { return Land(getString("land")); }
};

struct ClientAdventureUndo : ClientMessage
{
    ClientAdventureUndo() : ClientMessage(Action::ClientAdventureUndo) {}
};

struct ClientBattleReady : ClientMessage
{
    ClientBattleReady() : ClientMessage(Action::ClientBattleReady) {}
};


#endif
