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

#include <algorithm>

#include "settings.h"
#include "gamedata.h"
#include "gametheme.h"
#include "dialogs.h"
#include "actions.h"
#include "aibattle.h"
#include "aiprofile.h"
#include "adventurepart.h"
#include "crashreport.h"
#include "swe/swe_music.h"
#ifdef BUILD_DEBUG
#include "developertools.h"
#endif

namespace
{
class ScopedAdventureTickPause
{
    Window & window;

public:
    explicit ScopedAdventureTickPause(Window & win) : window(win)
    {
        window.disableTickEvent(true);
    }

    ~ScopedAdventureTickPause()
    {
        window.disableTickEvent(false);
    }

    ScopedAdventureTickPause(const ScopedAdventureTickPause &) = delete;
    ScopedAdventureTickPause & operator=(const ScopedAdventureTickPause &) = delete;
};
}

enum { LandPolygonClickLeft = 1111, LandPolygonClickRight, LandPolygonFocus, LandPolygonFlagAnimationReInit, LandPolygonCombatStatus, LandPolygonCombatStatusReset,
	ClanIconClickLeft, ClanIconClickRight, CreatureIconClickLeft, CreatureIconClickRight, MapScreenClose,
	MapScreenSelectLand,
	AdventureTurnPlayer, AdventureTurnMoveStart, AdventureTurnMoveStop, AdventureTurnCreatureSelect, AdventureTurnShowConsole };

LandPolygon::LandPolygon(const LandInfo & info, const JsonObject & jo, Window & win) : WindowToolTipArea(& win), landInfo(info), owner(info.clan), poly(info.points)
{
    Rect area = poly.around();

    setSize(area);
    setPosition(area);

    combatFightTexture = GameTheme::jsonSprite(jo, "sprite:combat_fight");
    combatFightStatus = false;

    const JsonToolTip & tips = GameTheme::jsonToolTipInfo();
    renderToolTip(info.name, GameTheme::fontRender(tips.font), tips.fncolor, tips.bgcolor, tips.rtcolor);

    resetState(FlagModality);
    setVisible(false);

    // add animations
    if(! info.id.isTowerWinds())
    {
	// power state
	if(info.stat.power)
	{
	    animationPower = SpritesAnimation(jo, "animation:town");
	    animationPower.setPosition(info.center - position() - animationPower.spriteSize() / 2);
	    animationPower.setEnabled(true);
	}

	// party flag
	animationFlag.loop = true;
	animationFlag.delay = 400;
	userEvent(LandPolygonFlagAnimationReInit, const_cast<LandInfo*>(& landInfo));

    }
}

bool LandPolygon::isAreaPoint(const Point & pos) const
{
    return Window::isAreaPoint(pos) ? poly & pos : false;
}

void LandPolygon::animationsDisabled(bool f)
{
    animationPower.setEnabled(f);
    animationFlag.setEnabled(f);
}

void LandPolygon::tickEvent(u32 ms)
{
    bool render = false;

    if(animationPower.isEnabled() && animationPower.next(ms)) render = true;
    if(animationFlag.isEnabled() && animationFlag.next(ms)) render = true;

    if(render) renderWindow();
}

bool LandPolygon::userEvent(int act, void* data)
{
    if(act == LandPolygonCombatStatusReset)
    {
	// reset combat status
	if(!data || data == & landInfo)
	{
	    combatFightStatus = false;
	    renderWindow();
	    return true;
	}
    }

    if(act == LandPolygonCombatStatus)
    {
	// set combat status
	if(data && data == & landInfo)
	{
	    combatFightStatus = true;
	    renderWindow();
	    return true;
	}
    }

    if(act == LandPolygonFlagAnimationReInit &&
	! landInfo.id.isTowerWinds() && (!data || data == & landInfo))
    {
	auto win = static_cast<const MapScreenBase*>(parent());
	if(! win) return false;

	const ClanInfo & clanInfo = GameData::clanInfo(landInfo.clan);
	const RemotePlayer & clanOwner = win->ld.playerOfClan(landInfo.clan);

	const Sprite & sprite1 = GameTheme::texture(clanInfo.townflag1);
	const Sprite & sprite2 = GameTheme::texture(clanInfo.townflag2);

	animationFlag.sprites.clear();
	const BattleParty* party = clanOwner.army.findPartyConst(landInfo.id);

	if(party && !party->isEmpty())
	{
	    animationFlag.sprites << sprite1 << sprite2;
	    animationFlag.sprites[0].setPosition(landInfo.center - position() - Size(sprite1.width() / 2, sprite1.height()) + Size(0, 15));
	    animationFlag.sprites[1].setPosition(landInfo.center - position() - Size(sprite2.width() / 2, sprite2.height()) + Size(0, 15));
	    animationFlag.setEnabled(true);
	}

	// target event: fixed owner
	if(data == & landInfo)
	    owner = landInfo.clan;

	renderWindow();
	return true;
    }

    return false;
}

bool LandPolygon::mousePressEvent(const ButtonEvent & be)
{
    auto win = static_cast<MapScreenBase*>(parent());

    if(be.isButtonLeft() && win && win->isAllowMoveFlag(landInfo))
    {
	// Start immediately so a quick trackpad drag cannot outrun the queued event.
	return win->userEvent(AdventureTurnMoveStart, const_cast<LandInfo*>(& landInfo));
    }

    return false;
}

void LandPolygon::mouseFocusEvent(void)
{
    pushEventAction(LandPolygonFocus, parent(), const_cast<LandInfo*>(& landInfo));
}

bool LandPolygon::mouseClickEvent(const ButtonsEvent & be)
{
    if(be.isButtonLeft())
	pushEventAction(LandPolygonClickLeft, parent(), const_cast<LandInfo*>(& landInfo));
    else
    if(be.isButtonRight())
	pushEventAction(LandPolygonClickRight, parent(), const_cast<LandInfo*>(& landInfo));

    return true;
}

void LandPolygon::renderWindow(void)
{
    auto win = static_cast<const MapScreenBase*>(parent());

    if(win)
    {
	if(! landInfo.id.isTowerWinds())
	{
	    const ClanInfo & clanInfo = GameData::clanInfo(owner);
	    const Texture & textureTown = GameTheme::texture(clanInfo.town);
	    const Size offy(0, 15);

	    // render animation power
	    if(animationPower.isEnabled()) animationPower.render(*this);
	    // render town sprite
	    renderTexture(textureTown, landInfo.center - position() - Size(textureTown.width() / 2, textureTown.height()) + offy);
	    // render combat status
	    if(combatFightStatus)
		renderTexture(combatFightTexture, landInfo.center - position() - Size(combatFightTexture.width() / 2, combatFightTexture.height()) + offy);
	    else
	    // render animation flag
	    if(animationFlag.isEnabled()) animationFlag.render(*this);

	    if(win->isAllowLandClaim(landInfo))
	    {
		const Points & outline = poly.boundaryVertices();
		for(auto it = outline.begin(); it != outline.end(); ++it)
		{
		    auto next = std::next(it);
		    if(next == outline.end()) next = outline.begin();
		    renderLine(Color::Yellow, *it - position(), *next - position());
		}
	    }
	}
	else
	// TowerOf4Winds: all army flags render
	{
	    std::vector<Texture> flags;
	    int width = 0;

	    for(auto & clan : clans_all)
	    {
		const RemotePlayer & other = win->ld.playerOfClan(clan);
		const BattleParty* party = other.army.findPartyConst(landInfo.id);

		if(party && !party->isEmpty())
		{
		    flags.push_back(GameTheme::texture(GameData::clanInfo(other.clan).flag2));
		    width += flags.back().width();
		}
	    }

	    int offx = 0;
	    int offy = 15;

	    for(auto & flg : flags)
	    {
		renderTexture(flg, landInfo.center - position() - Size(width / 2, 0) + Size(offx, offy));
		offx += flg.width();
	    }
	}
    }

    if(isFocused())
    {
	for(auto it = poly.begin(); it != poly.end(); ++it)
	    renderPoint(Color::Red, *it - position());
    }
}

