/***************************************************************************
 *   Four Winds Reborn save and recovery selection                         *
 ***************************************************************************/

#include <algorithm>
#include <cctype>
#include <ctime>

#include "gametheme.h"
#include "dialogs.h"
#include "crashreport.h"
#include "loadrecovery.h"
#include "recovery.h"
#include "savegames.h"
#include "swe/swe_tools.h"

namespace
{
std::string gamePartName(int part)
{
    switch(part)
    {
        case Menu::MahjongPart: return _("Mahjong");
        case Menu::MahjongSummaryPart: return _("Mahjong Summary");
        case Menu::AdventurePart: return _("Adventure");
        case Menu::BattleSummaryPart: return _("Battle Summary");
        case Menu::GameSummaryPart: return _("Game Summary");
        case Menu::MahjongInitPart: return _("Between rounds");
        default: return _("Saved game");
    }
}

std::string formatTimestamp(long long epoch)
{
    if(epoch <= 0) return _("Unknown time");

    const std::time_t value = static_cast<std::time_t>(epoch);
    std::tm localTime{};
#if defined(_WIN32)
    if(localtime_s(&localTime, &value) != 0) return _("Unknown time");
#else
    if(!localtime_r(&value, &localTime)) return _("Unknown time");
#endif

    char buffer[32] = {};
    if(!std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime))
        return _("Unknown time");
    return buffer;
}

long long parseEpoch(const std::string & value)
{
    if(value.empty()) return 0;
    try { return std::stoll(value); }
    catch(...) { return 0; }
}

std::string displayValue(std::string value)
{
    std::replace(value.begin(), value.end(), '-', ' ');
    std::replace(value.begin(), value.end(), '_', ' ');
    if(!value.empty()) value.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(value.front())));
    return value;
}
}

