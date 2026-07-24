/***************************************************************************
 *   Four Winds Reborn replay library                                     *
 ***************************************************************************/

#include "replaybrowser.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>

#include "dialogs.h"
#include "filedialog.h"
#include "gametheme.h"
#include "replayviewer.h"
#include "swe/swe_systems.h"
#include "swe/swe_tools.h"

namespace
{
namespace fs = std::filesystem;

class ReplayPathDialog : public DialogWindow
{
    std::string path;
    std::string title;
    std::string action;
    Rect        inputArea;
    Rect        acceptArea;
    Rect        cancelArea;
    std::string titleFont;
    std::string inputFont;
    std::string smallFont;
    Color       inputColor;
    Color       inputBorderColor;
    Color       buttonColor;
    Color       buttonBorderColor;
    Color       mutedColor;

    void eraseLastCodepoint(void)
    {
        if(path.empty()) return;
        std::size_t position = path.size() - 1;
        while(position && (static_cast<unsigned char>(path[position]) & 0xC0) == 0x80)
            --position;
        path.erase(position);
    }

    bool accept(void)
    {
        if(path.empty()) return true;
        setResultCode(1);
        JsonWindow::playSound("button");
        actionDialogClose();
        return true;
    }

protected:
    bool keyPressEvent(const KeySym & key) override
    {
        switch(key.keycode())
        {
            case Key::BACKSPACE:
                eraseLastCodepoint();
                renderWindow();
                return true;
#ifndef SWE_SDL12
            case Key::INSERT:
                if(key.keymod().isShift() && SDL_HasClipboardText())
                {
                    char* clipboard = SDL_GetClipboardText();
                    if(clipboard)
                    {
                        textInputEvent(clipboard);
                        SDL_free(clipboard);
                    }
                    return true;
                }
                break;
#endif
            case Key::RETURN: return accept();
            case Key::ESCAPE:
                setResultCode(0);
                actionDialogClose();
                return true;
            default: break;
        }
        return false;
    }

    bool textInputEvent(const std::string & value) override
    {
        if(value.empty() || 2048 <= path.size()) return true;
        for(unsigned char symbol : value)
            if(symbol < 0x20 || symbol == 0x7F) return true;
        if(path.size() + value.size() <= 2048) path.append(value);
        renderWindow();
        return true;
    }

    bool mouseClickEvent(const ButtonsEvent & event) override
    {
        if(!event.isButtonLeft()) return false;
        if(event.isClick(acceptArea)) return accept();
        if(event.isClick(cancelArea))
        {
            setResultCode(0);
            JsonWindow::playSound("button");
            actionDialogClose();
        }
        return true;
    }

public:
    ReplayPathDialog(const std::string & header, const std::string & actionLabel,
                     const std::string & initialPath, Window & parent) :
        DialogWindow("dialog_replaypath.json", parent), path(initialPath),
        title(header), action(actionLabel)
    {
        inputArea = JsonUnpack::rect(jobject, "input:area");
        acceptArea = JsonUnpack::rect(jobject, "button:accept");
        cancelArea = JsonUnpack::rect(jobject, "button:cancel");
        titleFont = jobject.getString("font:title", "dejavus26");
        inputFont = jobject.getString("font:input", "dejavus16");
        smallFont = jobject.getString("font:small", "dejavus14");
        inputColor = GameTheme::jsonColor(jobject, "color:input");
        inputBorderColor = GameTheme::jsonColor(jobject, "color:input_border");
        buttonColor = GameTheme::jsonColor(jobject, "color:button");
        buttonBorderColor = GameTheme::jsonColor(jobject, "color:button_border");
        mutedColor = GameTheme::jsonColor(jobject, "color:muted");
        setKeyHandle(true);
        setResultCode(0);
#ifndef SWE_SDL12
        SDL_StartTextInput();
#endif
        setVisible(true);
    }

    ~ReplayPathDialog()
    {
#ifndef SWE_SDL12
        SDL_StopTextInput();
#endif
    }

    const std::string & selectedPath(void) const { return path; }

