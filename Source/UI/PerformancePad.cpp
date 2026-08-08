#include "PerformancePad.h"
#include "MelaLookAndFeel.h"

#include <cmath>

namespace
{
constexpr std::array<const char*, 5> modeNames {
    "REPEAT", "REVERSE", "GLITCH", "FILTRO", "FLANGER"
};
constexpr std::array<const char*, 4> sliceNames { "400 ms", "200 ms", "100 ms", "50 ms" };
constexpr std::array<float, 4> sliceValues { 400.0f, 200.0f, 100.0f, 50.0f };

float sliceMsFromX(float x)
{
    return 30.0f * std::pow(500.0f / 30.0f, 1.0f - x);
}
}

PerformancePad::PerformancePad()
{
    setWantsKeyboardFocus(false);
    for (int index = 0; index < static_cast<int>(modeButtons.size()); ++index)
    {
        auto& button = modeButtons[static_cast<size_t>(index)];
        button.setButtonText(modeNames[static_cast<size_t>(index)]);
        button.onClick = [this, index] { setMode(static_cast<Mode>(index)); };
        addAndMakeVisible(button);
    }
    for (int index = 0; index < static_cast<int>(sliceButtons.size()); ++index)
    {
        auto& button = sliceButtons[static_cast<size_t>(index)];
        button.setButtonText(sliceNames[static_cast<size_t>(index)]);
        button.onClick = [this, index] { setSliceMs(sliceValues[static_cast<size_t>(index)]); };
        addAndMakeVisible(button);
    }
    updateButtonColours();
}

juce::Rectangle<float> PerformancePad::padBounds() const
{
    auto area = getLocalBounds().toFloat().reduced(14.0f);
    area.removeFromTop(54.0f);
    area.removeFromRight(270.0f);
    return area.reduced(5.0f, 2.0f);
}

void PerformancePad::paint(juce::Graphics& graphics)
{
    graphics.setColour(MelaColours::panelDark);
    graphics.fillRoundedRectangle(getLocalBounds().toFloat(), 22.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 22.0f, 4.0f);

    graphics.setColour(MelaColours::cream);
    graphics.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    graphics.drawText("TIENI PREMUTO E MUOVI IL DITO", 28, 12, getWidth() - 56, 34,
                      juce::Justification::centredLeft);

    const auto pad = padBounds();
    juce::ColourGradient field(MelaColours::sky.darker(0.55f), pad.getBottomLeft(),
                               MelaColours::coral, pad.getTopRight(), false);
    field.addColour(0.52, MelaColours::panel);
    graphics.setGradientFill(field);
    graphics.fillRoundedRectangle(pad, 24.0f);

    graphics.setColour(MelaColours::cream.withAlpha(0.16f));
    for (int index = 1; index < 8; ++index)
    {
        const auto fraction = static_cast<float>(index) / 8.0f;
        graphics.drawVerticalLine(juce::roundToInt(pad.getX() + fraction * pad.getWidth()),
                                  pad.getY() + 12.0f, pad.getBottom() - 12.0f);
        graphics.drawHorizontalLine(juce::roundToInt(pad.getY() + fraction * pad.getHeight()),
                                    pad.getX() + 12.0f, pad.getRight() - 12.0f);
    }
    graphics.setColour(touching ? MelaColours::custard : MelaColours::cream.withAlpha(0.65f));
    graphics.drawRoundedRectangle(pad, 24.0f, touching ? 6.0f : 3.0f);

    const auto point = juce::Point<float>(pad.getX() + x * pad.getWidth(),
                                          pad.getBottom() - y * pad.getHeight());
    graphics.setColour(MelaColours::ink.withAlpha(0.45f));
    graphics.fillEllipse(point.x - 32.0f, point.y - 26.0f, 64.0f, 64.0f);
    graphics.setColour(touching ? MelaColours::custard : MelaColours::cream);
    graphics.fillEllipse(point.x - 28.0f, point.y - 32.0f, 56.0f, 56.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawEllipse(point.x - 28.0f, point.y - 32.0f, 56.0f, 56.0f, 4.0f);

    graphics.setColour(MelaColours::cream.withAlpha(0.8f));
    graphics.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    const auto xAxis = mode == Mode::filter ? "CUTOFF  BASSO  >  ALTO"
                     : mode == Mode::flanger ? "RATE  LENTO  >  VELOCE"
                                             : "SLICE  LUNGA  >  MICRO";
    graphics.drawText(xAxis, pad.toNearestInt().removeFromBottom(30),
                      juce::Justification::centred);
    graphics.saveState();
    graphics.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
                                                           pad.getX() + 18.0f,
                                                           pad.getCentreY()));
    const auto yAxis = mode == Mode::filter ? "RISONANZA  >"
                     : mode == Mode::flanger ? "DEPTH  >" : "INTENSITA  >";
    graphics.drawText(yAxis, juce::roundToInt(pad.getX() - pad.getHeight() * 0.5f + 18.0f),
                      juce::roundToInt(pad.getCentreY() - 14.0f),
                      juce::roundToInt(pad.getHeight()), 28, juce::Justification::centred);
    graphics.restoreState();

    auto side = getLocalBounds().reduced(18);
    side.removeFromTop(58);
    side = side.removeFromRight(245);
    graphics.setColour(MelaColours::custard);
    graphics.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    graphics.drawText("CARATTERE", side.removeFromTop(28), juce::Justification::centredLeft);
    side.removeFromTop(274);
    graphics.drawText("TAGLI RAPIDI", side.removeFromTop(32), juce::Justification::centredLeft);
    side.removeFromTop(154);
    graphics.setColour(MelaColours::cream);
    graphics.setFont(juce::FontOptions(16.0f));
    juce::String readout;
    if (mode == Mode::filter)
    {
        const auto cutoff = 80.0f * std::pow(20000.0f / 80.0f, x);
        readout = "CUTOFF  " + juce::String(cutoff >= 1000.0f ? cutoff / 1000.0f : cutoff,
                                             cutoff >= 1000.0f ? 1 : 0)
                + (cutoff >= 1000.0f ? " kHz" : " Hz")
                + "\nRISONANZA  " + juce::String(juce::roundToInt(y * 100.0f)) + "%";
    }
    else if (mode == Mode::flanger)
    {
        const auto rate = 0.05f * std::pow(120.0f, x);
        readout = "RATE  " + juce::String(rate, 2) + " Hz\nDEPTH  "
                + juce::String(juce::roundToInt(y * 100.0f)) + "%";
    }
    else
    {
        readout = "SLICE  " + juce::String(sliceMsFromX(x), 0) + " ms\nINTENSITA  "
                + juce::String(juce::roundToInt(y * 100.0f)) + "%";
    }
    graphics.drawFittedText(readout, side, juce::Justification::centredLeft, 2);
}

