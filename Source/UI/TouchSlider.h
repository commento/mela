#pragma once

#include <JuceHeader.h>

class TouchSlider final : public juce::Slider
{
public:
    TouchSlider()
    {
        addMouseListener(this, true);
    }

    ~TouchSlider() override
    {
        removeMouseListener(this);
    }

    void setDefaultValue(double value)
    {
        defaultValue = value;
        setDoubleClickReturnValue(true, value);
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        setValue(defaultValue, juce::sendNotificationSync);
    }

private:
    double defaultValue = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchSlider)
};
