#include "EffectPanel.h"

void EffectPanel::configure(const juce::String& effectName,
                            std::initializer_list<Parameter> newParameters)
{
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(enabledButton);
    titleLabel.setText(effectName, juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    enabledButton.setToggleState(false, juce::dontSendNotification);
    enabledButton.onClick = [this]
    {
        repaint();
        if (onChange)
            onChange();
    };

    parameterCount = juce::jmin(5, static_cast<int>(newParameters.size()));
    auto parameter = newParameters.begin();
    for (int index = 0; index < parameterCount; ++index, ++parameter)
    {
        auto& slider = sliders[static_cast<size_t>(index)];
        auto& label = labels[static_cast<size_t>(index)];
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
        label.setText(parameter->name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 23);
        slider.setMouseDragSensitivity(300);
        slider.setRange(parameter->minimum, parameter->maximum, parameter->step);
        slider.setValue(parameter->defaultValue, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, parameter->defaultValue);
        slider.setTextValueSuffix(parameter->suffix);
        slider.onValueChange = [this]
        {
            if (onChange)
                onChange();
        };
    }
}

bool EffectPanel::isEnabled() const
{
    return enabledButton.getToggleState();
}

double EffectPanel::value(int parameterIndex) const
{
    if (! juce::isPositiveAndBelow(parameterIndex, parameterCount))
        return 0.0;
    return sliders[static_cast<size_t>(parameterIndex)].getValue();
}

void EffectPanel::setEnabled(bool shouldBeEnabled)
{
    enabledButton.setToggleState(shouldBeEnabled, juce::dontSendNotification);
    repaint();
}

void EffectPanel::setValue(int parameterIndex, double newValue)
{
    if (juce::isPositiveAndBelow(parameterIndex, parameterCount))
        sliders[static_cast<size_t>(parameterIndex)].setValue(
            newValue, juce::dontSendNotification);
}

void EffectPanel::paint(juce::Graphics& graphics)
{
    const auto colour = isEnabled() ? juce::Colour(0xff303d49) : juce::Colour(0xff252a31);
    graphics.setColour(colour);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
    graphics.setColour(isEnabled() ? juce::Colour(0xff5ee6a8)
                                   : juce::Colours::white.withAlpha(0.15f));
    graphics.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 12.0f, 2.0f);
}

void EffectPanel::resized()
{
    auto area = getLocalBounds().reduced(12);
    auto heading = area.removeFromTop(32);
    enabledButton.setBounds(heading.removeFromRight(65));
    titleLabel.setBounds(heading);
    area.removeFromTop(5);

    const auto columns = parameterCount <= 2 ? parameterCount
                       : parameterCount <= 4 ? 2 : 3;
    const auto rows = parameterCount <= 2 ? 1 : 2;
    if (columns == 0)
        return;

    const auto cellWidth = area.getWidth() / columns;
    const auto cellHeight = area.getHeight() / rows;
    for (int index = 0; index < parameterCount; ++index)
    {
        const auto column = index % columns;
        const auto row = index / columns;
        auto cell = area.withTrimmedLeft(column * cellWidth)
                        .withTrimmedTop(row * cellHeight)
                        .withWidth(cellWidth)
                        .withHeight(cellHeight)
                        .reduced(4);
        labels[static_cast<size_t>(index)].setBounds(cell.removeFromTop(20));
        sliders[static_cast<size_t>(index)].setBounds(cell);
    }
}
