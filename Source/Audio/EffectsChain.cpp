#include "EffectsChain.h"

#include <cmath>

void EffectsChain::prepare(double newSampleRate, int maximumBlockSize, int channels)
{
    sampleRate = juce::jmax(1.0, newSampleRate);
    preparedChannels = juce::jlimit(1, 2, channels);

    const juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(juce::jmax(1, maximumBlockSize)),
        static_cast<juce::uint32>(preparedChannels)
    };
    chorus.prepare(spec);
    reverb.prepare(spec);
    masterLimiter.prepare(spec);
    masterLimiter.setThreshold(-0.3f);
    masterLimiter.setRelease(50.0f);

    equalizerLowCoefficient = static_cast<float>(
        std::exp(-juce::MathConstants<double>::twoPi * 250.0 / sampleRate));
    equalizerHighCoefficient = static_cast<float>(
        std::exp(-juce::MathConstants<double>::twoPi * 4000.0 / sampleRate));
    for (auto* gain : { &equalizerLowGain, &equalizerMidGain, &equalizerHighGain })
        gain->reset(sampleRate, 0.02);

    flangerBuffer.setSize(preparedChannels,
                          static_cast<int>(std::ceil(sampleRate * 0.02)) + 2);
    granularBuffer.setSize(preparedChannels,
                           static_cast<int>(std::ceil(sampleRate * 4.0)) + 2);
    delayBuffer.setSize(preparedChannels,
                        static_cast<int>(std::ceil(sampleRate * 2.0)) + 2);
    stutterHistoryBuffer.setSize(preparedChannels,
                                 static_cast<int>(std::ceil(sampleRate * 2.0)) + 2);
    stutterSliceBuffer.setSize(preparedChannels,
                               static_cast<int>(std::ceil(sampleRate * 0.5)) + 2);
    stutterWetMix.reset(sampleRate, 0.012);
    stutterLoopGain.reset(sampleRate, 0.012);
    performanceFilterG.reset(sampleRate, 0.025);
    performanceFilterK.reset(sampleRate, 0.025);
    performanceFlangerRate.reset(sampleRate, 0.025);
    performanceFlangerDepth.reset(sampleRate, 0.025);
    performanceFlangerFeedback.reset(sampleRate, 0.025);
    smoothedDelaySamples.reset(sampleRate, 0.05);
    smoothedDelaySamples.setCurrentAndTargetValue(sampleRate * 0.35);
    reset();
}

void EffectsChain::reset()
{
    equalizerLowState.fill(0.0f);
    equalizerHighState.fill(0.0f);
    equalizerLowGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(equalizer.lowDb.load()));
    equalizerMidGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(equalizer.midDb.load()));
    equalizerHighGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(equalizer.highDb.load()));
    distortionToneState.fill(0.0f);
    flangerFeedbackState.fill(0.0f);
    for (auto& grain : grains)
        grain = {};
    granularBuffer.clear();
    granularWritePosition = 0;
    samplesUntilNextGrain = 0.0;
    granularWasEnabled = false;
    flangerBuffer.clear();
    delayBuffer.clear();
    flangerWritePosition = 0;
    delayWritePosition = 0;
    stutterHistoryBuffer.clear();
    stutterSliceBuffer.clear();
    stutterHistoryWritePosition = 0;
    stutterPlaybackPosition = 0;
    stutterSliceLength = 1;
    stutterRequestedLength = 1;
    stutterCurrentMode = 0;
    stutterLoopCount = 0;
    stutterWasEnabled = false;
    stutterSliceActive = false;
    stutterLoopGain.setCurrentAndTargetValue(1.0f);
    stutterWetMix.setCurrentAndTargetValue(0.0f);
    performanceFilterG.setCurrentAndTargetValue(0.1f);
    performanceFilterK.setCurrentAndTargetValue(1.0f);
    performanceFlangerRate.setCurrentAndTargetValue(0.25f);
    performanceFlangerDepth.setCurrentAndTargetValue(0.5f);
    performanceFlangerFeedback.setCurrentAndTargetValue(0.2f);
    performanceFilterState1.fill(0.0f);
    performanceFilterState2.fill(0.0f);
    flangerPhase = 0.0;
    chorus.reset();
    reverb.reset();
    masterLimiter.reset();
}

