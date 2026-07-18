/***************************************************************************
 *   Four Winds Reborn encyclopedia                                       *
 ***************************************************************************/

#include <algorithm>

#include "encyclopedia.h"
#include "gametheme.h"
#include "runewars.h"
#include "settings.h"

namespace
{
bool isRussianLanguage(void)
{
    const std::string language = String::toLower(Settings::language());
    return language == "ru" || language == "russian" ||
        language.rfind("ru_", 0) == 0 || language.rfind("ru-", 0) == 0;
}

std::string localized(const JsonObject & object, const char* key)
{
    const std::string suffix = isRussianLanguage() ? "_ru" : "_en";
    std::string value = object.getString(std::string(key).append(suffix));
    if(value.empty()) value = object.getString(std::string(key).append("_en"));
    return value;
}

std::string localizedParagraphs(const JsonObject & object, const char* key)
{
    const std::string suffix = isRussianLanguage() ? "_ru" : "_en";
    StringList paragraphs = object.getStdList<std::string>(std::string(key).append(suffix));
    if(paragraphs.empty())
        paragraphs = object.getStdList<std::string>(std::string(key).append("_en"));
    return paragraphs.join("\n\n");
}

std::string stripColorMarkup(std::string text)
{
    for(size_t begin = text.find("[color:"); begin != std::string::npos;
        begin = text.find("[color:", begin))
    {
        const size_t end = text.find(']', begin);
        if(end == std::string::npos) break;
        text.erase(begin, end - begin + 1);
    }
    return text;
}

std::string avatarStyle(const std::string & profile)
{
    if(profile == "aggressive") return _("Aggressive");
    if(profile == "economic") return _("Economic");
    if(profile == "control") return _("Control");
    return _("Balanced");
}

std::string creatureRealm(const Creature & creature)
{
    if(creature() <= Creature::MazRa) return _("Skull Realm");
    if(creature() <= Creature::KingDrago) return _("Sword Realm");
    if(creature() <= Creature::Shanahan) return _("Magic Number Realm");
    if(creature() <= Creature::RedDragon) return _("The Three Dragons");
    if(creature() <= Creature::WaterElemental) return _("Elementals of the Four Winds");
    return _("Special Creatures");
}

std::string spellTarget(const SpellTarget & target)
{
    switch(target())
    {
        case SpellTarget::Friendly: return _("Friendly creature");
        case SpellTarget::Enemy: return _("Enemy creature");
        case SpellTarget::Any: return _("Any creature");
        case SpellTarget::Friendly | SpellTarget::Party: return _("Friendly party");
        case SpellTarget::Enemy | SpellTarget::Party: return _("Enemy party");
        case SpellTarget::Land: return _("Territory or party");
        case SpellTarget::MyPlayer: return _("Caster");
        case SpellTarget::OtherPlayer: return _("Another wizard");
        case SpellTarget::AllPlayers: return _("All wizards");
        default: break;
    }
    return _("Special effect");
}

std::string spellRunes(const Stones & stones)
{
    if(stones.empty()) return _("No fixed rune formula");

    StringList names;
    for(const Stone & stone : stones)
        names << GameData::stoneInfo(stone).name;
    return names.join(" + ");
}
}

EncyclopediaScreen::EncyclopediaScreen() :
    JsonWindow("screen_encyclopedia.json", nullptr), categoryIndex(0), entryIndex(0),
    categoryOffset(0), entryOffset(0), textOffset(0), focusColumn(CategoriesColumn)
{
    categoryArea = JsonUnpack::rect(jobject, "area:categories", Rect(24, 104, 174, 570));
    entryArea = JsonUnpack::rect(jobject, "area:entries", Rect(210, 104, 236, 570));
    contentArea = JsonUnpack::rect(jobject, "area:content", Rect(458, 104, 542, 570));
    backArea = JsonUnpack::rect(jobject, "area:back", Rect(24, 700, 174, 46));
    categoryRowHeight = jobject.getInteger("row:category", 48);
    entryRowHeight = jobject.getInteger("row:entry", 36);

    titleFont = jobject.getString("font:title", "dejavus32");
    headingFont = jobject.getString("font:heading", "dejavus22");
    bodyFont = jobject.getString("font:body", "dejavus14");
    smallFont = jobject.getString("font:small", "dejavus12");

    panelColor = GameTheme::jsonColor(jobject, "color:panel");
    panelAltColor = GameTheme::jsonColor(jobject, "color:panel_alt");
    panelBorderColor = GameTheme::jsonColor(jobject, "color:panel_border");
    titleColor = GameTheme::jsonColor(jobject, "color:title");
    textColor = GameTheme::jsonColor(jobject, "color:text");
    mutedColor = GameTheme::jsonColor(jobject, "color:muted");
    selectedColor = GameTheme::jsonColor(jobject, "color:selected");
    selectedBorderColor = GameTheme::jsonColor(jobject, "color:selected_border");

    loadArticles();
    addGameDataCategories();
    rebuildArticle();

    setMouseTrack(true);
    setKeyHandle(true);
    setVisible(true);
}

