#pragma once

#include <JuceHeader.h>
#include "Audio/LoopEngine.h"
#include "UI/WaveformEditor.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void chooseFile();
    void timerCallback() override;

    juce::AudioDeviceManager deviceManager;
    LoopEngine engine;
    WaveformEditor waveform;
    juce::TextButton loadButton { "CARICA LOOP" };
    juce::TextButton playButton { "PLAY" };
    juce::TextButton stopButton { "STOP" };
    juce::ToggleButton loopButton { "RIPETI LOOP" };
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
    std::unique_ptr<juce::FileChooser> fileChooser;

    void updateEnvelope();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