PartyCreaturesBar::PartyCreaturesBar(Window & win) : Window(&win)
{
    resetState(FlagModality);
    setVisible(false);
}

BattleParty* PartyCreaturesBar::currentParty(void) const
{
    if(!clan.isValid() || !land.isValid()) return nullptr;
    return GameData::getBattleArmy(clan).findParty(land);
}

bool PartyCreaturesBar::mouseClickEvent(const ButtonsEvent & be)
{
    const int offset = 5;
    const Texture & icon = GameTheme::texture(GameData::clanInfo(clan).button);
    BattleParty* party = currentParty();

    // click clan icon
    if(be.isClick(Rect(Point(0, 0), icon.size())))
    {
	pushEventAction(be.isButtonLeft() ? ClanIconClickLeft : ClanIconClickRight, parent(), party);
	return true;
    }

    if(party)
    {
	const BattleCreature* bcr = nullptr;
	// click bcr1 icon
	if(be.isClick(Rect(Point(icon.width() + offset, 0), icon.size())))
	{
	    bcr = party->index(0);
	}
	else
	// click bcr2 icon
	if(be.isClick(Rect(Point(2 * (icon.width() + offset), 0), icon.size())))
	{
	    bcr = party->index(1);
	}
	else
	// click bcr3 icon
	if(be.isClick(Rect(Point(2 * (icon.width() + offset), 0), icon.size())))
	{
	    bcr = party->index(2);
	}

	if(bcr && bcr->isValid())
	{
	    pushEventAction(be.isButtonLeft() ? CreatureIconClickLeft : CreatureIconClickRight, parent(), const_cast<BattleCreature*>(bcr));

	    auto win = static_cast<const MapScreenBase*>(parent());
	    if(win && win->isMyClan(clan))
		pushEventAction(AdventureTurnCreatureSelect, parent(), const_cast<BattleCreature*>(bcr));

	    return true;
	}
    }

    return false;
}

void PartyCreaturesBar::renderWindow(void)
{
    const int offset = 5;
    const BattleParty* party = currentParty();

    // clan icon
    if(clan.isValid())
    {
	const Texture & textureClan = GameTheme::texture(GameData::clanInfo(clan).button);
	renderTexture(textureClan, Point(0, 0));
    }

    if(party)
    {
	const Texture & textureClan = GameTheme::texture(GameData::clanInfo(clan).button);
	Point dst = Size(textureClan.width() + offset, 0);

	for(int it = 0; it < 3; ++it)
	{
	    const BattleCreature* bcr = party->index(it);
	    if(bcr && bcr->isValid())
	    {
		const CreatureInfo & creatureInfo = GameData::creatureInfo(*bcr);
		Texture textureCreature = GameTheme::texture(creatureInfo.image2);
		renderTexture(textureCreature, dst);

		// can selected only my party
		auto win = static_cast<const MapScreenBase*>(parent());
		if(bcr->isSelected() && win && win->isMyClan(clan))
		    renderRect(Color::Lime, Rect(dst, Size(58, 58)));

		dst.x += textureCreature.width() + offset;
	    }
	    else
	    {
		dst.x += textureClan.width() + offset;
	    }
	}
    }
}

void PartyCreaturesBar::setParty(const Clan & cl, const Land & value)
{
    clan = cl;
    land = value;
    renderWindow();
    setVisible(true);
}

AffectedSpellsIcon::AffectedSpellsIcon(const JsonObject & jo) : bcrUid(-1)
{
    leftArea = GameTheme::jsonRect(jo, "area:spell_affected1");
    rightArea = GameTheme::jsonRect(jo, "area:spell_affected2");
}

void AffectedSpellsIcon::updateSpells(const BattleCreature & bcr, Window & win)
{
    icons.clear();
    bcrUid = bcr.battleUnit();

    Point posr = rightArea.toPoint();
    Point posl = leftArea.toPoint();

    for(auto & as : bcr.affectedSpells())
    {
	const SpellInfo & si = GameData::spellInfo(as);

	Point & pos = si.target() == SpellTarget::Friendly ? posl : posr;
	const Rect & art = si.target() == SpellTarget::Friendly ? leftArea : rightArea;

	icons.emplace_back(si, pos, win);
	SpellIcon & icon = icons.back();

	// fixed centered
	pos.x = art.x + (art.w - icon.width()) / 2;
	icon.setPosition(pos);

	pos.y += icon.height() + 4;
    }
}

void AffectedSpellsIcon::setVisible(bool f)
{
    for(auto & win : icons)
	win.setVisible(f);
}

MapScreenBase::MapScreenBase(const LocalData & data, Window* win) : JsonWindow("screen_adventurepart.json", win), ld(data),
    selectedLand(Land(Land::TowerOf4Winds)), forecastSamples(0), forecastCaptureChance(0),
    forecastAttackerSurvival(0), affectedSpells(jobject), bar1(*this), bar2(*this)
{
    townTowerWindsTexture = GameTheme::jsonSprite(jobject, "sprite:town_tower_winds");
    townTowerWindsPos = GameData::landInfo(Land::TowerOf4Winds).center - townTowerWindsTexture.size() / 2;

    defaultFont = jobject.getString("default:font");
    defaultColor = jobject.getString("default:color");

    statChangedUpColor = jobject.getString("color:stat_up");
    statChangedDownColor = jobject.getString("color:stat_down");

    Point topClanPos = GameTheme::jsonPoint(jobject, "offset:topclan");
    Point botClanPos = GameTheme::jsonPoint(jobject, "offset:botclan");
    landNamePos = GameTheme::jsonPoint(jobject, "offset:nameland");
    info1NamePos = GameTheme::jsonPoint(jobject, "offset:info1");
    info2NamePos = GameTheme::jsonPoint(jobject, "offset:info2");
    viewMapPos = GameTheme::jsonPoint(jobject, "offset:viewmap");
    selectedIconPos = GameTheme::jsonRect(jobject, "area:icon_portrait");

    const JsonObject* forecast = jobject.getObject("battle:forecast");
    if(forecast)
    {
        forecastTitle = GameTheme::jsonTextInfo(*forecast, "textinfo:title");
        forecastCapture = GameTheme::jsonTextInfo(*forecast, "textinfo:capture");
        forecastSurvival = GameTheme::jsonTextInfo(*forecast, "textinfo:survival");
    }

    // const Texture & tmp = GameTheme::texture(GameData::clanInfo(Clan::Red).button);

    bar1.setPosition(topClanPos);
    bar2.setPosition(botClanPos);

    bar1.setSize(Size(238, 56));
    bar2.setSize(Size(238, 56));

    spriteLandStat1 = GameTheme::jsonSprite(jobject, "sprite:stat11");
    spriteLandStat2 = GameTheme::jsonSprite(jobject, "sprite:stat12");
    spriteLandStat3 = GameTheme::jsonSprite(jobject, "sprite:stat13");
    spriteLandStat4 = GameTheme::jsonSprite(jobject, "sprite:stat14");
    spriteLandStat5 = GameTheme::jsonSprite(jobject, "sprite:stat15");
    spriteLandStat6 = GameTheme::jsonSprite(jobject, "sprite:stat16");

    spriteCreatureStat1 = GameTheme::jsonSprite(jobject, "sprite:stat21");
    spriteCreatureStat2 = GameTheme::jsonSprite(jobject, "sprite:stat22");
    spriteCreatureStat3 = GameTheme::jsonSprite(jobject, "sprite:stat23");
    spriteCreatureStat4 = GameTheme::jsonSprite(jobject, "sprite:stat24");
    spriteCreatureStat5 = GameTheme::jsonSprite(jobject, "sprite:stat25");
    spriteCreatureStat6 = GameTheme::jsonSprite(jobject, "sprite:stat26");

    for(auto & id : lands_all)
    {
	const LandInfo & landInfo = GameData::landInfo(id);
	lands.push_back(new LandPolygon(landInfo, jobject, *this));
    }

    for(int ii = 1; ii < 10; ++ii)
    {
	std::string key = StringFormat("animation:maps%1").arg(ii);

	if(jobject.getObject(key))
	{
	    animationMapObjects.push_back(SpritesAnimation(jobject, key));
	    animationMapObjects.back().setEnabled(true);
	}
    }

    selectedLand.reset();
    selectedCreature.reset();
    selectedClan = GameData::myPerson().clan;
}