void EncyclopediaScreen::loadArticles(void)
{
    JsonContent content = GameTheme::jsonResource("encyclopedia.json");
    if(! content.isObject()) return;

    // Keep the object alive while walking child arrays. JsonObject owns the
    // views returned by getArray/getObject, so taking them from a temporary
    // silently discarded the entire archival half of the encyclopedia.
    const JsonObject root = content.toObject();
    const JsonArray* encodedCategories = root.getArray("categories");
    if(! encodedCategories) return;

    for(int categoryNo = 0; categoryNo < encodedCategories->size(); ++categoryNo)
    {
        const JsonObject* encodedCategory = encodedCategories->getObject(categoryNo);
        if(! encodedCategory) continue;
        if(! encodedCategory->getBoolean("show_in_game", true)) continue;

        Category category;
        category.id = encodedCategory->getString("id");
        category.title = localized(*encodedCategory, "title");

        if(const JsonArray* encodedEntries = encodedCategory->getArray("entries"))
        {
            for(int entryNo = 0; entryNo < encodedEntries->size(); ++entryNo)
            {
                const JsonObject* encodedEntry = encodedEntries->getObject(entryNo);
                if(! encodedEntry) continue;

                Entry entry;
                entry.kind = Article;
                entry.id = encodedEntry->getString("id");
                entry.title = localized(*encodedEntry, "title");
                entry.subtitle = localized(*encodedEntry, "subtitle");
                entry.body = localizedParagraphs(*encodedEntry, "body");
                entry.image = encodedEntry->getString("image");
                category.entries.push_back(entry);
            }
        }

        if(! category.id.empty()) categories.push_back(category);
    }
}

void EncyclopediaScreen::addGameDataCategories(void)
{
    Category wizards;
    wizards.id = "wizards";
    wizards.title = _("Wizards");
    for(const Avatar::avatar_t avatarId : avatars_all)
    {
        const Avatar avatar(avatarId);
        const AvatarInfo & info = GameData::avatarInfo(avatar);
        Entry entry;
        entry.kind = AvatarEntry;
        entry.title = info.name;
        entry.subtitle = info.dignity;
        entry.image = info.portrait;
        entry.avatar = avatar;
        wizards.entries.push_back(entry);
    }

    Category creatures;
    creatures.id = "creatures";
    creatures.title = _("Denizens");
    for(int value = Creature::SkeletonHorde; value <= Creature::Chameleon; ++value)
    {
        const Creature creature(static_cast<Creature::creature_t>(value));
        const CreatureInfo & info = GameData::creatureInfo(creature);
        Entry entry;
        entry.kind = CreatureEntry;
        entry.title = info.name;
        entry.subtitle = creatureRealm(creature);
        entry.image = info.image2;
        entry.creature = creature;
        creatures.entries.push_back(entry);
    }

    Category spells;
    spells.id = "spells";
    spells.title = _("Spells");
    for(int value = Spell::Smoke; value <= Spell::MassDispel; ++value)
    {
        // These three records are implementation variants selected by
        // Mystical Fountain, not independent spells a player can cast.
        if(value == Spell::MysticalFountainAttack ||
           value == Spell::MysticalFountainRange ||
           value == Spell::MysticalFountainLoyalty)
            continue;

        const Spell spell(static_cast<Spell::spell_t>(value));
        const SpellInfo & info = GameData::spellInfo(spell);
        Entry entry;
        entry.kind = SpellEntry;
        entry.title = info.name;
        entry.subtitle = spellTarget(info.target);
        entry.image = info.image;
        entry.spell = spell;
        spells.entries.push_back(entry);
    }

    const auto insertBefore = [&](const std::string & before, Category category)
    {
        auto pos = std::find_if(categories.begin(), categories.end(),
            [&](const Category & current){ return current.id == before; });
        categories.insert(pos, std::move(category));
    };
    insertBefore("rules", std::move(wizards));
    insertBefore("rules", std::move(creatures));
    insertBefore("rules", std::move(spells));
}

