#pragma once

#include <JuceHeader.h>
#include "AudioRecorder.h"
#include "EffectsChain.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

class LoopEngine final : public juce::AudioIODeviceCallback
{
public:
    static constexpr int numberOfSlots = 4;
    static constexpr int instrumentPolyphony = 8;

    enum class InstrumentMode
    {
        gate,
        oneShot,
        loop
    };

    struct Clip
    {
        juce::AudioBuffer<float> audio;
        double sourceSampleRate = 44100.0;
        juce::String name;
        std::vector<float> waveformMinimum;
        std::vector<float> waveformMaximum;
    };

    LoopEngine();
    ~LoopEngine() override;

    bool loadFile(int slotIndex, const juce::File& file, juce::String& errorMessage);
    void clearSlot(int slotIndex);
    void play(int slotIndex);
    void stop(int slotIndex);
    void stopAll();
    void setLooping(int slotIndex, bool shouldLoop);
    void setReverse(int slotIndex, bool shouldReverse);
    void setGain(int slotIndex, float newGain);
    void setPlaybackRate(int slotIndex, double newRate);
    void setTimeStretch(int slotIndex, bool enabled, float pitchSemitones);
    void setTrimRange(int slotIndex, double newStart, double newEnd);
    void setEnvelope(int slotIndex, double attackSeconds, double decaySeconds,
                     float sustainLevel, double releaseSeconds);
    void setEnvelopeCycle(int slotIndex, bool shouldRepeat);
    void setInstrumentRootNote(int slotIndex, int midiNote);
    void setInstrumentMode(int slotIndex, InstrumentMode mode);
    void setInstrumentEnvelope(int slotIndex, double attackSeconds, double decaySeconds,
                               float sustainLevel, double releaseSeconds);
    void noteOn(int slotIndex, int midiNote, float velocity);
    void noteOff(int slotIndex, int midiNote);
    void allNotesOff(int slotIndex);
    void allNotesOff();
    void setDroneEnabled(bool enabled);
    void setDroneNote(int midiNote);
    void setDroneDetune(float cents);
    void setDroneGain(float gain);
    void setDroneEnvelope(double attackSeconds, double decaySeconds,
                          float sustainLevel, double releaseSeconds);
    void setDroneWaveform(int waveform);
    void setDroneEqualizer(float lowDb, float midDb, float highDb);
    void setDroneDistortion(bool enabled, float drive, float toneHz, float mix);
    void setDroneGranular(bool enabled, float sizeMs, float densityHz,
                          float positionMs, float pitchSemitones, float mix);
    void setDroneFlanger(bool enabled, float rateHz, float depth,
                         float feedback, float mix);
    void setDroneChorus(bool enabled, float rateHz, float depth, float mix);
    void setDroneDelaySend(float amount);
    void setDroneReverbSend(float amount);