void EffectsChain::process(juce::AudioBuffer<float>& buffer)
{
    processInserts(buffer);
    processDelay(buffer, false);

    if (reverbParameters.enabled.load())
    {
        juce::dsp::Reverb::Parameters parameters;
        parameters.roomSize = reverbParameters.size.load();
        parameters.damping = reverbParameters.damping.load();
        parameters.wetLevel = reverbParameters.mix.load();
        parameters.dryLevel = 1.0f - parameters.wetLevel;
        parameters.width = 1.0f;
        parameters.freezeMode = 0.0f;
        reverb.setParameters(parameters);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }

    processLimiter(buffer);
}

void EffectsChain::processInserts(juce::AudioBuffer<float>& buffer)
{
    processEqualizer(buffer);
    processDistortion(buffer);
    processGranular(buffer);
    processFlanger(buffer);

    if (chorusParameters.enabled.load())
    {
        chorus.setRate(chorusParameters.rateHz.load());
        chorus.setDepth(chorusParameters.depth.load());
        chorus.setCentreDelay(12.0f);
        chorus.setFeedback(0.05f);
        chorus.setMix(chorusParameters.mix.load());
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        chorus.process(context);
    }

}

void EffectsChain::processEqualizer(juce::AudioBuffer<float>& buffer)
{
    const auto lowDb = equalizer.lowDb.load();
    const auto midDb = equalizer.midDb.load();
    const auto highDb = equalizer.highDb.load();
    equalizerLowGain.setTargetValue(juce::Decibels::decibelsToGain(lowDb));
    equalizerMidGain.setTargetValue(juce::Decibels::decibelsToGain(midDb));
    equalizerHighGain.setTargetValue(juce::Decibels::decibelsToGain(highDb));

    if (std::abs(lowDb) < 0.0001f && std::abs(midDb) < 0.0001f
        && std::abs(highDb) < 0.0001f
        && ! equalizerLowGain.isSmoothing() && ! equalizerMidGain.isSmoothing()
        && ! equalizerHighGain.isSmoothing())
        return;

    const auto channels = juce::jmin(buffer.getNumChannels(), preparedChannels);
    for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
    {
        const auto lowGain = equalizerLowGain.getNextValue();
        const auto midGain = equalizerMidGain.getNextValue();
        const auto highGain = equalizerHighGain.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
        {
            auto& lowState = equalizerLowState[static_cast<size_t>(channel)];
            auto& highState = equalizerHighState[static_cast<size_t>(channel)];
            auto* samples = buffer.getWritePointer(channel);
            const auto input = samples[frame];
            lowState = (1.0f - equalizerLowCoefficient) * input
                     + equalizerLowCoefficient * lowState;
            highState = (1.0f - equalizerHighCoefficient) * input
                      + equalizerHighCoefficient * highState;
            const auto low = lowState;
            const auto mid = highState - lowState;
            const auto high = input - highState;
            samples[frame] = low * lowGain + mid * midGain + high * highGain;
        }
    }
}

void EffectsChain::processDelayReturn(juce::AudioBuffer<float>& buffer)
{
    processDelay(buffer, true);
}

void EffectsChain::processReverbReturn(juce::AudioBuffer<float>& buffer)
{
    if (reverbParameters.enabled.load())
    {
        juce::dsp::Reverb::Parameters parameters;
        parameters.roomSize = reverbParameters.size.load();
        parameters.damping = reverbParameters.damping.load();
        parameters.wetLevel = reverbParameters.mix.load();
        parameters.dryLevel = 0.0f;
        parameters.width = 1.0f;
        parameters.freezeMode = 0.0f;
        reverb.setParameters(parameters);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverb.process(context);
    }
    else
    {
        buffer.clear();
    }
}

void EffectsChain::processLimiter(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> masterBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> masterContext(masterBlock);
    masterLimiter.process(masterContext);
}

