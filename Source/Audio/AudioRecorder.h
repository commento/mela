#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

class AudioRecorder final
{
public:
    AudioRecorder();
    ~AudioRecorder();

    bool start(const juce::File& destination, double sampleRate,
               int numberOfChannels, juce::String& errorMessage);
    void stop();
    void process(const float* const* inputChannelData,
                 int numberOfInputChannels, int numberOfSamples);

    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] double getDurationSeconds() const;
    [[nodiscard]] juce::File getCurrentFile() const;

private:
    juce::TimeSliceThread backgroundThread { "Mela recording writer" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    juce::CriticalSection writerLock;
    juce::File currentFile;
    std::atomic<int64_t> samplesWritten { 0 };
    double recordingSampleRate = 0.0;
    int recordingChannels = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};
