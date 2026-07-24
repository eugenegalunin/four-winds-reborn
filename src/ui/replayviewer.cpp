/***************************************************************************
 *   Four Winds Reborn controlled replay viewer                           *
 ***************************************************************************/

#include "replayviewer.h"

#include <algorithm>

#include "adventurepart.h"
#include "battlesummarypart.h"
#include "dialogs.h"
#include "gamesummarypart.h"
#include "gametheme.h"
#include "mahjongpart.h"
#include "mahjongsummarypart.h"
#include "settings.h"

namespace
{
constexpr int ReplaySpeeds[] = { 50, 100, 200, 400 };

struct MusicVolumeGuard
{
    const int previous;

    MusicVolumeGuard() : previous(Settings::musicVolume())
    {
        Settings::setMusicVolume(0);
    }

    ~MusicVolumeGuard()
    {
        Settings::setMusicVolume(previous);
    }
};

template <typename Screen>
void renderPreview(Screen & screen)
{
    screen.disableTickEvent(true);
    screen.setKeyHandle(false);
    screen.setMouseTrack(false);
    screen.renderWindow();
    screen.setVisible(false);
}
}

ReplayViewerScreen::ReplayViewerScreen(const JsonObject & journal, std::string* error) :
    JsonWindow("screen_replayviewer.json", nullptr), readyState(false), playing(false),
    speedIndex(1), lastAdvanceAt(0)
{
    overlayArea = JsonUnpack::rect(jobject, "overlay:area", Rect(0, 0, 1024, 78));
    backArea = JsonUnpack::rect(jobject, "button:back", Rect(12, 25, 78, 44));
    previousPhaseArea = JsonUnpack::rect(jobject, "button:previous_phase", Rect(96, 25, 94, 44));
    previousStepArea = JsonUnpack::rect(jobject, "button:previous_step", Rect(196, 25, 48, 44));
    playArea = JsonUnpack::rect(jobject, "button:play", Rect(250, 25, 110, 44));
    nextStepArea = JsonUnpack::rect(jobject, "button:next_step", Rect(366, 25, 48, 44));
    nextPhaseArea = JsonUnpack::rect(jobject, "button:next_phase", Rect(420, 25, 94, 44));
    speedArea = JsonUnpack::rect(jobject, "button:speed", Rect(520, 25, 90, 44));
    progressArea = JsonUnpack::rect(jobject, "progress:area", Rect(628, 36, 372, 12));

    controlFont = jobject.getString("font:control", "dejavus18");
    smallFont = jobject.getString("font:small", "dejavus14");
    overlayColor = GameTheme::jsonColor(jobject, "color:overlay");
    overlayColor.setA(232);
    borderColor = GameTheme::jsonColor(jobject, "color:border");
    textColor = GameTheme::jsonColor(jobject, "color:text");
    mutedColor = GameTheme::jsonColor(jobject, "color:muted");
    activeColor = GameTheme::jsonColor(jobject, "color:active");

    readyState = playback.open(journal, error);
    setMouseTrack(true);
    setKeyHandle(true);
    setVisible(readyState);
    if(readyState) captureFrame();
}

std::string ReplayViewerScreen::phaseName(void) const
{
    switch(playback.gamePart())
    {
        case Menu::MahjongPart: return _("Rune Game");
        case Menu::MahjongSummaryPart: return _("Rune Game Summary");
        case Menu::AdventurePart: return _("Adventure");
        case Menu::BattleSummaryPart: return _("Battle Summary");
        case Menu::GameSummaryPart: return _("Game Summary");
        case Menu::MahjongInitPart: return _("Between rounds");
        default: return _("Game session");
    }
}

std::string ReplayViewerScreen::speedName(void) const
{
    const int speed = ReplaySpeeds[speedIndex];
    if(speed < 100) return "0.5x";
    return StringFormat("%1x").arg(speed / 100);
}

u32 ReplayViewerScreen::playbackDelay(void) const
{
    return static_cast<u32>(std::max(40, 50000 / ReplaySpeeds[speedIndex]));
}

