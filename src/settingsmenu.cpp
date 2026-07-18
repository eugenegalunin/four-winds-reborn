/***************************************************************************
 *   Four Winds Reborn settings menu                                       *
 ***************************************************************************/

#include <algorithm>

#include "swe/swe_music.h"

#include "gametheme.h"
#include "dialogs.h"
#include "runewars.h"
#include "settings.h"
#include "settingsmenu.h"

SettingsMenuScreen::SettingsMenuScreen() :
    JsonWindow("screen_settings.json", nullptr), selected(0),
    language(Settings::language()), gameSpeed(Settings::gameSpeed()),
    musicVolume(Settings::musicVolume()), effectsVolume(Settings::effectsVolume()),
    voiceVolume(Settings::voiceVolume()),
    guardianVoices(Settings::soundGuardianRules()),
    fullscreen(Settings::fullscreen())
{
    leftPanel = JsonUnpack::rect(jobject, "panel:left", Rect(0, 0, 224, 768));
    rightPanel = JsonUnpack::rect(jobject, "panel:right", Rect(800, 0, 224, 768));
    menuArea = JsonUnpack::rect(jobject, "menu:area", Rect(812, 196, 200, 440));
    itemHeight = jobject.getInteger("menu:item_height", 54);
    itemGap = jobject.getInteger("menu:item_gap", 8);

    titleFont = jobject.getString("font:title", "dejavus40");
    menuFont = jobject.getString("font:menu", "dejavus22");
    smallFont = jobject.getString("font:small", "terminus14");

    panelColor = GameTheme::jsonColor(jobject, "color:panel");
    panelBorderColor = GameTheme::jsonColor(jobject, "color:panel_border");
    dividerColor = GameTheme::jsonColor(jobject, "color:divider");
    titleColor = GameTheme::jsonColor(jobject, "color:title");
    textColor = GameTheme::jsonColor(jobject, "color:text");
    mutedColor = GameTheme::jsonColor(jobject, "color:muted");
    buttonColor = GameTheme::jsonColor(jobject, "color:button");
    buttonSelectedColor = GameTheme::jsonColor(jobject, "color:button_selected");
    buttonBorderColor = GameTheme::jsonColor(jobject, "color:button_border");
    buttonSelectedBorderColor = GameTheme::jsonColor(jobject, "color:button_selected_border");

    addEntry(Language);
    addEntry(GameSpeed);
    addEntry(MusicVolume);
    addEntry(EffectsVolume);
    addEntry(VoiceVolume);
    addEntry(GuardianVoices);
    addEntry(DisplayMode);
    addEntry(Apply);
    addEntry(Back);

    setMouseTrack(true);
    setKeyHandle(true);
    setVisible(true);
}

void SettingsMenuScreen::addEntry(EntryKind kind)
{
    const int index = static_cast<int>(entries.size());
    const int posy = menuArea.y + index * (itemHeight + itemGap);
    entries.push_back(Entry{ kind, Rect(menuArea.x, posy, menuArea.w, itemHeight) });
}

std::string SettingsMenuScreen::entryLabel(EntryKind kind) const
{
    switch(kind)
    {
        case Language: return _("Language");
        case GameSpeed: return _("Game Speed");
        case MusicVolume: return _("Music");
        case EffectsVolume: return _("Sound Effects");
        case VoiceVolume: return _("Voices");
        case GuardianVoices: return _("Guardian Voices");
        case DisplayMode: return _("Display Mode");
        case Apply: return _("Apply");
        case Back: return _("Back");
    }
    return std::string();
}

std::string SettingsMenuScreen::entryValue(EntryKind kind) const
{
    switch(kind)
    {
        case Language: return language == "ru" ? _("Russian") : _("English");
        case GameSpeed:
            if(gameSpeed == "classic") return _("Classic");
            if(gameSpeed == "fast") return _("Fast");
            return _("Normal");
        case MusicVolume:
        case EffectsVolume:
        case VoiceVolume: return std::to_string(volumeValue(kind)).append("%");
        case GuardianVoices: return guardianVoices ? _("ON") : _("OFF");
        case DisplayMode: return fullscreen ? _("Fullscreen") : _("Windowed");
        default: break;
    }
    return std::string();
}

bool SettingsMenuScreen::isVolumeEntry(EntryKind kind) const
{
    return kind == MusicVolume || kind == EffectsVolume || kind == VoiceVolume;
}

int SettingsMenuScreen::volumeValue(EntryKind kind) const
{
    if(kind == MusicVolume) return musicVolume;
    if(kind == EffectsVolume) return effectsVolume;
    if(kind == VoiceVolume) return voiceVolume;
    return 0;
}

void SettingsMenuScreen::setVolumeValue(EntryKind kind, int value)
{
    value = std::max(0, std::min(100, value));
    if(kind == MusicVolume) musicVolume = value;
    else if(kind == EffectsVolume) effectsVolume = value;
    else if(kind == VoiceVolume) voiceVolume = value;
}