    void renderWindow(void) override
    {
        renderColor(backgroundColor, rect());
        renderRect(borderColor, rect());
        const FontRender & heading = GameTheme::fontRender(titleFont);
        const FontRender & input = GameTheme::fontRender(inputFont);
        const FontRender & small = GameTheme::fontRender(smallFont);
        renderText(heading, title, headerColor, Point(width() / 2, 20), AlignCenter);
        renderText(small, _("Enter or paste the full .fwr path"), mutedColor,
                   Point(inputArea.x, inputArea.y - 24));
        renderColor(inputColor, inputArea);
        renderRect(inputBorderColor, inputArea);

        std::string shown = path.empty() ? std::string(_("Full replay path")) : path;
        bool clipped = false;
        while(!shown.empty() && input.stringSize(shown).w > inputArea.w - 24)
        {
            std::size_t count = 1;
            const unsigned char first = static_cast<unsigned char>(shown.front());
            if((first & 0xE0) == 0xC0) count = 2;
            else if((first & 0xF0) == 0xE0) count = 3;
            else if((first & 0xF8) == 0xF0) count = 4;
            shown.erase(0, std::min(count, shown.size()));
            clipped = true;
        }
        if(clipped) shown = "..." + shown;
        const Color valueColor = path.empty() ? mutedColor : textColor;
        const Rect rendered = renderText(input, shown, valueColor,
            Point(inputArea.x + 12, inputArea.y + inputArea.h / 2),
            AlignLeft, AlignCenter);
        if(!path.empty())
        {
            const int cursorX = std::min(inputArea.x + inputArea.w - 10,
                                         rendered.x + rendered.w + 2);
            renderLine(textColor, Point(cursorX, inputArea.y + 9),
                       Point(cursorX, inputArea.y + inputArea.h - 10));
        }

        auto drawButton = [&](const Rect & area, const std::string & label)
        {
            renderColor(buttonColor, area);
            renderRect(buttonBorderColor, area);
            renderText(input, label, textColor,
                       Point(area.x + area.w / 2, area.y + area.h / 2),
                       AlignCenter, AlignCenter);
        };
        drawButton(acceptArea, action);
        drawButton(cancelArea, _("Cancel"));
        renderText(small, _("Shift+Insert: paste"), mutedColor,
                   Point(width() / 2, acceptArea.y - 24), AlignCenter);
    }
};

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
        default: return _("Game session");
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

std::string displayValue(std::string value)
{
    std::replace(value.begin(), value.end(), '-', ' ');
    std::replace(value.begin(), value.end(), '_', ' ');
    if(!value.empty()) value.front() = static_cast<char>(
        std::toupper(static_cast<unsigned char>(value.front())));
    return value.empty() ? _("Unknown") : value;
}

std::string defaultExportPath(const std::string & filename)
{
    if(const char* home = Systems::environment("USERPROFILE"))
        return Systems::concatePath(home, filename);
    if(const char* home = Systems::environment("HOME"))
        return Systems::concatePath(home, filename);
    return filename;
}
}

ReplayBrowserScreen::ReplayBrowserScreen() :
    JsonWindow("screen_replaybrowser.json", nullptr), selected(-1), firstVisible(0),
    lastClickedEntry(-1), lastClickAt(0)
{
    panelArea = JsonUnpack::rect(jobject, "panel:area", Rect(132, 54, 760, 660));
    listArea = JsonUnpack::rect(jobject, "list:area", Rect(176, 146, 672, 382));
    backArea = JsonUnpack::rect(jobject, "button:back", Rect(176, 642, 105, 50));
    importArea = JsonUnpack::rect(jobject, "button:import", Rect(289, 642, 105, 50));
    exportArea = JsonUnpack::rect(jobject, "button:export", Rect(402, 642, 105, 50));
    detailsArea = JsonUnpack::rect(jobject, "button:details", Rect(515, 642, 105, 50));
    deleteArea = JsonUnpack::rect(jobject, "button:delete", Rect(628, 642, 105, 50));
    playArea = JsonUnpack::rect(jobject, "button:play", Rect(741, 642, 107, 50));
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

    reloadEntries();

    setMouseTrack(true);
    setKeyHandle(true);
    setVisible(true);
}