bool MapScreenBase::isMyClan(const Clan & clan) const
{
    return clan == ld.myPlayer().clan;
}

bool MapScreenBase::isAllowMoveFlag(const LandInfo & landInfo) const
{
    const LocalPlayer & player = ld.myPlayer();

    return (landInfo.id.isTowerWinds() || landInfo.clan == player.clan) && landInfo.id == selectedLand &&
        0 < player.army.partySelected(landInfo.id).size();
}

void MapScreenBase::animationsDisabled(bool f)
{
    for(auto & obj : animationMapObjects)
	obj.setEnabled(! f);

    for(auto & land : lands)
	land->animationsDisabled(f);
}

void MapScreenBase::tickEvent(u32 ms)
{
    for(auto & obj : animationMapObjects)
    {
	if(obj.isEnabled())
	{
	    bool res = obj.next(ms);
	    if(res) setDirty(true);
	}
    }
}

const Color & MapScreenBase::getBaseStatColor(int base, int current) const
{
    if(base < current) return statChangedUpColor;
    else
    if(base > current) return statChangedDownColor;

    return defaultColor;
}

void MapScreenBase::renderLandInfo(const Land & land)
{
    const LandInfo & landInfo = GameData::landInfo(land);
    const FontRender & frs = GameTheme::fontRender(defaultFont);

    // name
    renderText(frs, landInfo.name, defaultColor, landNamePos, AlignCenter);

    // stats
    renderTexture(spriteLandStat1);
    renderText(frs, String::number(landInfo.stat.attack), defaultColor, spriteLandStat1.position() + Size(2 + spriteLandStat1.width(), -4));

    renderTexture(spriteLandStat2);
    renderText(frs, String::number(landInfo.stat.ranger), defaultColor, spriteLandStat2.position() + Size(2 + spriteLandStat2.width(), -4));

    renderTexture(spriteLandStat3);
    renderText(frs, String::number(landInfo.stat.defense), defaultColor, spriteLandStat3.position() + Size(2 + spriteLandStat3.width(), -4));

    renderTexture(spriteLandStat4);
    renderText(frs, String::number(landInfo.stat.loyalty), defaultColor, spriteLandStat4.position() + Size(2 + spriteLandStat4.width(), -4));

    renderTexture(spriteLandStat5);
    renderText(frs, landInfo.stat.power ? "Y" : "N", defaultColor, spriteLandStat5.position() + Size(2 + spriteLandStat5.width(), -4));

    renderTexture(spriteLandStat6);
    renderText(frs, String::number(landInfo.stat.point), defaultColor, spriteLandStat6.position() + Size(2 + spriteLandStat6.width(), -4));
}

void MapScreenBase::clearBattleForecast(void)
{
    forecastTarget.reset();
    forecastSamples = 0;
    forecastCaptureChance = 0;
    forecastAttackerSurvival = 0;
}

void MapScreenBase::updateBattleForecast(const Land & origin, const Land & target)
{
    clearBattleForecast();
    const AI::DifficultyRules & difficulty = AI::difficultyRules(GameData::aiDifficulty());
    if(!difficulty.showPlayerBattleForecast) return;
    if(!origin.isValid() || !target.isValid() || origin == target || target.isTowerWinds()) return;

    const LocalPlayer & player = ld.myPlayer();
    const Clan owner = GameData::landInfo(target).clan;
    if(!owner.isValid() || owner == player.clan) return;

    const BattleCreatures selected = player.army.partySelected(origin);
    if(selected.empty()) return;

    BattleParty attackers(player.clan, origin);
    for(const BattleCreature* creature : selected)
        attackers.join(*creature);
    if(attackers.isEmpty()) return;

    const RemotePlayer & defender = ld.playerOfClan(owner);
    const BattleParty* defenders = defender.army.findPartyConst(target);
    const AI::BehaviorProfile defenderProfile = defender.isAI() ?
        AI::behaviorProfile(defender) : AI::BehaviorProfile::Balanced;
    const int samples = difficulty.battleForecastSamples;
    const AI::BattleForecast result = AI::forecastBattle(attackers, BattleTown(target), defenders,
        AI::BehaviorProfile::Balanced, defenderProfile, samples);

    forecastTarget = target;
    forecastSamples = result.samples;
    forecastCaptureChance = result.captureChance;
    forecastAttackerSurvival = result.attackerSurvival;
}

void MapScreenBase::renderBattleForecast(void)
{
    if(!forecastTarget.isValid() || forecastSamples <= 0) return;

    renderTextInfo(forecastTitle, _("Battle Forecast"));
    renderTextInfo(forecastCapture,
                   StringFormat(_("Capture: %1%")).arg(forecastCaptureChance));
    renderTextInfo(forecastSurvival,
                   StringFormat(_("Attackers remain: %1%")).arg(forecastAttackerSurvival));
}

void MapScreenBase::renderCreatureInfo(const BattleCreature & battle)
{
    const CreatureInfo & creatureInfo = GameData::creatureInfo(battle);
    const FontRender & frs = GameTheme::fontRender(defaultFont);

    // creature portrait
    Texture texturePortrait = GameTheme::texture(creatureInfo.image1);
    renderTexture(texturePortrait, selectedIconPos);

    // creature name
    renderText(frs, creatureInfo.name, defaultColor, info1NamePos, AlignCenter);

    // creature stats
    renderTexture(spriteCreatureStat1);
    renderText(frs, String::number(battle.attack()), getBaseStatColor(battle.baseAttack(), battle.attack()),
		    spriteCreatureStat1.position() + Size(2 + spriteCreatureStat1.width(), -4));

    renderTexture(spriteCreatureStat2);
    renderText(frs, String::number(battle.ranger()), getBaseStatColor(battle.baseRanger(), battle.ranger()),
		    spriteCreatureStat2.position() + Size(2 + spriteCreatureStat2.width(), -4));

    renderTexture(spriteCreatureStat3);
    renderText(frs, String::number(battle.defense()), getBaseStatColor(battle.baseDefense(), battle.defense()),
		    spriteCreatureStat3.position() + Size(2 + spriteCreatureStat3.width(), -4));

    renderTexture(spriteCreatureStat4);
    renderText(frs, String::number(battle.baseLoyalty()), defaultColor, spriteCreatureStat4.position() + Size(2 + spriteCreatureStat4.width(), -4));

    // cur loyalty
    renderTexture(spriteCreatureStat5);
    renderText(frs, String::number(battle.loyalty()), getBaseStatColor(battle.baseLoyalty(), battle.loyalty()), spriteCreatureStat5.position() + Size(2 + spriteCreatureStat5.width(), -4));

    // cur move
    renderTexture(spriteCreatureStat6);
    renderText(frs, String::number(battle.freeMovePoint()), defaultColor, spriteCreatureStat6.position() + Size(2 + spriteCreatureStat6.width(), -4));
}