const EncyclopediaScreen::Category* EncyclopediaScreen::currentCategory(void) const
{
    return 0 <= categoryIndex && categoryIndex < static_cast<int>(categories.size()) ?
        &categories[categoryIndex] : nullptr;
}

const EncyclopediaScreen::Entry* EncyclopediaScreen::currentEntry(void) const
{
    const Category* category = currentCategory();
    return category && 0 <= entryIndex && entryIndex < static_cast<int>(category->entries.size()) ?
        &category->entries[entryIndex] : nullptr;
}

void EncyclopediaScreen::rebuildArticle(void)
{
    wrappedText.clear();
    textOffset = 0;
    const Entry* entry = currentEntry();
    if(! entry) return;

    std::string body = entry->body;
    if(entry->kind == AvatarEntry)
    {
        const AvatarInfo & info = GameData::avatarInfo(entry->avatar);
        body = StringFormat(_("Clans: %1\nStrategy: %2"))
            .arg(info.toStringClans()).arg(avatarStyle(info.aiProfile));
        if(info.ability.isValid())
        {
            const AbilityInfo & ability = GameData::abilityInfo(info.ability);
            body.append("\n\n").append(ability.name).append(": ")
                .append(String::replace(ability.description, "%1", info.name));
        }
        body.append("\n\n").append(String::replace(stripColorMarkup(info.description), "%1", info.name));
    }
    else if(entry->kind == CreatureEntry)
    {
        const CreatureInfo & info = GameData::creatureInfo(entry->creature);
        body = StringFormat(_("Cost: %1 spell points\nAttack: %2   Missile: %3   Defense: %4\nLoyalty: %5   Movement: %6"))
            .arg(info.cost).arg(info.stat.attack).arg(info.stat.ranger).arg(info.stat.defense)
            .arg(info.stat.loyalty).arg(info.stat.move);
        if(info.fly) body.append("\n").append(_("Flying creature"));
        for(const Speciality & speciality : info.specials.toList())
        {
            const SpecialityInfo & special = GameData::specialityInfo(speciality);
            body.append("\n\n").append(special.name);
            if(! special.description.empty()) body.append(": ").append(special.description);
        }
        body.append("\n\n").append(info.description);
    }
    else if(entry->kind == SpellEntry)
    {
        const SpellInfo & info = GameData::spellInfo(entry->spell);
        body = StringFormat(_("Cost: %1 spell points\nTarget: %2\nRune formula: %3"))
            .arg(info.cost).arg(spellTarget(info.target)).arg(spellRunes(info.stones));

        const std::string effect = info.effectDescription();
        if(! effect.empty() && effect != info.name)
            body.append("\n").append(_("Effect: ")).append(effect);
        if(info.isCurse()) body.append("\n").append(_("Curse"));
        else if(info.persistent) body.append("\n").append(_("Persistent enchantment"));
        if(0 < info.duration())
            body.append("\n").append(StringFormat(_("Duration: %1 turns")).arg(info.duration()));
        if(! info.description.empty()) body.append("\n\n").append(info.description);
    }

    const FontRender & font = GameTheme::fontRender(bodyFont);
    const StringList paragraphs = String::split(body, '\n');
    for(const std::string & paragraph : paragraphs)
    {
        if(paragraph.empty())
        {
            wrappedText.push_back(std::string());
            continue;
        }
        const StringList lines = font.splitStringWidth(paragraph, contentArea.w - 34);
        wrappedText.insert(wrappedText.end(), lines.begin(), lines.end());
    }
    setDirty(true);
}

int EncyclopediaScreen::visibleCategoryRows(void) const
{
    return std::max(1, categoryArea.h / categoryRowHeight);
}