void EffectsChain::setEqualizer(float lowDb, float midDb, float highDb)
{
    equalizer.lowDb.store(juce::jlimit(-12.0f, 12.0f, lowDb));
    equalizer.midDb.store(juce::jlimit(-12.0f, 12.0f, midDb));
    equalizer.highDb.store(juce::jlimit(-12.0f, 12.0f, highDb));
}

void EffectsChain::setDistortion(bool enabled, float drive, float toneHz, float mix)
{
    distortion.enabled.store(enabled);
    distortion.drive.store(juce::jlimit(1.0f, 20.0f, drive));
    distortion.toneHz.store(juce::jlimit(200.0f, 20000.0f, toneHz));
    distortion.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setGranular(bool enabled, float sizeMs, float densityHz,
                               float positionMs, float pitchSemitones, float mix)
{
    granular.enabled.store(enabled);
    granular.sizeMs.store(juce::jlimit(10.0f, 250.0f, sizeMs));
    granular.densityHz.store(juce::jlimit(1.0f, 40.0f, densityHz));
    granular.positionMs.store(juce::jlimit(0.0f, 2000.0f, positionMs));
    granular.pitchSemitones.store(juce::jlimit(-12.0f, 12.0f, pitchSemitones));
    granular.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setMaximumActiveGrains(int maximum)
{
    activeGrainLimit.store(juce::jlimit(1, maximumGrains, maximum));
}

void EffectsChain::setFlanger(bool enabled, float rateHz, float depth,
                              float feedback, float mix)
{
    flanger.enabled.store(enabled);
    flanger.rateHz.store(juce::jlimit(0.01f, 10.0f, rateHz));
    flanger.depth.store(juce::jlimit(0.0f, 1.0f, depth));
    flanger.feedback.store(juce::jlimit(-0.95f, 0.95f, feedback));
    flanger.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setChorus(bool enabled, float rateHz, float depth, float mix)
{
    chorusParameters.enabled.store(enabled);
    chorusParameters.rateHz.store(juce::jlimit(0.01f, 10.0f, rateHz));
    chorusParameters.depth.store(juce::jlimit(0.0f, 1.0f, depth));
    chorusParameters.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setDelay(bool enabled, float timeMs, float feedback, float mix)
{
    delay.enabled.store(enabled);
    delay.timeMs.store(juce::jlimit(1.0f, 1500.0f, timeMs));
    delay.feedback.store(juce::jlimit(0.0f, 0.95f, feedback));
    delay.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setReverb(bool enabled, float size, float damping, float mix)
{
    reverbParameters.enabled.store(enabled);
    reverbParameters.size.store(juce::jlimit(0.0f, 1.0f, size));
    reverbParameters.damping.store(juce::jlimit(0.0f, 1.0f, damping));
    reverbParameters.mix.store(juce::jlimit(0.0f, 1.0f, mix));
}

void EffectsChain::setStutter(bool enabled, float lengthMs, float mix,
                              float feedback, int mode)
{
    stutter.lengthMs.store(juce::jlimit(30.0f, 500.0f, lengthMs));
    stutter.mix.store(juce::jlimit(0.0f, 1.0f, mix));
    stutter.feedback.store(juce::jlimit(0.0f, 1.0f, feedback));
    stutter.mode.store(juce::jlimit(0, 4, mode));
    stutter.enabled.store(enabled);
}

void EffectsChain::processStutter(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(buffer.getNumChannels(), preparedChannels);
    const auto historySize = stutterHistoryBuffer.getNumSamples();
    const auto sliceCapacity = stutterSliceBuffer.getNumSamples();
    if (channels <= 0 || historySize <= 1 || sliceCapacity <= 1)
        return;

    const auto enabled = stutter.enabled.load();
    const auto requestedMode = stutter.mode.load();
    stutterRequestedLength = juce::jlimit(
        2, sliceCapacity,
        static_cast<int>(std::round(sampleRate * stutter.lengthMs.load() * 0.001)));
    const auto normalisedX = juce::jlimit(0.0f, 1.0f,
        1.0f - std::log(stutter.lengthMs.load() / 30.0f) / std::log(500.0f / 30.0f));
    const auto normalisedY = juce::jlimit(0.0f, 1.0f,
        (stutter.mix.load() - 0.25f) / 0.75f);

    const auto cutoff = juce::jmin(static_cast<float>(sampleRate * 0.42),
        80.0f * std::pow(20000.0f / 80.0f, normalisedX));
    performanceFilterG.setTargetValue(std::tan(
        juce::MathConstants<float>::pi * cutoff / static_cast<float>(sampleRate)));
    performanceFilterK.setTargetValue(2.0f - 1.85f * normalisedY);
    performanceFlangerRate.setTargetValue(0.05f * std::pow(120.0f, normalisedX));
    performanceFlangerDepth.setTargetValue(0.15f + 0.85f * normalisedY);
    performanceFlangerFeedback.setTargetValue(0.1f + 0.7f * normalisedY);

    const auto stutterModeRequested = requestedMode <= 2;
    if (enabled && stutterModeRequested
        && (! stutterWasEnabled || stutterCurrentMode > 2))
    {
        // Capture one 500 ms window only on touch-down. Finger movement merely
        // changes the loop boundary, avoiding repeated copies and discontinuities.
        auto sourcePosition = stutterHistoryWritePosition - sliceCapacity;
        while (sourcePosition < 0)
            sourcePosition += historySize;
        for (int channel = 0; channel < channels; ++channel)
        {
            auto* destination = stutterSliceBuffer.getWritePointer(channel);
            const auto* source = stutterHistoryBuffer.getReadPointer(channel);
            for (int sample = 0; sample < sliceCapacity; ++sample)
                destination[sample] = source[(sourcePosition + sample) % historySize];
        }
        stutterSliceLength = stutterRequestedLength;
        stutterCurrentMode = requestedMode;
        stutterPlaybackPosition = 0;
        stutterLoopCount = 0;
        stutterLoopGain.setCurrentAndTargetValue(1.0f);
        stutterSliceActive = true;
    }
    else if (enabled && ! stutterModeRequested)
    {
        stutterCurrentMode = requestedMode;
        stutterSliceActive = false;
    }

    const auto requestedWet = requestedMode == 4
        ? 0.15f + normalisedY * 0.6f : stutter.mix.load();
    stutterWetMix.setTargetValue(enabled ? requestedWet : 0.0f);
    const auto feedback = stutter.feedback.load();
    const auto flangerBufferLength = flangerBuffer.getNumSamples();
    // Keep the captured slice intact. Intensity may lower its repeat level, but
    // it must never cumulatively erase the audio during a long gesture.
    stutterLoopGain.setTargetValue(feedback);

    if (! enabled && ! stutterWetMix.isSmoothing()
        && stutterWetMix.getCurrentValue() <= 0.0001f)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            for (int channel = 0; channel < channels; ++channel)
                stutterHistoryBuffer.setSample(channel, stutterHistoryWritePosition,
                                               buffer.getSample(channel, sample));
            stutterHistoryWritePosition = (stutterHistoryWritePosition + 1) % historySize;
        }
        stutterSliceActive = false;
        stutterWasEnabled = false;
        return;
    }

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto wetMix = stutterWetMix.getNextValue();
        const auto loopGain = stutterLoopGain.getNextValue();
        const auto filterG = performanceFilterG.getNextValue();
        const auto filterK = performanceFilterK.getNextValue();
        const auto flangerRate = performanceFlangerRate.getNextValue();
        const auto flangerDepth = performanceFlangerDepth.getNextValue();
        const auto flangerFeedback = performanceFlangerFeedback.getNextValue();
        auto flangerFirst = 0;
        auto flangerSecond = 0;
        auto flangerAlpha = 0.0f;
        if (stutterCurrentMode == 4 && flangerBufferLength > 1)
        {
            const auto lfo = 0.5 + 0.5 * std::sin(flangerPhase);
            const auto delaySamples = sampleRate
                * (0.0007 + 0.0055 * flangerDepth * lfo);
            auto readPosition = static_cast<double>(flangerWritePosition) - delaySamples;
            while (readPosition < 0.0)
                readPosition += flangerBufferLength;
            flangerFirst = static_cast<int>(readPosition) % flangerBufferLength;
            flangerSecond = (flangerFirst + 1) % flangerBufferLength;
            flangerAlpha = static_cast<float>(readPosition - std::floor(readPosition));
        }

        const auto filterA1 = 1.0f / (1.0f + filterG * (filterG + filterK));
        const auto filterA2 = filterG * filterA1;
        const auto filterA3 = filterG * filterA2;

        const auto fadeSamples = juce::jlimit(4, 128, stutterSliceLength / 10);
        auto reverse = stutterCurrentMode == 1;
        if (stutterCurrentMode == 2)
            reverse = (stutterLoopCount & 1) != 0;
        const auto sliceBase = sliceCapacity - stutterSliceLength;
        const auto readOffset = reverse
            ? stutterSliceLength - 1 - stutterPlaybackPosition
            : stutterPlaybackPosition;
        const auto readPosition = sliceBase + readOffset;
        const auto crossfadeStart = stutterSliceLength - fadeSamples;
        const auto crossfadeAmount = stutterPlaybackPosition >= crossfadeStart
            ? static_cast<float>(stutterPlaybackPosition - crossfadeStart + 1)
                / static_cast<float>(fadeSamples)
            : 0.0f;
        auto nextReverse = requestedMode == 1;
        if (requestedMode == 2)
            nextReverse = ((stutterLoopCount + 1) & 1) != 0;
        const auto nextSliceBase = sliceCapacity - stutterRequestedLength;
        const auto nextLoopStart = nextSliceBase
            + (nextReverse ? stutterRequestedLength - 1 : 0);

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* output = buffer.getWritePointer(channel);
            const auto dry = output[sample];
            stutterHistoryBuffer.setSample(channel, stutterHistoryWritePosition, dry);
            auto wet = dry;

            if (stutterCurrentMode == 3)
            {
                auto& state1 = performanceFilterState1[static_cast<size_t>(channel)];
                auto& state2 = performanceFilterState2[static_cast<size_t>(channel)];
                const auto v3 = dry - state2;
                const auto v1 = filterA1 * state1 + filterA2 * v3;
                const auto v2 = state2 + filterA2 * state1 + filterA3 * v3;
                state1 = 2.0f * v1 - state1;
                state2 = 2.0f * v2 - state2;
                wet = v2;
            }
            else if (stutterCurrentMode == 4 && flangerBufferLength > 1)
            {
                const auto firstValue = flangerBuffer.getSample(channel, flangerFirst);
                const auto delayed = firstValue + flangerAlpha
                    * (flangerBuffer.getSample(channel, flangerSecond) - firstValue);
                flangerBuffer.setSample(channel, flangerWritePosition,
                                        dry + delayed * flangerFeedback);
                wet = 0.5f * (dry + delayed);
            }
            else if (stutterSliceActive)
            {
                wet = stutterSliceBuffer.getSample(channel, readPosition);
                if (crossfadeAmount > 0.0f)
                    wet += crossfadeAmount
                        * (stutterSliceBuffer.getSample(channel, nextLoopStart) - wet);
                wet *= loopGain;
                if (stutterCurrentMode == 2)
                    wet = std::round(wet * 28.0f) / 28.0f;
            }

            if (wetMix > 0.0001f)
                output[sample] = dry + wetMix * (wet - dry);
        }

        stutterHistoryWritePosition = (stutterHistoryWritePosition + 1) % historySize;
        if (stutterCurrentMode == 4 && flangerBufferLength > 1)
        {
            flangerWritePosition = (flangerWritePosition + 1) % flangerBufferLength;
            flangerPhase += juce::MathConstants<double>::twoPi * flangerRate / sampleRate;
            if (flangerPhase >= juce::MathConstants<double>::twoPi)
                flangerPhase -= juce::MathConstants<double>::twoPi;
        }
        else if (stutterSliceActive)
        {
            if (++stutterPlaybackPosition >= stutterSliceLength)
            {
                stutterPlaybackPosition = 0;
                ++stutterLoopCount;
                stutterLoopGain.setTargetValue(feedback);
                stutterSliceLength = stutterRequestedLength;
                stutterCurrentMode = requestedMode;
            }
        }
        if (! enabled && wetMix <= 0.0001f && ! stutterWetMix.isSmoothing())
            stutterSliceActive = false;
    }
    stutterWasEnabled = enabled;
}

void EffectsChain::processDistortion(juce::AudioBuffer<float>& buffer)
{
    if (! distortion.enabled.load())
        return;

    const auto drive = distortion.drive.load();
    const auto mix = distortion.mix.load();
    const auto cutoff = juce::jmin(distortion.toneHz.load(),
                                   static_cast<float>(sampleRate * 0.45));
    const auto coefficient = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi
                                                          * cutoff / sampleRate));
    const auto normalisation = 1.0f / std::tanh(drive);

    for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), preparedChannels); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto state = distortionToneState[static_cast<size_t>(channel)];
        for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
        {
            const auto dry = samples[frame];
            const auto shaped = std::tanh(dry * drive) * normalisation;
            state = (1.0f - coefficient) * shaped + coefficient * state;
            samples[frame] = dry + mix * (state - dry);
        }
        distortionToneState[static_cast<size_t>(channel)] = state;
    }
}

