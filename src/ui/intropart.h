/***************************************************************************
 *   Four Winds Reborn original story intro                                *
 ***************************************************************************/

#ifndef _FWR_INTROPART_
#define _FWR_INTROPART_

#include <vector>

#include "jsongui.h"

class IntroScreen : public JsonWindow
{
    struct Frame
    {
        Sprite      image;
        std::string sound;
        u32         duration;
    };

    std::vector<Frame> frames;
    Rect               frameArea;
    Rect               progressArea;
    Point              hintPosition;
    std::string        hintFont;
    Color              hintColor;
    Color              progressBackColor;
    Color              progressColor;
    std::size_t         currentFrame;
    u32                 frameStartedAt;
    u32                 frameElapsed;
    u32                 fadeDuration;
    int                 narrationChannel;
    bool                frameStarted;

    void                startCurrentFrame(u32);
    void                stopNarration(void);
    void                finish(void);

protected:
    void                tickEvent(u32) override;
    bool                keyPressEvent(const KeySym &) override;
    bool                mouseClickEvent(const ButtonsEvent &) override;

public:
    IntroScreen();
    ~IntroScreen() override;

    static bool         supportsLanguage(const std::string &);

    void                renderWindow(void) override;
};

#endif
