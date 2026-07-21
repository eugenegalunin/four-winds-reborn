/***************************************************************************
 *   Four Winds Reborn main menu                                           *
 ***************************************************************************/

#ifndef _FWR_MAINMENU_
#define _FWR_MAINMENU_

#include <vector>

#include "gamedata.h"

class MainMenuScreen : public JsonWindow
{
    struct Entry
    {
        std::string label;
        std::string note;
        Rect        area;
        int         target;
        bool        enabled;
    };

    std::vector<Entry> entries;
    bool               hasSave;
    int                selected;

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

    void                addEntry(const std::string &, const std::string &, int, bool);
    bool                activateSelected(void);
    bool                selectNext(int);

protected:
    bool                keyPressEvent(const KeySym &) override;
    bool                mouseClickEvent(const ButtonsEvent &) override;
    bool                mouseMotionEvent(const Point &, u32) override;

public:
    MainMenuScreen(bool, bool, bool, bool);

    void                renderWindow(void) override;
};

#endif
