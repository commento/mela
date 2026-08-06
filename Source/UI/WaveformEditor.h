#pragma once

#include <JuceHeader.h>
#include "Audio/LoopEngine.h"
#include <array>
#include <optional>

class WaveformEditor final : public juce::Component
{
public:
    std::function<void(double, double)> onTrimChanged;

    void setClip(std::shared_ptr<const LoopEngine::Clip> newClip);
    void setPlayhead(double newPosition);
    void resetTrim();
    void setTrimRange(double start, double end);
    void setEnvelope(double attackSeconds, double decaySeconds,
                     float sustainLevel, double releaseSeconds,
                     double playbackRate, bool cycleEnabled, bool reversed);
    void setHardwareTouchEnabled(bool shouldUseHardwareTouch);
    void setHardwareTouches(const std::array<std::optional<juce::Point<float>>, 10>& points);

    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseMagnify(const juce::MouseEvent& event, float scaleFactor) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

private:
    enum class Handle
    {
        none,
        start,
        end,
        pan
    };

    void moveHandle(float x);
    void panView(float x);
    void zoomAround(float x, double scaleFactor);
    void updateTouch(const juce::MouseEvent& event, bool isActive);
    void beginTouchPinch();
    void updateTouchPinch();
    void beginSingleTouch(juce::Point<float> position);
    void moveSingleTouch(juce::Point<float> position);
    [[nodiscard]] juce::Rectangle<float> plotBounds() const;
    [[nodiscard]] float normalisedToX(double position) const;
    [[nodiscard]] double xToNormalised(float x) const;
    [[nodiscard]] double durationSeconds() const;

    struct TouchState
    {
        bool active = false;
        juce::Point<float> position;
    };

    std::shared_ptr<const LoopEngine::Clip> clip;
    double trimStart = 0.0;
    double trimEnd = 1.0;
    double playhead = 0.0;
    double viewStart = 0.0;
    double viewEnd = 1.0;
    Handle draggedHandle = Handle::none;
    float dragOriginX = 0.0f;
    double dragOriginViewStart = 0.0;
    std::array<TouchState, 16> touches;
    bool touchPinching = false;
    float pinchInitialDistance = 0.0f;
    double pinchInitialSpan = 1.0;
    double pinchAnchor = 0.5;
    double envelopeAttack = 0.02;
    double envelopeDecay = 0.1;
    float envelopeSustain = 1.0f;
    double envelopeRelease = 0.0;
    double currentPlaybackRate = 1.0;
    bool envelopeCycleEnabled = true;
    bool playbackReversed = false;
    bool hardwareTouchEnabled = false;
    int hardwareTouchCount = 0;
};