void EffectsChain::processGranular(juce::AudioBuffer<float>& buffer)
{
    if (granularBuffer.getNumSamples() == 0)
        return;

    const auto enabled = granular.enabled.load();
    if (! enabled)
    {
        if (granularWasEnabled)
        {
            for (auto& grain : grains)
                grain.active = false;
            samplesUntilNextGrain = 0.0;
        }
        granularWasEnabled = false;
        return;
    }
    granularWasEnabled = true;

    const auto sizeMs = granular.sizeMs.load();
    const auto density = granular.densityHz.load();
    const auto positionMs = granular.positionMs.load();
    const auto pitchRatio = std::pow(2.0, granular.pitchSemitones.load() / 12.0);
    const auto mix = granular.mix.load();
    const auto grainLength = juce::jlimit(2,
        static_cast<int>(sampleRate * 0.25),
        static_cast<int>(std::round(sampleRate * sizeMs / 1000.0)));
    const auto bufferLength = granularBuffer.getNumSamples();
    const auto channels = juce::jmin(buffer.getNumChannels(), preparedChannels);
    const auto expectedOverlap = density * static_cast<float>(grainLength / sampleRate);
    const auto wetNormalisation = 1.0f / juce::jmax(1.0f, expectedOverlap * 0.5f);

    for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
    {
        for (int channel = 0; channel < channels; ++channel)
            granularBuffer.setSample(channel, granularWritePosition,
                                     buffer.getSample(channel, frame));

        if (enabled)
        {
            if (samplesUntilNextGrain <= 0.0)
            {
                auto grainsConsidered = 0;
                for (auto& grain : grains)
                {
                    if (grainsConsidered++ >= activeGrainLimit.load())
                        break;
                    if (grain.active)
                        continue;

                    const auto historySamples = sampleRate * positionMs / 1000.0;
                    const auto safetyDistance = grainLength * juce::jmax(1.0, pitchRatio) + 2.0;
                    const auto spray = granularRandom.nextDouble() * grainLength * 0.25;
                    grain.active = true;
                    grain.age = 0;
                    grain.length = grainLength;
                    grain.increment = pitchRatio;
                    grain.readPosition = granularWritePosition
                                       - juce::jmax(historySamples, safetyDistance) - spray;
                    while (grain.readPosition < 0.0)
                        grain.readPosition += bufferLength;
                    break;
                }
                samplesUntilNextGrain += sampleRate / juce::jmax(1.0f, density);
            }

            std::array<float, 2> wet {};
            for (auto& grain : grains)
            {
                if (! grain.active)
                    continue;

                const auto phase = static_cast<double>(grain.age)
                                 / static_cast<double>(juce::jmax(1, grain.length - 1));
                const auto window = static_cast<float>(0.5 - 0.5
                    * std::cos(juce::MathConstants<double>::twoPi * phase));
                const auto first = static_cast<int>(grain.readPosition) % bufferLength;
                const auto second = (first + 1) % bufferLength;
                const auto alpha = static_cast<float>(grain.readPosition
                                                       - std::floor(grain.readPosition));

                for (int channel = 0; channel < channels; ++channel)
                {
                    const auto firstSample = granularBuffer.getSample(channel, first);
                    const auto secondSample = granularBuffer.getSample(channel, second);
                    wet[static_cast<size_t>(channel)] += window
                        * (firstSample + alpha * (secondSample - firstSample));
                }

                grain.readPosition += grain.increment;
                while (grain.readPosition >= bufferLength)
                    grain.readPosition -= bufferLength;
                if (++grain.age >= grain.length)
                    grain.active = false;
            }

            for (int channel = 0; channel < channels; ++channel)
            {
                auto* samples = buffer.getWritePointer(channel);
                const auto dry = samples[frame];
                const auto processed = wet[static_cast<size_t>(channel)] * wetNormalisation;
                samples[frame] = dry + mix * (processed - dry);
            }
            --samplesUntilNextGrain;
        }

        granularWritePosition = (granularWritePosition + 1) % bufferLength;
    }
}

