/***************************************************************************
 *   Four Winds Reborn settings menu                                       *
 ***************************************************************************/

#ifndef _FWR_SETTINGSMENU_
#define _FWR_SETTINGSMENU_

#include <vector>

#include "gamedata.h"

class SettingsMenuScreen : public JsonWindow
{
    enum EntryKind
    {
        Language,
        GameSpeed,
        MusicEnabled,
        SoundEnabled,
        GuardianVoices,
        DisplayMode,
        Apply,
        Back
    };

    struct Entry
    {
        EntryKind kind;
        Rect      area;
    };

    std::vector<Entry> entries;
    int                selected;
    std::string        language;
    std::string        gameSpeed;
    bool               music;
    bool               sound;
    bool               guardianVoices;
    bool               fullscreen;

    Rect               leftPanel;
    Rect               rightPanel;
    Rect               menuArea;
    int                itemHeight;
    int                itemGap;

    std::string        titleFont;
    std::string        menuFont;
    std::string        smallFont;

    Color              panelColor;
    Color              panelBorderColor;
    Color              dividerColor;
    Color              titleColor;
    Color              textColor;
    Color              mutedColor;
    Color              buttonColor;
    Color              buttonSelectedColor;
    Color              buttonBorderColor;
    Color              buttonSelectedBorderColor;

    void               addEntry(EntryKind);
    std::string        entryLabel(EntryKind) const;
    std::string        entryValue(EntryKind) const;
    bool               activateSelected(void);
    bool               selectNext(int);

protected:
    bool               keyPressEvent(const KeySym &) override;
    bool               mouseClickEvent(const ButtonsEvent &) override;
    bool               mouseMotionEvent(const Point &, u32) override;

public:
    SettingsMenuScreen();

    void               renderWindow(void) override;
};

#endif
