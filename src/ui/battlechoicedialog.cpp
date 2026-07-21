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

#include <algorithm>

#include "gametheme.h"
#include "battle.h"
#include "battleuitext.h"
#include "battlechoicedialog.h"

BattleChoiceDialog::BattleChoiceDialog(const BattleLegend & leg,
                                       const std::string & battlePhase,
                                       const BattleStrikes & battleHistory,
                                       const std::vector<int> & legalActors,
                                       const std::vector<int> & legalTargets,
                                       int preferredActor, int preferredTarget,
                                       int currentChoiceNumber, int currentChoiceCount,
                                       Window & win)
    : DialogWindow("dialog_combat.json", win), legend(leg), phase(battlePhase),
      history(battleHistory), actors(legalActors),
      targets(legalTargets), recommendedActor(preferredActor),
      recommendedTarget(preferredTarget), selectedActor(-1), selectedTarget(-1),
      choiceNumber(currentChoiceNumber), choiceCount(currentChoiceCount),
      resolveAutomatically(false),
      acceptedFlashVisible(battlePhase != "opening_leader"),
      acceptedFlashStarted(0), acceptedFlashDuration(0)
{
    background = GameTheme::jsonSprite(jobject, "background");
    const Texture lifeStatus = GameTheme::jsonSprite(jobject, "sprite:lifestatus");
    const Texture cellFill = GameTheme::jsonSprite(jobject, "sprite:cellfill");

    choiceColor = GameTheme::jsonColor(jobject, "color:choice");
    targetColor = GameTheme::jsonColor(jobject, "color:target");
    selectedColor = GameTheme::jsonColor(jobject, "color:selected");
    recommendedColor = GameTheme::jsonColor(jobject, "color:recommended");
    acceptedColor = GameTheme::jsonColor(jobject, "color:accepted");
    damageColors.first = GameTheme::jsonColor(jobject, "color:alive");
    damageColors.second = GameTheme::jsonColor(jobject, "color:damage");
    hintFont = jobject.getString("font:choice", "dejavus18");
    statusFont = jobject.getString("font:choice_status", "dejavus14");
    hintPos = GameTheme::jsonPoint(jobject, "offset:choice");
    autoArea = GameTheme::jsonRect(jobject, "area:auto");
    autoTextPos = GameTheme::jsonPoint(jobject, "offset:auto");

    const AvatarInfo & avatar1Info = GameData::avatarInfo(legend.attacker);
    const AvatarInfo & avatar2Info = GameData::avatarInfo(legend.defender);
    name1 = avatar1Info.name;
    name2 = avatar2Info.name;
    spritePort1.setTexture(GameTheme::texture(avatar1Info.portrait));
    spritePort1.setPosition(GameTheme::jsonPoint(jobject, "offset:port1"));
    spritePort2.setTexture(GameTheme::texture(avatar2Info.portrait));
    spritePort2.setPosition(GameTheme::jsonPoint(jobject, "offset:port2"));
    center1Pos = GameTheme::jsonPoint(jobject, "offset:center1");
    center2Pos = GameTheme::jsonPoint(jobject, "offset:center2");
    phasePos = GameTheme::jsonPoint(jobject, "offset:choice_phase");
    timelinePos = GameTheme::jsonPoint(jobject, "offset:choice_timeline");
    previewPos = GameTheme::jsonPoint(jobject, "offset:choice_preview");
    statusPos = GameTheme::jsonPoint(jobject, "offset:choice_status");
    acceptedFlashDuration = static_cast<u32>(
        std::max(0, jobject.getInteger("delay:choice_status", 900)));

    units.push_back(CombatUnit(&legend.town, GameTheme::jsonPoint(jobject, "offset:tower"),
                               cellFill, lifeStatus));
    units.push_back(GameTheme::jsonPoint(jobject, "offset:party11"),
                    GameTheme::jsonPoint(jobject, "offset:party12"),
                    GameTheme::jsonPoint(jobject, "offset:party13"),
                    cellFill, lifeStatus, legend.attackers.toBattleCreatures());
    units.push_back(GameTheme::jsonPoint(jobject, "offset:party21"),
                    GameTheme::jsonPoint(jobject, "offset:party22"),
                    GameTheme::jsonPoint(jobject, "offset:party23"),
                    cellFill, lifeStatus, legend.defenders.toBattleCreatures());

    renderWindow();
    setVisible(true);
}

bool BattleChoiceDialog::contains(const std::vector<int> & values, int value) const
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