void MapScreenBase::renderClanAvatarInfo(const RemotePlayer & player)
{
    const ClanInfo & clanInfo = GameData::clanInfo(player.clan);
    const AvatarInfo & avatarInfo = GameData::avatarInfo(player.avatar);
    const FontRender & frs = GameTheme::fontRender(defaultFont);

    // avatar portrait
    Texture texturePortrait = GameTheme::texture(avatarInfo.portrait);
    renderTexture(texturePortrait, selectedIconPos);

    // avatar name
    renderText(frs, avatarInfo.name, defaultColor, info1NamePos, AlignCenter);

    // clan name
    Rect pos = renderText(frs, clanInfo.name, defaultColor, info2NamePos, AlignCenter);
    Texture clanFlag = GameTheme::texture(clanInfo.flag1);
    renderTexture(clanFlag, pos.toPoint() - Size(clanFlag.width() + 5, 5));
    renderTexture(clanFlag, pos.toPoint() + Size(pos.w + 5, -5));
}

void MapScreenBase::renderWindow(void)
{
    JsonWindow::renderWindow();

    // render map objects animation
    for(auto & spritesAnim : animationMapObjects)
	if(spritesAnim.isEnabled()) spritesAnim.render(*this);

    renderLabel();

    // town tower winds image
    renderTexture(townTowerWindsTexture, townTowerWindsPos);

    if(selectedCreature.isValid() && !forecastTarget.isValid())
	renderCreatureInfo(selectedCreature);

    if(selectedClan.isValid() && !forecastTarget.isValid())
	renderClanAvatarInfo(ld.playerOfClan(selectedClan));

    const Land infoLand = forecastTarget.isValid() ? forecastTarget : selectedLand;
    if(infoLand.isValid()) renderLandInfo(infoLand);
    renderBattleForecast();
}

bool MapScreenBase::userEvent(int event, void* data)
{
    switch(event)
    {
	// click land polygon
	case MapScreenSelectLand:
        case LandPolygonClickLeft:
        case LandPolygonClickRight:
	    if(data)
	    {
		auto landInfo = static_cast<LandInfo*>(data);
		if(selectedLand != landInfo->id || event == MapScreenSelectLand)
		{
		    if(MapScreenSelectLand != event)
			playSound("bselect");
		    else
		    {
			if(landInfo->id != selectedLand)
			    DEBUG("selected land: " << landInfo->name);
		    }

		    selectedLand = landInfo->id;
		    selectedCreature.reset();
		    selectedClan.reset();
		    affectedSpells.setVisible(false);

		    if(landInfo->id.isTowerWinds())
		    {
			Clan clan1 = Clan(GameData::myPerson().clan);
			Clan clan2 = clan1.next();

			bar1.setParty(clan1, selectedLand);
			bar2.setParty(clan2, selectedLand);

			selectedClan = GameData::myPerson().clan;
		    }
		    else
		    {
			selectedClan = landInfo->clan;

			bar1.setParty(selectedClan, selectedLand);
			bar2.reset();
		    }
		    renderWindow();
		}
		return true;
	    }
	    break;

	// click clan icon
	case ClanIconClickLeft:
	case ClanIconClickRight:
	    clearBattleForecast();
	    if(selectedLand.isTowerWinds())
	    {
		Clan clan1, clan2;
		selectedCreature.reset();
		affectedSpells.setVisible(false);

		// click bar1
		if(bar1.area() & Display::mouseCursorPosition())
		{
		    selectedClan = bar1.currentClan();
		    clan1 = selectedClan.prev();
		    clan2 = selectedClan;
		}
		else
		// click bar2
		{
		    selectedClan = bar2.currentClan();
		    clan1 = selectedClan;
		    clan2 = selectedClan.next();
		}

		bar1.setParty(clan1, selectedLand);
		bar2.setParty(clan2, selectedLand);

		renderWindow();
		return true;
	    }
	    else
	    // other lands
	    {
		auto party = static_cast<BattleParty*>(data);
		if(party && ! selectedClan.isValid())
		    selectedClan = party->clan();

		selectedCreature.reset();
		affectedSpells.setVisible(false);

		renderWindow();
		return true;
	    }
	    break;

	// click creatures icon
	case CreatureIconClickLeft:
	case CreatureIconClickRight:
	    clearBattleForecast();
	    if(data)
	    {
		auto bcr = static_cast<BattleCreature*>(data);
		selectedCreature = *bcr;
		selectedClan.reset();
		affectedSpells.updateSpells(selectedCreature, *this);
		affectedSpells.setVisible(true);
		renderWindow();
		return true;
	    }
	    break;

	case MapScreenClose:
	    setVisible(false);
	    return true;

	case AdventureTurnPlayer:
	    clearBattleForecast();
	    if(data)
	    {
		auto player = static_cast<RemotePlayer*>(data);
		selectedClan = player->clan;
		selectedLand.reset();
		selectedCreature.reset();
		affectedSpells.setVisible(false);
		renderWindow();
		return true;
	    }
	    break;

	default: break;
    }

    return false;
}

bool MapScreenBase::isAllowLandClaim(const LandInfo & info) const
{
    return isAdventureMode() && ld.yourTurn() && GameData::canClaimLand(ld.myPlayer(), info.id);
}

/* ShowMapDialog */
ShowMapDialog::ShowMapDialog(const LocalData & data, Window & win) : MapScreenBase(data, & win)
{
    setVisible(true);
    buttons.setVisible(false);

    JsonButton* button = buttons.findIds("but_close");

    if(button)
    {
	button->setAction(MapScreenClose);
	button->setHotKey(Key::ESCAPE);
	button->setVisible(true);
    }
}

void ShowMapDialog::renderLabel(void)
{
    const FontRender & frs = GameTheme::fontRender(defaultFont);
    renderText(frs, _("View Map"), defaultColor, viewMapPos, AlignCenter);
}

/* ShowSummonCreatureDialog */
ShowSummonCreatureDialog::ShowSummonCreatureDialog(const LocalData & data, const Creature & creature, Window & win)
    : MapScreenBase(data, & win)
{
    selectedLand = Land(Land::TowerOf4Winds);

    setVisible(true);
    buttons.setVisible(false);

    JsonButton* button = buttons.findIds("but_cancel");
    if(button)
    {
	button->setAction(MapScreenClose);
	button->setHotKey(Key::ESCAPE);
	button->setVisible(true);
    }

    for(auto & id : lands_all)
    {
	const LandInfo & landInfo = GameData::landInfo(id);

	if(landAllowJoin(landInfo, ld.myPlayer()))
	{
	    animationMapObjects.emplace_back(jobject, "animation:place");

	    auto & anim = animationMapObjects.back();
	    anim.setPosition(landInfo.center - anim.spriteSize() / 2);
	    anim.setEnabled(true);
	}
    }
}

bool ShowSummonCreatureDialog::landAllowJoin(const LandInfo & info, const LocalPlayer & player)
{
    if(info.stat.power)
    {
        if(info.id.isTowerWinds())
	    return true;
	else
	if(info.clan == player.clan)
	{
	    const BattleParty* party = player.army.findPartyConst(info.id);
	    return party ? party->canJoin() : true;
	}
    }

    return false;
}

bool ShowSummonCreatureDialog::userEvent(int event, void* data)
{
    if(MapScreenBase::userEvent(event, data))
    {
        if(event == LandPolygonClickLeft)
	{
	    auto landInfo = static_cast<LandInfo*>(data);

	    if(landInfo && landAllowJoin(*landInfo, ld.myPlayer()))
	    {
		setResultCode(selectedLand());
		// close dialog
		setVisible(false);
	    }
	}

	return true;
    }

    return false;
}