void EffectsChain::processFlanger(juce::AudioBuffer<float>& buffer)
{
    if (! flanger.enabled.load() || flangerBuffer.getNumSamples() == 0)
        return;

    const auto rate = flanger.rateHz.load();
    const auto depth = flanger.depth.load();
    const auto feedback = flanger.feedback.load();
    const auto mix = flanger.mix.load();
    const auto bufferLength = flangerBuffer.getNumSamples();

    for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
    {
        const auto lfo = 0.5 + 0.5 * std::sin(flangerPhase);
        const auto delaySamples = sampleRate * (0.001 + 0.005 * depth * lfo);
        auto readPosition = static_cast<double>(flangerWritePosition) - delaySamples;
        while (readPosition < 0.0)
            readPosition += bufferLength;
        const auto first = static_cast<int>(readPosition) % bufferLength;
        const auto second = (first + 1) % bufferLength;
        const auto alpha = static_cast<float>(readPosition - std::floor(readPosition));

        for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), preparedChannels); ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);
            const auto delayed = flangerBuffer.getSample(channel, first)
                + alpha * (flangerBuffer.getSample(channel, second)
                         - flangerBuffer.getSample(channel, first));
            const auto dry = samples[frame];
            flangerBuffer.setSample(channel, flangerWritePosition,
                                    dry + delayed * feedback);
            samples[frame] = dry + mix * (delayed - dry);
        }

        flangerWritePosition = (flangerWritePosition + 1) % bufferLength;
        flangerPhase += juce::MathConstants<double>::twoPi * rate / sampleRate;
        if (flangerPhase >= juce::MathConstants<double>::twoPi)
            flangerPhase -= juce::MathConstants<double>::twoPi;
    }
}

