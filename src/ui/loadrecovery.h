/***************************************************************************
 *   Four Winds Reborn save and recovery selection                         *
 ***************************************************************************/

#ifndef _FWR_LOADRECOVERY_
#define _FWR_LOADRECOVERY_

#include <vector>

#include "gamedata.h"

class LoadRecoveryScreen : public JsonWindow
{
    enum class EntryKind
    {
        Autosave,
        Manual,
        Recovery
    };

    struct Entry
    {
        std::string label;
        std::string summary;
        std::string detail;
        std::string path;
        Rect        area;
        EntryKind   kind = EntryKind::Autosave;
        bool        valid = false;
    };

    std::vector<Entry> saveEntries;
    std::vector<Entry> recoveryEntries;
    int                selected;
    int                firstVisible;
    int                lastClickedEntry;
    u32                lastClickAt;
    bool               lastClickRecovery;
    std::string        selectedFile;
    bool               restoreRecovery;
    bool               showRecovery;
    bool               loading;

    Rect               panelArea;
    Rect               listArea;
    Rect               backArea;
    Rect               modeArea;
    Rect               deleteArea;
    Rect               loadArea;
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

    std::vector<Entry> &       activeEntries(void);
    const std::vector<Entry> & activeEntries(void) const;
    int                        visibleCount(void) const;
    void                       selectInitial(void);
    void                       layoutVisibleEntries(void);
    bool                       activateSelected(void);
    bool                       deleteSelected(void);
    bool                       actionBack(void);
    bool                       toggleMode(void);
    bool                       selectNext(int);

protected:
    bool                keyPressEvent(const KeySym &) override;
    bool                mouseClickEvent(const ButtonsEvent &) override;
    bool                mouseMotionEvent(const Point &, u32) override;
    bool                scrollUpEvent(void) override { return selectNext(-1); }
    bool                scrollDownEvent(void) override { return selectNext(1); }

public:
    explicit LoadRecoveryScreen(const std::string &);

    void                renderWindow(void) override;
    const std::string &  loadFile(void) const { return selectedFile; }
    bool                shouldPromoteRecovery(void) const { return restoreRecovery; }
};

#endif