const Land & ShowSummonCreatureDialog::land(void) const
{
    return selectedLand;
}

void ShowSummonCreatureDialog::renderLabel(void)
{
    const FontRender & frs = GameTheme::fontRender(defaultFont);
    renderText(frs, _("Summoning"), defaultColor, viewMapPos, AlignCenter);
}

/* ShowCastSpellDialog */
ShowCastSpellDialog::ShowCastSpellDialog(const LocalData & data, const Spell & sp, Window & win)
    : MapScreenBase(data, & win), spell(sp), targetUnit(-1)
{
    setVisible(true);
    buttons.setVisible(false);

    JsonButton* button = buttons.findIds("but_cancel");
    if(button)
    {
	button->setAction(MapScreenClose);
	button->setHotKey(Key::ESCAPE);
	button->setVisible(true);
    }
}

bool ShowCastSpellDialog::userEvent(int act, void* data)
{
    if(MapScreenBase::userEvent(act, data))
    {
	const LocalPlayer & player = ld.myPlayer();
	const SpellInfo & info = GameData::spellInfo(spell);

	// click land polygon
        if(act == LandPolygonClickLeft)
	{
	    auto landInfo = static_cast<LandInfo*>(data);
	    if(info.target() == SpellTarget::Land && landInfo)
	    {
		std::string msg = StringFormat(_("Apply spell: %1, on land: %2?")).arg(info.name).arg(landInfo->name);
		if(MessageBox(_("Error"), msg, *this, true).exec())
		{
		    setResultCode(1);
		    setVisible(false);
		}
	    }
	    else
	    if(spell() == Spell::Teleport && landInfo && 0 < targetUnit)
	    {
		const BattleParty* party = player.army.findPartyConst(landInfo->id);
		const bool friendlyLand = landInfo->id.isTowerWinds() || landInfo->clan == player.clan;
		const bool canJoin = ! party || party->canJoin();

		if(friendlyLand && canJoin && landInfo->id != targetSource)
		{
		    std::string msg = StringFormat(_("Teleport selected creature to %1?")).arg(landInfo->name);
		    if(MessageBox(_("Teleport"), msg, *this, true).exec())
		    {
			setResultCode(3);
			setVisible(false);
		    }
		}
		else
		{
		    MessageBox(_("Error"), _("Select another friendly territory with a free party slot."), *this, false).exec();
		}
	    }
	}

	// click creature icon
	if(act == CreatureIconClickLeft)
	{
	    if(info.target() != SpellTarget::Land && selectedCreature.canReceiveSpell(spell) &&
		(((info.target() & SpellTarget::Friendly) && player.clan == selectedCreature.clan()) ||
		((info.target() & SpellTarget::Enemy) && player.clan != selectedCreature.clan())))
	    {
		if(spell() == Spell::Teleport)
		{
		    targetUnit = selectedCreature.battleUnit();
		    targetSource = selectedLand;
		}
		else
		{
		    setResultCode(2);
		    setVisible(false);
		}
	    }
	    else
	    {
		std::string msg = StringFormat(_("Can'not apply spell: %1, incorrect target: %2")).arg(info.name).arg(info.target.toString());
		MessageBox(_("Error"), msg, *this, false).exec();
	    }
	}

	return true;
    }

    return false;
}

const Land & ShowCastSpellDialog::land(void) const
{
    return selectedLand;
}

int ShowCastSpellDialog::unit(void) const
{
    return 0 < targetUnit ? targetUnit : selectedCreature.battleUnit();
}

void ShowCastSpellDialog::renderLabel(void)
{
    const FontRender & frs = GameTheme::fontRender(defaultFont);
    renderText(frs, GameData::spellInfo(spell).name, defaultColor, viewMapPos, AlignCenter);
}

/* MoveFlagWindow */
MoveFlagWindow::MoveFlagWindow(const Clan & clan, Window & win) : Window(& win)
{
    flagTexture = GameTheme::texture(GameData::clanInfo(clan).flag1);
    setSize(flagTexture.size());

    setState(FlagLayoutForeground);
    setState(FlagMouseTracking);
    resetState(FlagModality);
}

void MoveFlagWindow::renderWindow(void)
{
    renderTexture(flagTexture, Point(0, 0));
}

void MoveFlagWindow::mouseTrackingEvent(const Point & pos, u32 buttons)
{
    if(buttons & ButtonLeft)
    {
	setPosition(pos - flagTexture.size() / 2);
    }
}

void MoveFlagWindow::setVisible(bool f)
{
    if(f)
	setPosition(Display::mouseCursorPosition() - flagTexture.size() / 2);

    Window::setVisible(f);
}

/* AdventurePartScreen */
AdventurePartScreen::AdventurePartScreen(const Avatar & ava) : MapScreenBase(GameData::toLocalData(ava), nullptr), myAvatar(ava), allowTickEvent(true),
    moveFlag(ld.myPlayer().clan, *this), buttonOrder(nullptr), buttonDone(nullptr), buttonUndo(nullptr), buttonDismiss(nullptr)
{
    history.reserve(6);
    LocalPlayer & player = ld.myPlayer();

    player.army.setAllSelected();

    setVisible(true);
    moveFlag.setVisible(false);
    buttons.setVisible(false);

    delayCombatResult = Settings::presentationDelay(jobject.getInteger("delay:combatresult", 600));
    JsonButton* button = nullptr;

    buttonOrder = buttons.findIds("but_order");
    if(buttonOrder)
    {
	buttonOrder->setVisible(true);
	buttonOrder->setAction(Action::ButtonOrder);
	buttonOrder->setDisabled(true);
    }

    button = buttons.findIds("but_info");
    if(button)
    {
	button->setVisible(true);
	button->setAction(Action::ButtonInfo);
    }

    button = buttons.findIds("but_menu");
    if(button)
    {
	button->setVisible(true);
	button->setAction(Action::ButtonMenu);
    }

    buttonDone = buttons.findIds("but_done");
    if(buttonDone)
    {
	buttonDone->setVisible(true);
	buttonDone->setAction(Action::ButtonDone);
	buttonDone->setDisabled(true);
    }

    buttonUndo = buttons.findIds("but_undo");
    if(buttonUndo)
    {
	buttonUndo->setVisible(true);
	buttonUndo->setAction(Action::ButtonUndo);
	buttonUndo->setDisabled(true);
    }

    buttonDismiss = buttons.findIds("but_dismiss");
    if(buttonDismiss)
    {
	buttonDismiss->setVisible(true);
	buttonDismiss->setAction(Action::ButtonDismiss);
	buttonDismiss->setDisabled(true);
    }

    selectedClan.reset();

#ifdef BUILD_DEBUG
    debugLand = Land::TowerOf4Winds;
#endif
}

void AdventurePartScreen::renderLabel(void)
{
    const FontRender & frs = GameTheme::fontRender(defaultFont);
    renderText(frs, orderSource.isValid() ? _("Select destination") : _("Movement Phase"),
	       defaultColor, viewMapPos, AlignCenter);
}

bool AdventurePartScreen::submitHumanAction(const ClientMessage & action)
{
    ActionRejection rejection;
    if(GameData::client2Adventure(myAvatar, action, actions, &rejection)) return true;

    showActionRejection(rejection);
    return false;
}

void AdventurePartScreen::showActionRejection(const ActionRejection & rejection)
{
    MessageBox(_("Action Rejected"), GameData::actionRejectionMessage(rejection),
               *this, false).exec();
}

