#pragma once

#include <JuceHeader.h>
#include <array>

class LinuxMultiTouchInput final : private juce::Thread,
                                   private juce::AsyncUpdater
{
public:
    static constexpr int maximumTouches = 10;

    struct Contact
    {
        bool active = false;
        juce::Point<float> normalisedPosition;
    };

    using Snapshot = std::array<Contact, maximumTouches>;

    LinuxMultiTouchInput();
    ~LinuxMultiTouchInput() override;

    void start();
    void stop();

    std::function<void(const Snapshot&)> onTouchesChanged;
    std::function<void(bool)> onAvailabilityChanged;

private:
    void run() override;
    void handleAsyncUpdate() override;
    void publishSnapshot(const Snapshot& newSnapshot);
    void publishAvailability(bool isAvailable);

    juce::CriticalSection stateLock;
    Snapshot latestSnapshot;
    bool latestAvailability = false;
    bool availabilityDirty = false;
};
