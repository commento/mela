#pragma once

#include <JuceHeader.h>
#include "EffectsChain.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

class LoopEngine final : public juce::AudioIODeviceCallback
{
public:
    static constexpr int numberOfSlots = 4;

    struct Clip
    {
        juce::AudioBuffer<float> audio;
        double sourceSampleRate = 44100.0;
        juce::String name;
        std::vector<float> waveformMinimum;
        std::vector<float> waveformMaximum;
    };

    LoopEngine();

    bool loadFile(int slotIndex, const juce::File& file, juce::String& errorMessage);
    void play(int slotIndex);
    void stop(int slotIndex);
    void stopAll();
    void setLooping(int slotIndex, bool shouldLoop);
    void setReverse(int slotIndex, bool shouldReverse);
    void setGain(int slotIndex, float newGain);
    void setPlaybackRate(int slotIndex, double newRate);
    void setTrimRange(int slotIndex, double newStart, double newEnd);
    void setEnvelope(int slotIndex, double attackSeconds, double decaySeconds,
                     float sustainLevel, double releaseSeconds);
    void setEnvelopeCycle(int slotIndex, bool shouldRepeat);

    void setDistortion(bool enabled, float drive, float toneHz, float mix);
    void setGranular(bool enabled, float sizeMs, float densityHz,
                     float positionMs, float pitchSemitones, float mix);
    void setFlanger(bool enabled, float rateHz, float depth, float feedback, float mix);
    void setChorus(bool enabled, float rateHz, float depth, float mix);
    void setDelay(bool enabled, float timeMs, float feedback, float mix);
    void setReverb(bool enabled, float size, float damping, float mix);

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
        std::atomic<double> trimStart { 0.0 };
        std::atomic<double> trimEnd { 1.0 };
        std::atomic<double> attackSeconds { 0.02 };
        std::atomic<double> decaySeconds { 0.1 };
        std::atomic<float> sustainLevel { 1.0f };
        std::atomic<double> releaseSeconds { 0.0 };
        std::atomic<bool> envelopeCycle { true };
        std::atomic<double> playheadNormalised { 0.0 };
        std::atomic<bool> playing { false };
        EnvelopeStage envelopeStage = EnvelopeStage::idle;
        float envelopeLevel = 0.0f;
        float releaseStep = 0.0f;
        double playhead = 0.0;
    };

    static bool isValidSlot(int slotIndex);
    static void createWaveformOverview(Clip& clip);
    void renderVoice(Voice& voice, float* const* outputs, int outputChannels, int numSamples);
    float advanceEnvelope(Voice& voice);
    void beginRelease(Voice& voice);
    float cycleEnvelopeLevel(const Voice& voice, double position, double startSample,
                             double endSample, double sourceSampleRate, double rate,
                             bool isReversed) const;

    juce::AudioFormatManager formatManager;
    std::array<Voice, numberOfSlots> voices;
    double deviceSampleRate = 44100.0;
    EffectsChain effectsChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopEngine)
};
