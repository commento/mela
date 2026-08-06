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

void LoopEngine::clearSlot(int slotIndex)
{
    if (! isValidSlot(slotIndex))
        return;

    auto& voice = voices[static_cast<size_t>(slotIndex)];
    voice.command.store(Command::stopImmediate);
    voice.playing.store(false);
    voice.playheadNormalised.store(0.0);
    std::atomic_store(&voice.clip, std::shared_ptr<const Clip> {});
    allNotesOff(slotIndex);
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
    allNotesOff();
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

    const auto currentClip = std::atomic_load(
        &voices[static_cast<size_t>(slotIndex)].clip);
    const auto minimumRange = currentClip != nullptr && currentClip->audio.getNumSamples() >= 2
        ? 1.0 / static_cast<double>(currentClip->audio.getNumSamples())
        : 0.000001;
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

void LoopEngine::setInstrumentRootNote(int slotIndex, int midiNote)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].instrumentRootNote.store(
            juce::jlimit(0, 127, midiNote));
}

void LoopEngine::setInstrumentMode(int slotIndex, InstrumentMode mode)
{
    if (isValidSlot(slotIndex))
        voices[static_cast<size_t>(slotIndex)].instrumentMode.store(mode);
}

void LoopEngine::setInstrumentEnvelope(int slotIndex, double attack, double decay,
                                       float sustain, double release)
{
    if (! isValidSlot(slotIndex))
        return;

    auto& voice = voices[static_cast<size_t>(slotIndex)];
    voice.instrumentAttack.store(juce::jlimit(0.0, 10.0, attack));
    voice.instrumentDecay.store(juce::jlimit(0.0, 10.0, decay));
    voice.instrumentSustain.store(juce::jlimit(0.0f, 1.0f, sustain));
    voice.instrumentRelease.store(juce::jlimit(0.0, 20.0, release));
}

void LoopEngine::noteOn(int slotIndex, int midiNote, float velocity)
{
    if (isValidSlot(slotIndex) && juce::isPositiveAndBelow(midiNote, 128))
        pushNoteCommand({ NoteCommand::Type::noteOn, slotIndex, midiNote,
                          juce::jlimit(0.0f, 1.0f, velocity) });
}

void LoopEngine::noteOff(int slotIndex, int midiNote)
{
    if (isValidSlot(slotIndex) && juce::isPositiveAndBelow(midiNote, 128))
        pushNoteCommand({ NoteCommand::Type::noteOff, slotIndex, midiNote, 0.0f });
}

void LoopEngine::allNotesOff()
{
    pushNoteCommand({ NoteCommand::Type::allNotesOff, 0, 0, 0.0f });
}

void LoopEngine::allNotesOff(int slotIndex)
{
    if (isValidSlot(slotIndex))
        pushNoteCommand({ NoteCommand::Type::allNotesForSlot, slotIndex, 0, 0.0f });
}

bool LoopEngine::startRecording(const juce::File& destination, juce::String& errorMessage)
{
    return recorder.start(destination, deviceSampleRate,
                          activeInputChannels.load(), errorMessage);
}

juce::File LoopEngine::stopRecording()
{
    const auto file = recorder.getCurrentFile();
    recorder.stop();
    return file;
}

bool LoopEngine::isRecording() const
{
    return recorder.isRecording();
}

double LoopEngine::getRecordingDurationSeconds() const
{
    return recorder.getDurationSeconds();
}

void LoopEngine::setEqualizer(int slotIndex, float lowDb, float midDb, float highDb)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].chain.setEqualizer(
            lowDb, midDb, highDb);
}

void LoopEngine::setDistortion(int slotIndex, bool enabled,
                               float drive, float toneHz, float mix)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].chain.setDistortion(
            enabled, drive, toneHz, mix);
}

void LoopEngine::setGranular(int slotIndex, bool enabled, float sizeMs, float densityHz,
                             float positionMs, float pitchSemitones, float mix)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].chain.setGranular(
            enabled, sizeMs, densityHz, positionMs, pitchSemitones, mix);
}

void LoopEngine::setFlanger(int slotIndex, bool enabled, float rateHz, float depth,
                            float feedback, float mix)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].chain.setFlanger(
            enabled, rateHz, depth, feedback, mix);
}

void LoopEngine::setChorus(int slotIndex, bool enabled, float rateHz, float depth, float mix)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].chain.setChorus(
            enabled, rateHz, depth, mix);
}

void LoopEngine::setDelaySend(int slotIndex, float amount)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].delaySend.store(
            juce::jlimit(0.0f, 1.0f, amount));
}

