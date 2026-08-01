#include "LoopEngine.h"

#include <cmath>
#include <cstdint>
#include <limits>

LoopEngine::LoopEngine()
{
    formatManager.registerBasicFormats();
}

bool LoopEngine::loadFile(int slotIndex, const juce::File& file, juce::String& errorMessage)
{
    if (! isValidSlot(slotIndex))
    {
        errorMessage = "Slot non valido";
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "Formato audio non riconosciuto";
        return false;
    }

    if (reader->lengthInSamples <= 1 || reader->lengthInSamples > std::numeric_limits<int>::max())
    {
        errorMessage = "Il file e' vuoto o troppo lungo per questa versione";
        return false;
    }

    auto newClip = std::make_shared<Clip>();
    const auto channelsToLoad = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));
    newClip->audio.setSize(channelsToLoad, static_cast<int>(reader->lengthInSamples));
    newClip->sourceSampleRate = reader->sampleRate;
    newClip->name = file.getFileNameWithoutExtension();

    if (! reader->read(&newClip->audio, 0, newClip->audio.getNumSamples(), 0, true, true))
    {
        errorMessage = "Impossibile leggere i dati audio";
        return false;
    }

    createWaveformOverview(*newClip);
    auto& voice = voices[static_cast<size_t>(slotIndex)];
    voice.command.store(Command::stopImmediate);
    voice.trimStart.store(0.0);
    voice.trimEnd.store(1.0);
    std::atomic_store(&voice.clip, std::shared_ptr<const Clip>(std::move(newClip)));
    return true;
}

void LoopEngine::play(int slotIndex)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].command.store(Command::play);
}

void LoopEngine::stop(int slotIndex)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].command.store(Command::stop);
}

void LoopEngine::stopAll()
{
    for (auto& voice : voices)
        voice.command.store(Command::stop);
}

void LoopEngine::setLooping(int slotIndex, bool shouldLoop)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].looping.store(shouldLoop);
}

void LoopEngine::setReverse(int slotIndex, bool shouldReverse)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].reverse.store(shouldReverse);
}

void LoopEngine::setGain(int slotIndex, float newGain)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].gain.store(juce::jlimit(0.0f, 1.0f, newGain));
}

void LoopEngine::setPlaybackRate(int slotIndex, double newRate)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].playbackRate.store(juce::jlimit(0.25, 1.5, newRate));
}

void LoopEngine::setTrimRange(int slotIndex, double newStart, double newEnd)
{
    if (! isValidSlot(slotIndex))
        return;

    constexpr auto minimumRange = 0.001;
    const auto start = juce::jlimit(0.0, 1.0 - minimumRange, newStart);
    const auto end = juce::jlimit(start + minimumRange, 1.0, newEnd);
    auto& voice = voices[static_cast<size_t>(slotIndex)];
    voice.trimStart.store(start);
    voice.trimEnd.store(end);
}

void LoopEngine::setEnvelope(int slotIndex, double attack, double decay,
                             float sustain, double release)
{
    if (! isValidSlot(slotIndex))
        return;

    auto& voice = voices[static_cast<size_t>(slotIndex)];
    voice.attackSeconds.store(juce::jlimit(0.0, 10.0, attack));
    voice.decaySeconds.store(juce::jlimit(0.0, 10.0, decay));
    voice.sustainLevel.store(juce::jlimit(0.0f, 1.0f, sustain));
    voice.releaseSeconds.store(juce::jlimit(0.0, 20.0, release));
}

void LoopEngine::setEnvelopeCycle(int slotIndex, bool shouldRepeat)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].envelopeCycle.store(shouldRepeat);
}

void LoopEngine::setDistortion(bool enabled, float drive, float toneHz, float mix)
{
    effectsChain.setDistortion(enabled, drive, toneHz, mix);
}

void LoopEngine::setGranular(bool enabled, float sizeMs, float densityHz,
                             float positionMs, float pitchSemitones, float mix)
{
    effectsChain.setGranular(enabled, sizeMs, densityHz, positionMs, pitchSemitones, mix);
}

void LoopEngine::setFlanger(bool enabled, float rateHz, float depth,
                            float feedback, float mix)
{
    effectsChain.setFlanger(enabled, rateHz, depth, feedback, mix);
}

void LoopEngine::setChorus(bool enabled, float rateHz, float depth, float mix)
{
    effectsChain.setChorus(enabled, rateHz, depth, mix);
}

void LoopEngine::setDelay(bool enabled, float timeMs, float feedback, float mix)
{
    effectsChain.setDelay(enabled, timeMs, feedback, mix);
}

