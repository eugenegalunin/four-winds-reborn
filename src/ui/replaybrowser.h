/***************************************************************************
 *   Four Winds Reborn replay library                                     *
 ***************************************************************************/

#ifndef _FWR_REPLAYBROWSER_
#define _FWR_REPLAYBROWSER_

#include <vector>

#include "jsongui.h"
#include "replayfiles.h"

class ReplayBrowserScreen : public JsonWindow
{
    struct Entry
    {
        ReplayFiles::Info info;
        std::string       label;
        std::string       summary;
        std::string       detail;
        Rect              area;
    };

    std::vector<Entry> entries;
    int                selected;
    int                firstVisible;
    int                lastClickedEntry;
    u32                lastClickAt;

    Rect               panelArea;
    Rect               listArea;
    Rect               backArea;
    Rect               importArea;
    Rect               exportArea;
    Rect               deleteArea;
    Rect               detailsArea;
    Rect               playArea;
    int                itemHeight;
    int                itemGap;

    std::string        titleFont;
    std::string        entryFont;
    std::string        smallFont;

    Color              panelBorderColor;
    Color              titleColor;
    Color              textColor;
    Color              mutedColor;
    Color              errorColor;
    Color              itemColor;
    Color              itemSelectedColor;
    Color              itemBorderColor;
    Color              selectedBorderColor;

    int                visibleCount(void) const;
    void               reloadEntries(const std::string & selectedPath = {});
    void               layoutVisibleEntries(void);
    bool               selectNext(int);
    bool               importReplay(void);
    bool               exportSelected(void);
    bool               showDetails(void);
    bool               playSelected(void);
    bool               deleteSelected(void);
    bool               actionBack(void);

protected:
    bool               keyPressEvent(const KeySym &) override;
    bool               mouseClickEvent(const ButtonsEvent &) override;
    bool               mouseMotionEvent(const Point &, u32) override;
    bool               scrollUpEvent(void) override { return selectNext(-1); }
    bool               scrollDownEvent(void) override { return selectNext(1); }

public:
    ReplayBrowserScreen();
    void               renderWindow(void) override;
};

#endif