void LoopEngine::setReverbSend(int slotIndex, float amount)
{
    if (isValidSlot(slotIndex))
        slotEffects[static_cast<size_t>(slotIndex)].reverbSend.store(
            juce::jlimit(0.0f, 1.0f, amount));
}

void LoopEngine::setDelay(bool enabled, float timeMs, float feedback, float mix)
{
    masterEffects.setDelay(enabled, timeMs, feedback, mix);
}

void LoopEngine::setReverb(bool enabled, float size, float damping, float mix)
{
    masterEffects.setReverb(enabled, size, damping, mix);
}

void LoopEngine::setMasterEqualizer(float lowDb, float midDb, float highDb)
{
    masterEffects.setEqualizer(lowDb, midDb, highDb);
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

void LoopEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels, int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    recorder.process(inputChannelData, numInputChannels, numSamples);

    for (int channel = 0; channel < numOutputChannels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);

    processNoteCommands();
    const auto channels = juce::jmin(numOutputChannels, masterMixBuffer.getNumChannels());
    if (channels <= 0 || masterMixBuffer.getNumSamples() < numSamples)
        return;

    juce::AudioBuffer<float> mix(masterMixBuffer.getArrayOfWritePointers(),
                                 channels, numSamples);
    juce::AudioBuffer<float> delayBus(delaySendBuffer.getArrayOfWritePointers(),
                                      channels, numSamples);
    juce::AudioBuffer<float> reverbBus(reverbSendBuffer.getArrayOfWritePointers(),
                                       channels, numSamples);
    mix.clear();
    delayBus.clear();
    reverbBus.clear();

    for (int slot = 0; slot < numberOfSlots; ++slot)
    {
        auto& storage = slotEffectBuffers[static_cast<size_t>(slot)];
        juce::AudioBuffer<float> slotBuffer(storage.getArrayOfWritePointers(),
                                            channels, numSamples);
        slotBuffer.clear();
        renderVoice(voices[static_cast<size_t>(slot)],
                    slotBuffer.getArrayOfWritePointers(), channels, numSamples);
        for (auto& instrumentVoice : instrumentVoices)
            if (instrumentVoice.slotIndex == slot)
                renderInstrumentVoice(instrumentVoice,
                                      slotBuffer.getArrayOfWritePointers(),
                                      channels, numSamples);

        auto& effects = slotEffects[static_cast<size_t>(slot)];
        effects.chain.processInserts(slotBuffer);
        const auto delaySend = effects.delaySend.load();
        const auto reverbSend = effects.reverbSend.load();
        for (int channel = 0; channel < channels; ++channel)
        {
            mix.addFrom(channel, 0, slotBuffer, channel, 0, numSamples);
            if (delaySend > 0.0f)
                delayBus.addFrom(channel, 0, slotBuffer, channel, 0,
                                 numSamples, delaySend);
            if (reverbSend > 0.0f)
                reverbBus.addFrom(channel, 0, slotBuffer, channel, 0,
                                  numSamples, reverbSend);
        }
    }

    masterEffects.processEqualizer(mix);
    masterEffects.processDelayReturn(delayBus);
    masterEffects.processReverbReturn(reverbBus);
    for (int channel = 0; channel < channels; ++channel)
    {
        mix.addFrom(channel, 0, delayBus, channel, 0, numSamples);
        mix.addFrom(channel, 0, reverbBus, channel, 0, numSamples);
    }
    masterEffects.processLimiter(mix);

    for (int channel = 0; channel < channels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::copy(outputChannelData[channel],
                                              mix.getReadPointer(channel), numSamples);
}

void LoopEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto previousSampleRate = deviceSampleRate;
    deviceSampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    const auto rateCorrection = previousSampleRate / juce::jmax(1.0, deviceSampleRate);

    // Keep active notes and releases at the same pitch/duration if the new
    // device uses a different sample rate.
    if (! juce::approximatelyEqual(rateCorrection, 1.0))
    {
        for (auto& voice : voices)
            if (voice.envelopeStage == EnvelopeStage::release)
                voice.releaseStep *= static_cast<float>(rateCorrection);

        for (auto& instrumentVoice : instrumentVoices)
        {
            if (! instrumentVoice.active)
                continue;
            instrumentVoice.increment *= rateCorrection;
            if (instrumentVoice.envelopeStage == EnvelopeStage::release)
                instrumentVoice.releaseStep *= static_cast<float>(rateCorrection);
        }
    }

    if (device != nullptr)
    {
        activeInputChannels.store(
            device->getActiveInputChannels().countNumberOfSetBits());
        const auto maximumBlockSize = juce::jmax(1, device->getCurrentBufferSizeSamples());
        const auto channels = juce::jmax(
            1, device->getActiveOutputChannels().countNumberOfSetBits());
        for (auto& effects : slotEffects)
            effects.chain.prepare(deviceSampleRate, maximumBlockSize, channels);
        masterEffects.prepare(deviceSampleRate, maximumBlockSize, channels);
        for (auto& buffer : slotEffectBuffers)
            buffer.setSize(channels, maximumBlockSize, false, true, false);
        masterMixBuffer.setSize(channels, maximumBlockSize, false, true, false);
        delaySendBuffer.setSize(channels, maximumBlockSize, false, true, false);
        reverbSendBuffer.setSize(channels, maximumBlockSize, false, true, false);
    }
}