void AdventurePartScreen::updateCommandButtons(void)
{
    const bool hasSelected = selectedLand.isValid() &&
	0 < ld.myPlayer().army.partySelected(selectedLand).size();
    const bool canDismiss = ld.yourTurn() && selectedClan == ld.myPlayer().clan && hasSelected;
    const bool canOrder = ld.yourTurn() && selectedLand.isValid() &&
	isAllowMoveFlag(GameData::landInfo(selectedLand));

    if(buttonDismiss) buttonDismiss->setDisabled(!canDismiss);
    if(buttonOrder) buttonOrder->setDisabled(!orderSource.isValid() && !canOrder);
}

bool AdventurePartScreen::isAllowMoveFlag(const LandInfo & info) const
{
    return !orderSource.isValid() && MapScreenBase::isAllowMoveFlag(info);
}

bool AdventurePartScreen::commitPendingOrders(void)
{
    if(history.empty()) return true;

    bool accepted = true;
    for(const CreatureMoved & creatureMoved : history)
    {
        if(!submitHumanAction(ClientUnitMoved(creatureMoved.first, creatureMoved.second)))
        {
            accepted = false;
            break;
        }
    }

    history.clear();
    MapScreenBase::ld = GameData::toLocalData(myAvatar);
    selectedCreature.reset();
    affectedSpells.setVisible(false);
    bar1.reset();
    bar2.reset();
    updateCommandButtons();
    return accepted;
}

void AdventurePartScreen::cancelOrderMode(bool redraw)
{
    if(!orderSource.isValid()) return;

    orderSource.reset();
    clearBattleForecast();
    updateCommandButtons();
    if(redraw) renderWindow();
}

bool AdventurePartScreen::moveSelectedParty(const Land & fromLand, const Land & toLand)
{
    LocalPlayer & player = ld.myPlayer();
    const std::vector<int> moved = player.army.moveSelectedCreatures(fromLand, toLand);
    if(moved.empty()) return false;

    for(const int unit : moved)
	history.emplace_back(unit, toLand);

    // A split leaves the remaining source party ready for the next command.
    player.army.partySetAllSelected(fromLand);
    selectedCreature.reset();
    affectedSpells.setVisible(false);
    bar1.reset();
    bar2.reset();

    if(buttonUndo) buttonUndo->setDisabled(false);
    playSound("snddrop");

    const LandInfo & landInfo = GameData::landInfo(toLand);
    if(player.clan != landInfo.clan)
	DisplayScene::pushEvent(nullptr, LandPolygonCombatStatus, const_cast<LandInfo*>(& landInfo));

    DisplayScene::pushEvent(nullptr, LandPolygonFlagAnimationReInit, nullptr);
    pushEventAction(MapScreenSelectLand, this, const_cast<LandInfo*>(& landInfo));
    updateCommandButtons();
    return true;
}

bool AdventurePartScreen::userEvent(int act, void* data)
{
    if(act == LandPolygonClickLeft || act == LandPolygonClickRight)
        clearBattleForecast();

    if(orderSource.isValid() && (act == LandPolygonClickLeft || act == LandPolygonClickRight))
    {
	if(act == LandPolygonClickRight || !data)
	{
	    cancelOrderMode();
	    return true;
	}

	const Land target = static_cast<LandInfo*>(data)->id;
	if(target == orderSource)
	{
	    cancelOrderMode();
	    return true;
	}

	const Land source = orderSource;
	if(moveSelectedParty(source, target))
	    cancelOrderMode();
	else
	{
	    updateBattleForecast(source, target);
	    MessageBox(_("Order"), _("Selected creatures cannot move to this territory."), *this, false).exec();
	    renderWindow();
	}
	return true;
    }

    bool requestClaim = false;
    if(act == LandPolygonClickLeft && data && ld.yourTurn())
    {
	auto landInfo = static_cast<LandInfo*>(data);
	// The first click selects the territory; a second click claims its deed.
	requestClaim = selectedLand == landInfo->id && GameData::canClaimLand(ld.myPlayer(), landInfo->id);
    }

    if(MapScreenBase::userEvent(act, data))
    {
	if(requestClaim)
	{
	    auto landInfo = static_cast<LandInfo*>(data);
	    submitHumanAction(ClientLandClaim(landInfo->id));
	}
	updateCommandButtons();
	return true;
    }

    switch(act)
    {
	case Action::ButtonDone:	actionButtonDone(); return true;
	case Action::ButtonOrder:	actionButtonOrder(); return true;
	case Action::ButtonDismiss:	actionButtonDismiss(); return true;
	case Action::ButtonInfo:	actionButtonInfo(); return true;
	case Action::ButtonUndo:	actionButtonUndo(); return true;
	case Action::ButtonMenu:	actionButtonMenu(); return true;
        default: break;
    }

    if(ld.yourTurn())
    {
#ifdef BUILD_DEBUG
	if(act == AdventureTurnShowConsole)
	{
            if(! console)
                console.reset(new DebugConsole(Size(width(), 200), GameTheme::fontRender("console"), *this));
            console->setVisible(true);
            return true;
	}
        else
#endif
	if(act == AdventureTurnCreatureSelect)
	{
	    clearBattleForecast();
	    auto bcr = data ? static_cast<BattleCreature*>(data) : nullptr;
	    if(bcr)
	    {
		bcr->switchSelected();
		updateCommandButtons();
	    }
	    return true;
	}
	else
	if(act == AdventureTurnMoveStart)
	{
	    cancelOrderMode(false);
	    clearBattleForecast();
	    moveFlag.setLand(selectedLand);
	    moveFlag.setVisible(true);
	    playSound("sndtake");
	    return true;
	}
	else
	if(act == AdventureTurnMoveStop)
	{
	    clearBattleForecast();
	    moveFlag.setVisible(false);

	    moveSelectedParty(moveFlag.fromLand(), selectedLand);
	    return true;
	}
	else
	if(act == LandPolygonFocus)
	{
	    if(data)
	    {
		auto landInfo = static_cast<LandInfo*>(data);
		const Land origin = moveFlag.isVisible() ? moveFlag.fromLand() :
		    (orderSource.isValid() ? orderSource : selectedLand);
		updateBattleForecast(origin, landInfo->id);
		renderWindow();

		if(moveFlag.isVisible())
		    pushEventAction(MapScreenSelectLand, this, data);
	    }

	    return true;
	}
    }

    return false;
}

void AdventurePartScreen::actionButtonOrder(void)
{
    if(orderSource.isValid())
    {
	cancelOrderMode();
	return;
    }

    if(!ld.yourTurn() || !selectedLand.isValid()) return;
    const LandInfo & info = GameData::landInfo(selectedLand);
    if(!isAllowMoveFlag(info)) return;

    orderSource = selectedLand;
    clearBattleForecast();
    updateCommandButtons();
    playSound("sndtake");
    renderWindow();
}

void AdventurePartScreen::actionButtonMenu(void)
{
    ScopedAdventureTickPause pause(*this);
    const int choice = InGameMenuDialog(*this).exec();
    if(choice == InGameMenuResult::Cancel)
    {
        renderWindow();
        return;
    }

    JsonObject gui;
    gui.addString("type", "AdventurePart");
    if(choice == InGameMenuResult::SaveGame)
    {
        SaveGameAs(gui, *this, [this]()
        {
            if(commitPendingOrders()) return true;
            return false;
        });
        renderWindow();
        return;
    }

    if(!commitPendingOrders())
    {
        renderWindow();
        return;
    }

    if(!GameData::saveGame(gui))
    {
        MessageBox(_("Error"), _("Unable to save game."), *this, false).exec();
        renderWindow();
        return;
    }

    setResultCode(Menu::MainMenu);
    setVisible(false);
}

