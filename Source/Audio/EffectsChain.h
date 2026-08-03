#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class EffectsChain
{
public:
    void prepare(double newSampleRate, int maximumBlockSize, int channels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);
    void processInserts(juce::AudioBuffer<float>& buffer);
    void processDelayReturn(juce::AudioBuffer<float>& buffer);
    void processReverbReturn(juce::AudioBuffer<float>& buffer);
    void processLimiter(juce::AudioBuffer<float>& buffer);

    void setDistortion(bool enabled, float drive, float toneHz, float mix);
    void setGranular(bool enabled, float sizeMs, float densityHz,
                     float positionMs, float pitchSemitones, float mix);
    void setFlanger(bool enabled, float rateHz, float depth, float feedback, float mix);
    void setChorus(bool enabled, float rateHz, float depth, float mix);
    void setDelay(bool enabled, float timeMs, float feedback, float mix);
    void setReverb(bool enabled, float size, float damping, float mix);

private:
    struct DistortionParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> drive { 2.0f };
        std::atomic<float> toneHz { 12000.0f };
        std::atomic<float> mix { 0.5f };
    } distortion;

    struct GranularParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> sizeMs { 80.0f };
        std::atomic<float> densityHz { 12.0f };
        std::atomic<float> positionMs { 250.0f };
        std::atomic<float> pitchSemitones { 0.0f };
        std::atomic<float> mix { 0.5f };
    } granular;

    struct FlangerParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> rateHz { 0.25f };
        std::atomic<float> depth { 0.5f };
        std::atomic<float> feedback { 0.2f };
        std::atomic<float> mix { 0.35f };
    } flanger;

    struct ChorusParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> rateHz { 0.8f };
        std::atomic<float> depth { 0.35f };
        std::atomic<float> mix { 0.35f };
    } chorusParameters;

    struct DelayParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> timeMs { 350.0f };
        std::atomic<float> feedback { 0.35f };
        std::atomic<float> mix { 0.3f };
    } delay;

    struct ReverbParameters
    {
        std::atomic<bool> enabled { false };
        std::atomic<float> size { 0.5f };
        std::atomic<float> damping { 0.5f };
        std::atomic<float> mix { 0.25f };
    } reverbParameters;

    void processDistortion(juce::AudioBuffer<float>& buffer);
    void processGranular(juce::AudioBuffer<float>& buffer);
    void processFlanger(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer, bool wetOnly);

    double sampleRate = 44100.0;
    int preparedChannels = 2;
    std::array<float, 2> distortionToneState {};
    std::array<float, 2> flangerFeedbackState {};
    struct Grain
    {
        bool active = false;
        double readPosition = 0.0;
        double increment = 1.0;
        int age = 0;
        int length = 1;
    };
    // Four slot chains give a Pi-friendly maximum of 32 simultaneous grains.
    static constexpr int maximumGrains = 8;
    std::array<Grain, maximumGrains> grains;
    juce::AudioBuffer<float> granularBuffer;
    int granularWritePosition = 0;
    double samplesUntilNextGrain = 0.0;
    bool granularWasEnabled = false;
    juce::Random granularRandom;
    juce::AudioBuffer<float> flangerBuffer;
    juce::AudioBuffer<float> delayBuffer;
    int flangerWritePosition = 0;
    int delayWritePosition = 0;
    double flangerPhase = 0.0;
    juce::SmoothedValue<double> smoothedDelaySamples;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::dsp::Limiter<float> masterLimiter;
};
