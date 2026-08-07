#pragma once

#include <JuceHeader.h>

namespace MelaColours
{
inline const juce::Colour ink       { 0xff35102f };
inline const juce::Colour aubergine { 0xff3f163b };
inline const juce::Colour panel     { 0xff5a2453 };
inline const juce::Colour panelDark { 0xff2b1029 };
inline const juce::Colour custard   { 0xffffd966 };
inline const juce::Colour coral     { 0xffef796a };
inline const juce::Colour sky       { 0xff79b8d8 };
inline const juce::Colour cream     { 0xfffff2d4 };
inline const juce::Colour green     { 0xff86c982 };
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
    juce::Font cartoonFont(float height) const;
    juce::Typeface::Ptr cartoonTypeface;
};
