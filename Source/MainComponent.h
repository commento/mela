#pragma once

#include <JuceHeader.h>
#include "Audio/LoopEngine.h"
#include "Input/LinuxMultiTouchInput.h"
#include "UI/EffectPanel.h"
#include "UI/EqualizerPanel.h"
#include "UI/MelaLookAndFeel.h"
#include "UI/PerformancePad.h"
#include "UI/TouchSlider.h"
#include "UI/TouchKeyboard.h"
#include "UI/TouchSampleBrowser.h"
#include "UI/WaveformEditor.h"
#include <array>
#include <atomic>
#include <optional>
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
        network,
        loop,
        keys,
        effects,
        performance,
        scenes
    };

    struct SlotSettings
    {
        bool looping = true;
        bool reverse = false;
        bool envelopeCycle = true;
        double speed = 1.0;
        bool timeStretch = false;
        double pitch = 0.0;
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
        std::array<double, 3> equalizer { 0.0, 0.0, 0.0 };
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
        std::array<double, 3> equalizer { 0.0, 0.0, 0.0 };
        bool delayEnabled = false;
        std::array<double, 3> delay { 350.0, 0.35, 0.3 };
        bool reverbEnabled = false;
        std::array<double, 3> reverb { 0.5, 0.5, 0.25 };
    };

    struct DroneSettings
    {
        bool enabled = false;
        int midiNote = 24;
        int keyboardBaseNote = 24;
        double detuneCents = 7.0;
        double gain = 0.25;
        double attack = 0.1;
        double decay = 0.4;
        double sustain = 0.8;
        double release = 1.5;
        int waveform = 0;
        bool latch = true;
    };

    void chooseFile();
    void loadFileIntoActiveSlot(const juce::File& file);
    void deleteActiveSample();
    void clearActiveSlotAfterDelete();
    void toggleRecording();
    void stopAndLoadRecording();
    void initialiseAudio(bool microphonePermissionGranted);
    void refreshWifiLibrary(bool announceResult);
    void loadWifiSampleIntoSlot(int slotIndex);
    void deleteSelectedWifiSample();
    void refreshWifiNetworks(bool announceResult);
    void connectSelectedWifiNetwork();
    void updateWifiStatus();
    void setWifiBusy(bool busy, const juce::String& message);
    void updateWifiKeyboardLabels();
    void showPowerDialog();
    void performPowerAction(bool restart);
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
    void selectDroneKeys();
    void updateKeysTargetVisibility();
    void updateDroneEnabledState();
    juce::var createSceneState(const juce::String& sceneName) const;
    bool restoreSceneState(const juce::var& state, juce::String& errorMessage);
    void applyAllSettingsToEngine();
    void selectScene(int sceneIndex);
    void saveSelectedScene(bool askBeforeOverwrite);
    void loadSelectedScene();
    void renameSelectedScene();
    void deleteSelectedScene();
    void refreshSceneButtons();
    void saveAutosaveIfChanged();
    void handleHardwareTouches(const LinuxMultiTouchInput::Snapshot& touches);
    juce::File sceneFile(int sceneIndex) const;
    bool writeSceneFile(const juce::File& file, const juce::var& state) const;

    MelaLookAndFeel melaLookAndFeel;
    TouchSampleBrowser sampleBrowser;
    LinuxMultiTouchInput multiTouchInput;
    juce::AudioDeviceManager deviceManager;
    LoopEngine engine;
    Page currentPage = Page::audio;
    int activeSlot = 0;
    int recordingSlot = -1;
    std::array<SlotSettings, LoopEngine::numberOfSlots> slotSettings;
    std::array<SlotEffectSettings, LoopEngine::numberOfSlots> slotEffectSettings;
    SlotEffectSettings droneEffectSettings;
    std::array<juce::File, LoopEngine::numberOfSlots> slotSourceFiles;
    std::array<bool, LoopEngine::numberOfSlots> slotSourceIsRecording {};
    MasterEffectSettings masterEffectSettings;
    DroneSettings droneSettings;
    static constexpr int droneEffectTarget = LoopEngine::numberOfSlots;
    static constexpr int masterEffectTarget = LoopEngine::numberOfSlots + 1;
    int effectTarget = 0;
    bool droneKeysSelected = false;
    std::vector<int> droneHeldNotes;

    enum class HardwareTouchTarget
    {
        none,
        uiControl,
        waveform,
        keyboard,
        performance
    };
    std::array<HardwareTouchTarget, LinuxMultiTouchInput::maximumTouches>
        hardwareTouchTargets {};

    WaveformEditor waveform;
    TouchKeyboard touchKeyboard;
    std::array<juce::TextButton, LoopEngine::numberOfSlots> sampleButtons;
    juce::TextButton droneKeysButton { "DRONE" };
    juce::TextButton loadButton { "CARICA LOOP" };
    juce::TextButton deleteSampleButton { "ELIMINA SAMPLE" };
    juce::TextButton recordButton { "REC" };
    juce::TextButton playButton { "PLAY LOOP" };
    juce::TextButton stopButton { "STOP SLOT" };
    juce::TextButton stopAllButton { "STOP ALL" };
    juce::TextButton audioPageButton { "AUDIO" };
    juce::TextButton wifiPageButton { "WIFI" };
    juce::TextButton networkPageButton { "RETE" };
    juce::TextButton loopPageButton { "LOOP" };
    juce::TextButton keysPageButton { "KEYS" };
    juce::TextButton effectsPageButton { "FX" };
    juce::TextButton performancePageButton { "XY" };
    juce::TextButton scenesPageButton { "SCENE" };
    juce::TextButton powerButton { "POWER" };
    juce::ToggleButton loopButton { "RIPETI" };
    juce::ToggleButton reverseButton { "REVERSE" };
    juce::ToggleButton envelopeCycleButton { "ADSR CICLICO" };
    juce::ToggleButton timeStretchButton { "STRETCH" };
    juce::TextButton octaveDownButton { "OCT -" };
    juce::TextButton octaveUpButton { "OCT +" };
    juce::Label rootNoteLabel;
    juce::Label instrumentModeLabel;
    juce::ComboBox instrumentModeBox;
    TouchSlider keyAttackSlider;
    TouchSlider keyDecaySlider;
    TouchSlider keySustainSlider;
    TouchSlider keyReleaseSlider;
    juce::Label keyAttackLabel;
    juce::Label keyDecayLabel;
    juce::Label keySustainLabel;
    juce::Label keyReleaseLabel;
    juce::ToggleButton droneButton { "BASS DRONE" };
    juce::ToggleButton droneLatchButton { "LATCH" };
    TouchSlider droneDetuneSlider;
    TouchSlider droneGainSlider;
    juce::Label droneDetuneLabel;
    juce::Label droneGainLabel;
    juce::Label droneWaveformLabel;
    juce::ComboBox droneWaveformBox;

    TouchSlider speedSlider;
    TouchSlider pitchSlider;
    TouchSlider gainSlider;
    TouchSlider attackSlider;
    TouchSlider decaySlider;
    TouchSlider sustainSlider;
    TouchSlider releaseSlider;
    juce::Label speedLabel;
    juce::Label pitchLabel;
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
    juce::Label wifiLibraryInfoLabel;
    juce::Label wifiAddressLabel;
    juce::Label wifiPinLabel;
    juce::Label wifiInboxLabel;
    juce::ComboBox wifiFileBox;
    juce::TextButton wifiLibraryRefreshButton { "AGGIORNA" };
    juce::TextButton wifiDeleteButton { "ELIMINA" };
    std::array<juce::TextButton, LoopEngine::numberOfSlots> wifiLoadButtons;
    std::vector<juce::File> wifiFiles;
    juce::File wifiInboxDirectory;
    juce::Label wifiInfoLabel;
    juce::Label wifiStatusLabel;
    juce::Label wifiNetworkLabel;
    juce::ComboBox wifiNetworkBox;
    juce::Label wifiPasswordLabel;
    juce::TextEditor wifiPasswordEditor;
    juce::ToggleButton wifiShowPasswordButton { "MOSTRA PASSWORD" };
    juce::ToggleButton wifiShiftButton { "MAIUSC" };
    juce::TextButton wifiRefreshButton { "CERCA RETI" };
    juce::TextButton wifiConnectButton { "CONNETTI" };
    std::array<juce::TextButton, 45> wifiKeyboardButtons;
    std::vector<juce::String> wifiNetworks;
    std::atomic_bool wifiBusy { false };
    std::atomic_bool powerActionPending { false };
    int wifiRefreshTicks = 0;

    static constexpr int numberOfScenes = 8;
    std::array<juce::TextButton, numberOfScenes> sceneButtons;
    juce::TextButton sceneRecallButton { "RICHIAMA" };
    juce::TextButton sceneSaveButton { "SALVA / SOVRASCRIVI" };
    juce::TextButton sceneRenameButton { "RINOMINA" };
    juce::TextButton sceneDeleteButton { "ELIMINA" };
    juce::Label sceneInfoLabel;
    juce::File scenesDirectory;
    int selectedScene = 0;
    int autosaveTicks = 0;
    juce::String lastAutosaveState;

    EffectPanel distortionPanel;
    EffectPanel granularPanel;
    EffectPanel flangerPanel;
    EffectPanel chorusPanel;
    EffectPanel delayPanel;
    EffectPanel reverbPanel;
    EqualizerPanel equalizerPanel;
    PerformancePad performancePad;
    std::array<juce::TextButton, LoopEngine::numberOfSlots + 2> effectTargetButtons;
    TouchSlider delaySendSlider;
    TouchSlider reverbSendSlider;
    juce::Label delaySendLabel;
    juce::Label reverbSendLabel;
    juce::Label dspLoadLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
