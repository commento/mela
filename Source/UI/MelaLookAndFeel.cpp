#include "MelaLookAndFeel.h"

MelaLookAndFeel::MelaLookAndFeel()
{
    cartoonTypeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::luckiest_guy_regular_ttf,
        static_cast<size_t>(BinaryData::luckiest_guy_regular_ttfSize));

    setColour(juce::TextButton::buttonColourId, MelaColours::panel);
    setColour(juce::TextButton::buttonOnColourId, MelaColours::custard);
    setColour(juce::TextButton::textColourOffId, MelaColours::cream);
    setColour(juce::TextButton::textColourOnId, MelaColours::ink);
    setColour(juce::ToggleButton::textColourId, MelaColours::cream);
    setColour(juce::Label::textColourId, MelaColours::cream);
    setColour(juce::ComboBox::backgroundColourId, MelaColours::panelDark);
    setColour(juce::ComboBox::textColourId, MelaColours::cream);
    setColour(juce::ComboBox::outlineColourId, MelaColours::ink);
    setColour(juce::ComboBox::arrowColourId, MelaColours::custard);
    setColour(juce::Slider::textBoxTextColourId, MelaColours::cream);
    setColour(juce::Slider::textBoxBackgroundColourId, MelaColours::panelDark);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId, MelaColours::panelDark);
    setColour(juce::PopupMenu::textColourId, MelaColours::cream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, MelaColours::coral);
    setColour(juce::PopupMenu::highlightedTextColourId, MelaColours::ink);
}

juce::Font MelaLookAndFeel::cartoonFont(float height) const
{
    return juce::Font(juce::FontOptions(cartoonTypeface).withHeight(height))
        .withExtraKerningFactor(0.035f);
}

juce::Font MelaLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return cartoonFont(juce::jmin(20.0f, static_cast<float>(buttonHeight) * 0.42f));
}

juce::Font MelaLookAndFeel::getLabelFont(juce::Label& label)
{
    const auto requested = label.getFont().getHeight();
    return cartoonFont(juce::jlimit(13.0f, 26.0f, requested));
}

juce::Font MelaLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return cartoonFont(juce::jmin(19.0f, static_cast<float>(box.getHeight()) * 0.42f));
}

void MelaLookAndFeel::drawButtonBackground(juce::Graphics& graphics,
                                           juce::Button& button,
                                           const juce::Colour& background,
                                           bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(3.0f);
    if (down)
        area = area.translated(0.0f, 2.0f);

    const auto fill = down ? background.darker(0.12f)
                           : highlighted ? background.brighter(0.10f) : background;
    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillRoundedRectangle(area.translated(0.0f, 3.0f), 11.0f);
    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, 11.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 11.0f, 3.0f);
}

void MelaLookAndFeel::drawToggleButton(juce::Graphics& graphics,
                                       juce::ToggleButton& button,
                                       bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(3.0f);
    auto fill = button.getToggleState() ? MelaColours::custard : MelaColours::panelDark;
    if (highlighted)
        fill = fill.brighter(0.08f);
    if (down)
        area = area.translated(0.0f, 2.0f);

    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillRoundedRectangle(area.translated(0.0f, 3.0f), 11.0f);
    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, 11.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 11.0f, 3.0f);
    graphics.setColour(button.getToggleState() ? MelaColours::ink : MelaColours::cream);
    graphics.setFont(cartoonFont(juce::jmin(18.0f, area.getHeight() * 0.40f)));
    graphics.drawText(button.getButtonText(), area.toNearestInt().reduced(8, 2),
                      juce::Justification::centred);
}

void MelaLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height,
                                   bool down, int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                       static_cast<float>(height)).reduced(2.0f);
    graphics.setColour(down ? MelaColours::panel : MelaColours::panelDark);
    graphics.fillRoundedRectangle(area, 10.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 10.0f, 3.0f);

    const auto centreX = area.getRight() - 18.0f;
    const auto centreY = area.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath(centreX - 6.0f, centreY - 3.0f);
    arrow.lineTo(centreX, centreY + 4.0f);
    arrow.lineTo(centreX + 6.0f, centreY - 3.0f);
    graphics.setColour(MelaColours::custard);
    graphics.strokePath(arrow, juce::PathStrokeType(3.0f,
                        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MelaLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y,
                                       int width, int height, float position,
                                       float startAngle, float endAngle, juce::Slider&)
{
    const auto size = static_cast<float>(juce::jmin(width, height)) - 14.0f;
    const auto centre = juce::Point<float>(static_cast<float>(x)
                                               + static_cast<float>(width) * 0.5f,
                                           static_cast<float>(y)
                                               + static_cast<float>(height) * 0.5f);
    const auto radius = size * 0.5f;
    const auto angle = startAngle + position * (endAngle - startAngle);

    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillEllipse(centre.x - radius, centre.y - radius + 4.0f, size, size);
    graphics.setColour(MelaColours::sky);
    graphics.fillEllipse(centre.x - radius, centre.y - radius, size, size);
    graphics.setColour(MelaColours::ink);
    graphics.drawEllipse(centre.x - radius, centre.y - radius, size, size, 4.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-3.5f, -radius + 8.0f, 7.0f, radius * 0.58f, 3.0f);
    graphics.setColour(MelaColours::custard);
    graphics.fillPath(pointer, juce::AffineTransform::rotation(angle)
                                  .translated(centre.x, centre.y));
    graphics.setColour(MelaColours::ink);
    graphics.fillEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
}
