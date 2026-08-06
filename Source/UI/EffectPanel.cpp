#include "EffectPanel.h"
#include "MelaLookAndFeel.h"

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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 123, 35);
        slider.setMouseDragSensitivity(300);
        slider.setRange(parameter->minimum, parameter->maximum, parameter->step);
        slider.setValue(parameter->defaultValue, juce::dontSendNotification);
        slider.setDefaultValue(parameter->defaultValue);
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
    const auto colour = isEnabled() ? MelaColours::panel : MelaColours::panelDark;
    graphics.setColour(colour);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 24.0f);
    graphics.setColour(isEnabled() ? MelaColours::custard : MelaColours::ink);
    graphics.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.25f), 24.0f, 4.5f);
}

void EffectPanel::resized()
{
    auto area = getLocalBounds().reduced(18);
    auto heading = area.removeFromTop(48);
    enabledButton.setBounds(heading.removeFromRight(98));
    titleLabel.setBounds(heading);
    area.removeFromTop(8);

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
                        .reduced(6);
        labels[static_cast<size_t>(index)].setBounds(cell.removeFromTop(30));
        sliders[static_cast<size_t>(index)].setBounds(cell);
    }
}
