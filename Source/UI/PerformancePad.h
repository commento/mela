#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <optional>

class PerformancePad final : public juce::Component
{
public:
    enum class Mode
    {
        repeat,
        reverse,
        glitch
    };

    PerformancePad();

    std::function<void(bool active, float x, float y, Mode mode)> onGesture;

    void setHardwareTouchEnabled(bool enabled);
    void setHardwareTouch(std::optional<juce::Point<float>> position);
    void releaseGesture();
    [[nodiscard]] bool containsPadPosition(juce::Point<float> position) const;

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    [[nodiscard]] juce::Rectangle<float> padBounds() const;
    void updateGesture(juce::Point<float> position, bool active);
    void setMode(Mode newMode);
    void setSliceMs(float milliseconds);
    void updateButtonColours();

    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 4> sliceButtons;
    Mode mode = Mode::repeat;
    float x = 0.52f;
    float y = 0.72f;
    bool touching = false;
    bool hardwareTouchEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformancePad)
};
