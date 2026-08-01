#include "LoopEngine.h"

#include <cmath>
#include <cstdint>
#include <limits>

LoopEngine::LoopEngine()
{
    formatManager.registerBasicFormats();
}

bool LoopEngine::loadFile(const juce::File& file, juce::String& errorMessage)
{
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
    command.store(Command::stop);
    trimStart.store(0.0);
    trimEnd.store(1.0);
    std::atomic_store(&clip, std::shared_ptr<const Clip>(std::move(newClip)));
    return true;
}

void LoopEngine::play()
{
    command.store(Command::play);
}

void LoopEngine::stop()
{
    command.store(Command::stop);
}

void LoopEngine::setLooping(bool shouldLoop)
{
    looping.store(shouldLoop);
}

void LoopEngine::setGain(float newGain)
{
    gain.store(juce::jlimit(0.0f, 1.0f, newGain));
}

void LoopEngine::setPlaybackRate(double newRate)
{
    playbackRate.store(juce::jlimit(0.25, 1.5, newRate));
}

void LoopEngine::setTrimRange(double newStart, double newEnd)
{
    constexpr auto minimumRange = 0.001;
    const auto start = juce::jlimit(0.0, 1.0 - minimumRange, newStart);
    const auto end = juce::jlimit(start + minimumRange, 1.0, newEnd);
    trimStart.store(start);
    trimEnd.store(end);
}

void LoopEngine::setEnvelope(double newAttack, double newDecay,
                             float newSustain, double newRelease)
{
    attackSeconds.store(juce::jlimit(0.0, 10.0, newAttack));
    decaySeconds.store(juce::jlimit(0.0, 10.0, newDecay));
    sustainLevel.store(juce::jlimit(0.0f, 1.0f, newSustain));
    releaseSeconds.store(juce::jlimit(0.0, 20.0, newRelease));
}

void LoopEngine::setEnvelopeCycle(bool shouldRepeat)
{
    envelopeCycle.store(shouldRepeat);
}

bool LoopEngine::hasClip() const
{
    return std::atomic_load(&clip) != nullptr;
}

bool LoopEngine::isPlaying() const
{
    return playing.load();
}

juce::String LoopEngine::getClipName() const
{
    const auto currentClip = std::atomic_load(&clip);
    return currentClip != nullptr ? currentClip->name : juce::String {};
}

double LoopEngine::getPlayheadNormalised() const
{
    return playheadNormalised.load();
}

std::shared_ptr<const LoopEngine::Clip> LoopEngine::getClipForDisplay() const
{
    return std::atomic_load(&clip);
}

void LoopEngine::audioDeviceIOCallbackWithContext(const float* const*,
                                                   int,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    for (int channel = 0; channel < numOutputChannels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);

    render(outputChannelData, numOutputChannels, numSamples);
}

void LoopEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    deviceSampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    stop();
}

void LoopEngine::audioDeviceStopped()
{
    stop();
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
        // Use 64-bit intermediates: for clips longer than roughly 20 seconds at
        // 48 kHz, point * sampleCount can overflow a 32-bit int and make the
        // overview appear as repeated sections of the waveform.
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

void LoopEngine::render(float* const* outputs, int outputChannels, int numSamples)
{
    const auto currentClip = std::atomic_load(&clip);
    if (currentClip == nullptr || currentClip->audio.getNumSamples() < 2)
        return;

    const auto sourceLength = currentClip->audio.getNumSamples();
    const auto startSample = trimStart.load() * static_cast<double>(sourceLength - 1);
    const auto endSample = juce::jmax(startSample + 1.0,
                                     trimEnd.load() * static_cast<double>(sourceLength));

    const auto pendingCommand = command.exchange(Command::none);
    if (pendingCommand == Command::stop)
    {
        beginRelease();
    }
    else if (pendingCommand == Command::play)
    {
        playhead = startSample;
        if (envelopeCycle.load())
        {
            envelopeLevel = 1.0f;
            envelopeStage = EnvelopeStage::sustain;
        }
        else
        {
            envelopeLevel = 0.0f;
            envelopeStage = EnvelopeStage::attack;
        }
        playing.store(true);
    }

    if (! playing.load())
    {
        playheadNormalised.store(startSample / static_cast<double>(sourceLength));
        return;
    }

    const auto sourceChannels = currentClip->audio.getNumChannels();
    const auto currentRate = playbackRate.load();
    const auto increment = (currentClip->sourceSampleRate / deviceSampleRate) * currentRate;
    const auto currentGain = gain.load();
    const auto shouldLoop = looping.load();
    const auto cycleIsEnabled = envelopeCycle.load();
    const auto crossfadeSamples = shouldLoop
        ? juce::jmin(currentClip->sourceSampleRate * 0.005, (endSample - startSample) * 0.25)
        : 0.0;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        if (playhead < startSample)
            playhead = startSample;

        if (playhead >= endSample)
        {
            if (shouldLoop)
            {
                const auto loopSpan = juce::jmax(1.0,
                    endSample - startSample - crossfadeSamples);
                playhead = startSample + crossfadeSamples
                         + std::fmod(playhead - endSample, loopSpan);
            }
            else
            {
                playing.store(false);
                playhead = startSample;
                break;
            }
        }

        const auto first = juce::jlimit(0, sourceLength - 1, static_cast<int>(playhead));
        const auto second = (static_cast<double>(first + 1) < endSample) ? first + 1
                                                                        : static_cast<int>(startSample);
        const auto alpha = static_cast<float>(playhead - static_cast<double>(first));
        const auto fadeIn = crossfadeSamples > 0.0
            ? static_cast<float>(juce::jlimit(0.0, 1.0,
                (playhead - startSample) / crossfadeSamples))
            : 1.0f;
        const auto cycleLevel = cycleIsEnabled
            ? cycleEnvelopeLevel(playhead, startSample, endSample,
                                 currentClip->sourceSampleRate, currentRate)
            : 1.0f;

        for (int channel = 0; channel < outputChannels; ++channel)
        {
            if (outputs[channel] == nullptr)
                continue;

            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            const auto* source = currentClip->audio.getReadPointer(sourceChannel);
            auto sample = source[first] + alpha * (source[second] - source[first]);

            if (crossfadeSamples > 0.0 && playhead >= endSample - crossfadeSamples)
            {
                const auto fade = static_cast<float>(
                    (playhead - (endSample - crossfadeSamples)) / crossfadeSamples);
                const auto wrappedPosition = startSample
                                           + playhead - (endSample - crossfadeSamples);
                const auto wrappedFirst = juce::jlimit(
                    0, sourceLength - 1, static_cast<int>(wrappedPosition));
                const auto wrappedSecond = juce::jlimit(0, sourceLength - 1, wrappedFirst + 1);
                const auto wrappedAlpha = static_cast<float>(
                    wrappedPosition - static_cast<double>(wrappedFirst));
                const auto wrappedSample = source[wrappedFirst]
                    + wrappedAlpha * (source[wrappedSecond] - source[wrappedFirst]);
                sample = sample * (1.0f - fade) + wrappedSample * fade;
            }

            outputs[channel][frame] += sample * currentGain * fadeIn
                                     * envelopeLevel * cycleLevel;
        }

        playhead += increment;
        if (! cycleIsEnabled || envelopeStage == EnvelopeStage::release)
            advanceEnvelope();
        else
            envelopeLevel = 1.0f;

        if (envelopeStage == EnvelopeStage::idle)
        {
            playing.store(false);
            playhead = startSample;
            break;
        }
    }

    playheadNormalised.store(playhead / static_cast<double>(sourceLength));
}

