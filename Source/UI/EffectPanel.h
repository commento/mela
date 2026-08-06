#pragma once

#include <JuceHeader.h>
#include "TouchSlider.h"
#include <array>
#include <functional>

class EffectPanel final : public juce::Component
{
public:
    EffectPanel() = default;

    struct Parameter
    {
        juce::String name;
        double minimum = 0.0;
        double maximum = 1.0;
        double step = 0.01;
        double defaultValue = 0.0;
        juce::String suffix;
    };

    std::function<void()> onChange;

    void configure(const juce::String& effectName,
                   std::initializer_list<Parameter> newParameters);
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] double value(int parameterIndex) const;
    void setEnabled(bool shouldBeEnabled);
    void setValue(int parameterIndex, double newValue);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    juce::Label titleLabel;
    juce::ToggleButton enabledButton { "ON" };
    std::array<TouchSlider, 5> sliders;
    std::array<juce::Label, 5> labels;
    int parameterCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectPanel)
};