std::string ReplayViewerScreen::playbackFailureMessage(const std::string & error) const
{
    const Replay::Failure & failure = playback.failure();
    if(!failure.isValid())
        return StringFormat(_("Playback stopped: %1")).arg(error);

    const int action = static_cast<int>(failure.step + 1);
    if(failure.kind == Replay::FailureKind::StateHashMismatch)
        return StringFormat(_("Replay diverged at action %1.\nExpected state: %2\nActual state: %3"))
            .arg(action).arg(failure.expected).arg(failure.actual);

    return StringFormat(_("Replay diverged at action %1.\nExpected: %2\nActual: %3"))
        .arg(action).arg(failure.expected).arg(failure.actual);
}

void ReplayViewerScreen::captureFrame(void)
{
    MusicVolumeGuard quietPreview;
    switch(playback.gamePart())
    {
        case Menu::MahjongPart:
        {
            MahjongPartScreen screen;
            renderPreview(screen);
            break;
        }
        case Menu::MahjongSummaryPart:
        {
            MahjongSummaryPartScreen screen;
            renderPreview(screen);
            break;
        }
        case Menu::AdventurePart:
        {
            AdventurePartScreen screen(GameData::myPerson().avatar);
            renderPreview(screen);
            break;
        }
        case Menu::BattleSummaryPart:
        {
            BattleSummaryScreen screen;
            renderPreview(screen);
            break;
        }
        case Menu::GameSummaryPart:
        {
            GameSummaryScreen screen;
            renderPreview(screen);
            break;
        }
        default:
            renderClear(Color::Black);
            renderText(GameTheme::fontRender("dejavus34"), phaseName(), textColor,
                       Point(width() / 2, height() / 2), AlignCenter, AlignCenter);
            break;
    }
    frame = Display::createTexture(Display::texture());
    renderWindow();
}

bool ReplayViewerScreen::applyPlaybackAction(
    bool (Replay::Playback::* action)(std::string*))
{
    std::string error;
    if(!(playback.*action)(&error))
    {
        playing = false;
        MessageBox(_("Replay Playback"), playbackFailureMessage(error),
                   *this, false).exec();
        renderWindow();
        return true;
    }
    if(playback.atEnd()) playing = false;
    captureFrame();
    return true;
}

bool ReplayViewerScreen::seek(std::size_t position)
{
    std::string error;
    if(!playback.seek(position, &error))
    {
        playing = false;
        MessageBox(_("Replay Playback"), playbackFailureMessage(error),
                   *this, false).exec();
        renderWindow();
        return true;
    }
    captureFrame();
    return true;
}

bool ReplayViewerScreen::actionBack(void)
{
    playing = false;
    playSound("button");
    setVisible(false);
    return true;
}

bool ReplayViewerScreen::togglePlaying(void)
{
    if(playback.atEnd())
    {
        if(!seek(0)) return true;
    }
    playing = !playing;
    lastAdvanceAt = Tools::ticks();
    renderWindow();
    return true;
}

bool ReplayViewerScreen::changeSpeed(int delta)
{
    speedIndex = std::max(0, std::min(speedIndex + delta,
        static_cast<int>(sizeof(ReplaySpeeds) / sizeof(ReplaySpeeds[0])) - 1));
    lastAdvanceAt = Tools::ticks();
    renderWindow();
    return true;
}

void ReplayViewerScreen::tickEvent(u32 ms)
{
    if(!playing || playback.atEnd()) return;
    if(lastAdvanceAt != 0 && ms - lastAdvanceAt < playbackDelay()) return;
    lastAdvanceAt = ms;
    applyPlaybackAction(&Replay::Playback::stepForward);
}