int EncyclopediaScreen::visibleEntryRows(void) const
{
    return std::max(1, entryArea.h / entryRowHeight);
}

int EncyclopediaScreen::visibleTextRows(void) const
{
    const Entry* entry = currentEntry();
    const int top = contentArea.y + articleHeaderHeight(entry) + articleVisualHeight(entry);
    return std::max(1, (contentArea.y + contentArea.h - 22 - top) /
        GameTheme::fontRender(bodyFont).lineSkipHeight());
}

int EncyclopediaScreen::articleHeaderHeight(const Entry* entry) const
{
    if(! entry) return 70;

    const FontRender & heading = GameTheme::fontRender(headingFont);
    const FontRender & small = GameTheme::fontRender(smallFont);
    const int titleLines = std::max(1, std::min(2, static_cast<int>(
        heading.splitStringWidth(entry->title, contentArea.w - 32).size())));
    const int subtitleLines = entry->subtitle.empty() ? 0 : std::max(1, std::min(2,
        static_cast<int>(small.splitStringWidth(entry->subtitle, contentArea.w - 32).size())));
    return 10 + titleLines * heading.lineSkipHeight() +
        (subtitleLines ? 5 + subtitleLines * small.lineSkipHeight() : 0) + 10;
}

int EncyclopediaScreen::articleVisualHeight(const Entry* entry) const
{
    if(! entry) return 0;
    if(entry->id == "battle_guide") return 204;

    int height = 0;
    if(! entry->image.empty())
    {
        const Texture & image = GameTheme::texture(entry->image);
        if(image.isValid()) height += image.height() + 18;
    }

    return height + articleRuneHeight(entry);
}

Stones EncyclopediaScreen::articleRunes(const Entry* entry) const
{
    if(! entry) return Stones();
    if(entry->kind == CreatureEntry)
        return GameData::creatureInfo(entry->creature).stones;
    if(entry->kind == SpellEntry)
        return GameData::spellInfo(entry->spell).stones;
    return Stones();
}

int EncyclopediaScreen::articleRuneHeight(const Entry* entry) const
{
    if(! entry || (entry->kind != CreatureEntry && entry->kind != SpellEntry)) return 0;

    const FontRender & small = GameTheme::fontRender(smallFont);
    const Stones runes = articleRunes(entry);
    int runeHeight = small.lineSkipHeight();
    for(const Stone & rune : runes)
    {
        const Texture & image = GameTheme::texture(GameData::stoneInfo(rune).large);
        if(image.isValid()) runeHeight = std::max(runeHeight, image.height());
    }
    return small.lineSkipHeight() + 6 + runeHeight + 16;
}

int EncyclopediaScreen::renderRuneFormula(const Entry* entry, int top, const FontRender & small)
{
    const int height = articleRuneHeight(entry);
    if(height <= 0) return 0;

    const Stones runes = articleRunes(entry);
    const int center = contentArea.x + contentArea.w / 2;
    renderText(small, _("Rune formula:"), mutedColor, Point(center, top), AlignCenter);
    top += small.lineSkipHeight() + 6;

    if(runes.empty())
    {
        renderText(small, _("No fixed rune formula"), textColor, Point(center, top), AlignCenter);
        return height;
    }

    int totalWidth = 0;
    for(const Stone & rune : runes)
    {
        const Texture & image = GameTheme::texture(GameData::stoneInfo(rune).large);
        if(image.isValid()) totalWidth += image.width() + 4;
    }
    if(0 < totalWidth) totalWidth -= 4;

    int x = center - totalWidth / 2;
    for(const Stone & rune : runes)
    {
        const Texture & image = GameTheme::texture(GameData::stoneInfo(rune).large);
        if(! image.isValid()) continue;
        renderTexture(image, Point(x, top));
        x += image.width() + 4;
    }
    return height;
}

