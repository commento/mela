#include "LinuxMultiTouchInput.h"

#if JUCE_LINUX
 #include <cerrno>
 #include <fcntl.h>
 #include <linux/input.h>
 #include <poll.h>
 #include <sys/ioctl.h>
 #include <unistd.h>
#endif

LinuxMultiTouchInput::LinuxMultiTouchInput()
    : juce::Thread("Mela multitouch input")
{
}

LinuxMultiTouchInput::~LinuxMultiTouchInput()
{
    stop();
}

void LinuxMultiTouchInput::start()
{
   #if JUCE_LINUX
    startThread(juce::Thread::Priority::normal);
   #endif
}

void LinuxMultiTouchInput::stop()
{
    signalThreadShouldExit();
    stopThread(2000);
    cancelPendingUpdate();
}

void LinuxMultiTouchInput::publishSnapshot(const Snapshot& newSnapshot)
{
    {
        const juce::ScopedLock lock(stateLock);
        latestSnapshot = newSnapshot;
    }
    triggerAsyncUpdate();
}

void LinuxMultiTouchInput::publishAvailability(bool isAvailable)
{
    {
        const juce::ScopedLock lock(stateLock);
        if (latestAvailability == isAvailable && ! availabilityDirty)
            return;
        latestAvailability = isAvailable;
        availabilityDirty = true;
    }
    triggerAsyncUpdate();
}

void LinuxMultiTouchInput::handleAsyncUpdate()
{
    Snapshot snapshot;
    bool availability = false;
    bool notifyAvailability = false;
    {
        const juce::ScopedLock lock(stateLock);
        snapshot = latestSnapshot;
        availability = latestAvailability;
        notifyAvailability = availabilityDirty;
        availabilityDirty = false;
    }

    if (notifyAvailability && onAvailabilityChanged)
        onAvailabilityChanged(availability);
    if (onTouchesChanged)
        onTouchesChanged(snapshot);
}

void LinuxMultiTouchInput::run()
{
   #if JUCE_LINUX
    struct Device
    {
        int descriptor = -1;
        input_absinfo xAxis {};
        input_absinfo yAxis {};
        int slotCount = 0;
    };

    const auto hasBit = [](const auto& bits, int bit)
    {
        constexpr auto bitsPerWord = static_cast<int>(sizeof(unsigned long) * 8);
        return (bits[static_cast<size_t>(bit / bitsPerWord)]
                & (1UL << (bit % bitsPerWord))) != 0;
    };

    const auto findDevice = [&hasBit]()
    {
        Device result;
        constexpr auto bitsPerWord = static_cast<int>(sizeof(unsigned long) * 8);
        constexpr auto wordCount = (ABS_MAX + bitsPerWord) / bitsPerWord;

        for (int index = 0; index < 64; ++index)
        {
            const auto path = "/dev/input/event" + juce::String(index);
            const auto descriptor = ::open(path.toRawUTF8(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (descriptor < 0)
                continue;

            std::array<unsigned long, static_cast<size_t>(wordCount)> absBits {};
            if (::ioctl(descriptor, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits.data()) >= 0
                && hasBit(absBits, ABS_MT_SLOT)
                && hasBit(absBits, ABS_MT_TRACKING_ID)
                && hasBit(absBits, ABS_MT_POSITION_X)
                && hasBit(absBits, ABS_MT_POSITION_Y))
            {
                input_absinfo slotAxis {};
                if (::ioctl(descriptor, EVIOCGABS(ABS_MT_POSITION_X), &result.xAxis) >= 0
                    && ::ioctl(descriptor, EVIOCGABS(ABS_MT_POSITION_Y), &result.yAxis) >= 0
                    && ::ioctl(descriptor, EVIOCGABS(ABS_MT_SLOT), &slotAxis) >= 0)
                {
                    result.descriptor = descriptor;
                    result.slotCount = juce::jlimit(1, maximumTouches,
                                                    slotAxis.maximum - slotAxis.minimum + 1);
                    return result;
                }
            }

            ::close(descriptor);
        }
        return result;
    };

    while (! threadShouldExit())
    {
        auto device = findDevice();
        if (device.descriptor < 0)
        {
            publishAvailability(false);
            wait(1500);
            continue;
        }

        publishAvailability(true);
        Snapshot snapshot;
        int currentSlot = 0;

        while (! threadShouldExit())
        {
            pollfd pollDescriptor { device.descriptor, POLLIN, 0 };
            const auto pollResult = ::poll(&pollDescriptor, 1, 250);
            if (pollResult < 0 && errno != EINTR)
                break;
            if (pollResult <= 0)
                continue;
            if ((pollDescriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
                break;

            input_event events[32];
            const auto bytesRead = ::read(device.descriptor, events, sizeof(events));
            if (bytesRead <= 0)
            {
                if (errno != EAGAIN && errno != EINTR)
                    break;
                continue;
            }

            const auto eventCount = static_cast<int>(
                static_cast<size_t>(bytesRead) / sizeof(input_event));
            for (int eventIndex = 0; eventIndex < eventCount; ++eventIndex)
            {
                const auto& event = events[eventIndex];
                if (event.type == EV_ABS && event.code == ABS_MT_SLOT)
                {
                    currentSlot = juce::jlimit(0, device.slotCount - 1,
                                                event.value);
                }
                else if (event.type == EV_ABS && event.code == ABS_MT_TRACKING_ID)
                {
                    snapshot[static_cast<size_t>(currentSlot)].active = event.value >= 0;
                }
                else if (event.type == EV_ABS && event.code == ABS_MT_POSITION_X)
                {
                    const auto range = juce::jmax(1, device.xAxis.maximum - device.xAxis.minimum);
                    snapshot[static_cast<size_t>(currentSlot)].normalisedPosition.x =
                        juce::jlimit(0.0f, 1.0f,
                            static_cast<float>(event.value - device.xAxis.minimum)
                                / static_cast<float>(range));
                }
                else if (event.type == EV_ABS && event.code == ABS_MT_POSITION_Y)
                {
                    const auto range = juce::jmax(1, device.yAxis.maximum - device.yAxis.minimum);
                    snapshot[static_cast<size_t>(currentSlot)].normalisedPosition.y =
                        juce::jlimit(0.0f, 1.0f,
                            static_cast<float>(event.value - device.yAxis.minimum)
                                / static_cast<float>(range));
                }
                else if (event.type == EV_SYN && event.code == SYN_REPORT)
                {
                    publishSnapshot(snapshot);
                }
            }
        }

        ::close(device.descriptor);
        publishSnapshot({});
        publishAvailability(false);
    }
   #endif
}