void LoopEngine::setReverb(bool enabled, float size, float damping, float mix)
{
    effectsChain.setReverb(enabled, size, damping, mix);
}

bool LoopEngine::hasClip(int slotIndex) const
{
    return isValidSlot(slotIndex)
        && std::atomic_load(&voices[static_cast<size_t>(slotIndex)].clip) != nullptr;
}

bool LoopEngine::isPlaying(int slotIndex) const
{
    return isValidSlot(slotIndex) && voices[static_cast<size_t>(slotIndex)].playing.load();
}

juce::String LoopEngine::getClipName(int slotIndex) const
{
    if (! isValidSlot(slotIndex))
        return {};
    const auto currentClip = std::atomic_load(&voices[static_cast<size_t>(slotIndex)].clip);
    return currentClip != nullptr ? currentClip->name : juce::String {};
}

double LoopEngine::getPlayheadNormalised(int slotIndex) const
{
    return isValidSlot(slotIndex)
        ? voices[static_cast<size_t>(slotIndex)].playheadNormalised.load() : 0.0;
}

std::shared_ptr<const LoopEngine::Clip> LoopEngine::getClipForDisplay(int slotIndex) const
{
    return isValidSlot(slotIndex)
        ? std::atomic_load(&voices[static_cast<size_t>(slotIndex)].clip) : nullptr;
}

void LoopEngine::audioDeviceIOCallbackWithContext(const float* const*, int,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels, int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    for (int channel = 0; channel < numOutputChannels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);

    for (auto& voice : voices)
        renderVoice(voice, outputChannelData, numOutputChannels, numSamples);

    if (numOutputChannels > 0)
    {
        juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
        effectsChain.process(outputBuffer);
    }
}

void LoopEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    deviceSampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    if (device != nullptr)
    {
        effectsChain.prepare(deviceSampleRate, device->getCurrentBufferSizeSamples(),
                             juce::jmax(1, device->getActiveOutputChannels().countNumberOfSetBits()));
    }
    stopAll();
}

void LoopEngine::audioDeviceStopped()
{
    stopAll();
    effectsChain.reset();
}

bool LoopEngine::isValidSlot(int slotIndex)
{
    return juce::isPositiveAndBelow(slotIndex, numberOfSlots);
}

void LoopEngine::createWaveformOverview(Clip& target)
{
    constexpr int overviewPoints = 2048;
    const auto sampleCount = target.audio.getNumSamples();
    const auto pointCount = juce::jmin(overviewPoints, sampleCount);
    target.waveformMinimum.resize(static_cast<size_t>(pointCount));
    target.waveformMaximum.resize(static_cast<size_t>(pointCount));

    for (int point = 0; point < pointCount; ++point)
    {
        const auto firstSample = static_cast<int>(
            static_cast<int64_t>(point) * sampleCount / pointCount);
        const auto lastSample = juce::jlimit(firstSample + 1, sampleCount,
            static_cast<int>(static_cast<int64_t>(point + 1) * sampleCount / pointCount));
        auto minimum = 0.0f;
        auto maximum = 0.0f;

        for (int channel = 0; channel < target.audio.getNumChannels(); ++channel)
        {
            const auto range = target.audio.findMinMax(channel, firstSample, lastSample - firstSample);
            minimum = juce::jmin(minimum, range.getStart());
            maximum = juce::jmax(maximum, range.getEnd());
        }
        target.waveformMinimum[static_cast<size_t>(point)] = minimum;
        target.waveformMaximum[static_cast<size_t>(point)] = maximum;
    }
}

