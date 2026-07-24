#include "developertools.h"

#ifdef BUILD_DEBUG

#include <algorithm>
#include <cstdint>
#include <vector>

#include "actions.h"
#include "battle.h"
#include "crashreport.h"
#include "gameplayrng.h"
#include "gametheme.h"
#include "dialogs.h"
#include "recovery.h"
#include "replay.h"
#include "swe/swe_cstring.h"

namespace
{
using namespace SWE;

struct DeveloperEntry
{
    std::string label;
    std::string note;
    Rect area;
    DeveloperTools::Command command;
    bool copySeed = false;
};

class DeveloperToolsDialog final : public DialogWindow
{
    Avatar avatar;
    std::vector<DeveloperEntry> entries;
    int selected = 0;
    std::string status;
    std::string titleFont;
    std::string entryFont;
    std::string smallFont;
    Color itemColor;
    Color selectedColor;
    Color itemBorder;
    Color selectedBorder;
    Color mutedColor;

    std::string seedText(void) const
    {
        return std::string("Seed ") + std::to_string(GameplayRng::initialSeed()) +
            "   State " + std::to_string(GameplayRng::state()) +
            "   Draws " + std::to_string(GameplayRng::draws());
    }

    bool selectNext(int direction)
    {
        selected = (selected + direction + static_cast<int>(entries.size())) %
            static_cast<int>(entries.size());
        renderWindow();
        return true;
    }

    bool activateSelected(void)
    {
        if(selected < 0 || static_cast<std::size_t>(selected) >= entries.size()) return false;
        const DeveloperEntry & entry = entries[selected];
        if(entry.copySeed)
        {
            status = SDL_SetClipboardText(seedText().c_str()) == 0 ?
                _("RNG state copied to clipboard") : _("Unable to copy RNG state");
            renderWindow();
            return true;
        }

        setResultCode(static_cast<int>(entry.command));
        JsonWindow::playSound("button");
        actionDialogClose();
        return true;
    }

protected:
    bool keyPressEvent(const KeySym & key) override
    {
        switch(key.keycode())
        {
            case Key::UP: return selectNext(-1);
            case Key::DOWN: return selectNext(1);
            case Key::RETURN:
            case Key::SPACE: return activateSelected();
            case Key::ESCAPE:
            case Key::F9:
                setResultCode(static_cast<int>(DeveloperTools::Command::Cancel));
                actionDialogClose();
                return true;
            default: break;
        }
        return false;
    }

    bool mouseClickEvent(const ButtonsEvent & coords) override
    {
        if(!coords.isButtonLeft()) return false;
        for(std::size_t index = 0; index < entries.size(); ++index)
        {
            if(coords.isClick(entries[index].area))
            {
                selected = static_cast<int>(index);
                return activateSelected();
            }
        }
        return true;
    }

    bool mouseMotionEvent(const Point & pos, u32 buttons) override
    {
        (void) buttons;
        for(std::size_t index = 0; index < entries.size(); ++index)
        {
            if(entries[index].area & pos)
            {
                if(selected != static_cast<int>(index))
                {
                    selected = static_cast<int>(index);
                    renderWindow();
                }
                return true;
            }
        }
        return false;
    }

public:
    DeveloperToolsDialog(SWE::Window & parent, const Avatar & localAvatar) :
        DialogWindow("dialog_developer_tools.json", parent), avatar(localAvatar)
    {
        titleFont = jobject.getString("font:title", "dejavus26");
        entryFont = jobject.getString("font:entry", "dejavus20");
        smallFont = jobject.getString("font:small", "dejavus14");
        itemColor = GameTheme::jsonColor(jobject, "color:item");
        selectedColor = GameTheme::jsonColor(jobject, "color:item_selected");
        itemBorder = GameTheme::jsonColor(jobject, "color:item_border");
        selectedBorder = GameTheme::jsonColor(jobject, "color:selected_border");
        mutedColor = GameTheme::jsonColor(jobject, "color:muted");

        const bool autoplay = GameData::developerAutoplay(avatar);
        entries.push_back(DeveloperEntry{
            autoplay ? _("Return Human Control") : _("Let AI Control My Seat"),
            autoplay ? _("Control returns immediately at this safe input boundary") :
                       _("Observer-safe AI uses only legal player actions until the phase ends"),
            JsonUnpack::rect(jobject, "entry:autoplay"),
            DeveloperTools::Command::ToggleAutoplay });
        entries.push_back(DeveloperEntry{
            _("Finish Current Phase"), _("Skip presentation and stop at the next summary"),
            JsonUnpack::rect(jobject, "entry:phase"),
            DeveloperTools::Command::FinishPhase });
        entries.push_back(DeveloperEntry{
            _("Next Adventure Phase"), _("Stop at the beginning of the next map phase"),
            JsonUnpack::rect(jobject, "entry:adventure"),
            DeveloperTools::Command::NextAdventure });
        entries.push_back(DeveloperEntry{
            _("Battle Test Scenario"), _("Create balanced armies and stop at manual combat"),
            JsonUnpack::rect(jobject, "entry:battle"),
            DeveloperTools::Command::BattleFixture });
        entries.push_back(DeveloperEntry{
            _("Final Rune Game"), _("Deal the last hand of the North round"),
            JsonUnpack::rect(jobject, "entry:final_rune"),
            DeveloperTools::Command::FinalRuneFixture });
        entries.push_back(DeveloperEntry{
            _("Final Adventure Phase"), _("Open the last map phase before final scoring"),
            JsonUnpack::rect(jobject, "entry:final_adventure"),
            DeveloperTools::Command::FinalAdventureFixture });
        entries.push_back(DeveloperEntry{
            _("Finish Current Round"), _("Run legal turns and stop at the next Rune Game"),
            JsonUnpack::rect(jobject, "entry:round"),
            DeveloperTools::Command::FinishRound });
        entries.push_back(DeveloperEntry{
            _("Finish Game"), _("Run legal turns until the final score screen"),
            JsonUnpack::rect(jobject, "entry:game"),
            DeveloperTools::Command::FinishGame });
        entries.push_back(DeveloperEntry{
            _("Copy RNG State"), _("Copy the original seed, current state and draw count"),
            JsonUnpack::rect(jobject, "entry:copy"),
            DeveloperTools::Command::Cancel, true });
        entries.push_back(DeveloperEntry{
            _("Cancel"), "", JsonUnpack::rect(jobject, "entry:cancel"),
            DeveloperTools::Command::Cancel });

        setMouseTrack(true);
        setKeyHandle(true);
        setResultCode(static_cast<int>(DeveloperTools::Command::Cancel));
        setVisible(true);
    }