    bool startRecording(const juce::File& destination, juce::String& errorMessage);
    juce::File stopRecording();
    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] double getRecordingDurationSeconds() const;

    void setEqualizer(int slotIndex, float lowDb, float midDb, float highDb);
    void setDistortion(int slotIndex, bool enabled, float drive, float toneHz, float mix);
    void setGranular(int slotIndex, bool enabled, float sizeMs, float densityHz,
                     float positionMs, float pitchSemitones, float mix);
    void setFlanger(int slotIndex, bool enabled, float rateHz, float depth,
                    float feedback, float mix);
    void setChorus(int slotIndex, bool enabled, float rateHz, float depth, float mix);
    void setDelaySend(int slotIndex, float amount);
    void setReverbSend(int slotIndex, float amount);
    void setDelay(bool enabled, float timeMs, float feedback, float mix);
    void setReverb(bool enabled, float size, float damping, float mix);
    void setMasterEqualizer(float lowDb, float midDb, float highDb);
    void setStutter(bool enabled, float lengthMs, float mix, float feedback, int mode);

    [[nodiscard]] bool hasClip(int slotIndex) const;
    [[nodiscard]] bool isPlaying(int slotIndex) const;
    [[nodiscard]] juce::String getClipName(int slotIndex) const;
    [[nodiscard]] double getPlayheadNormalised(int slotIndex) const;
    [[nodiscard]] std::shared_ptr<const Clip> getClipForDisplay(int slotIndex) const;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    enum class Command
    {
        none,
        play,
        stop,
        stopImmediate
    };

    enum class EnvelopeStage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    };

    struct Voice
    {
        std::shared_ptr<const Clip> clip;
        std::atomic<Command> command { Command::none };
        std::atomic<bool> looping { true };
        std::atomic<bool> reverse { false };
        std::atomic<float> gain { 0.8f };
        std::atomic<double> playbackRate { 1.0 };
        std::atomic<bool> timeStretchEnabled { false };
        std::atomic<float> pitchSemitones { 0.0f };
        std::atomic<bool> stretchResetRequested { true };
        std::atomic<double> trimStart { 0.0 };
        std::atomic<double> trimEnd { 1.0 };
        std::atomic<double> attackSeconds { 0.02 };
        std::atomic<double> decaySeconds { 0.1 };
        std::atomic<float> sustainLevel { 1.0f };
        std::atomic<double> releaseSeconds { 0.0 };
        std::atomic<bool> envelopeCycle { true };
        std::atomic<int> instrumentRootNote { 60 };
        std::atomic<InstrumentMode> instrumentMode { InstrumentMode::gate };
        std::atomic<double> instrumentAttack { 0.01 };
        std::atomic<double> instrumentDecay { 0.1 };
        std::atomic<float> instrumentSustain { 1.0f };
        std::atomic<double> instrumentRelease { 0.15 };
        std::atomic<double> playheadNormalised { 0.0 };
        std::atomic<bool> playing { false };
        EnvelopeStage envelopeStage = EnvelopeStage::idle;
        float envelopeLevel = 0.0f;
        float releaseStep = 0.0f;
        double playhead = 0.0;
    };

    struct InstrumentVoice
    {
        std::shared_ptr<const Clip> clip;
        bool active = false;
        bool releasing = false;
        bool reversed = false;
        int slotIndex = 0;
        int midiNote = 60;
        InstrumentMode mode = InstrumentMode::gate;
        double playhead = 0.0;
        double startSample = 0.0;
        double endSample = 1.0;
        double increment = 1.0;
        float gain = 1.0f;
        double attack = 0.02;
        double decay = 0.1;
        float sustain = 1.0f;
        double release = 0.0;
        EnvelopeStage envelopeStage = EnvelopeStage::idle;
        float envelopeLevel = 0.0f;
        float releaseStep = 0.0f;
        uint64_t age = 0;
    };

    struct NoteCommand
    {
        enum class Type
        {
            noteOn,
            noteOff,
            allNotesForSlot,
            allNotesOff
        } type = Type::noteOn;
        int slotIndex = 0;
        int midiNote = 60;
        float velocity = 1.0f;
    };

    static bool isValidSlot(int slotIndex);
    static void createWaveformOverview(Clip& clip);
    void renderVoice(Voice& voice, float* const* outputs, int outputChannels, int numSamples);
    void processTimeStretch(Voice& voice, juce::AudioBuffer<float>& buffer,
                            int slotIndex, int numSamples);
    float advanceEnvelope(Voice& voice);
    void beginRelease(Voice& voice);
    float cycleEnvelopeLevel(const Voice& voice, double position, double startSample,
                             double endSample, double sourceSampleRate, double rate,
                             bool isReversed) const;
    void pushNoteCommand(NoteCommand commandToPush);
    void processNoteCommands();
    void startInstrumentVoice(const NoteCommand& commandToStart);
    void releaseInstrumentVoice(InstrumentVoice& instrumentVoice);
    void renderInstrumentVoice(InstrumentVoice& instrumentVoice,
                               float* const* outputs, int outputChannels, int numSamples);
    float advanceInstrumentEnvelope(InstrumentVoice& instrumentVoice);
    void renderDrone(juce::AudioBuffer<float>& mix, int numSamples);
    float advanceDroneEnvelope();

    juce::AudioFormatManager formatManager;
    std::array<Voice, numberOfSlots> voices;
    std::array<InstrumentVoice, instrumentPolyphony> instrumentVoices;
    static constexpr int noteCommandCapacity = 64;
    std::array<NoteCommand, noteCommandCapacity> noteCommands;
    juce::AbstractFifo noteCommandFifo { noteCommandCapacity };
    uint64_t instrumentVoiceCounter = 0;
    double deviceSampleRate = 44100.0;
    std::atomic<int> activeInputChannels { 0 };
    AudioRecorder recorder;
    struct SlotEffects
    {
        EffectsChain chain;
        std::atomic<float> delaySend { 0.15f };
        std::atomic<float> reverbSend { 0.15f };
    };
    std::array<SlotEffects, numberOfSlots> slotEffects;
    EffectsChain masterEffects;
    EffectsChain droneEffects;
    std::atomic<float> droneDelaySend { 0.15f };
    std::atomic<float> droneReverbSend { 0.15f };
    std::array<juce::AudioBuffer<float>, numberOfSlots> slotEffectBuffers;
    std::array<juce::AudioBuffer<float>, numberOfSlots> stretchOutputBuffers;
    struct StretchState;
    std::array<std::unique_ptr<StretchState>, numberOfSlots> stretchStates;
    juce::AudioBuffer<float> masterMixBuffer;
    juce::AudioBuffer<float> delaySendBuffer;
    juce::AudioBuffer<float> reverbSendBuffer;
    juce::AudioBuffer<float> droneEffectBuffer;

    std::atomic<bool> droneEnabled { false };
    std::atomic<int> droneMidiNote { 24 };
    std::atomic<float> droneDetuneCents { 7.0f };
    std::atomic<float> droneGain { 0.25f };
    std::atomic<double> droneAttack { 0.1 };
    std::atomic<double> droneDecay { 0.4 };
    std::atomic<float> droneSustain { 0.8f };
    std::atomic<double> droneRelease { 1.5 };
    std::atomic<int> droneWaveform { 0 };
    double dronePhaseA = 0.0;
    double dronePhaseB = 0.0;
    double smoothedDroneFrequency = 32.703196;
    float smoothedDroneGain = 0.0f;
    EnvelopeStage droneEnvelopeStage = EnvelopeStage::idle;
    float droneEnvelopeLevel = 0.0f;
    float droneReleaseStep = 0.0f;
    bool droneWasEnabled = false;
    int lastDroneMidiNote = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopEngine)
};