void EncyclopediaScreen::renderBattleGuideDiagram(int top, const FontRender & body,
                                                    const FontRender & small)
{
    const Rect diagram(contentArea.x + 16, top, contentArea.w - 32, 186);
    renderColor(panelAltColor, diagram);
    renderRect(panelBorderColor, diagram);

    const int left = diagram.x + 82;
    const int right = diagram.x + diagram.w - 82;
    const int middle = diagram.x + diagram.w / 2;
    renderText(small, _("ATTACKERS"), mutedColor,
               Point(left, diagram.y + 10), AlignCenter);
    renderText(small, _("DEFENDERS + LAND"), mutedColor,
               Point(right, diagram.y + 10), AlignCenter);

    const Texture & wraith = GameTheme::texture(
        GameData::creatureInfo(Creature(Creature::Wraith)).image2);
    const Texture & durlock = GameTheme::texture(
        GameData::creatureInfo(Creature(Creature::Durlock)).image2);
    const Texture & defender = GameTheme::texture(
        GameData::creatureInfo(Creature(Creature::AdventureParty)).image2);
    const Texture & town = GameTheme::texture(GameData::clanInfo(Clan(Clan::Red)).town);

    if(wraith.isValid()) renderTexture(wraith, Point(diagram.x + 25, diagram.y + 56));
    if(durlock.isValid()) renderTexture(durlock, Point(diagram.x + 67, diagram.y + 56));
    if(town.isValid()) renderTexture(town, Point(diagram.x + diagram.w - 126, diagram.y + 67));
    if(defender.isValid())
        renderTexture(defender, Point(diagram.x + diagram.w - 82, diagram.y + 56));

    const int arrowLeft = diagram.x + 150;
    const int arrowRight = diagram.x + diagram.w - 150;
    const auto lineArrow = [&](int y, bool pointsLeft, bool pointsRight)
    {
        renderLine(selectedBorderColor, Point(arrowLeft, y), Point(arrowRight, y));
        if(pointsLeft)
        {
            renderLine(selectedBorderColor, Point(arrowLeft, y), Point(arrowLeft + 7, y - 4));
            renderLine(selectedBorderColor, Point(arrowLeft, y), Point(arrowLeft + 7, y + 4));
        }
        if(pointsRight)
        {
            renderLine(selectedBorderColor, Point(arrowRight, y), Point(arrowRight - 7, y - 4));
            renderLine(selectedBorderColor, Point(arrowRight, y), Point(arrowRight - 7, y + 4));
        }
    };

    lineArrow(diagram.y + 58, true, false);
    lineArrow(diagram.y + 96, true, true);
    lineArrow(diagram.y + 134, true, true);
    renderText(small, _("1. TERRITORY FIRE"), titleColor,
               Point(middle, diagram.y + 39), AlignCenter);
    renderText(small, _("2. MISSILE VOLLEY"), titleColor,
               Point(middle, diagram.y + 77), AlignCenter);
    renderText(small, _("3. MELEE"), titleColor,
               Point(middle, diagram.y + 115), AlignCenter);
    renderText(body, _("Loyalty 0 = defeated"), textColor,
               Point(middle, diagram.y + 157), AlignCenter);
}

void EncyclopediaScreen::keepSelectionVisible(void)
{
    const int categoriesVisible = visibleCategoryRows();
    if(categoryIndex < categoryOffset) categoryOffset = categoryIndex;
    if(categoryOffset + categoriesVisible <= categoryIndex)
        categoryOffset = categoryIndex - categoriesVisible + 1;

    const int entriesVisible = visibleEntryRows();
    if(entryIndex < entryOffset) entryOffset = entryIndex;
    if(entryOffset + entriesVisible <= entryIndex)
        entryOffset = entryIndex - entriesVisible + 1;
}

void EncyclopediaScreen::selectCategory(int index)
{
    if(categories.empty()) return;
    index = std::max(0, std::min(index, static_cast<int>(categories.size()) - 1));
    if(index == categoryIndex) return;
    categoryIndex = index;
    entryIndex = 0;
    entryOffset = 0;
    keepSelectionVisible();
    rebuildArticle();
}

void EncyclopediaScreen::selectEntry(int index)
{
    const Category* category = currentCategory();
    if(! category || category->entries.empty()) return;
    index = std::max(0, std::min(index, static_cast<int>(category->entries.size()) - 1));
    if(index == entryIndex) return;
    entryIndex = index;
    keepSelectionVisible();
    rebuildArticle();
}