bool ReplayViewerScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::ESCAPE: return actionBack();
        case Key::SPACE: return togglePlaying();
        case Key::LEFT:
            playing = false;
            return applyPlaybackAction(&Replay::Playback::stepBackward);
        case Key::RIGHT:
            playing = false;
            return applyPlaybackAction(&Replay::Playback::stepForward);
        case Key::PAGEUP:
            playing = false;
            return applyPlaybackAction(&Replay::Playback::previousPhase);
        case Key::PAGEDOWN:
            playing = false;
            return applyPlaybackAction(&Replay::Playback::nextPhase);
        case Key::HOME:
            playing = false;
            return seek(0);
        case Key::END:
            playing = false;
            return seek(playback.size());
        case Key::PLUS:
        case Key::KP_PLUS: return changeSpeed(1);
        case Key::MINUS:
        case Key::KP_MINUS: return changeSpeed(-1);
        default: break;
    }
    return false;
}

bool ReplayViewerScreen::mouseClickEvent(const ButtonsEvent & event)
{
    if(!event.isButtonLeft()) return false;
    if(event.isClick(backArea)) return actionBack();
    if(event.isClick(previousPhaseArea))
    {
        playing = false;
        return applyPlaybackAction(&Replay::Playback::previousPhase);
    }
    if(event.isClick(previousStepArea))
    {
        playing = false;
        return applyPlaybackAction(&Replay::Playback::stepBackward);
    }
    if(event.isClick(playArea)) return togglePlaying();
    if(event.isClick(nextStepArea))
    {
        playing = false;
        return applyPlaybackAction(&Replay::Playback::stepForward);
    }
    if(event.isClick(nextPhaseArea))
    {
        playing = false;
        return applyPlaybackAction(&Replay::Playback::nextPhase);
    }
    if(event.isClick(speedArea)) return changeSpeed(1);
    if(event.isClick(progressArea) && 0 < progressArea.w)
    {
        playing = false;
        const int offset = std::max(0, std::min(event.release().position().x - progressArea.x,
                                               progressArea.w));
        const std::size_t requested = playback.size() *
            static_cast<std::size_t>(offset) / static_cast<std::size_t>(progressArea.w);
        return seek(requested);
    }
    return false;
}

void ReplayViewerScreen::renderWindow(void)
{
    if(frame.isValid()) renderTexture(frame, Point(0, 0));
    else JsonWindow::renderWindow();

    renderColor(overlayColor, overlayArea);
    renderRect(borderColor, overlayArea);
    const FontRender & control = GameTheme::fontRender(controlFont);
    const FontRender & small = GameTheme::fontRender(smallFont);

    auto drawButton = [&](const Rect & area, const std::string & label, bool enabled = true)
    {
        renderColor(Color(12, 12, 12, 225), area);
        renderRect(enabled ? activeColor : borderColor, area);
        renderText(control, label, enabled ? textColor : mutedColor,
                   Point(area.x + area.w / 2, area.y + area.h / 2),
                   AlignCenter, AlignCenter);
    };

    drawButton(backArea, _("Back"));
    drawButton(previousPhaseArea, _("<< Phase"), !playback.atBeginning());
    drawButton(previousStepArea, "<", !playback.atBeginning());
    drawButton(playArea, playing ? _("Pause") : _("Play"));
    drawButton(nextStepArea, ">", !playback.atEnd());
    drawButton(nextPhaseArea, _("Phase >>"), !playback.atEnd());
    drawButton(speedArea, speedName());

    const int filled = playback.size() == 0 ? 0 :
        static_cast<int>(progressArea.w * playback.position() / playback.size());
    renderColor(Color(24, 24, 24, 235), progressArea);
    if(0 < filled)
        renderColor(activeColor, Rect(progressArea.x, progressArea.y, filled, progressArea.h));
    renderRect(borderColor, progressArea);

    renderText(small,
        StringFormat(_("%1 | phase %2/%3 | action %4/%5"))
            .arg(phaseName()).arg(playback.phaseIndex() + 1).arg(playback.phaseCount())
            .arg(playback.position()).arg(playback.size()),
        textColor, Point(628, 5));
    renderText(small, _("Space: play/pause  Arrows: step  PgUp/PgDn: phase  +/-: speed"),
               mutedColor, Point(814, 56), AlignCenter);
}