LoadRecoveryScreen::LoadRecoveryScreen(const std::string & primarySave) :
    JsonWindow("screen_loadrecovery.json", nullptr), selected(-1), firstVisible(0),
    lastClickedEntry(-1), lastClickAt(0), lastClickRecovery(false),
    restoreRecovery(false), showRecovery(false), loading(false)
{
    panelArea = JsonUnpack::rect(jobject, "panel:area", Rect(132, 54, 760, 660));
    listArea = JsonUnpack::rect(jobject, "list:area", Rect(176, 146, 672, 382));
    backArea = JsonUnpack::rect(jobject, "button:back", Rect(176, 642, 150, 50));
    modeArea = JsonUnpack::rect(jobject, "button:mode", Rect(336, 642, 202, 50));
    deleteArea = JsonUnpack::rect(jobject, "button:delete", Rect(548, 642, 140, 50));
    loadArea = JsonUnpack::rect(jobject, "button:load", Rect(698, 642, 150, 50));
    itemHeight = jobject.getInteger("list:item_height", 82);
    itemGap = jobject.getInteger("list:item_gap", 12);

    titleFont = jobject.getString("font:title", "dejavus34");
    entryFont = jobject.getString("font:entry", "dejavus22");
    smallFont = jobject.getString("font:small", "terminus14");

    panelBorderColor = GameTheme::jsonColor(jobject, "color:panel_border");
    titleColor = GameTheme::jsonColor(jobject, "color:title");
    textColor = GameTheme::jsonColor(jobject, "color:text");
    mutedColor = GameTheme::jsonColor(jobject, "color:muted");
    errorColor = GameTheme::jsonColor(jobject, "color:error");
    itemColor = GameTheme::jsonColor(jobject, "color:item");
    itemSelectedColor = GameTheme::jsonColor(jobject, "color:item_selected");
    itemBorderColor = GameTheme::jsonColor(jobject, "color:item_border");
    selectedBorderColor = GameTheme::jsonColor(jobject, "color:selected_border");

    if(Systems::isFile(primarySave))
    {
        std::string saveData;
        std::string error;
        const bool valid = Recovery::validateSaveFile(primarySave, &error, &saveData);
        JsonObject saveState;
        if(valid) saveState = JsonContentString(saveData).toObject();

        Entry entry;
        entry.label = _("Autosave");
        entry.summary = valid ?
            StringFormat("%1 | %2: %3").arg(gamePartName(saveState.getInteger("gamepart")))
                .arg(_("Difficulty")).arg(displayValue(saveState.getString("ai:difficulty"))) :
            StringFormat("%1 | %2").arg(_("INVALID")).arg(error);
        entry.detail = valid ?
            std::string(StringFormat("%1 | %2").arg(formatTimestamp(parseEpoch(
                saveState.getString("save:savedAtEpoch")))).arg(_("Updated automatically"))) :
            std::string(_("The file will be left untouched"));
        entry.path = primarySave;
        entry.valid = valid;
        entry.kind = EntryKind::Autosave;
        saveEntries.push_back(std::move(entry));
    }

    for(const SaveGames::Info & save : SaveGames::inspect())
    {
        Entry entry;
        entry.label = save.name;
        entry.summary = save.valid ?
            StringFormat("%1 | %2 | %3: %4")
                .arg(gamePartName(save.gamePart)).arg(formatTimestamp(save.savedAtEpoch))
                .arg(_("Difficulty")).arg(displayValue(save.difficulty)) :
            StringFormat("%1 | %2").arg(_("INVALID")).arg(save.error);
        entry.detail = save.valid ? _("Named save") : _("The file can be deleted but not loaded");
        entry.path = save.path;
        entry.valid = save.valid;
        entry.kind = EntryKind::Manual;
        saveEntries.push_back(std::move(entry));
    }

    for(const Recovery::CheckpointInfo & checkpoint :
        Recovery::inspectCheckpoints(Recovery::defaultDirectory()))
    {
        Entry entry;
        entry.label = StringFormat(_("Emergency checkpoint %1")).arg(checkpoint.slot + 1);
        if(checkpoint.valid)
        {
            entry.summary = StringFormat("%1 | %2 | %3: %4")
                .arg(gamePartName(checkpoint.gamePart)).arg(formatTimestamp(checkpoint.savedAtEpoch))
                .arg(_("Difficulty")).arg(displayValue(checkpoint.aiDifficulty));
            entry.detail = StringFormat("%1 | %2")
                .arg(displayValue(checkpoint.reason))
                .arg(checkpoint.gameVersion.empty() ? _("Unknown version") : checkpoint.gameVersion);
        }
        else
        {
            entry.summary = StringFormat("%1 | %2").arg(_("INVALID")).arg(checkpoint.error);
            entry.detail = _("The checkpoint will be left untouched");
        }
        entry.path = checkpoint.saveFile;
        entry.valid = checkpoint.valid;
        entry.kind = EntryKind::Recovery;
        recoveryEntries.push_back(std::move(entry));
    }

    showRecovery = saveEntries.empty() && !recoveryEntries.empty();
    selectInitial();
    setMouseTrack(true);
    setKeyHandle(true);
    setVisible(true);
}

std::vector<LoadRecoveryScreen::Entry> & LoadRecoveryScreen::activeEntries(void)
{
    return showRecovery ? recoveryEntries : saveEntries;
}

const std::vector<LoadRecoveryScreen::Entry> & LoadRecoveryScreen::activeEntries(void) const
{
    return showRecovery ? recoveryEntries : saveEntries;
}

int LoadRecoveryScreen::visibleCount(void) const
{
    return std::max(1, (listArea.h + itemGap) / (itemHeight + itemGap));
}

void LoadRecoveryScreen::selectInitial(void)
{
    const std::vector<Entry> & entries = activeEntries();
    selected = -1;
    firstVisible = 0;
    lastClickedEntry = -1;
    lastClickAt = 0;
    for(size_t index = 0; index < entries.size(); ++index)
    {
        if(entries[index].valid)
        {
            selected = static_cast<int>(index);
            break;
        }
    }
    if(selected < 0 && !entries.empty()) selected = 0;
}

void LoadRecoveryScreen::layoutVisibleEntries(void)
{
    std::vector<Entry> & entries = activeEntries();
    for(Entry & entry : entries) entry.area = Rect();

    if(entries.empty()) return;
    const int count = visibleCount();
    if(selected < firstVisible) firstVisible = selected;
    if(firstVisible + count <= selected) firstVisible = selected - count + 1;
    firstVisible = std::max(0, std::min(firstVisible,
        std::max(0, static_cast<int>(entries.size()) - count)));

    const int end = std::min(static_cast<int>(entries.size()), firstVisible + count);
    for(int index = firstVisible; index < end; ++index)
        entries[index].area = Rect(listArea.x,
            listArea.y + (index - firstVisible) * (itemHeight + itemGap),
            listArea.w, itemHeight);
}