    void renderWindow(void) override
    {
        renderColor(backgroundColor, rect());
        renderRect(borderColor, rect());

        const FontRender & title = GameTheme::fontRender(titleFont);
        const FontRender & entryRender = GameTheme::fontRender(entryFont);
        const FontRender & small = GameTheme::fontRender(smallFont);
        renderText(title, _("DEVELOPER TOOLS"), headerColor,
                   Point(width() / 2, 18), AlignCenter);
        renderText(small, seedText(), mutedColor, Point(width() / 2, 58), AlignCenter);

        for(std::size_t index = 0; index < entries.size(); ++index)
        {
            const DeveloperEntry & entry = entries[index];
            const bool active = selected == static_cast<int>(index);
            renderColor(active ? selectedColor : itemColor, entry.area);
            renderRect(active ? selectedBorder : itemBorder, entry.area);
            if(entry.note.empty())
            {
                renderText(entryRender, entry.label, active ? headerColor : textColor,
                           Point(entry.area.x + entry.area.w / 2,
                                 entry.area.y + entry.area.h / 2), AlignCenter, AlignCenter);
            }
            else
            {
                renderText(entryRender, entry.label, active ? headerColor : textColor,
                           Point(entry.area.x + entry.area.w / 2, entry.area.y + 4), AlignCenter);
                renderText(small, entry.note, mutedColor,
                           Point(entry.area.x + entry.area.w / 2, entry.area.y + 34), AlignCenter);
            }
        }

        if(!status.empty())
            renderText(small, status, headerColor, Point(width() / 2, height() - 20), AlignCenter);
    }
};

bool resolveAutomatedBattleChoice(ActionList & emitted)
{
    const auto prompt = std::find_if(emitted.begin(), emitted.end(), [](const ActionMessage & action)
    {
        return action.type() == Action::AdventureBattleChoice;
    });
    if(prompt == emitted.end()) return false;

    ActionList followup;
    const Avatar actor = GameData::currentPerson().avatar;
    return GameData::client2Adventure(actor, ClientBattleChoice(-1, -1, true), followup);
}

bool pumpPart(std::size_t & ticks, std::string & error)
{
    constexpr std::size_t MaximumTicks = 250000;
    constexpr std::size_t MaximumUnchangedTicks = 8;
    const int initialPart = GameData::loadedGamePart();
    std::string previousHash = Recovery::stateHash(GameData::authoritativeState());
    std::size_t unchanged = 0;

    while(GameData::loadedGamePart() == initialPart && ticks < MaximumTicks)
    {
        ++ticks;
        ActionList emitted;
        bool advanced = false;
        if(initialPart == Menu::MahjongPart)
            advanced = GameData::mahjong2Client(GameData::currentPerson().avatar, emitted);
        else if(initialPart == Menu::AdventurePart)
        {
            advanced = GameData::adventure2Client(GameData::currentPerson().avatar, emitted);
            if(resolveAutomatedBattleChoice(emitted)) advanced = true;
        }
        else
        {
            error = "fast-forward can start only in Rune Game or Adventure";
            return false;
        }

        const std::string currentHash = Recovery::stateHash(GameData::authoritativeState());
        if(currentHash == previousHash)
            ++unchanged;
        else
        {
            previousHash = currentHash;
            unchanged = 0;
        }

        if(!advanced && MaximumUnchangedTicks <= unchanged)
        {
            error = "authoritative state stopped advancing";
            return false;
        }
    }

    if(MaximumTicks <= ticks)
    {
        error = "fast-forward watchdog reached its tick limit";
        return false;
    }
    return GameData::loadedGamePart() != initialPart;
}

bool enterAdventure(void)
{
    return Replay::recordSystemOperation("init_adventure", []()
    {
        return GameData::initAdventure();
    });
}

bool enterMahjong(void)
{
    return Replay::recordSystemOperation("init_mahjong", []()
    {
        return GameData::initMahjong();
    });
}

void enterGameSummary(void)
{
    const JsonObject before = GameData::authoritativeState();
    GameData::setGamePart(Menu::GameSummaryPart);
    Replay::recordSystemTransition(before, "game_summary", GameData::authoritativeState());
}
}