void SettingsMenuScreen::renderWindow(void)
{
    JsonWindow::renderWindow();

    renderColor(panelColor, leftPanel);
    renderColor(panelColor, rightPanel);

    renderLine(panelBorderColor, Point(leftPanel.x + leftPanel.w - 1, 0),
               Point(leftPanel.x + leftPanel.w - 1, height() - 1));
    renderLine(dividerColor, Point(leftPanel.x + leftPanel.w - 3, 0),
               Point(leftPanel.x + leftPanel.w - 3, height() - 1));
    renderLine(panelBorderColor, Point(rightPanel.x, 0), Point(rightPanel.x, height() - 1));
    renderLine(dividerColor, Point(rightPanel.x + 2, 0),
               Point(rightPanel.x + 2, height() - 1));

    const FontRender & title = GameTheme::fontRender(titleFont);
    const FontRender & menu = GameTheme::fontRender(menuFont);
    const FontRender & small = GameTheme::fontRender(smallFont);
    const FontRender & footer = GameTheme::fontRender("dejavus12");
    const int leftCenter = leftPanel.x + leftPanel.w / 2;
    const int rightCenter = rightPanel.x + rightPanel.w / 2;

    renderText(small, _("THE RUNE WAR"), dividerColor, Point(leftCenter, 42), AlignCenter);
    renderText(title, _("FOUR"), titleColor, Point(leftCenter, 78), AlignCenter);
    renderText(title, _("WINDS"), titleColor, Point(leftCenter, 124), AlignCenter);
    renderText(title, _("REBORN"), titleColor, Point(leftCenter, 170), AlignCenter);
    renderLine(panelBorderColor, Point(34, 229), Point(190, 229));
    renderText(small, _("OLD RUNES,"), textColor, Point(leftCenter, 239), AlignCenter);
    renderText(small, _("A NEW WAR"), textColor, Point(leftCenter, 258), AlignCenter);

    renderText(footer, _("COMMUNITY RESTORATION"), mutedColor,
               Point(leftCenter, height() - 55), AlignCenter);
    renderText(footer, _("OF THE ORIGINAL GAME"), mutedColor,
               Point(leftCenter, height() - 36), AlignCenter);

    renderText(small, _("SETTINGS"), panelBorderColor, Point(rightCenter, 139), AlignCenter);
    renderLine(panelBorderColor, Point(rightPanel.x + 28, 164),
               Point(rightPanel.x + rightPanel.w - 28, 164));

    for(size_t ii = 0; ii < entries.size(); ++ii)
    {
        const Entry & entry = entries[ii];
        const bool isSelected = static_cast<int>(ii) == selected;
        const Color fill = isSelected ? buttonSelectedColor : buttonColor;
        const Color border = isSelected ? buttonSelectedBorderColor : buttonBorderColor;
        const Color labelColor = isSelected ? titleColor : textColor;
        const std::string value = entryValue(entry.kind);

        renderColor(fill, entry.area);
        renderRect(border, entry.area);

        if(isSelected)
            renderLine(buttonSelectedBorderColor,
                       Point(entry.area.x + 3, entry.area.y + 3),
                       Point(entry.area.x + 3, entry.area.y + entry.area.h - 4));

        if(isVolumeEntry(entry.kind))
        {
            renderText(small, entryLabel(entry.kind), labelColor,
                       Point(entry.area.x + 10, entry.area.y + 7));
            renderText(small, value, isSelected ? titleColor : mutedColor,
                       Point(entry.area.x + entry.area.w - 10, entry.area.y + 7), AlignRight);

            const Rect track(entry.area.x + 12, entry.area.y + 34, entry.area.w - 24, 7);
            renderColor(mutedColor, track);
            const int fillWidth = track.w * volumeValue(entry.kind) / 100;
            if(0 < fillWidth)
                renderColor(isSelected ? buttonSelectedBorderColor : panelBorderColor,
                            Rect(track.x, track.y, fillWidth, track.h));
        }
        else if(value.empty())
        {
            renderText(menu, entryLabel(entry.kind), labelColor,
                       Point(entry.area.x + entry.area.w / 2, entry.area.y + entry.area.h / 2),
                       AlignCenter, AlignCenter);
        }
        else
        {
            renderText(menu, entryLabel(entry.kind), labelColor,
                       Point(entry.area.x + entry.area.w / 2, entry.area.y + 4), AlignCenter);
            renderText(small, value, isSelected ? titleColor : mutedColor,
                       Point(entry.area.x + entry.area.w / 2, entry.area.y + 34), AlignCenter);
        }
    }

    renderText(small, _("LEFT / RIGHT TO ADJUST"), mutedColor,
               Point(rightCenter, height() - 27), AlignCenter);
}