void LoopEngine::audioDeviceStopped()
{
    activeInputChannels.store(0);
    for (auto& effects : slotEffects)
        effects.chain.reset();
    masterEffects.reset();
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
        const auto beforeStart = voice.playhead < startSample;
        const auto afterEnd = voice.playhead >= endSample;
        const auto trimMovedPastPlayhead = isReversed ? afterEnd : beforeStart;
        if (trimMovedPastPlayhead)
        {
            // Live trim edits can move the entry boundary over the current
            // playhead. Jump straight into the new range instead of outputting
            // silence until the old playhead catches up.
            voice.playhead = isReversed ? endSample - 0.0001 : startSample;
        }

        const auto reachedPlaybackEnd = isReversed ? beforeStart : afterEnd;
        if (reachedPlaybackEnd)
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

void LoopEngine::pushNoteCommand(NoteCommand commandToPush)
{
    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    noteCommandFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 == 0)
        return;

    noteCommands[static_cast<size_t>(start1)] = commandToPush;
    noteCommandFifo.finishedWrite(1);
}

void LoopEngine::processNoteCommands()
{
    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    noteCommandFifo.prepareToRead(noteCommandCapacity, start1, size1, start2, size2);

    const auto processCommand = [this](const NoteCommand& noteCommand)
    {
        if (noteCommand.type == NoteCommand::Type::noteOn)
        {
            startInstrumentVoice(noteCommand);
            return;
        }

        for (auto& instrumentVoice : instrumentVoices)
        {
            if (! instrumentVoice.active)
                continue;

            if (noteCommand.type == NoteCommand::Type::allNotesOff
                || (noteCommand.type == NoteCommand::Type::allNotesForSlot
                    && instrumentVoice.slotIndex == noteCommand.slotIndex)
                || (instrumentVoice.slotIndex == noteCommand.slotIndex
                    && instrumentVoice.midiNote == noteCommand.midiNote
                    && instrumentVoice.mode != InstrumentMode::oneShot))
                releaseInstrumentVoice(instrumentVoice);
        }
    };

    for (int index = 0; index < size1; ++index)
        processCommand(noteCommands[static_cast<size_t>(start1 + index)]);
    for (int index = 0; index < size2; ++index)
        processCommand(noteCommands[static_cast<size_t>(start2 + index)]);
    noteCommandFifo.finishedRead(size1 + size2);
}

void LoopEngine::startInstrumentVoice(const NoteCommand& commandToStart)
{
    if (! isValidSlot(commandToStart.slotIndex))
        return;

    auto& sourceVoice = voices[static_cast<size_t>(commandToStart.slotIndex)];
    const auto sourceClip = std::atomic_load(&sourceVoice.clip);
    if (sourceClip == nullptr || sourceClip->audio.getNumSamples() < 2)
        return;

    auto* targetVoice = &instrumentVoices.front();
    for (auto& candidate : instrumentVoices)
    {
        if (! candidate.active)
        {
            targetVoice = &candidate;
            break;
        }
        if (candidate.age < targetVoice->age)
            targetVoice = &candidate;
    }

    const auto sourceLength = sourceClip->audio.getNumSamples();
    targetVoice->clip = sourceClip;
    targetVoice->active = true;
    targetVoice->releasing = false;
    targetVoice->reversed = sourceVoice.reverse.load();
    targetVoice->slotIndex = commandToStart.slotIndex;
    targetVoice->midiNote = commandToStart.midiNote;
    targetVoice->mode = sourceVoice.instrumentMode.load();
    targetVoice->startSample = sourceVoice.trimStart.load() * (sourceLength - 1.0);
    targetVoice->endSample = juce::jmax(targetVoice->startSample + 1.0,
                                        sourceVoice.trimEnd.load() * sourceLength);
    targetVoice->playhead = targetVoice->reversed
        ? targetVoice->endSample - 0.0001 : targetVoice->startSample;

    const auto semitones = commandToStart.midiNote
                         - sourceVoice.instrumentRootNote.load();
    const auto pitchRatio = std::pow(2.0, static_cast<double>(semitones) / 12.0);
    const auto incrementMagnitude = sourceClip->sourceSampleRate / deviceSampleRate
                                  * sourceVoice.playbackRate.load() * pitchRatio;
    targetVoice->increment = targetVoice->reversed ? -incrementMagnitude : incrementMagnitude;
    targetVoice->gain = sourceVoice.gain.load() * commandToStart.velocity;
    targetVoice->attack = sourceVoice.instrumentAttack.load();
    targetVoice->decay = sourceVoice.instrumentDecay.load();
    targetVoice->sustain = sourceVoice.instrumentSustain.load();
    targetVoice->release = sourceVoice.instrumentRelease.load();
    targetVoice->envelopeStage = EnvelopeStage::attack;
    targetVoice->envelopeLevel = 0.0f;
    targetVoice->releaseStep = 0.0f;
    targetVoice->age = ++instrumentVoiceCounter;
}

