#pragma once

#include <JuceHeader.h>
#include "Audio/LoopEngine.h"
#include "UI/EffectPanel.h"
#include "UI/WaveformEditor.h"
#include <array>

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    enum class Page
    {
        loop,
        effects
    };

    struct SlotSettings
    {
        bool looping = true;
        bool reverse = false;
        bool envelopeCycle = true;
        double speed = 1.0;
        double gain = 0.8;
        double attack = 0.02;
        double decay = 0.1;
        double sustain = 1.0;
        double release = 0.0;
        double trimStart = 0.0;
        double trimEnd = 1.0;
    };

    void chooseFile();
    void timerCallback() override;
    void updateEnvelope();
    void updateEffects();
    void selectSlot(int slotIndex);
    void showPage(Page pageToShow);
    void updateSlotButtonColours();

    juce::AudioDeviceManager deviceManager;
    LoopEngine engine;
    Page currentPage = Page::loop;
    int activeSlot = 0;
    std::array<SlotSettings, LoopEngine::numberOfSlots> slotSettings;

    WaveformEditor waveform;
    std::array<juce::TextButton, LoopEngine::numberOfSlots> sampleButtons;
    juce::TextButton loadButton { "CARICA LOOP" };
    juce::TextButton playButton { "PLAY SLOT" };
    juce::TextButton stopButton { "STOP SLOT" };
    juce::TextButton stopAllButton { "STOP ALL" };
    juce::TextButton loopPageButton { "LOOP" };
    juce::TextButton effectsPageButton { "FX" };
    juce::ToggleButton loopButton { "RIPETI" };
    juce::ToggleButton reverseButton { "REVERSE" };
    juce::ToggleButton envelopeCycleButton { "ADSR CICLICO" };

    juce::Slider speedSlider;
    juce::Slider gainSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Label speedLabel;
    juce::Label gainLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label clipName;
    juce::Label statusLabel;

    EffectPanel distortionPanel;
    EffectPanel granularPanel;
    EffectPanel flangerPanel;
    EffectPanel chorusPanel;
    EffectPanel delayPanel;
    EffectPanel reverbPanel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