bool SettingsMenuScreen::adjustSelected(int direction)
{
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return false;
    const EntryKind kind = entries[selected].kind;

    if(isVolumeEntry(kind))
        setVolumeValue(kind, volumeValue(kind) + (direction < 0 ? -10 : 10));
    else if(kind == Language)
        language = language == "ru" ? "en" : "ru";
    else if(kind == GameSpeed)
    {
        static const std::vector<std::string> speeds = { "classic", "normal", "fast" };
        auto it = std::find(speeds.begin(), speeds.end(), gameSpeed);
        int index = it == speeds.end() ? 0 : static_cast<int>(std::distance(speeds.begin(), it));
        index = (index + (direction < 0 ? -1 : 1) + static_cast<int>(speeds.size())) %
                static_cast<int>(speeds.size());
        gameSpeed = speeds[index];
    }
    else if(kind == GuardianVoices)
        guardianVoices = !guardianVoices;
    else if(kind == DisplayMode)
        fullscreen = !fullscreen;
    else
        return false;

    playSound("button");
    renderWindow();
    return true;
}

bool SettingsMenuScreen::activateSelected(void)
{
    if(selected < 0 || static_cast<size_t>(selected) >= entries.size()) return false;

    switch(entries[selected].kind)
    {
        case Language:
        case GameSpeed:
        case MusicVolume:
        case EffectsVolume:
        case VoiceVolume:
        case GuardianVoices:
        case DisplayMode: return adjustSelected(1);
        case Apply:
        {
            if(Display::isFullscreenWindow() != fullscreen &&
               !Display::setFullscreenMode(fullscreen))
            {
                fullscreen = Display::isFullscreenWindow();
                MessageBox(_("Settings"), _("Could not switch display mode."),
                           *this, false).exec();
                renderWindow();
                return true;
            }

            Settings::setLanguage(language);
            Settings::setGameSpeed(gameSpeed);
            Settings::setMusicVolume(musicVolume);
            Settings::setEffectsVolume(effectsVolume);
            Settings::setVoiceVolume(voiceVolume);
            Settings::setSoundGuardianRules(guardianVoices);
            Settings::setFullscreen(fullscreen);

            std::string error;
            if(!Settings::write(&error))
            {
                MessageBox(_("Settings"),
                           StringFormat(_("Could not save settings: %1")).arg(error),
                           *this, false).exec();
                renderWindow();
                return true;
            }

#ifndef SWE_DISABLE_AUDIO
            if(Settings::music())
                Music::volume(Settings::mixerVolume(Settings::musicVolume()));
            else
                Music::reset();
            Sound::stop(-1);
#endif
            setResultCode(Menu::MainMenu);
            setVisible(false);
            return true;
        }
        case Back:
            setResultCode(Menu::MainMenu);
            setVisible(false);
            return true;
    }

    playSound("button");
    renderWindow();
    return true;
}

bool SettingsMenuScreen::selectNext(int direction)
{
    if(entries.empty()) return false;
    selected = (selected + direction + static_cast<int>(entries.size())) %
               static_cast<int>(entries.size());
    renderWindow();
    return true;
}

bool SettingsMenuScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::UP: return selectNext(-1);
        case Key::DOWN: return selectNext(1);
        case Key::LEFT: return adjustSelected(-1);
        case Key::RIGHT: return adjustSelected(1);
        case Key::RETURN:
        case Key::SPACE: return activateSelected();
        case Key::ESCAPE:
            selected = static_cast<int>(entries.size()) - 1;
            return activateSelected();
        default: break;
    }
    return false;
}

bool SettingsMenuScreen::mouseClickEvent(const ButtonsEvent & coords)
{
    if(!coords.isButtonLeft()) return false;

    for(size_t ii = 0; ii < entries.size(); ++ii)
    {
        if(coords.isClick(entries[ii].area))
        {
            selected = static_cast<int>(ii);
            if(isVolumeEntry(entries[ii].kind))
            {
                const Rect & area = entries[ii].area;
                const int left = area.x + 12;
                const int width = area.w - 24;
                const int mouseX = coords.release().position().x;
                int volume = width <= 0 ? 0 : (mouseX - left) * 100 / width;
                volume = std::max(0, std::min(100, volume));
                volume = ((volume + 2) / 5) * 5;
                setVolumeValue(entries[ii].kind, volume);
                playSound("button");
                renderWindow();
                return true;
            }
            return activateSelected();
        }
    }
    return false;
}

bool SettingsMenuScreen::mouseMotionEvent(const Point & pos, u32 buttons)
{
    (void) buttons;

    for(size_t ii = 0; ii < entries.size(); ++ii)
    {
        if(entries[ii].area & pos)
        {
            if(selected != static_cast<int>(ii))
            {
                selected = static_cast<int>(ii);
                renderWindow();
            }
            return true;
        }
    }
    return false;
}