void LoopEngine::renderVoice(Voice& voice, float* const* outputs,
                             int outputChannels, int numSamples)
{
    const auto currentClip = std::atomic_load(&voice.clip);
    if (currentClip == nullptr || currentClip->audio.getNumSamples() < 2)
        return;

    const auto sourceLength = currentClip->audio.getNumSamples();
    const auto startSample = voice.trimStart.load() * static_cast<double>(sourceLength - 1);
    const auto endSample = juce::jmax(startSample + 1.0,
                                     voice.trimEnd.load() * static_cast<double>(sourceLength));
    const auto isReversed = voice.reverse.load();

    const auto pendingCommand = voice.command.exchange(Command::none);
    if (pendingCommand == Command::stop)
    {
        beginRelease(voice);
    }
    else if (pendingCommand == Command::stopImmediate)
    {
        voice.playing.store(false);
        voice.envelopeLevel = 0.0f;
        voice.envelopeStage = EnvelopeStage::idle;
        voice.playhead = isReversed ? endSample - 0.0001 : startSample;
    }
    else if (pendingCommand == Command::play)
    {
        voice.playhead = isReversed ? endSample - 0.0001 : startSample;
        if (voice.envelopeCycle.load())
        {
            voice.envelopeLevel = 1.0f;
            voice.envelopeStage = EnvelopeStage::sustain;
        }
        else
        {
            voice.envelopeLevel = 0.0f;
            voice.envelopeStage = EnvelopeStage::attack;
        }
        voice.playing.store(true);
    }

    if (! voice.playing.load())
    {
        voice.playheadNormalised.store((isReversed ? endSample : startSample)
                                       / static_cast<double>(sourceLength));
        return;
    }

    const auto sourceChannels = currentClip->audio.getNumChannels();
    const auto currentRate = voice.playbackRate.load();
    const auto incrementMagnitude = (currentClip->sourceSampleRate / deviceSampleRate) * currentRate;
    const auto increment = isReversed ? -incrementMagnitude : incrementMagnitude;
    const auto currentGain = voice.gain.load();
    const auto shouldLoop = voice.looping.load();
    const auto cycleIsEnabled = voice.envelopeCycle.load();
    const auto crossfadeSamples = shouldLoop
        ? juce::jmin(currentClip->sourceSampleRate * 0.005, (endSample - startSample) * 0.25)
        : 0.0;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        const auto outsideRange = isReversed ? voice.playhead < startSample
                                             : voice.playhead >= endSample;
        if (outsideRange)
        {
            if (! shouldLoop)
            {
                voice.playing.store(false);
                voice.playhead = isReversed ? endSample - 0.0001 : startSample;
                break;
            }

            const auto loopSpan = juce::jmax(1.0, endSample - startSample - crossfadeSamples);
            if (isReversed)
            {
                const auto overshoot = startSample - voice.playhead;
                voice.playhead = endSample - crossfadeSamples - std::fmod(overshoot, loopSpan);
            }
            else
            {
                voice.playhead = startSample + crossfadeSamples
                               + std::fmod(voice.playhead - endSample, loopSpan);
            }
        }

        const auto first = juce::jlimit(0, sourceLength - 1,
                                        static_cast<int>(voice.playhead));
        const auto candidateSecond = first + 1;
        const auto second = static_cast<double>(candidateSecond) < endSample
            ? juce::jlimit(0, sourceLength - 1, candidateSecond)
            : (isReversed ? first : static_cast<int>(startSample));
        const auto alpha = static_cast<float>(voice.playhead - static_cast<double>(first));
        const auto distanceFromBeginning = isReversed ? endSample - voice.playhead
                                                      : voice.playhead - startSample;
        const auto fadeIn = crossfadeSamples > 0.0
            ? static_cast<float>(juce::jlimit(0.0, 1.0,
                                             distanceFromBeginning / crossfadeSamples))
            : 1.0f;
        const auto cycleLevel = cycleIsEnabled
            ? cycleEnvelopeLevel(voice, voice.playhead, startSample, endSample,
                                 currentClip->sourceSampleRate, currentRate, isReversed)
            : 1.0f;

        for (int channel = 0; channel < outputChannels; ++channel)
        {
            if (outputs[channel] == nullptr)
                continue;

            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            const auto* source = currentClip->audio.getReadPointer(sourceChannel);
            auto sample = source[first] + alpha * (source[second] - source[first]);

            const auto inCrossfade = crossfadeSamples > 0.0
                && (isReversed ? voice.playhead <= startSample + crossfadeSamples
                               : voice.playhead >= endSample - crossfadeSamples);
            if (inCrossfade)
            {
                const auto distance = isReversed
                    ? startSample + crossfadeSamples - voice.playhead
                    : voice.playhead - (endSample - crossfadeSamples);
                const auto fade = static_cast<float>(juce::jlimit(0.0, 1.0,
                                                                 distance / crossfadeSamples));
                const auto wrappedPosition = isReversed
                    ? endSample - distance
                    : startSample + distance;
                const auto wrappedFirst = juce::jlimit(0, sourceLength - 1,
                                                       static_cast<int>(wrappedPosition));
                const auto wrappedSecond = juce::jlimit(0, sourceLength - 1, wrappedFirst + 1);
                const auto wrappedAlpha = static_cast<float>(
                    wrappedPosition - static_cast<double>(wrappedFirst));
                const auto wrappedSample = source[wrappedFirst]
                    + wrappedAlpha * (source[wrappedSecond] - source[wrappedFirst]);
                sample = sample * (1.0f - fade) + wrappedSample * fade;
            }

            outputs[channel][frame] += sample * currentGain * fadeIn
                                     * voice.envelopeLevel * cycleLevel;
        }

        voice.playhead += increment;
        if (! cycleIsEnabled || voice.envelopeStage == EnvelopeStage::release)
            advanceEnvelope(voice);
        else
            voice.envelopeLevel = 1.0f;

        if (voice.envelopeStage == EnvelopeStage::idle)
        {
            voice.playing.store(false);
            voice.playhead = isReversed ? endSample - 0.0001 : startSample;
            break;
        }
    }

    voice.playheadNormalised.store(voice.playhead / static_cast<double>(sourceLength));
}

