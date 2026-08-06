#include "EqualizerPanel.h"
#include "MelaLookAndFeel.h"

EqualizerPanel::EqualizerPanel()
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("EQ SAMPLE 1", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    constexpr std::array<const char*, 3> names { "LOW", "MID", "HIGH" };
    for (int band = 0; band < static_cast<int>(sliders.size()); ++band)
    {
        auto& slider = sliders[static_cast<size_t>(band)];
        auto& label = labels[static_cast<size_t>(band)];
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
        label.setText(names[static_cast<size_t>(band)], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 108, 32);
        slider.setMouseDragSensitivity(260);
        slider.setRange(-12.0, 12.0, 0.1);
        slider.setValue(0.0, juce::dontSendNotification);
        slider.setDefaultValue(0.0);
        slider.setTextValueSuffix(" dB");
        slider.onValueChange = [this]
        {
            if (onChange)
                onChange();
        };
    }
}

void EqualizerPanel::setTitle(const juce::String& title)
{
    titleLabel.setText(title, juce::dontSendNotification);
}

double EqualizerPanel::value(int band) const
{
    return juce::isPositiveAndBelow(band, static_cast<int>(sliders.size()))
        ? sliders[static_cast<size_t>(band)].getValue() : 0.0;
}

void EqualizerPanel::setValue(int band, double newValue)
{
    if (juce::isPositiveAndBelow(band, static_cast<int>(sliders.size())))
        sliders[static_cast<size_t>(band)].setValue(newValue, juce::dontSendNotification);
}

void EqualizerPanel::paint(juce::Graphics& graphics)
{
    const auto area = getLocalBounds().toFloat();
    graphics.setColour(MelaColours::panel);
    graphics.fillRoundedRectangle(area, 20.0f);
    graphics.setColour(MelaColours::custard);
    graphics.drawRoundedRectangle(area.reduced(2.25f), 20.0f, 4.5f);
}

void EqualizerPanel::resized()
{
    auto area = getLocalBounds().reduced(16, 10);
    if (getWidth() < 700)
        titleLabel.setBounds(area.removeFromTop(42).reduced(4));
    else
        titleLabel.setBounds(area.removeFromLeft(175).reduced(4));
    const auto bandWidth = area.getWidth() / static_cast<int>(sliders.size());
    for (int band = 0; band < static_cast<int>(sliders.size()); ++band)
    {
        auto bandArea = area.removeFromLeft(bandWidth).reduced(6, 0);
        labels[static_cast<size_t>(band)].setBounds(bandArea.removeFromTop(24));
        sliders[static_cast<size_t>(band)].setBounds(bandArea);
    }
}
