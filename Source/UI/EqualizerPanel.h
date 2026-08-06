#pragma once

#include <JuceHeader.h>
#include "TouchSlider.h"
#include <array>
#include <functional>

class EqualizerPanel final : public juce::Component
{
public:
    EqualizerPanel();

    std::function<void()> onChange;

    void setTitle(const juce::String& title);
    [[nodiscard]] double value(int band) const;
    void setValue(int band, double newValue);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    juce::Label titleLabel;
    std::array<TouchSlider, 3> sliders;
    std::array<juce::Label, 3> labels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerPanel)
};