void PerformancePad::resized()
{
    auto side = getLocalBounds().reduced(18);
    side.removeFromTop(86);
    side = side.removeFromRight(245);
    for (auto& button : modeButtons)
        button.setBounds(side.removeFromTop(54).reduced(2, 4));
    side.removeFromTop(50);
    for (auto& button : sliceButtons)
        button.setBounds(side.removeFromTop(42).reduced(2, 3));
}

void PerformancePad::mouseDown(const juce::MouseEvent& event)
{
    if (! hardwareTouchEnabled && padBounds().contains(event.position))
        updateGesture(event.position, true);
}

void PerformancePad::mouseDrag(const juce::MouseEvent& event)
{
    if (! hardwareTouchEnabled && touching)
        updateGesture(event.position, true);
}

void PerformancePad::mouseUp(const juce::MouseEvent& event)
{
    if (! hardwareTouchEnabled && touching)
        updateGesture(event.position, false);
}

void PerformancePad::setHardwareTouchEnabled(bool enabled)
{
    hardwareTouchEnabled = enabled;
    if (! enabled)
        releaseGesture();
}

void PerformancePad::setHardwareTouch(std::optional<juce::Point<float>> position)
{
    if (! hardwareTouchEnabled)
        return;
    if (position.has_value())
        updateGesture(*position, true);
    else
        releaseGesture();
}

void PerformancePad::releaseGesture()
{
    if (! touching)
        return;
    touching = false;
    if (onGesture)
        onGesture(false, x, y, mode);
    repaint();
}

bool PerformancePad::containsPadPosition(juce::Point<float> position) const
{
    return padBounds().contains(position);
}

void PerformancePad::updateGesture(juce::Point<float> position, bool active)
{
    const auto pad = padBounds();
    x = juce::jlimit(0.0f, 1.0f, (position.x - pad.getX()) / pad.getWidth());
    y = juce::jlimit(0.0f, 1.0f, (pad.getBottom() - position.y) / pad.getHeight());
    touching = active;
    if (onGesture)
        onGesture(touching, x, y, mode);
    repaint();
}

void PerformancePad::setMode(Mode newMode)
{
    mode = newMode;
    updateButtonColours();
    if (onGesture)
        onGesture(touching, x, y, mode);
}

void PerformancePad::setSliceMs(float milliseconds)
{
    x = 1.0f - std::log(milliseconds / 30.0f) / std::log(500.0f / 30.0f);
    x = juce::jlimit(0.0f, 1.0f, x);
    if (onGesture)
        onGesture(touching, x, y, mode);
    repaint();
}

void PerformancePad::updateButtonColours()
{
    for (int index = 0; index < static_cast<int>(modeButtons.size()); ++index)
        modeButtons[static_cast<size_t>(index)].setColour(
            juce::TextButton::buttonColourId,
            static_cast<int>(mode) == index ? MelaColours::coral : MelaColours::panel);
    for (auto& button : sliceButtons)
        button.setColour(juce::TextButton::buttonColourId, MelaColours::panel);
}