bool AdventurePartScreen::mouseReleaseEvent(const ButtonEvent & be)
{
    if(be.isButtonLeft() && moveFlag.isVisible())
        return userEvent(AdventureTurnMoveStop, nullptr);

    return false;
}

void AdventurePartScreen::actionButtonDismiss(void)
{
    LocalPlayer & player = ld.myPlayer();
    BattleCreatures bcrs = player.army.partySelected(selectedLand);

    if(bcrs.size())
    {
        for(auto & bcr : bcrs)
	{
	    DEBUG("remove creature: " << (*bcr).toString());
	    history.emplace_back((*bcr).battleUnit(), Land::None);
            player.army.remove(*bcr);
	}

	player.army.partySetAllSelected(selectedLand);

	cancelOrderMode(false);
	updateCommandButtons();
	if(buttonUndo) buttonUndo->setDisabled(false);

        selectedCreature.reset();
	affectedSpells.setVisible(false);
	bar1.reset();
	bar2.reset();

	// broadscast event
	DisplayScene::pushEvent(nullptr, LandPolygonFlagAnimationReInit, nullptr);
    }
}

void AdventurePartScreen::actionButtonInfo(void)
{
    MapStatusDialog(ld, *this).exec();
}

void AdventurePartScreen::actionButtonUndo(void)
{
    cancelOrderMode(false);
    if(!submitHumanAction(ClientAdventureUndo())) return;
    MapScreenBase::ld = GameData::toLocalData(myAvatar);
    const LandInfo & landInfo = GameData::landInfo(selectedLand);

    selectedLand.reset();
    selectedCreature.reset();
    selectedClan = ld.myPlayer().clan;
    affectedSpells.setVisible(false);

    bar1.reset();
    bar2.reset();

    DEBUG("all changes revert");
    history.clear();

    updateCommandButtons();
    if(buttonUndo) buttonUndo->setDisabled(true);

    // broadscast event
    DisplayScene::pushEvent(nullptr, LandPolygonFlagAnimationReInit, nullptr);
    DisplayScene::pushEvent(nullptr, LandPolygonCombatStatusReset, nullptr);
    // set selected land event
    pushEventAction(MapScreenSelectLand, this, const_cast<LandInfo*>(& landInfo));
}

void AdventurePartScreen::actionButtonDone(void)
{
    cancelOrderMode(false);
    if(buttonDone) buttonDone->setDisabled(true);

    if(!commitPendingOrders())
    {
        if(buttonDone) buttonDone->setDisabled(false);
        renderWindow();
        return;
    }

    if(!submitHumanAction(ClientBattleReady()) && buttonDone)
        buttonDone->setDisabled(false);
}

void AdventurePartScreen::tickEvent(u32 ms)
{
    if(allowTickEvent && tt.check(ms, 100))
    {
        GameData::adventure2Client(myAvatar, actions);
        bool redraw = false;
        bool processedAction = false;
        int lastActionType = Action::None;

        while(actions.size())
        {
            auto action = actions.front();
            actions.pop_front();
            processedAction = true;
            lastActionType = action.type();
            CrashReport::breadcrumb(std::string("Adventure action type=")
                .append(String::number(action.type())).append(" payload=").append(action.toString()));

            switch(action.type())
            {
		case Action::AdventureTurn:
		    redraw = actionAdventureTurn(action);
		    break;

		case Action::AdventureMoves:
		    redraw = actionAdventureMoves(action);
		    break;

		case Action::AdventureClaim:
		    redraw = actionAdventureClaim(action);
		    break;

		case Action::AdventureBattleChoice:
		    redraw = actionAdventureBattleChoice(action);
		    break;

		case Action::AdventureCombat:
		    redraw = actionAdventureCombat(action);
		    break;

		case Action::AdventureEnd:
		    redraw = actionAdventureEnd(action);
		    break;

                default:
                    ERROR("unknown action: " << action.type());
                    break;
            }
        }

        if(processedAction)
        {
            JsonObject gui;
            gui.addString("type", "AdventureRecovery");
            const std::string reason = std::string("adventure-action-") + String::number(lastActionType);
            if(!GameData::saveRecovery(gui, reason))
                ERROR("recovery checkpoint failed: " << reason);
        }

        if(redraw) renderWindow();
    }
}

bool AdventurePartScreen::actionAdventureTurn(const ActionMessage & v)
{
    auto action = static_cast<const AdventureTurn &>(v);

    ld.currentWind = action.currentWind();
    const RemotePlayer & player = ld.playerOfWind(ld.currentWind);

    if(!ld.yourTurn()) cancelOrderMode(false);
    updateCommandButtons();

    if(ld.yourTurn() && buttonDone)
    {
	buttonDone->setDisabled(false);
    }

    pushEventAction(AdventureTurnPlayer, this, const_cast<RemotePlayer*>(& player));
    DEBUG("current wind: " << ld.currentWind.toString() << ", person: " << player.name());

    return true;
}

bool AdventurePartScreen::actionAdventureMoves(const ActionMessage & v)
{
    auto action = static_cast<const AdventureMoves &>(v);
    ld.currentWind = action.currentWind();

    if(! ld.yourTurn())
    {
	// read raw info
	MapScreenBase::ld = GameData::toLocalData(myAvatar);
    }

    DEBUG("current wind: " << ld.currentWind.toString() << ", uid: " << action.unit() << ", to land: " << action.land().toString());
    return true;
}

bool AdventurePartScreen::actionAdventureClaim(const ActionMessage & v)
{
    auto action = static_cast<const AdventureClaim &>(v);
    ld = GameData::toLocalData(myAvatar);
    ld.currentWind = action.currentWind();

    const LandInfo & info = GameData::landInfo(action.land());
    DisplayScene::pushEvent(nullptr, LandPolygonFlagAnimationReInit, const_cast<LandInfo*>(& info));
    pushEventAction(MapScreenSelectLand, this, const_cast<LandInfo*>(& info));

    DEBUG("land owner changed: " << action.land().toString() << ", previous owner: " <<
          action.previousOwner().toString() << ", owner: " << action.owner().toString() <<
          ", cost: " << action.cost() << ", reverted: " << action.reverted());
    return true;
}

bool AdventurePartScreen::actionAdventureBattleChoice(const ActionMessage & v)
{
    const AdventureBattleChoice & action = static_cast<const AdventureBattleChoice &>(v);
    ld.currentWind = action.currentWind();
    const BattleLegend legend = action.legend();
    if(myAvatar != legend.attacker)
    {
	ERROR("battle choice delivered to non-attacker: " << myAvatar.toString());
	return false;
    }

    allowTickEvent = false;
    animationsDisabled(true);

#ifndef SWE_DISABLE_AUDIO
    if(Settings::music()) Music::reset();
#endif

    BattleChoiceDialog dialog(legend, action.phase(), action.strikes(),
                              action.actors(), action.targets(),
	                      action.recommendedActor(), action.recommendedTarget(), *this);
    dialog.exec();

    const ClientBattleChoice choice(dialog.actor(), dialog.target(), dialog.autoResolve());
    const bool accepted = submitHumanAction(choice);
    if(!accepted)
	ERROR("interactive battle choice was rejected");

    allowTickEvent = true;
    animationsDisabled(false);
    return true;
}