bool EncyclopediaScreen::moveSelection(int direction)
{
    if(focusColumn == CategoriesColumn)
        selectCategory(categoryIndex + direction);
    else if(focusColumn == EntriesColumn)
        selectEntry(entryIndex + direction);
    else
        return scrollArticle(direction);
    renderWindow();
    return true;
}

bool EncyclopediaScreen::moveFocus(int direction)
{
    const int next = std::max(0, std::min(2, static_cast<int>(focusColumn) + direction));
    focusColumn = static_cast<FocusColumn>(next);
    renderWindow();
    return true;
}

bool EncyclopediaScreen::scrollArticle(int direction)
{
    const int maximum = std::max(0, static_cast<int>(wrappedText.size()) - visibleTextRows());
    const int next = std::max(0, std::min(maximum, textOffset + direction));
    if(next == textOffset) return false;
    textOffset = next;
    renderWindow();
    return true;
}

bool EncyclopediaScreen::scrollFocusedPane(int direction)
{
    if(focusColumn == CategoriesColumn)
    {
        selectCategory(categoryIndex + direction);
        renderWindow();
        return true;
    }
    if(focusColumn == EntriesColumn)
    {
        selectEntry(entryIndex + direction * 3);
        renderWindow();
        return true;
    }
    scrollArticle(direction * 3);
    return true;
}

bool EncyclopediaScreen::activateSelection(void)
{
    if(focusColumn == CategoriesColumn) focusColumn = EntriesColumn;
    else if(focusColumn == EntriesColumn) focusColumn = ArticleColumn;
    renderWindow();
    return true;
}

void EncyclopediaScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    renderColor(panelColor, categoryArea);
    renderColor(panelAltColor, entryArea);
    renderColor(panelColor, contentArea);
    renderRect(focusColumn == CategoriesColumn ? selectedBorderColor : panelBorderColor, categoryArea);
    renderRect(focusColumn == EntriesColumn ? selectedBorderColor : panelBorderColor, entryArea);
    renderRect(focusColumn == ArticleColumn ? selectedBorderColor : panelBorderColor, contentArea);

    const FontRender & title = GameTheme::fontRender(titleFont);
    const FontRender & heading = GameTheme::fontRender(headingFont);
    const FontRender & body = GameTheme::fontRender(bodyFont);
    const FontRender & small = GameTheme::fontRender(smallFont);

    const auto renderListScrollBar = [&](const Rect & area, int total, int visible, int offset)
    {
        const int maximum = std::max(0, total - visible);
        if(maximum <= 0) return;
        const Rect track(area.x + area.w - 8, area.y + 7, 3, area.h - 14);
        renderColor(mutedColor, track);
        const int cursorHeight = std::max(18, track.h * visible / total);
        const int cursorY = track.y + (track.h - cursorHeight) * offset / maximum;
        renderColor(selectedBorderColor, Rect(track.x, cursorY, track.w, cursorHeight));
    };

    renderText(title, _("ENCYCLOPEDIA"), titleColor, Point(width() / 2, 34), AlignCenter);
    renderText(small, _("THE OFFICIAL ARCANUM OF THE ISLE OF FOUR WINDS"), mutedColor,
               Point(width() / 2, 76), AlignCenter);

    const int categoryLast = std::min(static_cast<int>(categories.size()), categoryOffset + visibleCategoryRows());
    for(int index = categoryOffset; index < categoryLast; ++index)
    {
        const Rect row(categoryArea.x + 6,
                       categoryArea.y + 6 + (index - categoryOffset) * categoryRowHeight,
                       categoryArea.w - 12, categoryRowHeight - 6);
        if(index == categoryIndex)
        {
            renderColor(selectedColor, row);
            renderRect(selectedBorderColor, row);
        }
        const Color categoryColor = index == categoryIndex ? titleColor : textColor;
        if(body.stringSize(categories[index].title).w <= row.w - 20)
            renderText(body, categories[index].title, categoryColor,
                       Point(row.x + 10, row.y + row.h / 2), AlignLeft, AlignCenter);
        else
        {
            const StringList lines = small.splitStringWidth(categories[index].title, row.w - 20);
            const int count = std::min(2, static_cast<int>(lines.size()));
            int y = row.y + (row.h - count * small.lineSkipHeight()) / 2;
            auto line = lines.begin();
            for(int rendered = 0; rendered < count && line != lines.end();
                ++rendered, ++line, y += small.lineSkipHeight())
                renderText(small, *line, categoryColor, Point(row.x + 10, y));
        }
    }
    renderListScrollBar(categoryArea, static_cast<int>(categories.size()),
                        visibleCategoryRows(), categoryOffset);

    const Category* category = currentCategory();
    if(category)
    {
        const int entryLast = std::min(static_cast<int>(category->entries.size()), entryOffset + visibleEntryRows());
        for(int index = entryOffset; index < entryLast; ++index)
        {
            const Rect row(entryArea.x + 6,
                           entryArea.y + 6 + (index - entryOffset) * entryRowHeight,
                           entryArea.w - 12, entryRowHeight - 4);
            if(index == entryIndex)
            {
                renderColor(selectedColor, row);
                renderRect(selectedBorderColor, row);
            }
            const Color entryColor = index == entryIndex ? titleColor : textColor;
            if(body.stringSize(category->entries[index].title).w <= row.w - 20)
                renderText(body, category->entries[index].title, entryColor,
                           Point(row.x + 8, row.y + row.h / 2), AlignLeft, AlignCenter);
            else
            {
                const StringList lines = small.splitStringWidth(category->entries[index].title,
                                                                 row.w - 20);
                const int count = std::min(2, static_cast<int>(lines.size()));
                int y = row.y + (row.h - count * small.lineSkipHeight()) / 2;
                auto line = lines.begin();
                for(int rendered = 0; rendered < count && line != lines.end();
                    ++rendered, ++line, y += small.lineSkipHeight())
                    renderText(small, *line, entryColor, Point(row.x + 8, y));
            }
        }
        if(static_cast<int>(category->entries.size()) > visibleEntryRows())
        {
            renderListScrollBar(entryArea, static_cast<int>(category->entries.size()),
                                visibleEntryRows(), entryOffset);
            renderText(small, StringFormat("%1 / %2").arg(entryIndex + 1).arg(category->entries.size()),
                       mutedColor, Point(entryArea.x + entryArea.w - 10, entryArea.y + entryArea.h - 18), AlignRight);
        }
    }

    const Entry* entry = currentEntry();
    if(entry)
    {
        const int center = contentArea.x + contentArea.w / 2;
        int headerY = contentArea.y + 10;
        const StringList titleLines = heading.splitStringWidth(entry->title, contentArea.w - 32);
        auto titleLine = titleLines.begin();
        for(int rendered = 0; rendered < 2 && titleLine != titleLines.end();
            ++rendered, ++titleLine, headerY += heading.lineSkipHeight())
            renderText(heading, *titleLine, titleColor, Point(center, headerY), AlignCenter);

        if(! entry->subtitle.empty())
        {
            headerY += 5;
            const StringList subtitleLines = small.splitStringWidth(entry->subtitle,
                                                                     contentArea.w - 32);
            auto subtitleLine = subtitleLines.begin();
            for(int rendered = 0; rendered < 2 && subtitleLine != subtitleLines.end();
                ++rendered, ++subtitleLine, headerY += small.lineSkipHeight())
                renderText(small, *subtitleLine, mutedColor, Point(center, headerY), AlignCenter);
        }

        int textTop = contentArea.y + articleHeaderHeight(entry);
        if(entry->id == "battle_guide")
        {
            renderBattleGuideDiagram(textTop, body, small);
            textTop += articleVisualHeight(entry);
        }
        else if(! entry->image.empty())
        {
            const Texture & image = GameTheme::texture(entry->image);
            if(image.isValid())
            {
                renderTexture(image, Point(contentArea.x + (contentArea.w - image.width()) / 2, textTop));
                textTop += image.height() + 18;
            }
        }
        textTop += renderRuneFormula(entry, textTop, small);

        const int lineHeight = body.lineSkipHeight();
        const int rows = std::max(1, (contentArea.y + contentArea.h - 20 - textTop) / lineHeight);
        const int last = std::min(static_cast<int>(wrappedText.size()), textOffset + rows);
        int y = textTop;
        for(int index = textOffset; index < last; ++index, y += lineHeight)
            if(! wrappedText[index].empty())
                renderText(body, wrappedText[index], textColor, Point(contentArea.x + 16, y));

        const int maximum = std::max(0, static_cast<int>(wrappedText.size()) - rows);
        if(0 < maximum)
        {
            const Rect track(contentArea.x + contentArea.w - 10, textTop, 4,
                             contentArea.y + contentArea.h - 16 - textTop);
            renderColor(mutedColor, track);
            const int cursorHeight = std::max(18, track.h * rows / static_cast<int>(wrappedText.size()));
            const int cursorY = track.y + (track.h - cursorHeight) * textOffset / maximum;
            renderColor(selectedBorderColor, Rect(track.x, cursorY, track.w, cursorHeight));
        }
    }

    renderColor(focusColumn == CategoriesColumn ? selectedColor : panelColor, backArea);
    renderRect(panelBorderColor, backArea);
    renderText(body, _("Back"), textColor,
               Point(backArea.x + backArea.w / 2, backArea.y + backArea.h / 2), AlignCenter, AlignCenter);
    renderText(small, _("ARROWS: NAVIGATE   ENTER: OPEN   WHEEL / PGUP / PGDN: READ   ESC: BACK"),
               mutedColor, Point(1000, 729), AlignRight);
    renderText(small, _("Source: official Arcanium Rune War website, archived 2001"),
               mutedColor, Point(1000, 748), AlignRight);
}

