#pragma once

#include <JuceHeader.h>

namespace MelaColours
{
inline const juce::Colour background { 0xff000000 };
inline const juce::Colour surface    { 0xff0c0c0c };
inline const juce::Colour active     { 0xff242424 };
inline const juce::Colour selected   { 0xff383838 };
inline const juce::Colour border     { 0xff606060 };
inline const juce::Colour muted      { 0xffb3b3b3 };
inline const juce::Colour text       { 0xffffffff };
}

class MelaLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MelaLookAndFeel();

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(
        juce::ComboBox&, juce::Label&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour&, bool highlighted, bool down) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool highlighted, bool down) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool down,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float position, float startAngle, float endAngle,
                          juce::Slider&) override;

private:
    juce::Font interfaceFont(float height) const;
};