void LoadRecoveryScreen::renderWindow(void)
{
    JsonWindow::renderWindow();
    renderRect(panelBorderColor, panelArea);
    layoutVisibleEntries();

    const FontRender & title = GameTheme::fontRender(titleFont);
    const FontRender & entryFontRender = GameTheme::fontRender(entryFont);
    const FontRender & small = GameTheme::fontRender(smallFont);

    renderText(title, _("LOAD GAME"), titleColor,
               Point(width() / 2, panelArea.y + 25), AlignCenter);
    renderText(small, showRecovery ? _("EMERGENCY RECOVERY") : _("AUTOSAVE AND NAMED SAVES"),
               mutedColor, Point(width() / 2, panelArea.y + 68), AlignCenter);
    renderLine(panelBorderColor, Point(panelArea.x + 42, panelArea.y + 88),
               Point(panelArea.x + panelArea.w - 42, panelArea.y + 88));

    const std::vector<Entry> & entries = activeEntries();
    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        const Entry & entry = entries[index];
        const bool isSelected = selected == index;
        const Color labelColor = entry.valid ? (isSelected ? titleColor : textColor) : errorColor;

        renderColor(isSelected ? itemSelectedColor : itemColor, entry.area);
        renderRect(isSelected ? selectedBorderColor : itemBorderColor, entry.area);
        renderText(entryFontRender, entry.label, labelColor,
                   Point(entry.area.x + 18, entry.area.y + 8));
        renderText(small, entry.summary, entry.valid ? textColor : errorColor,
                   Point(entry.area.x + 18, entry.area.y + 43));
        renderText(small, entry.detail, mutedColor,
                   Point(entry.area.x + 18, entry.area.y + 62));
    }

    if(entries.empty())
        renderText(entryFontRender,
                   showRecovery ? _("No recovery checkpoint was found.") : _("No autosave or named save was found."),
                   mutedColor, Point(width() / 2, 340), AlignCenter);

    const bool selectedInRange = selected >= 0 && static_cast<size_t>(selected) < entries.size();
    const bool canLoad = selectedInRange && entries[selected].valid;
    const bool canDelete = selectedInRange && entries[selected].kind == EntryKind::Manual;
    const bool canSwitch = !recoveryEntries.empty() || showRecovery;

    auto drawButton = [&](const Rect & area, const std::string & label, bool enabled)
    {
        renderColor(enabled ? itemSelectedColor : itemColor, area);
        renderRect(enabled ? selectedBorderColor : itemBorderColor, area);
        renderText(entryFontRender, label, enabled ? titleColor : mutedColor,
                   Point(area.x + area.w / 2, area.y + area.h / 2), AlignCenter, AlignCenter);
    };
    drawButton(backArea, _("Back"), true);
    drawButton(modeArea, showRecovery ? _("Saves") : _("Recovery"), canSwitch);
    drawButton(deleteArea, _("Delete"), canDelete);
    drawButton(loadArea, loading ? _("Loading...") : _("Load"), canLoad && !loading);

    const std::string hint = showRecovery ?
        _("Double-click or use Load. Restoring recovery replaces the autosave.") :
        _("Double-click a valid save to load. Delete removes named saves only.");
    renderText(small, hint, mutedColor, Point(width() / 2, 612), AlignCenter);
}

bool LoadRecoveryScreen::activateSelected(void)
{
    std::vector<Entry> & entries = activeEntries();
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return true;
    const Entry & entry = entries[selected];
    if(!entry.valid || loading) return true;

    if(entry.kind == EntryKind::Recovery && !MessageBox(_("Restore Recovery"),
        _("Restore this emergency checkpoint as the current autosave? The existing autosave will be kept as a backup."),
        *this).exec())
    {
        renderWindow();
        return true;
    }

    playSound("button");
    selectedFile = entry.path;
    restoreRecovery = entry.kind == EntryKind::Recovery;
    loading = true;
    CrashReport::breadcrumb(std::string("Load selection type=")
        .append(restoreRecovery ? "recovery" : (entry.kind == EntryKind::Manual ? "manual" : "autosave"))
        .append(" file=").append(selectedFile));
    renderWindow();
    Display::renderPresent();
    setResultCode(Menu::GameLoadPart);
    setVisible(false);
    return true;
}

