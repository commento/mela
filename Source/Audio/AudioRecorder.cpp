#include "AudioRecorder.h"

AudioRecorder::AudioRecorder()
{
    backgroundThread.startThread();
}

AudioRecorder::~AudioRecorder()
{
    stop();
    backgroundThread.stopThread(2000);
}

bool AudioRecorder::start(const juce::File& destination, double sampleRate,
                          int numberOfChannels, juce::String& errorMessage)
{
    stop();

    if (sampleRate <= 0.0 || numberOfChannels <= 0)
    {
        errorMessage = "Nessun ingresso audio attivo";
        return false;
    }

    const auto channels = juce::jlimit(1, 2, numberOfChannels);
    destination.deleteFile();
    auto fileStream = destination.createOutputStream();
    if (fileStream == nullptr || ! fileStream->openedOk())
    {
        const auto detail = fileStream != nullptr
            ? fileStream->getStatus().getErrorMessage() : juce::String();
        errorMessage = "File REC non scrivibile: " + destination.getFullPathName();
        if (detail.isNotEmpty())
            errorMessage += " - " + detail;
        return false;
    }
    std::unique_ptr<juce::OutputStream> stream = std::move(fileStream);

    juce::WavAudioFormat wavFormat;
    const auto options = juce::AudioFormatWriterOptions {}
        .withSampleRate(sampleRate)
        .withNumChannels(channels)
        .withBitsPerSample(24);
    auto writer = wavFormat.createWriterFor(stream, options);
    if (writer == nullptr)
    {
        errorMessage = "Impossibile creare il registratore WAV";
        return false;
    }

    currentFile = destination;
    recordingSampleRate = sampleRate;
    recordingChannels = channels;
    samplesWritten.store(0);
    threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        writer.release(), backgroundThread, 131072);

    const juce::ScopedLock lock(writerLock);
    activeWriter.store(threadedWriter.get());
    return true;
}

void AudioRecorder::stop()
{
    {
        const juce::ScopedLock lock(writerLock);
        activeWriter.store(nullptr);
    }

    threadedWriter.reset();
    recordingChannels = 0;
}

void AudioRecorder::process(const float* const* inputChannelData,
                            int numberOfInputChannels, int numberOfSamples)
{
    const juce::ScopedLock lock(writerLock);
    auto* writer = activeWriter.load();
    if (writer == nullptr || inputChannelData == nullptr
        || numberOfInputChannels < recordingChannels)
        return;

    for (int channel = 0; channel < recordingChannels; ++channel)
        if (inputChannelData[channel] == nullptr)
            return;

    if (writer->write(inputChannelData, numberOfSamples))
        samplesWritten.fetch_add(numberOfSamples);
}

bool AudioRecorder::isRecording() const
{
    return activeWriter.load() != nullptr;
}

double AudioRecorder::getDurationSeconds() const
{
    return recordingSampleRate > 0.0
        ? static_cast<double>(samplesWritten.load()) / recordingSampleRate : 0.0;
}

juce::File AudioRecorder::getCurrentFile() const
{
    return currentFile;
}
