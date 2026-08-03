#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>

class TouchKeyboard final : public juce::Component
{
public:
    std::function<void(int, float)> onNoteOn;
    std::function<void(int)> onNoteOff;

    void setBaseMidiNote(int midiNote);
    [[nodiscard]] int getBaseMidiNote() const;
    void releaseAll();

    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    [[nodiscard]] int noteAt(juce::Point<float> position) const;
    [[nodiscard]] int touchIndex(const juce::MouseEvent& event) const;
    void moveTouch(const juce::MouseEvent& event);
    [[nodiscard]] bool isNoteActive(int midiNote) const;

    static constexpr std::array<int, 14> whiteOffsets {
        0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23
    };
    static constexpr std::array<int, 10> blackOffsets {
        1, 3, 6, 8, 10, 13, 15, 18, 20, 22
    };
    static constexpr std::array<int, 10> blackBoundaries {
        1, 2, 4, 5, 6, 8, 9, 11, 12, 13
    };

    int baseMidiNote = 48;
    std::array<int, 16> activeTouchNotes {
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1
    };
};
