#include "TouchKeyboard.h"
#include "MelaLookAndFeel.h"

#include <algorithm>

void TouchKeyboard::setBaseMidiNote(int midiNote)
{
    releaseAll();
    baseMidiNote = juce::jlimit(0, 104, midiNote);
    repaint();
}

int TouchKeyboard::getBaseMidiNote() const
{
    return baseMidiNote;
}

void TouchKeyboard::releaseAll()
{
    for (auto& note : activeTouchNotes)
    {
        if (note >= 0 && onNoteOff)
            onNoteOff(note);
        note = -1;
    }
    repaint();
}

void TouchKeyboard::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto whiteWidth = bounds.getWidth() / static_cast<float>(whiteOffsets.size());

    for (int index = 0; index < static_cast<int>(whiteOffsets.size()); ++index)
    {
        const auto note = baseMidiNote + whiteOffsets[static_cast<size_t>(index)];
        const auto key = juce::Rectangle<float>(static_cast<float>(index) * whiteWidth, 0.0f,
                                                whiteWidth, bounds.getHeight());
        graphics.setColour(isNoteActive(note) ? MelaColours::custard
                                              : MelaColours::cream);
        graphics.fillRoundedRectangle(key.reduced(2.0f), 7.0f);
        graphics.setColour(MelaColours::ink);
        graphics.drawRoundedRectangle(key.reduced(2.0f), 7.0f, 2.5f);

        if (note % 12 == 0)
        {
            graphics.setFont(14.0f);
            graphics.drawText(juce::MidiMessage::getMidiNoteName(note, true, true, 4),
                              key.toNearestInt().removeFromBottom(28),
                              juce::Justification::centred);
        }
    }

    const auto blackWidth = whiteWidth * 0.62f;
    const auto blackHeight = bounds.getHeight() * 0.62f;
    for (int index = 0; index < static_cast<int>(blackOffsets.size()); ++index)
    {
        const auto note = baseMidiNote + blackOffsets[static_cast<size_t>(index)];
        const auto centre = static_cast<float>(
            blackBoundaries[static_cast<size_t>(index)]) * whiteWidth;
        const auto key = juce::Rectangle<float>(centre - blackWidth * 0.5f, 0.0f,
                                                blackWidth, blackHeight);
        graphics.setColour(isNoteActive(note) ? MelaColours::coral
                                              : MelaColours::panelDark);
        graphics.fillRoundedRectangle(key, 4.0f);
        graphics.setColour(MelaColours::ink);
        graphics.drawRoundedRectangle(key, 5.0f, 2.5f);
    }
}

void TouchKeyboard::mouseDown(const juce::MouseEvent& event)
{
    moveTouch(event);
}

void TouchKeyboard::mouseDrag(const juce::MouseEvent& event)
{
    moveTouch(event);
}

void TouchKeyboard::mouseUp(const juce::MouseEvent& event)
{
    auto& activeNote = activeTouchNotes[static_cast<size_t>(touchIndex(event))];
    if (activeNote >= 0 && onNoteOff)
        onNoteOff(activeNote);
    activeNote = -1;
    repaint();
}

int TouchKeyboard::noteAt(juce::Point<float> position) const
{
    if (! getLocalBounds().toFloat().contains(position))
        return -1;

    const auto whiteWidth = static_cast<float>(getWidth())
                          / static_cast<float>(whiteOffsets.size());
    const auto blackWidth = whiteWidth * 0.62f;
    const auto blackHeight = static_cast<float>(getHeight()) * 0.62f;

    if (position.y <= blackHeight)
    {
        for (int index = 0; index < static_cast<int>(blackOffsets.size()); ++index)
        {
            const auto centre = static_cast<float>(
                blackBoundaries[static_cast<size_t>(index)]) * whiteWidth;
            if (position.x >= centre - blackWidth * 0.5f
                && position.x <= centre + blackWidth * 0.5f)
                return baseMidiNote + blackOffsets[static_cast<size_t>(index)];
        }
    }

    const auto whiteIndex = juce::jlimit(0, static_cast<int>(whiteOffsets.size()) - 1,
                                         static_cast<int>(position.x / whiteWidth));
    return baseMidiNote + whiteOffsets[static_cast<size_t>(whiteIndex)];
}

int TouchKeyboard::touchIndex(const juce::MouseEvent& event) const
{
    return juce::jlimit(0, static_cast<int>(activeTouchNotes.size()) - 1,
                        event.source.getIndex());
}

void TouchKeyboard::moveTouch(const juce::MouseEvent& event)
{
    auto& activeNote = activeTouchNotes[static_cast<size_t>(touchIndex(event))];
    const auto newNote = noteAt(event.position);
    if (newNote == activeNote)
        return;

    if (activeNote >= 0 && onNoteOff)
        onNoteOff(activeNote);
    activeNote = newNote;
    if (activeNote >= 0 && onNoteOn)
    {
        const auto velocity = juce::jlimit(0.65f, 1.0f,
            1.0f - 0.35f * event.position.y / juce::jmax(1.0f, static_cast<float>(getHeight())));
        onNoteOn(activeNote, velocity);
    }
    repaint();
}

bool TouchKeyboard::isNoteActive(int midiNote) const
{
    return std::find(activeTouchNotes.begin(), activeTouchNotes.end(), midiNote)
        != activeTouchNotes.end();
}
