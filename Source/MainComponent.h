#pragma once

#include <JuceHeader.h>
#include "Audio/LoopEngine.h"
#include "UI/EffectPanel.h"
#include "UI/TouchKeyboard.h"
#include "UI/WaveformEditor.h"
#include <array>
#include <vector>

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
        audio,
        wifi,
        loop,
        keys,
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
        int instrumentRootNote = 60;
        int keyboardBaseNote = 48;
        LoopEngine::InstrumentMode instrumentMode = LoopEngine::InstrumentMode::gate;
        double keyAttack = 0.01;
        double keyDecay = 0.1;
        double keySustain = 1.0;
        double keyRelease = 0.15;
    };

    struct SlotEffectSettings
    {
        bool distortionEnabled = false;
        std::array<double, 3> distortion { 2.0, 12000.0, 0.5 };
        bool granularEnabled = false;
        std::array<double, 5> granular { 80.0, 12.0, 250.0, 0.0, 0.5 };
        bool flangerEnabled = false;
        std::array<double, 4> flanger { 0.25, 0.5, 0.2, 0.35 };
        bool chorusEnabled = false;
        std::array<double, 3> chorus { 0.8, 0.35, 0.35 };
        double delaySend = 0.15;
        double reverbSend = 0.15;
    };

    struct MasterEffectSettings
    {
        bool delayEnabled = false;
        std::array<double, 3> delay { 350.0, 0.35, 0.3 };
        bool reverbEnabled = false;
        std::array<double, 3> reverb { 0.5, 0.5, 0.25 };
    };

    void chooseFile();
    void toggleRecording();
    void stopAndLoadRecording();
    void initialiseAudio(bool microphonePermissionGranted);
    void refreshWifiLibrary(bool announceResult);
    void loadWifiSampleIntoSlot(int slotIndex);
    void timerCallback() override;
    void updateEnvelope();
    void updateEffects();
    void selectEffectTarget(int targetIndex);
    void updateEffectPageVisibility();
    void selectSlot(int slotIndex);
    void showPage(Page pageToShow);
    void updateSlotButtonColours();
    void updateInstrumentControls();
    void updateKeyboardEnvelope();

    juce::AudioDeviceManager deviceManager;
    LoopEngine engine;
    Page currentPage = Page::audio;
    int activeSlot = 0;
    int recordingSlot = -1;
    std::array<SlotSettings, LoopEngine::numberOfSlots> slotSettings;
    std::array<SlotEffectSettings, LoopEngine::numberOfSlots> slotEffectSettings;
    MasterEffectSettings masterEffectSettings;
    int effectTarget = 0;

    WaveformEditor waveform;
    TouchKeyboard touchKeyboard;
    std::array<juce::TextButton, LoopEngine::numberOfSlots> sampleButtons;
    juce::TextButton loadButton { "CARICA LOOP" };
    juce::TextButton recordButton { "REC" };
    juce::TextButton playButton { "PLAY LOOP" };
    juce::TextButton stopButton { "STOP SLOT" };
    juce::TextButton stopAllButton { "STOP ALL" };
    juce::TextButton audioPageButton { "AUDIO" };
    juce::TextButton wifiPageButton { "WIFI" };
    juce::TextButton loopPageButton { "LOOP" };
    juce::TextButton keysPageButton { "KEYS" };
    juce::TextButton effectsPageButton { "FX" };
    juce::ToggleButton loopButton { "RIPETI" };
    juce::ToggleButton reverseButton { "REVERSE" };
    juce::ToggleButton envelopeCycleButton { "ADSR CICLICO" };
    juce::TextButton octaveDownButton { "OCT -" };
    juce::TextButton octaveUpButton { "OCT +" };
    juce::TextButton rootDownButton { "ROOT -" };
    juce::TextButton rootUpButton { "ROOT +" };
    juce::Label rootNoteLabel;
    juce::Label instrumentModeLabel;
    juce::ComboBox instrumentModeBox;
    juce::Slider keyAttackSlider;
    juce::Slider keyDecaySlider;
    juce::Slider keySustainSlider;
    juce::Slider keyReleaseSlider;
    juce::Label keyAttackLabel;
    juce::Label keyDecayLabel;
    juce::Label keySustainLabel;
    juce::Label keyReleaseLabel;

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
    juce::Label audioInfoLabel;
    juce::TextButton continueButton { "APRI I SAMPLE" };
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioDeviceSelector;
    juce::Label wifiInfoLabel;
    juce::Label wifiAddressLabel;
    juce::Label wifiPinLabel;
    juce::Label wifiInboxLabel;
    juce::ComboBox wifiFileBox;
    juce::TextButton wifiRefreshButton { "AGGIORNA" };
    std::array<juce::TextButton, LoopEngine::numberOfSlots> wifiLoadButtons;
    std::vector<juce::File> wifiFiles;
    juce::File wifiInboxDirectory;
    int wifiRefreshTicks = 0;

    EffectPanel distortionPanel;
    EffectPanel granularPanel;
    EffectPanel flangerPanel;
    EffectPanel chorusPanel;
    EffectPanel delayPanel;
    EffectPanel reverbPanel;
    std::array<juce::TextButton, LoopEngine::numberOfSlots + 1> effectTargetButtons;
    juce::Slider delaySendSlider;
    juce::Slider reverbSendSlider;
    juce::Label delaySendLabel;
    juce::Label reverbSendLabel;
    juce::Label dspLoadLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