void BattleChoiceDialog::renderWindow(void)
{
    for(CombatUnit & unit : units)
        unit.render(*this, damageColors, false);

    renderTexture(background, Point(0, 0));
    renderTexture(spritePort1);
    renderTexture(spritePort2);

    const FontRender & defaultFont = GameTheme::fontRender(font);
    renderText(defaultFont, name1, textColor, center1Pos, AlignCenter, AlignCenter);
    renderText(defaultFont, name2, textColor, center2Pos, AlignCenter, AlignCenter);

    renderText(GameTheme::fontRender(hintFont),
               std::string(_("Phase: ")).append(
                   BattleUiText::choicePhase(phase, choiceNumber, choiceCount)), textColor,
               phasePos, AlignCenter, AlignCenter);
    if(!history.empty())
    {
        const BattleStrike & recent = history.back();
        const std::string event = std::string(_("Last event: "))
            .append(BattleUiText::strikePhase(recent.type)).append(" - ")
            .append(BattleUiText::strikeSummary(recent));
        renderText(GameTheme::fontRender(hintFont), event, textColor,
                   timelinePos, AlignCenter, AlignCenter);
    }
    if(acceptedFlashVisible)
        renderText(GameTheme::fontRender(statusFont), _("Choice accepted"), acceptedColor,
                   statusPos, AlignCenter, AlignCenter);

    for(const CombatUnit & unit : units)
    {
        if(!unit.battle || !unit.battle->isAlive()) continue;
        if(unit.uid() == selectedActor)
            renderRect(selectedColor, unit.area());
        else if(unit.uid() == recommendedActor)
            renderRect(recommendedColor, unit.area());
        else if(contains(actors, unit.uid()))
            renderRect(choiceColor, unit.area());

        if(unit.uid() == selectedTarget)
            renderRect(selectedColor, unit.area());
        else if(unit.uid() == recommendedTarget)
            renderRect(recommendedColor, unit.area());
        else if(contains(targets, unit.uid()))
            renderRect(targetColor, unit.area());
    }

    if(phase != "opening_leader")
    {
        const int previewActor = 0 < selectedActor ? selectedActor : recommendedActor;
        int previewTarget = 0 < selectedTarget ? selectedTarget : recommendedTarget;
        if(!contains(targets, previewTarget) && !targets.empty()) previewTarget = targets.front();
        const Battle::AttackPreview preview = Battle::previewAttack(
            legend.attackers, legend.town, &legend.defenders, previewActor, previewTarget,
            phase == "attacker_ranged");
        if(preview.valid)
        {
            std::string previewText = std::string(_("Preview: "))
                .append(std::to_string(preview.damage)).append(" ").append(_("damage"));
            if(preview.hitChance < 100)
                previewText.append(" (").append(std::to_string(preview.hitChance)).append("% ")
                    .append(_("hit")).append(")");
            if(0 < preview.mightyChance)
                previewText.append("; ").append(_("Mighty Blow: "))
                    .append(std::to_string(preview.mightyDamage)).append(" @ ")
                    .append(std::to_string(preview.mightyChance)).append("%");
            renderText(GameTheme::fontRender(hintFont), previewText, textColor,
                       previewPos, AlignCenter, AlignCenter);
        }
    }

    std::string hint;
    if(phase == "opening_leader")
        hint = _("Choose the opening leader. Enter: recommended. Esc: Auto Resolve.");
    else if(phase == "attacker_ranged")
        hint = selectedActor < 0 ?
            _("Choose a missile attacker. Enter: recommended. Esc: Auto Resolve.") :
            _("Choose a missile target. Enter: recommended. Esc: Auto Resolve.");
    else
        hint = selectedActor < 0 ?
            _("Choose an attacker. Enter: recommended. Esc: Auto Resolve.") :
            _("Choose a target. Enter: recommended. Esc: Auto Resolve.");
    renderText(GameTheme::fontRender(hintFont), hint, textColor,
               hintPos, AlignCenter, AlignCenter);
    renderRect(recommendedColor, autoArea);
    renderText(GameTheme::fontRender(hintFont), _("Auto Resolve"), textColor,
               autoTextPos, AlignCenter, AlignCenter);
}

void BattleChoiceDialog::tickEvent(u32 ms)
{
    if(!acceptedFlashVisible || !acceptedFlashDuration) return;
    if(!acceptedFlashStarted)
    {
        acceptedFlashStarted = ms;
        return;
    }
    if(ms - acceptedFlashStarted >= acceptedFlashDuration)
    {
        acceptedFlashVisible = false;
        renderWindow();
    }
}

bool BattleChoiceDialog::mouseClickEvent(const ButtonsEvent & coords)
{
    if(!coords.isButtonLeft()) return true;

    if(coords.isClick(autoArea))
    {
        resolveAutomatically = true;
        actionDialogClose();
        return true;
    }

    for(const CombatUnit & unit : units)
    {
        if(!unit.battle || !coords.isClick(unit.area())) continue;
        if(contains(actors, unit.uid()))
        {
            selectedActor = unit.uid();
            selectedTarget = -1;
            if(phase == "opening_leader")
            {
                actionDialogClose();
                return true;
            }
            renderWindow();
            return true;
        }
        if(0 < selectedActor && contains(targets, unit.uid()))
        {
            selectedTarget = unit.uid();
            actionDialogClose();
            return true;
        }
    }
    return true;
}

bool BattleChoiceDialog::keyPressEvent(const KeySym & key)
{
    if(key.keycode() == Key::ESCAPE)
    {
        resolveAutomatically = true;
        actionDialogClose();
        return true;
    }
    if(key.keycode() == Key::RETURN)
    {
        selectedActor = recommendedActor;
        selectedTarget = recommendedTarget;
        actionDialogClose();
        return true;
    }
    return true;
}