void ReplayBrowserScreen::reloadEntries(const std::string & selectedPath)
{
    entries.clear();
    selected = -1;
    firstVisible = 0;
    for(const ReplayFiles::Info & replay : ReplayFiles::inspect())
    {
        Entry entry;
        entry.info = replay;
        const long long timestamp = replay.journal.savedAtEpoch > 0 ?
            replay.journal.savedAtEpoch : replay.journal.startedAtEpoch;
        entry.label = replay.valid ?
            StringFormat(_("Replay %1")).arg(formatTimestamp(timestamp)) :
            std::string(_("Invalid replay"));
        entry.summary = replay.valid ?
            StringFormat("%1 | %2: %3 | %4: %5")
                .arg(gamePartName(replay.journal.gamePart))
                .arg(_("Actions")).arg(replay.journal.actionCount)
                .arg(_("Difficulty")).arg(displayValue(replay.journal.difficulty)) :
            StringFormat("%1 | %2").arg(_("INVALID")).arg(replay.error);
        entry.detail = replay.valid ?
            StringFormat("%1 v%2 | %3 v%4%5")
                .arg(replay.journal.contentPackageId)
                .arg(replay.journal.contentPackageVersion)
                .arg(replay.journal.rulesetId)
                .arg(replay.journal.rulesetVersion)
                .arg(replay.journal.developerAssisted ?
                     std::string(" | ") + _("Developer assisted") : std::string()) :
            std::string(_("The file can be deleted but not opened"));
        entries.push_back(std::move(entry));
        if(!selectedPath.empty() && replay.path == selectedPath)
            selected = static_cast<int>(entries.size()) - 1;
    }

    if(selected < 0 && !entries.empty()) selected = 0;
    lastClickedEntry = -1;
    lastClickAt = 0;
}

int ReplayBrowserScreen::visibleCount(void) const
{
    return std::max(1, (listArea.h + itemGap) / (itemHeight + itemGap));
}

void ReplayBrowserScreen::layoutVisibleEntries(void)
{
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

void ReplayBrowserScreen::renderWindow(void)
{
    JsonWindow::renderWindow();
    renderRect(panelBorderColor, panelArea);
    layoutVisibleEntries();

    const FontRender & title = GameTheme::fontRender(titleFont);
    const FontRender & entryRender = GameTheme::fontRender(entryFont);
    const FontRender & small = GameTheme::fontRender(smallFont);

    renderText(title, _("REPLAY LIBRARY"), titleColor,
               Point(width() / 2, panelArea.y + 25), AlignCenter);
    renderText(small, _("RECORDED GAME SESSIONS"), mutedColor,
               Point(width() / 2, panelArea.y + 68), AlignCenter);
    renderLine(panelBorderColor, Point(panelArea.x + 42, panelArea.y + 88),
               Point(panelArea.x + panelArea.w - 42, panelArea.y + 88));

    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        const Entry & entry = entries[index];
        const bool isSelected = selected == index;
        const Color labelColor = entry.info.valid ?
            (isSelected ? titleColor : textColor) : errorColor;
        renderColor(isSelected ? itemSelectedColor : itemColor, entry.area);
        renderRect(isSelected ? selectedBorderColor : itemBorderColor, entry.area);
        renderText(entryRender, entry.label, labelColor,
                   Point(entry.area.x + 18, entry.area.y + 8));
        renderText(small, entry.summary, entry.info.valid ? textColor : errorColor,
                   Point(entry.area.x + 18, entry.area.y + 43));
        renderText(small, entry.detail, mutedColor,
                   Point(entry.area.x + 18, entry.area.y + 62));
    }

    if(entries.empty())
        renderText(entryRender, _("No recorded replay was found."), mutedColor,
                   Point(width() / 2, 340), AlignCenter);

    const bool selectedInRange = 0 <= selected &&
        static_cast<size_t>(selected) < entries.size();
    auto drawButton = [&](const Rect & area, const std::string & label, bool enabled)
    {
        renderColor(enabled ? itemSelectedColor : itemColor, area);
        renderRect(enabled ? selectedBorderColor : itemBorderColor, area);
        renderText(small, label, enabled ? titleColor : mutedColor,
                   Point(area.x + area.w / 2, area.y + area.h / 2),
                   AlignCenter, AlignCenter);
    };
    drawButton(backArea, _("Back"), true);
    drawButton(importArea, _("Import"), true);
    drawButton(exportArea, _("Export"), selectedInRange && entries[selected].info.valid);
    drawButton(deleteArea, _("Delete"), selectedInRange);
    drawButton(detailsArea, _("Details"), selectedInRange);
    drawButton(playArea, _("Play"), selectedInRange && entries[selected].info.valid &&
                                           entries[selected].info.journal.contiguousToCheckpoint);

    renderText(small, _("Double-click a replay to watch its recorded session."),
               mutedColor, Point(width() / 2, 612), AlignCenter);
}

