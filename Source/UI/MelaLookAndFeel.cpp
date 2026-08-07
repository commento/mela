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
    return cartoonFont(juce::jmin(30.0f, static_cast<float>(buttonHeight) * 0.42f));
}

juce::Font MelaLookAndFeel::getLabelFont(juce::Label& label)
{
    const auto requested = label.getFont().getHeight();
    return cartoonFont(juce::jlimit(19.5f, 39.0f, requested * 1.5f));
}

juce::Font MelaLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return cartoonFont(juce::jmin(28.5f, static_cast<float>(box.getHeight()) * 0.42f));
}

juce::PopupMenu::Options MelaLookAndFeel::getOptionsForComboBoxPopupMenu(
    juce::ComboBox& box, juce::Label& label)
{
    auto options = juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label);
   #if JUCE_LINUX
    // On Raspberry Pi/X11 a temporary popup window can leave stale pixels after
    // it closes. Keeping the popup inside Mela's main window makes JUCE repaint
    // the uncovered area as part of the normal component hierarchy.
    if (auto* topLevel = box.getTopLevelComponent())
        options = options.withParentComponent(topLevel);
   #endif
    return options;
}

void MelaLookAndFeel::drawButtonBackground(juce::Graphics& graphics,
                                           juce::Button& button,
                                           const juce::Colour& background,
                                           bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(4.5f);
    if (down)
        area = area.translated(0.0f, 3.0f);

    const auto fill = down ? background.darker(0.12f)
                           : highlighted ? background.brighter(0.10f) : background;
    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillRoundedRectangle(area.translated(0.0f, 4.5f), 16.5f);
    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, 16.5f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 16.5f, 4.5f);
}

void MelaLookAndFeel::drawToggleButton(juce::Graphics& graphics,
                                       juce::ToggleButton& button,
                                       bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(4.5f);
    auto fill = button.getToggleState() ? MelaColours::custard : MelaColours::panelDark;
    if (highlighted)
        fill = fill.brighter(0.08f);
    if (down)
        area = area.translated(0.0f, 3.0f);

    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillRoundedRectangle(area.translated(0.0f, 4.5f), 16.5f);
    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, 16.5f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 16.5f, 4.5f);
    graphics.setColour(button.getToggleState() ? MelaColours::ink : MelaColours::cream);
    graphics.setFont(cartoonFont(juce::jmin(27.0f, area.getHeight() * 0.40f)));
    graphics.drawText(button.getButtonText(), area.toNearestInt().reduced(12, 3),
                      juce::Justification::centred);
}

void MelaLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height,
                                   bool down, int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                       static_cast<float>(height)).reduced(3.0f);
    graphics.setColour(down ? MelaColours::panel : MelaColours::panelDark);
    graphics.fillRoundedRectangle(area, 15.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(area, 15.0f, 4.5f);

    const auto centreX = area.getRight() - 27.0f;
    const auto centreY = area.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath(centreX - 9.0f, centreY - 4.5f);
    arrow.lineTo(centreX, centreY + 6.0f);
    arrow.lineTo(centreX + 9.0f, centreY - 4.5f);
    graphics.setColour(MelaColours::custard);
    graphics.strokePath(arrow, juce::PathStrokeType(4.5f,
                        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MelaLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y,
                                       int width, int height, float position,
                                       float startAngle, float endAngle, juce::Slider&)
{
    const auto size = static_cast<float>(juce::jmin(width, height)) - 21.0f;
    const auto centre = juce::Point<float>(static_cast<float>(x)
                                               + static_cast<float>(width) * 0.5f,
                                           static_cast<float>(y)
                                               + static_cast<float>(height) * 0.5f);
    const auto radius = size * 0.5f;
    const auto angle = startAngle + position * (endAngle - startAngle);

    graphics.setColour(MelaColours::ink.withAlpha(0.65f));
    graphics.fillEllipse(centre.x - radius, centre.y - radius + 6.0f, size, size);
    graphics.setColour(MelaColours::sky);
    graphics.fillEllipse(centre.x - radius, centre.y - radius, size, size);
    graphics.setColour(MelaColours::ink);
    graphics.drawEllipse(centre.x - radius, centre.y - radius, size, size, 6.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-5.25f, -radius + 12.0f, 10.5f, radius * 0.58f, 4.5f);
    graphics.setColour(MelaColours::custard);
    graphics.fillPath(pointer, juce::AffineTransform::rotation(angle)
                                  .translated(centre.x, centre.y));
    graphics.setColour(MelaColours::ink);
    graphics.fillEllipse(centre.x - 7.5f, centre.y - 7.5f, 15.0f, 15.0f);
}