void EffectsChain::processDelay(juce::AudioBuffer<float>& buffer, bool wetOnly)
{
    if (! delay.enabled.load() || delayBuffer.getNumSamples() == 0)
    {
        if (wetOnly)
            buffer.clear();
        return;
    }

    smoothedDelaySamples.setTargetValue(sampleRate * delay.timeMs.load() / 1000.0);
    const auto feedback = delay.feedback.load();
    const auto mix = delay.mix.load();
    const auto bufferLength = delayBuffer.getNumSamples();

    for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
    {
        auto readPosition = static_cast<double>(delayWritePosition)
                          - smoothedDelaySamples.getNextValue();
        while (readPosition < 0.0)
            readPosition += bufferLength;
        const auto first = static_cast<int>(readPosition) % bufferLength;
        const auto second = (first + 1) % bufferLength;
        const auto alpha = static_cast<float>(readPosition - std::floor(readPosition));

        for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), preparedChannels); ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);
            const auto delayed = delayBuffer.getSample(channel, first)
                + alpha * (delayBuffer.getSample(channel, second)
                         - delayBuffer.getSample(channel, first));
            const auto dry = samples[frame];
            delayBuffer.setSample(channel, delayWritePosition, dry + delayed * feedback);
            samples[frame] = wetOnly ? delayed * mix
                                     : dry + mix * (delayed - dry);
        }
        delayWritePosition = (delayWritePosition + 1) % bufferLength;
    }
}