bool ReplayBrowserScreen::importReplay(void)
{
    std::string source;
    std::string error;
    FileDialog::Result result = FileDialog::openReplay(source, &error);
    if(result == FileDialog::Result::Unavailable)
    {
        ReplayPathDialog dialog(_("IMPORT REPLAY"), _("Import"), std::string(), *this);
        if(dialog.exec() != 1)
        {
            renderWindow();
            return true;
        }
        source = dialog.selectedPath();
        result = FileDialog::Result::Selected;
    }
    if(result == FileDialog::Result::Cancelled)
    {
        renderWindow();
        return true;
    }
    if(result == FileDialog::Result::Failed)
    {
        MessageBox(_("Replay Import"),
            StringFormat(_("The replay could not be imported: %1")).arg(error),
            *this, false).exec();
        renderWindow();
        return true;
    }

    std::string imported;
    if(!ReplayFiles::importReplay(source, &imported, &error))
    {
        MessageBox(_("Replay Import"),
            StringFormat(_("The replay could not be imported: %1")).arg(error),
            *this, false).exec();
        renderWindow();
        return true;
    }
    reloadEntries(imported);
    MessageBox(_("Replay Import"), _("Imported replay into the library."),
               *this, false).exec();
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::exportSelected(void)
{
    if(selected < 0 || static_cast<std::size_t>(selected) >= entries.size()) return true;
    const Entry & entry = entries[selected];
    if(!entry.info.valid) return showDetails();

    const std::string filename = fs::u8path(entry.info.path).filename().u8string();
    std::string destination;
    std::string error;
    FileDialog::Result result = FileDialog::saveReplay(filename, destination, &error);
    bool fallback = false;
    if(result == FileDialog::Result::Unavailable)
    {
        fallback = true;
        ReplayPathDialog dialog(_("EXPORT REPLAY"), _("Export"),
                                defaultExportPath(filename), *this);
        if(dialog.exec() != 1)
        {
            renderWindow();
            return true;
        }
        destination = dialog.selectedPath();
        result = FileDialog::Result::Selected;
    }
    if(result == FileDialog::Result::Cancelled)
    {
        renderWindow();
        return true;
    }
    if(result == FileDialog::Result::Failed)
    {
        MessageBox(_("Replay Export"),
            StringFormat(_("The replay could not be exported: %1")).arg(error),
            *this, false).exec();
        renderWindow();
        return true;
    }

    if(fs::u8path(destination).extension().empty()) destination += ".fwr";
    const bool exists = Systems::isFile(destination);
    if(fallback && exists && !MessageBox(_("Replay Export"),
        _("A replay already exists at that path. Replace it?"), *this).exec())
    {
        renderWindow();
        return true;
    }

    std::string exported;
    if(!ReplayFiles::exportReplay(entry.info.path, destination, exists,
                                  &exported, &error))
    {
        MessageBox(_("Replay Export"),
            StringFormat(_("The replay could not be exported: %1")).arg(error),
            *this, false).exec();
        renderWindow();
        return true;
    }
    MessageBox(_("Replay Export"),
        StringFormat(_("Exported replay to:\n%1")).arg(exported),
        *this, false).exec();
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::selectNext(int direction)
{
    if(entries.empty()) return false;
    selected = (selected + direction + static_cast<int>(entries.size())) %
               static_cast<int>(entries.size());
    lastClickedEntry = -1;
    lastClickAt = 0;
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::showDetails(void)
{
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return true;
    const Entry & entry = entries[selected];
    std::string message;
    if(!entry.info.valid)
        message = StringFormat(_("This replay is incompatible or damaged: %1"))
            .arg(entry.info.error);
    else
        message = StringFormat(_("%1\nActions: %2\nContent: %3 v%4\nRules: %5 v%6\nStatus: %7"))
            .arg(entry.label)
            .arg(entry.info.journal.actionCount)
            .arg(entry.info.journal.contentPackageId)
            .arg(entry.info.journal.contentPackageVersion)
            .arg(entry.info.journal.rulesetId)
            .arg(entry.info.journal.rulesetVersion)
            .arg(entry.info.journal.contiguousToCheckpoint ?
                 _("Ready for playback") : _("Incomplete"));
    MessageBox(_("Replay Details"), message, *this, false).exec();
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::playSelected(void)
{
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return true;
    const Entry & entry = entries[selected];
    if(!entry.info.valid || !entry.info.journal.contiguousToCheckpoint)
        return showDetails();

    const JsonObject journal = JsonContentFile(entry.info.path).toObject();
    std::string error;
    {
        ReplayViewerScreen viewer(journal, &error);
        if(!viewer.ready())
        {
            MessageBox(_("Replay Playback"), viewer.failureMessage(error),
                       *this, false).exec();
        }
        else viewer.exec();
    }
    playMusic("intro");
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::deleteSelected(void)
{
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return true;
    const Entry entry = entries[selected];
    if(!MessageBox(_("Delete Replay"),
        _("Delete this recorded replay? This cannot be undone from the game menu."),
        *this).exec())
    {
        renderWindow();
        return true;
    }

    std::string error;
    if(!ReplayFiles::deleteReplay(entry.info.path, &error))
    {
        MessageBox(_("Delete Replay"),
            StringFormat(_("The replay could not be deleted: %1")).arg(error),
            *this, false).exec();
        renderWindow();
        return true;
    }

    entries.erase(entries.begin() + selected);
    if(entries.empty()) selected = -1;
    else if(static_cast<size_t>(selected) >= entries.size())
        selected = static_cast<int>(entries.size()) - 1;
    renderWindow();
    return true;
}

bool ReplayBrowserScreen::actionBack(void)
{
    playSound("button");
    setResultCode(Menu::MainMenu);
    setVisible(false);
    return true;
}

bool ReplayBrowserScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::UP: return selectNext(-1);
        case Key::DOWN: return selectNext(1);
        case Key::DELETE: return deleteSelected();
        case Key::RETURN:
        case Key::SPACE: return playSelected();
        case Key::ESCAPE: return actionBack();
        default: break;
    }
    return false;
}

bool ReplayBrowserScreen::mouseClickEvent(const ButtonsEvent & event)
{
    if(!event.isButtonLeft()) return false;
    if(event.isClick(backArea)) return actionBack();
    if(event.isClick(importArea)) return importReplay();
    if(event.isClick(exportArea)) return exportSelected();
    if(event.isClick(deleteArea)) return deleteSelected();
    if(event.isClick(detailsArea)) return showDetails();
    if(event.isClick(playArea)) return playSelected();

    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        if(event.isClick(entries[index].area))
        {
            const u32 now = Tools::ticks();
            const bool doubleClick = lastClickedEntry == index && lastClickAt != 0 &&
                                     now - lastClickAt <= 500;
            selected = index;
            lastClickedEntry = index;
            lastClickAt = now;
            if(doubleClick)
            {
                lastClickedEntry = -1;
                lastClickAt = 0;
                return playSelected();
            }
            renderWindow();
            return true;
        }
    }
    lastClickedEntry = -1;
    lastClickAt = 0;
    return false;
}

bool ReplayBrowserScreen::mouseMotionEvent(const Point & point, u32 buttons)
{
    (void) buttons;
    const int end = std::min(static_cast<int>(entries.size()), firstVisible + visibleCount());
    for(int index = firstVisible; index < end; ++index)
    {
        if(entries[index].area & point)
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