float LoopEngine::advanceEnvelope(Voice& voice)
{
    const auto oneSample = 1.0 / juce::jmax(1.0, deviceSampleRate);
    switch (voice.envelopeStage)
    {
        case EnvelopeStage::idle:
            voice.envelopeLevel = 0.0f;
            break;
        case EnvelopeStage::attack:
        {
            const auto duration = voice.attackSeconds.load();
            if (duration <= oneSample)
            {
                voice.envelopeLevel = 1.0f;
                voice.envelopeStage = EnvelopeStage::decay;
            }
            else if ((voice.envelopeLevel += static_cast<float>(oneSample / duration)) >= 1.0f)
            {
                voice.envelopeLevel = 1.0f;
                voice.envelopeStage = EnvelopeStage::decay;
            }
            break;
        }
        case EnvelopeStage::decay:
        {
            const auto target = voice.sustainLevel.load();
            const auto duration = voice.decaySeconds.load();
            if (duration <= oneSample || voice.envelopeLevel <= target)
            {
                voice.envelopeLevel = target;
                voice.envelopeStage = EnvelopeStage::sustain;
            }
            else
            {
                voice.envelopeLevel -= static_cast<float>((1.0 - target) * oneSample / duration);
                if (voice.envelopeLevel <= target)
                {
                    voice.envelopeLevel = target;
                    voice.envelopeStage = EnvelopeStage::sustain;
                }
            }
            break;
        }
        case EnvelopeStage::sustain:
            voice.envelopeLevel = voice.sustainLevel.load();
            break;
        case EnvelopeStage::release:
            voice.envelopeLevel -= voice.releaseStep;
            if (voice.envelopeLevel <= 0.0f)
            {
                voice.envelopeLevel = 0.0f;
                voice.envelopeStage = EnvelopeStage::idle;
            }
            break;
    }
    return voice.envelopeLevel;
}

void LoopEngine::beginRelease(Voice& voice)
{
    if (! voice.playing.load())
        return;

    const auto duration = voice.releaseSeconds.load();
    if (duration <= 0.0 || voice.envelopeLevel <= 0.0f)
    {
        voice.envelopeLevel = 0.0f;
        voice.envelopeStage = EnvelopeStage::idle;
        voice.playing.store(false);
        return;
    }

    voice.releaseStep = voice.envelopeLevel
        / static_cast<float>(juce::jmax(1.0, duration * deviceSampleRate));
    voice.envelopeStage = EnvelopeStage::release;
}

float LoopEngine::cycleEnvelopeLevel(const Voice& voice, double position,
                                     double startSample, double endSample,
                                     double sourceSampleRate, double rate,
                                     bool isReversed) const
{
    const auto safeRate = juce::jmax(0.01, rate);
    const auto duration = (endSample - startSample) / (sourceSampleRate * safeRate);
    if (duration <= 0.0)
        return 0.0f;

    auto attack = voice.attackSeconds.load();
    auto decay = voice.decaySeconds.load();
    auto release = voice.releaseSeconds.load();
    const auto shapedDuration = attack + decay + release;
    if (shapedDuration > duration && shapedDuration > 0.0)
    {
        const auto scale = duration / shapedDuration;
        attack *= scale;
        decay *= scale;
        release *= scale;
    }

    const auto travelledSamples = isReversed ? endSample - position : position - startSample;
    const auto elapsed = juce::jlimit(0.0, duration,
                                     travelledSamples / (sourceSampleRate * safeRate));
    const auto sustain = voice.sustainLevel.load();
    if (attack > 0.0 && elapsed < attack)
        return static_cast<float>(elapsed / attack);
    if (decay > 0.0 && elapsed < attack + decay)
    {
        const auto phase = static_cast<float>((elapsed - attack) / decay);
        return 1.0f + phase * (sustain - 1.0f);
    }
    if (release > 0.0 && elapsed > duration - release)
        return sustain * static_cast<float>((duration - elapsed) / release);
    return sustain;
}