bool EncyclopediaScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::UP: return moveSelection(-1);
        case Key::DOWN: return moveSelection(1);
        case Key::LEFT: return moveFocus(-1);
        case Key::RIGHT: return moveFocus(1);
        case Key::PAGEUP: return scrollArticle(-visibleTextRows());
        case Key::PAGEDOWN: return scrollArticle(visibleTextRows());
        case Key::HOME:
            if(focusColumn == CategoriesColumn) selectCategory(0);
            else if(focusColumn == EntriesColumn) selectEntry(0);
            else textOffset = 0;
            renderWindow();
            return true;
        case Key::END:
            if(focusColumn == CategoriesColumn) selectCategory(static_cast<int>(categories.size()) - 1);
            else if(focusColumn == EntriesColumn && currentCategory())
                selectEntry(static_cast<int>(currentCategory()->entries.size()) - 1);
            else textOffset = std::max(0, static_cast<int>(wrappedText.size()) - visibleTextRows());
            renderWindow();
            return true;
        case Key::RETURN:
        case Key::SPACE: return activateSelection();
        case Key::ESCAPE:
            setResultCode(Menu::MainMenu);
            setVisible(false);
            return true;
        default: break;
    }
    return false;
}

bool EncyclopediaScreen::mouseClickEvent(const ButtonsEvent & event)
{
    if(! event.isButtonLeft()) return false;
    const Point point = event.release().position();
    if(backArea & point)
    {
        setResultCode(Menu::MainMenu);
        setVisible(false);
        return true;
    }
    if(categoryArea & point)
    {
        const int index = categoryOffset + (point.y - categoryArea.y - 6) / categoryRowHeight;
        if(0 <= index && index < static_cast<int>(categories.size())) selectCategory(index);
        focusColumn = CategoriesColumn;
        renderWindow();
        return true;
    }
    if(entryArea & point)
    {
        const Category* category = currentCategory();
        const int index = entryOffset + (point.y - entryArea.y - 6) / entryRowHeight;
        if(category && 0 <= index && index < static_cast<int>(category->entries.size())) selectEntry(index);
        focusColumn = EntriesColumn;
        renderWindow();
        return true;
    }
    if(contentArea & point)
    {
        focusColumn = ArticleColumn;
        renderWindow();
        return true;
    }
    return false;
}

bool EncyclopediaScreen::mouseMotionEvent(const Point & point, u32 buttons)
{
    (void) buttons;
    if(categoryArea & point) focusColumn = CategoriesColumn;
    else if(entryArea & point) focusColumn = EntriesColumn;
    else if(contentArea & point) focusColumn = ArticleColumn;
    else return false;
    setDirty(true);
    return true;
}

bool EncyclopediaScreen::scrollUpEvent(void)
{
    return scrollFocusedPane(-1);
}

bool EncyclopediaScreen::scrollDownEvent(void)
{
    return scrollFocusedPane(1);
}