float LoopEngine::advanceEnvelope()
{
    const auto oneSample = 1.0 / juce::jmax(1.0, deviceSampleRate);

    switch (envelopeStage)
    {
        case EnvelopeStage::idle:
            envelopeLevel = 0.0f;
            break;

        case EnvelopeStage::attack:
        {
            const auto duration = attackSeconds.load();
            if (duration <= oneSample)
            {
                envelopeLevel = 1.0f;
                envelopeStage = EnvelopeStage::decay;
            }
            else
            {
                envelopeLevel += static_cast<float>(oneSample / duration);
                if (envelopeLevel >= 1.0f)
                {
                    envelopeLevel = 1.0f;
                    envelopeStage = EnvelopeStage::decay;
                }
            }
            break;
        }

        case EnvelopeStage::decay:
        {
            const auto target = sustainLevel.load();
            const auto duration = decaySeconds.load();
            if (duration <= oneSample || envelopeLevel <= target)
            {
                envelopeLevel = target;
                envelopeStage = EnvelopeStage::sustain;
            }
            else
            {
                envelopeLevel -= static_cast<float>((1.0 - target) * oneSample / duration);
                if (envelopeLevel <= target)
                {
                    envelopeLevel = target;
                    envelopeStage = EnvelopeStage::sustain;
                }
            }
            break;
        }

        case EnvelopeStage::sustain:
            envelopeLevel = sustainLevel.load();
            break;

        case EnvelopeStage::release:
            envelopeLevel -= releaseStep;
            if (envelopeLevel <= 0.0f)
            {
                envelopeLevel = 0.0f;
                envelopeStage = EnvelopeStage::idle;
            }
            break;
    }

    return envelopeLevel;
}

void LoopEngine::beginRelease()
{
    if (! playing.load())
        return;

    const auto duration = releaseSeconds.load();
    if (duration <= 0.0 || envelopeLevel <= 0.0f)
    {
        envelopeLevel = 0.0f;
        envelopeStage = EnvelopeStage::idle;
        playing.store(false);
        return;
    }

    releaseStep = envelopeLevel
        / static_cast<float>(juce::jmax(1.0, duration * deviceSampleRate));
    envelopeStage = EnvelopeStage::release;
}

float LoopEngine::cycleEnvelopeLevel(double position, double startSample, double endSample,
                                     double sourceSampleRate, double rate) const
{
    const auto safeRate = juce::jmax(0.01, rate);
    const auto duration = (endSample - startSample) / (sourceSampleRate * safeRate);
    if (duration <= 0.0)
        return 0.0f;

    auto attack = attackSeconds.load();
    auto decay = decaySeconds.load();
    auto release = releaseSeconds.load();
    const auto shapedDuration = attack + decay + release;

    if (shapedDuration > duration && shapedDuration > 0.0)
    {
        const auto scale = duration / shapedDuration;
        attack *= scale;
        decay *= scale;
        release *= scale;
    }

    const auto elapsed = juce::jlimit(0.0, duration,
        (position - startSample) / (sourceSampleRate * safeRate));
    const auto sustain = sustainLevel.load();

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