DeveloperTools::Command DeveloperTools::showPanel(SWE::Window & parent, const Avatar & avatar)
{
    return static_cast<Command>(DeveloperToolsDialog(parent, avatar).exec());
}

DeveloperTools::FastForwardResult DeveloperTools::fastForward(Command command,
                                                               const Avatar & avatar)
{
    FastForwardResult result;
    result.menu = GameData::loadedGamePart();
    if(command != Command::FinishPhase && command != Command::NextAdventure &&
       command != Command::BattleFixture && command != Command::FinalRuneFixture &&
       command != Command::FinalAdventureFixture && command != Command::FinishRound &&
       command != Command::FinishGame)
    {
        result.error = "no fast-forward target selected";
        return result;
    }

    if(command == Command::BattleFixture)
    {
        const JsonObject initialState = GameData::authoritativeState();
        ActionList emitted;
        result.success = GameData::initDeveloperBattleFixture(avatar, emitted, &result.error);
        if(!result.success)
        {
            GameData::restoreState(initialState);
            if(result.error.empty()) result.error = "unable to create developer battle fixture";
        }
        result.menu = GameData::loadedGamePart();
        result.ticks = result.success ? 1 : 0;
        return result;
    }

    if(command == Command::FinalRuneFixture || command == Command::FinalAdventureFixture)
    {
        const JsonObject initialState = GameData::authoritativeState();
        result.success = command == Command::FinalRuneFixture ?
            GameData::initDeveloperFinalRuneFixture(avatar, &result.error) :
            GameData::initDeveloperFinalAdventureFixture(avatar, &result.error);
        if(!result.success)
        {
            GameData::restoreState(initialState);
            if(result.error.empty()) result.error = "unable to create developer near-end fixture";
        }
        result.menu = GameData::loadedGamePart();
        result.ticks = result.success ? 1 : 0;
        return result;
    }

    GameData::setDeveloperAutoplay(avatar, true);
    CrashReport::breadcrumb(std::string("Developer fast-forward begin target=")
        .append(std::to_string(static_cast<int>(command))));

    bool completedAdventure = false;
    bool reachedTarget = false;
    bool ok = true;
    while(ok)
    {
        const int part = GameData::loadedGamePart();
        if(part == Menu::MahjongPart || part == Menu::AdventurePart)
        {
            if(part == Menu::AdventurePart) completedAdventure = true;
            ok = pumpPart(result.ticks, result.error);
            if(!ok) break;
            if(command == Command::FinishPhase)
            {
                reachedTarget = true;
                break;
            }
            continue;
        }

        if(part == Menu::MahjongSummaryPart)
        {
            ok = enterAdventure();
            if(!ok) result.error = "unable to initialize Adventure";
            if(ok && command == Command::NextAdventure)
            {
                reachedTarget = true;
                break;
            }
            continue;
        }

        if(part == Menu::BattleSummaryPart)
        {
            if(GameData::isGameOver())
            {
                enterGameSummary();
                reachedTarget = true;
                break;
            }
            ok = enterMahjong();
            if(!ok)
            {
                result.error = "unable to initialize the next Rune Game";
                break;
            }
            if(command == Command::FinishRound && completedAdventure)
            {
                reachedTarget = true;
                break;
            }
            continue;
        }

        if(part == Menu::GameSummaryPart)
        {
            reachedTarget = true;
            break;
        }

        result.error = "unexpected game phase during fast-forward";
        ok = false;
        break;
    }

    GameData::setDeveloperAutoplay(avatar, false);
    result.menu = GameData::loadedGamePart();
    result.success = ok && reachedTarget;
    if(command == Command::FinishGame)
    {
        result.success = ok && result.menu == Menu::GameSummaryPart;
        if(result.success)
        {
            std::string replayError;
            if(!GameData::archiveCurrentReplay(nullptr, &replayError))
                ERROR("developer fast-forward replay archive failed: " << replayError);
        }
    }
    CrashReport::breadcrumb(std::string("Developer fast-forward end status=")
        .append(result.success ? "ok" : "failed")
        .append(" menu=").append(std::to_string(result.menu))
        .append(" ticks=").append(std::to_string(result.ticks)));
    return result;
}

#endif
