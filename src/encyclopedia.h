/***************************************************************************
 *   Four Winds Reborn encyclopedia                                       *
 ***************************************************************************/

#ifndef _FWR_ENCYCLOPEDIA_
#define _FWR_ENCYCLOPEDIA_

#include <vector>

#include "jsongui.h"

class EncyclopediaScreen : public JsonWindow
{
    enum EntryKind { Article, AvatarEntry, CreatureEntry, SpellEntry };
    enum FocusColumn { CategoriesColumn, EntriesColumn, ArticleColumn };

    struct Entry
    {
        EntryKind   kind;
        std::string id;
        std::string title;
        std::string subtitle;
        std::string body;
        std::string image;
        Avatar      avatar;
        Creature    creature;
        Spell       spell;
    };

    struct Category
    {
        std::string        id;
        std::string        title;
        std::vector<Entry> entries;
    };

    std::vector<Category> categories;
    int                   categoryIndex;
    int                   entryIndex;
    int                   categoryOffset;
    int                   entryOffset;
    int                   textOffset;
    FocusColumn           focusColumn;
    std::vector<std::string> wrappedText;

    Rect                  categoryArea;
    Rect                  entryArea;
    Rect                  contentArea;
    Rect                  backArea;
    int                   categoryRowHeight;
    int                   entryRowHeight;

    std::string           titleFont;
    std::string           headingFont;
    std::string           bodyFont;
    std::string           smallFont;

    Color                 panelColor;
    Color                 panelAltColor;
    Color                 panelBorderColor;
    Color                 titleColor;
    Color                 textColor;
    Color                 mutedColor;
    Color                 selectedColor;
    Color                 selectedBorderColor;

    void                  loadArticles(void);
    void                  addGameDataCategories(void);
    void                  rebuildArticle(void);
    void                  keepSelectionVisible(void);
    void                  selectCategory(int);
    void                  selectEntry(int);
    bool                  moveSelection(int);
    bool                  moveFocus(int);
    bool                  scrollArticle(int);
    bool                  scrollFocusedPane(int);
    int                   articleHeaderHeight(const Entry*) const;
    int                   articleVisualHeight(const Entry*) const;
    Stones                articleRunes(const Entry*) const;
    int                   articleRuneHeight(const Entry*) const;
    int                   renderRuneFormula(const Entry*, int, const FontRender &);
    void                  renderBattleGuideDiagram(int, const FontRender &, const FontRender &);
    bool                  activateSelection(void);
    int                   visibleCategoryRows(void) const;
    int                   visibleEntryRows(void) const;
    int                   visibleTextRows(void) const;

    const Category*       currentCategory(void) const;
    const Entry*          currentEntry(void) const;

protected:
    bool                  keyPressEvent(const KeySym &) override;
    bool                  mouseClickEvent(const ButtonsEvent &) override;
    bool                  mouseMotionEvent(const Point &, u32) override;
    bool                  scrollUpEvent(void) override;
    bool                  scrollDownEvent(void) override;

public:
    EncyclopediaScreen();

    void                  renderWindow(void) override;
};

#endif