void LoopEngine::releaseInstrumentVoice(InstrumentVoice& instrumentVoice)
{
    if (! instrumentVoice.active || instrumentVoice.releasing)
        return;

    if (instrumentVoice.release <= 0.0 || instrumentVoice.envelopeLevel <= 0.0f)
    {
        instrumentVoice.active = false;
        instrumentVoice.clip.reset();
        return;
    }

    instrumentVoice.releasing = true;
    instrumentVoice.releaseStep = instrumentVoice.envelopeLevel
        / static_cast<float>(juce::jmax(1.0, instrumentVoice.release * deviceSampleRate));
    instrumentVoice.envelopeStage = EnvelopeStage::release;
}

void LoopEngine::renderInstrumentVoice(InstrumentVoice& instrumentVoice,
                                       float* const* outputs, int outputChannels,
                                       int numSamples)
{
    if (! instrumentVoice.active || instrumentVoice.clip == nullptr)
        return;

    const auto& sourceClip = *instrumentVoice.clip;
    const auto sourceLength = sourceClip.audio.getNumSamples();
    const auto sourceChannels = sourceClip.audio.getNumChannels();
    const auto shouldLoop = instrumentVoice.mode == InstrumentMode::loop
                         || (instrumentVoice.releasing
                             && instrumentVoice.mode != InstrumentMode::oneShot);
    const auto crossfadeSamples = shouldLoop
        ? juce::jmin(sourceClip.sourceSampleRate * 0.005,
                     (instrumentVoice.endSample - instrumentVoice.startSample) * 0.25)
        : 0.0;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        if (instrumentVoice.mode == InstrumentMode::oneShot
            && ! instrumentVoice.releasing && instrumentVoice.release > 0.0)
        {
            const auto remainingSourceSamples = instrumentVoice.reversed
                ? instrumentVoice.playhead - instrumentVoice.startSample
                : instrumentVoice.endSample - instrumentVoice.playhead;
            const auto remainingOutputSamples = remainingSourceSamples
                / juce::jmax(0.000001, std::abs(instrumentVoice.increment));
            if (remainingOutputSamples <= instrumentVoice.release * deviceSampleRate)
                releaseInstrumentVoice(instrumentVoice);
        }

        const auto outsideRange = instrumentVoice.reversed
            ? instrumentVoice.playhead < instrumentVoice.startSample
            : instrumentVoice.playhead >= instrumentVoice.endSample;
        if (outsideRange)
        {
            if (! shouldLoop)
            {
                instrumentVoice.active = false;
                instrumentVoice.clip.reset();
                break;
            }

            const auto loopSpan = juce::jmax(1.0,
                instrumentVoice.endSample - instrumentVoice.startSample - crossfadeSamples);
            if (instrumentVoice.reversed)
            {
                const auto overshoot = instrumentVoice.startSample - instrumentVoice.playhead;
                instrumentVoice.playhead = instrumentVoice.endSample - crossfadeSamples
                                         - std::fmod(overshoot, loopSpan);
            }
            else
            {
                instrumentVoice.playhead = instrumentVoice.startSample + crossfadeSamples
                                         + std::fmod(instrumentVoice.playhead
                                                     - instrumentVoice.endSample, loopSpan);
            }
        }

        const auto first = juce::jlimit(0, sourceLength - 1,
                                        static_cast<int>(instrumentVoice.playhead));
        const auto candidateSecond = first + 1;
        const auto second = static_cast<double>(candidateSecond) < instrumentVoice.endSample
            ? juce::jlimit(0, sourceLength - 1, candidateSecond)
            : (instrumentVoice.reversed ? first
                                        : static_cast<int>(instrumentVoice.startSample));
        const auto alpha = static_cast<float>(instrumentVoice.playhead - first);

        for (int channel = 0; channel < outputChannels; ++channel)
        {
            if (outputs[channel] == nullptr)
                continue;
            const auto sourceChannel = juce::jmin(channel, sourceChannels - 1);
            const auto* source = sourceClip.audio.getReadPointer(sourceChannel);
            auto sample = source[first] + alpha * (source[second] - source[first]);

            const auto inCrossfade = crossfadeSamples > 0.0
                && (instrumentVoice.reversed
                    ? instrumentVoice.playhead <= instrumentVoice.startSample + crossfadeSamples
                    : instrumentVoice.playhead >= instrumentVoice.endSample - crossfadeSamples);
            if (inCrossfade)
            {
                const auto distance = instrumentVoice.reversed
                    ? instrumentVoice.startSample + crossfadeSamples - instrumentVoice.playhead
                    : instrumentVoice.playhead - (instrumentVoice.endSample - crossfadeSamples);
                const auto fade = static_cast<float>(juce::jlimit(
                    0.0, 1.0, distance / crossfadeSamples));
                const auto wrappedPosition = instrumentVoice.reversed
                    ? instrumentVoice.endSample - distance
                    : instrumentVoice.startSample + distance;
                const auto wrappedFirst = juce::jlimit(0, sourceLength - 1,
                                                       static_cast<int>(wrappedPosition));
                const auto wrappedSecond = juce::jlimit(0, sourceLength - 1, wrappedFirst + 1);
                const auto wrappedAlpha = static_cast<float>(wrappedPosition - wrappedFirst);
                const auto wrappedSample = source[wrappedFirst]
                    + wrappedAlpha * (source[wrappedSecond] - source[wrappedFirst]);
                sample = sample * (1.0f - fade) + wrappedSample * fade;
            }
            outputs[channel][frame] += sample * instrumentVoice.gain
                                     * instrumentVoice.envelopeLevel;
        }

        instrumentVoice.playhead += instrumentVoice.increment;
        advanceInstrumentEnvelope(instrumentVoice);
        if (instrumentVoice.envelopeStage == EnvelopeStage::idle)
        {
            instrumentVoice.active = false;
            instrumentVoice.clip.reset();
            break;
        }
    }
}