bool LoadRecoveryScreen::deleteSelected(void)
{
    lastClickedEntry = -1;
    lastClickAt = 0;
    std::vector<Entry> & entries = activeEntries();
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return true;
    const Entry entry = entries[selected];
    if(entry.kind != EntryKind::Manual) return true;

    if(!MessageBox(_("Delete Save"),
        StringFormat(_("Delete the named save '%1'? This cannot be undone from the game menu."))
            .arg(entry.label), *this).exec())
    {
        renderWindow();
        return true;
    }

    std::string error;
    if(!SaveGames::deleteManual(entry.path, &error))
    {
        MessageBox(_("Delete Save"),
                   StringFormat(_("The save could not be deleted: %1")).arg(error),
                   *this, false).exec();
        renderWindow();
        return true;
    }

    CrashReport::breadcrumb(std::string("Manual save deleted file=").append(entry.path));
    entries.erase(entries.begin() + selected);
    if(entries.empty()) selected = -1;
    else if(static_cast<size_t>(selected) >= entries.size()) selected = static_cast<int>(entries.size()) - 1;
    layoutVisibleEntries();
    renderWindow();
    return true;
}

bool LoadRecoveryScreen::actionBack(void)
{
    playSound("button");
    setResultCode(Menu::MainMenu);
    setVisible(false);
    return true;
}

bool LoadRecoveryScreen::toggleMode(void)
{
    if(recoveryEntries.empty() && !showRecovery) return true;
    showRecovery = !showRecovery;
    restoreRecovery = false;
    selectInitial();
    playSound("button");
    renderWindow();
    return true;
}

bool LoadRecoveryScreen::selectNext(int direction)
{
    std::vector<Entry> & entries = activeEntries();
    if(entries.empty()) return false;
    lastClickedEntry = -1;
    lastClickAt = 0;
    selected = (selected + direction + static_cast<int>(entries.size())) % static_cast<int>(entries.size());
    layoutVisibleEntries();
    renderWindow();
    return true;
}

bool LoadRecoveryScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::UP: return selectNext(-1);
        case Key::DOWN: return selectNext(1);
        case Key::LEFT:
        case Key::RIGHT: return toggleMode();
        case Key::DELETE: return deleteSelected();
        case Key::RETURN:
        case Key::SPACE: return activateSelected();
        case Key::ESCAPE: return actionBack();
        default: break;
    }
    return false;
}

bool LoadRecoveryScreen::mouseClickEvent(const ButtonsEvent & coords)
{
    if(!coords.isButtonLeft()) return false;
    if(coords.isClick(backArea))
    {
        lastClickedEntry = -1;
        return actionBack();
    }
    if(coords.isClick(modeArea))
    {
        lastClickedEntry = -1;
        return toggleMode();
    }
    if(coords.isClick(deleteArea)) return deleteSelected();
    if(coords.isClick(loadArea))
    {
        lastClickedEntry = -1;
        return activateSelected();
    }

    std::vector<Entry> & entries = activeEntries();
    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        if(coords.isClick(entries[index].area))
        {
            const u32 now = Tools::ticks();
            const bool doubleClick = lastClickedEntry == index &&
                lastClickRecovery == showRecovery && lastClickAt != 0 &&
                now - lastClickAt <= 500;
            selected = index;
            lastClickedEntry = index;
            lastClickAt = now;
            lastClickRecovery = showRecovery;
            if(doubleClick && entries[index].valid)
            {
                lastClickedEntry = -1;
                lastClickAt = 0;
                return activateSelected();
            }
            renderWindow();
            return true;
        }
    }
    lastClickedEntry = -1;
    lastClickAt = 0;
    return false;
}

bool LoadRecoveryScreen::mouseMotionEvent(const Point & pos, u32 buttons)
{
    (void) buttons;
    std::vector<Entry> & entries = activeEntries();
    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        if(entries[index].area & pos)
        {
            if(selected != index)
            {
                selected = index;
                renderWindow();
            }
            return true;
        }
    }
    return false;
}