bool AdventurePartScreen::actionAdventureCombat(const ActionMessage & v)
{
    auto action = static_cast<const AdventureCombat &>(v);
    ld.currentWind = action.currentWind();

    allowTickEvent = false;
    animationsDisabled(true);

    const BattleLegend legend = action.legend();
    const BattleStrikes strikes = action.strikes();
    const bool visibleBattle = myAvatar == legend.attacker || myAvatar == legend.defender;

#ifndef SWE_DISABLE_AUDIO
    if(visibleBattle && Settings::music()) Music::reset();
#endif

    DEBUG("current wind: " << ld.currentWind.toString() << ", " << "legend: " << legend.toString() << ", " << "strikes: " << strikes.toString());
    playSound("sndfight");

    // draw fight sprite
    const LandInfo & landInfo = GameData::landInfo(legend.land());

    // Land combatLand = ;
    // Clan combatClan = legend.town.previousClan();

    if(visibleBattle)
    {
	// wait fightsnd
	playSoundWait();

	CombatScreenDialog dialog(legend, strikes, *this);
	dialog.exec();
    }
    else
    {
	// redraw AI combat status
	DisplayScene::pushEvent(nullptr, LandPolygonCombatStatus, const_cast<LandInfo*>(& landInfo));
	DisplayScene::handleEvents(200);

	// wait fightsnd
	playSoundWait();
    }

    // loss: remove party
    if(! legend.wins)
    {
	// remove attacker army from land
	RemotePlayer & remote = const_cast<RemotePlayer &>(ld.playerOfAvatar(legend.attacker));
	BattleArmy & attackers = remote.army;
	BattleParty* party = attackers.findParty(legend.land());

	if(party)
	{
	    party->dismiss();
	    attackers.shrinkEmpty();
	}
    }

    DisplayScene::pushEvent(nullptr, LandPolygonCombatStatusReset, const_cast<LandInfo*>(& landInfo));
    DisplayScene::pushEvent(nullptr, LandPolygonFlagAnimationReInit, const_cast<LandInfo*>(& landInfo));

    // wait scene events
    DisplayScene::handleEvents(delayCombatResult);

    allowTickEvent = true;
    animationsDisabled(false);

    if(visibleBattle) playMusic("map");

    return true;
}

bool AdventurePartScreen::actionAdventureEnd(const ActionMessage & v)
{
    auto action = static_cast<const AdventureEnd &>(v);

    DEBUG("goto next screen");

#ifdef BUILD_DEBUG
    // A visible AI takeover is intentionally scoped to one complete phase.
    GameData::setDeveloperAutoplay(myAvatar, false);
#endif

    // AdventureEnd is the stable boundary of a complete Mahjong/adventure
    // round. Keep Continue current without exposing diagnostic checkpoints as
    // ordinary player saves.
    JsonObject gui;
    gui.addString("type", "AdventurePart");

    if(!GameData::saveGame(gui))
        ERROR("round autosave failed");

    setResultCode(Menu::BattleSummaryPart);
    setVisible(false);

    return true;
}

bool AdventurePartScreen::keyPressEvent(const KeySym & ks)
{
    if(ks.keycode() == SWE::Key::ESCAPE)
    {
	if(orderSource.isValid()) cancelOrderMode();
	else actionButtonMenu();
	return true;
    }

#ifdef BUILD_DEBUG
    if(ks.keycode() == SWE::Key::F9)
    {
        const DeveloperTools::Command command = DeveloperTools::showPanel(*this, myAvatar);
        if(command == DeveloperTools::Command::ToggleAutoplay)
        {
            GameData::setDeveloperAutoplay(myAvatar,
                !GameData::developerAutoplay(myAvatar));
            return true;
        }
        if(command != DeveloperTools::Command::Cancel)
        {
            const DeveloperTools::FastForwardResult result =
                DeveloperTools::fastForward(command, myAvatar);
            if(!result.success)
            {
                MessageTop(_("Developer Tools"), result.error, *this);
                return true;
            }
            setResultCode(result.menu);
            setVisible(false);
            return true;
        }
        return true;
    }

    if(ks.keycode() == SWE::Key::BACKQUOTE)
    {
        pushEventAction(AdventureTurnShowConsole, this, nullptr);
        return true;
    }
#endif

    return MapScreenBase::keyPressEvent(ks);
}

#ifdef BUILD_DEBUG
#include <sstream>
SWE::StringList commandSplitSpace(std::string str)
{
    std::istringstream is(str);
    SWE::StringList res;

    std::copy(std::istream_iterator<std::string>(is),
            std::istream_iterator<std::string>(), std::back_inserter(res));

    return res;
}

bool AdventurePartScreen::actionDebugCommandLands(void)
{
    std::string line;

    for(auto id : lands_all)
    {
        auto name = Land(id).toString();
        if(line.size() + name.size() + 4 > console->cols() - 2)
        {
            line.append(", ");
            console->contentLinesAppend(line);
            line.assign(name);
        }
        else
        if(line.size())
            line.append(", ").append(name);
        else
            line.assign(name);
    }

    console->contentLinesAppend(line);
    return true;
}

bool AdventurePartScreen::actionDebugCommandLand(const std::string & argv)
{
    StringList res;

    for(auto id : lands_all)
    {
        auto name = Land(id).toString();
        if(0 == name.compare(0, argv.size(), argv))
            res.push_back(name);
    }

    if(res.empty())
    {
        console->contentLinesAppend(SWE::StringFormat("error: unknown land id: %1").arg(argv));
        return false;
    }
    else
    if(1 == res.size())
    {
        debugLand = Land(res.front());
        console->contentLinesAppend(SWE::StringFormat("land %1").arg(res.front()));
    }
    else
    {
        console->contentLinesAppend(SWE::StringFormat("possible values: %1").arg(res.join(", ")));
        return false;
    }

    return true;
}

bool AdventurePartScreen::actionDebugCommandParty(void)
{
    if(debugLand.isTowerWinds())
    {
        for(auto clan : clans_all)
        {
            BattleArmy & army = GameData::getBattleArmy(clan);
            const BattleParty* party = army.findPartyConst(debugLand);

            if(party)
            {
                auto list = console->frs()->splitStringWidth(party->toString(), console->sym2gfx(TermSize(console->cols() - 2, 1)).w);
                for(auto & str : list)
                    console->contentLinesAppend(str);
            }
        }
    }
    else
    {
        const LandInfo & land = GameData::landInfo(debugLand);
        BattleArmy & army = GameData::getBattleArmy(land.clan);
        const BattleParty* party = army.findPartyConst(debugLand);

        if(party)
        {
            auto list = console->frs()->splitStringWidth(party->toString(), console->sym2gfx(TermSize(console->cols() - 2, 1)).w);
            for(auto & str : list)
                console->contentLinesAppend(str);
        }
        else
        {
            console->contentLinesAppend("party not found");
        }
    }

    return true;
}

bool AdventurePartScreen::actionDebugCommand(const std::string & str)
{
    auto words = commandSplitSpace(str);

    if(! words.empty())
    {
        auto & cmd = words.front();

        if(cmd == "help")
        {
            console->contentLinesAppend("lands - show all lands");
            console->contentLinesAppend("land <name> - set/show current land");
            return true;
        }
        else
        if(cmd == "lands")
        {
            console->contentLinesAppend(str);
            return actionDebugCommandLands();
        }
        else
        if(cmd == "land")
        {
            if(2 == words.size())
            {
                return actionDebugCommandLand(words.back());
            }
            else
            {
                console->contentLinesAppend(SWE::StringFormat("current land: %1").arg(debugLand.toString()));
                return true;
            }
        }
        else
        if(cmd == "party")
        {
            return actionDebugCommandParty();
        }
        else
        {
            console->contentLinesAppend(SWE::StringFormat("%1: command not found").arg(cmd));
        }
    }

    return false;
}

/* DebugConsole */
bool DebugConsole::actionCommand(const std::string & cmd)
{
    auto win = dynamic_cast<AdventurePartScreen*>(parent());
    if(win) win->actionDebugCommand(cmd);

    return CommandConsole::actionCommand(cmd);
}
#endif