float LoopEngine::advanceInstrumentEnvelope(InstrumentVoice& instrumentVoice)
{
    const auto oneSample = 1.0 / juce::jmax(1.0, deviceSampleRate);
    switch (instrumentVoice.envelopeStage)
    {
        case EnvelopeStage::idle:
            instrumentVoice.envelopeLevel = 0.0f;
            break;
        case EnvelopeStage::attack:
            if (instrumentVoice.attack <= oneSample)
            {
                instrumentVoice.envelopeLevel = 1.0f;
                instrumentVoice.envelopeStage = EnvelopeStage::decay;
            }
            else if ((instrumentVoice.envelopeLevel += static_cast<float>(
                          oneSample / instrumentVoice.attack)) >= 1.0f)
            {
                instrumentVoice.envelopeLevel = 1.0f;
                instrumentVoice.envelopeStage = EnvelopeStage::decay;
            }
            break;
        case EnvelopeStage::decay:
            if (instrumentVoice.decay <= oneSample
                || instrumentVoice.envelopeLevel <= instrumentVoice.sustain)
            {
                instrumentVoice.envelopeLevel = instrumentVoice.sustain;
                instrumentVoice.envelopeStage = EnvelopeStage::sustain;
            }
            else
            {
                instrumentVoice.envelopeLevel -= static_cast<float>(
                    (1.0 - instrumentVoice.sustain) * oneSample / instrumentVoice.decay);
            }
            break;
        case EnvelopeStage::sustain:
            instrumentVoice.envelopeLevel = instrumentVoice.sustain;
            break;
        case EnvelopeStage::release:
            instrumentVoice.envelopeLevel -= instrumentVoice.releaseStep;
            if (instrumentVoice.envelopeLevel <= 0.0f)
            {
                instrumentVoice.envelopeLevel = 0.0f;
                instrumentVoice.envelopeStage = EnvelopeStage::idle;
            }
            break;
    }
    return instrumentVoice.envelopeLevel;
}
