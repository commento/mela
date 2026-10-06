#include "MelaLookAndFeel.h"

MelaLookAndFeel::MelaLookAndFeel()
{
    setColourScheme(juce::LookAndFeel_V4::ColourScheme(
        MelaColours::background, MelaColours::surface, MelaColours::border,
        MelaColours::text, MelaColours::border, MelaColours::background,
        MelaColours::text, MelaColours::selected, MelaColours::text));
    setColour(juce::TextButton::buttonColourId, MelaColours::background);
    setColour(juce::TextButton::buttonOnColourId, MelaColours::selected);
    setColour(juce::TextButton::textColourOffId, MelaColours::text);
    setColour(juce::TextButton::textColourOnId, MelaColours::text);
    setColour(juce::ToggleButton::textColourId, MelaColours::text);
    setColour(juce::Label::textColourId, MelaColours::text);
    setColour(juce::ComboBox::backgroundColourId, MelaColours::background);
    setColour(juce::ComboBox::textColourId, MelaColours::text);
    setColour(juce::ComboBox::outlineColourId, MelaColours::border);
    setColour(juce::ComboBox::arrowColourId, MelaColours::text);
    setColour(juce::Slider::textBoxTextColourId, MelaColours::text);
    setColour(juce::Slider::textBoxBackgroundColourId, MelaColours::background);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId, MelaColours::background);
    setColour(juce::PopupMenu::textColourId, MelaColours::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, MelaColours::selected);
    setColour(juce::PopupMenu::highlightedTextColourId, MelaColours::text);
    setColour(juce::TextEditor::backgroundColourId, MelaColours::background);
    setColour(juce::TextEditor::textColourId, MelaColours::text);
    setColour(juce::TextEditor::outlineColourId, MelaColours::border);
    setColour(juce::TextEditor::focusedOutlineColourId, MelaColours::text);
    setColour(juce::TextEditor::highlightColourId, MelaColours::selected);
    setColour(juce::TextEditor::highlightedTextColourId, MelaColours::text);
    setColour(juce::CaretComponent::caretColourId, MelaColours::text);
}

juce::Font MelaLookAndFeel::interfaceFont(float height) const
{
    return juce::Font(juce::FontOptions(height));
}

juce::Font MelaLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return interfaceFont(juce::jmin(27.0f, static_cast<float>(buttonHeight) * 0.40f));
}

juce::Font MelaLookAndFeel::getLabelFont(juce::Label& label)
{
    return label.getFont().withHeight(juce::jlimit(19.5f, 36.0f,
                                                  label.getFont().getHeight() * 1.5f));
}

juce::Font MelaLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return interfaceFont(juce::jmin(27.0f, static_cast<float>(box.getHeight()) * 0.40f));
}

juce::PopupMenu::Options MelaLookAndFeel::getOptionsForComboBoxPopupMenu(
    juce::ComboBox& box, juce::Label& label)
{
    auto options = juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label);
   #if JUCE_LINUX
    // Keep popups inside the main window to avoid stale pixels on touch-only X11.
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
    const auto area = button.getLocalBounds().toFloat().reduced(4.5f);
    const auto selected = button.getToggleState() || background.getBrightness()
                                                   >= MelaColours::active.getBrightness();
    const auto fill = down ? MelaColours::selected
                          : highlighted ? background.brighter(0.08f) : background;
    const auto opacity = button.isEnabled() ? 1.0f : 0.4f;
    graphics.setColour(fill.withAlpha(opacity));
    graphics.fillRoundedRectangle(area, 6.0f);
    graphics.setColour((selected || highlighted || button.hasKeyboardFocus(true)
                            ? MelaColours::text : MelaColours::border).withAlpha(opacity));
    graphics.drawRoundedRectangle(area, 6.0f, selected ? 2.0f : 1.0f);
}

void MelaLookAndFeel::drawToggleButton(juce::Graphics& graphics,
                                       juce::ToggleButton& button,
                                       bool highlighted, bool down)
{
    const auto area = button.getLocalBounds().toFloat().reduced(4.5f);
    drawButtonBackground(graphics, button,
                         button.getToggleState() ? MelaColours::selected
                                                 : MelaColours::background,
                         highlighted, down);
    graphics.setColour(MelaColours::text.withAlpha(button.isEnabled() ? 1.0f : 0.4f));
    graphics.setFont(interfaceFont(juce::jmin(25.5f, area.getHeight() * 0.40f)));
    graphics.drawText(button.getButtonText(), area.toNearestInt().reduced(12, 3),
                      juce::Justification::centred);
}

void MelaLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height,
                                   bool down, int, int, int, int, juce::ComboBox& box)
{
    const auto area = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                            static_cast<float>(height)).reduced(3.0f);
    graphics.setColour(down ? MelaColours::active : MelaColours::background);
    graphics.fillRoundedRectangle(area, 6.0f);
    graphics.setColour(box.hasKeyboardFocus(true) ? MelaColours::text : MelaColours::border);
    graphics.drawRoundedRectangle(area, 6.0f, 1.0f);

    const auto centreX = area.getRight() - 27.0f;
    const auto centreY = area.getCentreY();
    juce::Path arrow;
    arrow.startNewSubPath(centreX - 7.0f, centreY - 3.5f);
    arrow.lineTo(centreX, centreY + 3.5f);
    arrow.lineTo(centreX + 7.0f, centreY - 3.5f);
    graphics.setColour(MelaColours::text.withAlpha(box.isEnabled() ? 1.0f : 0.4f));
    graphics.strokePath(arrow, juce::PathStrokeType(1.5f));
}

void MelaLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y,
                                       int width, int height, float position,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto size = juce::jmax(0.0f, static_cast<float>(juce::jmin(width, height)) - 21.0f);
    const auto centre = juce::Point<float>(static_cast<float>(x) + static_cast<float>(width) * 0.5f,
                                          static_cast<float>(y) + static_cast<float>(height) * 0.5f);
    const auto radius = size * 0.5f;
    const auto angle = startAngle + position * (endAngle - startAngle);
    const auto opacity = slider.isEnabled() ? 1.0f : 0.4f;

    graphics.setColour(MelaColours::background);
    graphics.fillEllipse(centre.x - radius, centre.y - radius, size, size);
    graphics.setColour(MelaColours::border.withAlpha(opacity));
    graphics.drawEllipse(centre.x - radius, centre.y - radius, size, size, 1.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    graphics.setColour(MelaColours::text.withAlpha(opacity));
    graphics.strokePath(arc, juce::PathStrokeType(2.0f));
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.5f, -radius + 7.0f, 3.0f, radius * 0.5f, 1.5f);
    graphics.fillPath(pointer, juce::AffineTransform::rotation(angle)
                                  .translated(centre.x, centre.y));
}
