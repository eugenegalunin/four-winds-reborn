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

#ifndef _RWNA_BATTLE_CHOICE_DIALOG_
#define _RWNA_BATTLE_CHOICE_DIALOG_

#include "dialogs.h"

class BattleChoiceDialog : public DialogWindow
{
    BattleLegend        legend;
    std::string         phase;
    BattleStrikes       history;
    std::vector<int>    actors;
    std::vector<int>    targets;
    int                 recommendedActor;
    int                 recommendedTarget;
    int                 selectedActor;
    int                 selectedTarget;
    int                 choiceNumber;
    int                 choiceCount;
    bool                resolveAutomatically;
    bool                acceptedFlashVisible;
    u32                 acceptedFlashStarted;
    u32                 acceptedFlashDuration;

    Texture             background;
    Color               choiceColor;
    Color               targetColor;
    Color               selectedColor;
    Color               recommendedColor;
    Color               acceptedColor;
    std::pair<Color, Color> damageColors;
    std::string         hintFont;
    std::string         statusFont;
    Point               hintPos;
    Point               phasePos;
    Point               timelinePos;
    Point               previewPos;
    Point               statusPos;
    Rect                autoArea;
    Point               autoTextPos;
    std::string         name1;
    std::string         name2;
    Sprite              spritePort1;
    Sprite              spritePort2;
    Point               center1Pos;
    Point               center2Pos;
    CombatUnits         units;

    bool contains(const std::vector<int> &, int) const;

protected:
    bool                mouseClickEvent(const ButtonsEvent &) override;
    bool                keyPressEvent(const KeySym &) override;
    void                tickEvent(u32) override;

public:
    BattleChoiceDialog(const BattleLegend &, const std::string & phase,
                       const BattleStrikes & history,
                       const std::vector<int> & actors,
                       const std::vector<int> & targets, int recommendedActor,
                       int recommendedTarget, int choiceNumber, int choiceCount,
                       Window &);

    void                renderWindow(void) override;
    int                 actor(void) const { return selectedActor; }
    int                 target(void) const { return selectedTarget; }
    bool                autoResolve(void) const { return resolveAutomatically; }
};

#endif
