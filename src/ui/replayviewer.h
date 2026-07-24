/***************************************************************************
 *   Four Winds Reborn controlled replay viewer                           *
 ***************************************************************************/

#ifndef _FWR_REPLAYVIEWER_
#define _FWR_REPLAYVIEWER_

#include "jsongui.h"
#include "replay.h"

class ReplayViewerScreen : public JsonWindow
{
    Replay::Playback playback;
    Texture          frame;
    bool             readyState;
    bool             playing;
    int              speedIndex;
    u32              lastAdvanceAt;

    Rect             overlayArea;
    Rect             backArea;
    Rect             previousPhaseArea;
    Rect             previousStepArea;
    Rect             playArea;
    Rect             nextStepArea;
    Rect             nextPhaseArea;
    Rect             speedArea;
    Rect             progressArea;

    std::string      controlFont;
    std::string      smallFont;
    Color            overlayColor;
    Color            borderColor;
    Color            textColor;
    Color            mutedColor;
    Color            activeColor;

    void             captureFrame(void);
    bool             applyPlaybackAction(bool (Replay::Playback::*)(std::string*));
    bool             seek(std::size_t);
    bool             actionBack(void);
    bool             togglePlaying(void);
    bool             changeSpeed(int);
    std::string      phaseName(void) const;
    std::string      speedName(void) const;
    u32              playbackDelay(void) const;
    std::string      playbackFailureMessage(const std::string &) const;

protected:
    void             tickEvent(u32) override;
    bool             keyPressEvent(const KeySym &) override;
    bool             mouseClickEvent(const ButtonsEvent &) override;
    bool             scrollUpEvent(void) override { return changeSpeed(1); }
    bool             scrollDownEvent(void) override { return changeSpeed(-1); }

public:
    ReplayViewerScreen(const JsonObject &, std::string* error = nullptr);

    bool             ready(void) const { return readyState; }
    std::string      failureMessage(const std::string & error) const
                     { return playbackFailureMessage(error); }
    void             renderWindow(void) override;
};

#endif
