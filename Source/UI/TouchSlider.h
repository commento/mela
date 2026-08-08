#pragma once

#include <JuceHeader.h>

class TouchSlider final : public juce::Slider
{
public:
    TouchSlider() {}

    void setDefaultValue(double value)
    {
        defaultValue = value;
        setDoubleClickReturnValue(false, value);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        pointerDownPosition = event.position;
        draggedSinceMouseDown = false;
        juce::Slider::mouseDown(event);
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (event.position.getDistanceFrom(pointerDownPosition) > maximumTapMovement)
            draggedSinceMouseDown = true;
        juce::Slider::mouseDrag(event);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        juce::Slider::mouseUp(event);
        if (draggedSinceMouseDown)
        {
            lastTapTimeMs = 0.0;
            return;
        }

        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto isSecondTap = lastTapTimeMs > 0.0
            && now - lastTapTimeMs <= maximumTapIntervalMs
            && event.position.getDistanceFrom(lastTapPosition) <= maximumTapMovement
            && event.source.getIndex() == lastTapSourceIndex;

        if (isSecondTap)
        {
            lastTapTimeMs = 0.0;
            setValue(defaultValue, juce::sendNotificationSync);
            return;
        }

        lastTapTimeMs = now;
        lastTapPosition = event.position;
        lastTapSourceIndex = event.source.getIndex();
    }

private:
    static constexpr double maximumTapIntervalMs = 500.0;
    static constexpr float maximumTapMovement = 24.0f;
    double defaultValue = 0.0;
    double lastTapTimeMs = 0.0;
    juce::Point<float> pointerDownPosition;
    juce::Point<float> lastTapPosition;
    int lastTapSourceIndex = -1;
    bool draggedSinceMouseDown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TouchSlider)
};
