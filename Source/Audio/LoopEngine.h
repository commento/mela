#pragma once

#include <JuceHeader.h>
#include "EffectsChain.h"
#include <atomic>
#include <memory>
#include <vector>

class LoopEngine final : public juce::AudioIODeviceCallback
{
public:
    struct Clip
    {
        juce::AudioBuffer<float> audio;
        double sourceSampleRate = 44100.0;
        juce::String name;
        std::vector<float> waveformMinimum;
        std::vector<float> waveformMaximum;
    };

    LoopEngine();

    bool loadFile(const juce::File& file, juce::String& errorMessage);
    void play();
    void stop();
    void setLooping(bool shouldLoop);
    void setGain(float newGain);
    void setPlaybackRate(double newRate);
    void setTrimRange(double newStart, double newEnd);
    void setEnvelope(double attackSeconds, double decaySeconds,
                     float sustainLevel, double releaseSeconds);
    void setEnvelopeCycle(bool shouldRepeat);
    void setDistortion(bool enabled, float drive, float toneHz, float mix);
    void setFlanger(bool enabled, float rateHz, float depth, float feedback, float mix);
    void setChorus(bool enabled, float rateHz, float depth, float mix);
    void setDelay(bool enabled, float timeMs, float feedback, float mix);
    void setReverb(bool enabled, float size, float damping, float mix);

    [[nodiscard]] bool hasClip() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] juce::String getClipName() const;
    [[nodiscard]] double getPlayheadNormalised() const;
    [[nodiscard]] std::shared_ptr<const Clip> getClipForDisplay() const;

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
        stop
    };

    enum class EnvelopeStage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    };

    static void createWaveformOverview(Clip& clip);
    void render(float* const* outputs, int outputChannels, int numSamples);
    float advanceEnvelope();
    void beginRelease();
    float cycleEnvelopeLevel(double position, double startSample, double endSample,
                             double sourceSampleRate, double rate) const;

    juce::AudioFormatManager formatManager;
    std::shared_ptr<const Clip> clip;
    std::atomic<Command> command { Command::none };
    std::atomic<bool> looping { true };
    std::atomic<float> gain { 0.8f };
    std::atomic<double> playbackRate { 1.0 };
    std::atomic<double> trimStart { 0.0 };
    std::atomic<double> trimEnd { 1.0 };
    std::atomic<double> attackSeconds { 0.02 };
    std::atomic<double> decaySeconds { 0.1 };
    std::atomic<float> sustainLevel { 1.0f };
    std::atomic<double> releaseSeconds { 0.3 };
    std::atomic<bool> envelopeCycle { true };
    std::atomic<double> playheadNormalised { 0.0 };
    std::atomic<bool> playing { false };
    EnvelopeStage envelopeStage = EnvelopeStage::idle;
    float envelopeLevel = 0.0f;
    float releaseStep = 0.0f;
    double playhead = 0.0;
    double deviceSampleRate = 44100.0;
    EffectsChain effectsChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopEngine)
};
