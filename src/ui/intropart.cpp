/***************************************************************************
 *   Four Winds Reborn original story intro                                *
 ***************************************************************************/

#include <algorithm>

#include "gametheme.h"
#include "settings.h"
#include "intropart.h"

namespace
{
bool isRussianLanguage(const std::string & value)
{
    const std::string language = String::toLower(value);
    return language == "ru" || language == "russian" ||
        language.rfind("ru_", 0) == 0 || language.rfind("ru-", 0) == 0 ||
        language.rfind("russian_", 0) == 0;
}

bool isEnglishLanguage(const std::string & value)
{
    const std::string language = String::toLower(value);
    return language.empty() || language == "c" || language == "posix" ||
        language == "en" || language == "english" ||
        language.rfind("en_", 0) == 0 || language.rfind("en-", 0) == 0 ||
        language.rfind("english_", 0) == 0;
}
}

IntroScreen::IntroScreen() :
    JsonWindow("screen_intro.json", nullptr),
    currentFrame(0), frameStartedAt(0), frameElapsed(0), fadeDuration(350),
    narrationChannel(-1), frameStarted(false)
{
    frameArea = JsonUnpack::rect(jobject, "frame:area", Rect(0, 18, 1024, 660));
    progressArea = JsonUnpack::rect(jobject, "progress:area", Rect(64, 712, 896, 2));
    hintPosition = JsonUnpack::point(jobject, "hint:position", Point(512, 731));
    hintFont = jobject.getString("font:hint", "dejavus12");
    hintColor = GameTheme::jsonColor(jobject, "color:hint");
    progressBackColor = GameTheme::jsonColor(jobject, "color:progress_back");
    progressColor = GameTheme::jsonColor(jobject, "color:progress");
    fadeDuration = static_cast<u32>(std::max(0, jobject.getInteger("frame:fade_ms", 350)));
    const std::string soundKey = isRussianLanguage(Settings::language()) ? "sound:ru" : "sound:en";

    const JsonArray* encodedFrames = jobject.getArray("frames");
    if(encodedFrames)
    {
        for(int index = 0; index < encodedFrames->size(); ++index)
        {
            const JsonObject* encoded = encodedFrames->getObject(index);
            if(! encoded) continue;

            const JsonValue* image = encoded->getValue("image");
            if(! image) continue;

            Frame frame;
            frame.image = GameTheme::jsonSprite(*image);
            frame.sound = encoded->getString(soundKey);
            frame.duration = static_cast<u32>(std::max(1, encoded->getInteger("duration_ms", 3000)));
            if(frame.image.isValid()) frames.push_back(frame);
        }
    }

    setKeyHandle(true);
    setVisible(! frames.empty());
}

IntroScreen::~IntroScreen()
{
    stopNarration();
}

bool IntroScreen::supportsLanguage(const std::string & language)
{
    return isRussianLanguage(language) || isEnglishLanguage(language);
}

void IntroScreen::startCurrentFrame(u32 ms)
{
    if(currentFrame >= frames.size())
    {
        finish();
        return;
    }

    stopNarration();
    frameStartedAt = ms;
    frameElapsed = 0;
    frameStarted = true;

#ifndef SWE_DISABLE_AUDIO
    if(0 < Settings::voiceVolume() && ! frames[currentFrame].sound.empty())
    {
        narrationChannel = Sound::playChannel(GameTheme::sound(frames[currentFrame].sound));
        if(0 <= narrationChannel)
            Sound::volume(narrationChannel, Settings::mixerVolume(Settings::voiceVolume()));
    }
#endif

    setDirty(true);
}

void IntroScreen::stopNarration(void)
{
#ifndef SWE_DISABLE_AUDIO
    if(narrationChannel >= 0)
        Sound::stop(narrationChannel);
#endif
    narrationChannel = -1;
}

void IntroScreen::finish(void)
{
    stopNarration();
    setVisible(false);
}

void IntroScreen::tickEvent(u32 ms)
{
    if(frames.empty())
    {
        finish();
        return;
    }

    if(! frameStarted)
    {
        startCurrentFrame(ms);
        return;
    }

    frameElapsed = ms - frameStartedAt;
    if(frameElapsed >= frames[currentFrame].duration)
    {
        ++currentFrame;
        frameStarted = false;
        if(currentFrame >= frames.size())
            finish();
        else
            startCurrentFrame(ms);
        return;
    }

    setDirty(true);
}

void IntroScreen::renderWindow(void)
{
    JsonWindow::renderWindow();
    if(currentFrame >= frames.size()) return;

    const Frame & frame = frames[currentFrame];
    int alpha = 255;
    if(fadeDuration)
    {
        if(frameElapsed < fadeDuration)
            alpha = static_cast<int>((255u * frameElapsed) / fadeDuration);
        else if(frameElapsed + fadeDuration > frame.duration)
            alpha = static_cast<int>((255u * (frame.duration - frameElapsed)) / fadeDuration);
    }

    Sprite visibleFrame = frame.image;
    visibleFrame.setAlphaMod(static_cast<u8>(std::max(0, std::min(255, alpha))));
    renderTexture(visibleFrame, frameArea.toPoint());

    renderColor(progressBackColor, progressArea);
    const double currentProgress = static_cast<double>(frameElapsed) / frame.duration;
    const double totalProgress =
        (static_cast<double>(currentFrame) + std::min(1.0, currentProgress)) / frames.size();
    Rect completed = progressArea;
    completed.w = static_cast<int>(progressArea.w * totalProgress);
    if(completed.w > 0) renderColor(progressColor, completed);

    renderText(GameTheme::fontRender(hintFont), _("ESC / ENTER / CLICK TO SKIP"),
               hintColor, hintPosition, AlignCenter);
}

bool IntroScreen::keyPressEvent(const KeySym & key)
{
    switch(key.keycode())
    {
        case Key::ESCAPE:
        case Key::RETURN:
        case Key::SPACE:
            finish();
            return true;

        default: break;
    }

    return false;
}

bool IntroScreen::mouseClickEvent(const ButtonsEvent & event)
{
    if(! event.isButtonLeft()) return false;
    finish();
    return true;
}
