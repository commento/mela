#include "Audio/EffectsChain.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (! condition)
        throw std::runtime_error(message);
}

void warmUp(EffectsChain& chain, int samples = 2048)
{
    juce::AudioBuffer<float> buffer(2, samples);
    buffer.clear();
    chain.processInserts(buffer);
}

void checkDry(EffectsChain& chain)
{
    juce::AudioBuffer<float> buffer(2, 127);
    for (int channel = 0; channel < 2; ++channel)
        for (int frame = 0; frame < buffer.getNumSamples(); ++frame)
            buffer.setSample(channel, frame,
                (channel == 0 ? 1.0f : -1.0f) * static_cast<float>(frame) / 127.0f);
    juce::AudioBuffer<float> original;
    original.makeCopyOf(buffer);
    chain.processInserts(buffer);
    for (int channel = 0; channel < 2; ++channel)
        require(std::memcmp(buffer.getReadPointer(channel), original.getReadPointer(channel),
                            static_cast<size_t>(buffer.getNumSamples()) * sizeof(float)) == 0,
                "Bypass or zero mix changed the input");
}

void testBypassAndUnity(double sampleRate)
{
    EffectsChain chain;
    chain.prepare(sampleRate, 2048, 2);
    checkDry(chain);
    chain.setDownsampler(true, 32.0f, 0.0f);
    chain.setBitcrusher(true, 2, 0.0f);
    checkDry(chain);
    chain.setDownsampler(true, 1.0f, 1.0f);
    warmUp(chain);
    checkDry(chain);
    chain.setDownsampler(true, 64.0f, 1.0f);
    chain.setBitcrusher(true, 2, 1.0f);
    warmUp(chain);
    chain.setDownsampler(false, 64.0f, 1.0f);
    chain.setBitcrusher(false, 2, 1.0f);
    warmUp(chain);
    checkDry(chain);
}

void testQuantisation()
{
    EffectsChain chain;
    chain.prepare(48000.0, 2048, 2);
    chain.setBitcrusher(true, 2, 1.0f);
    warmUp(chain);
    const std::array<float, 9> input { -2.0f, -1.0f, -0.6f, -0.1f, 0.0f,
                                       0.1f, 0.4f, 1.0f, 2.0f };
    const std::array<float, 9> expected { -1.0f, -1.0f, -0.5f, 0.0f, 0.0f,
                                          0.0f, 0.5f, 0.5f, 0.5f };
    juce::AudioBuffer<float> buffer(1, static_cast<int>(input.size()));
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        buffer.setSample(0, i, input[static_cast<size_t>(i)]);
    chain.processInserts(buffer);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        require(std::abs(buffer.getSample(0, i) - expected[static_cast<size_t>(i)]) < 1.0e-6f,
                "Bitcrusher did not produce the expected signed 2-bit levels");

    chain.setBitcrusher(true, 16, 0.5f);
    warmUp(chain);
    buffer.setSample(0, 0, 0.12345f);
    chain.processInserts(buffer);
    require(std::abs(buffer.getSample(0, 0) - 0.12345f) <= 1.0f / 65536.0f,
            "16-bit quantisation or partial mix is incorrect");
}

std::vector<float> renderHeld(float factor, int blockSize)
{
    EffectsChain chain;
    chain.setDownsampler(true, factor, 1.0f);
    chain.prepare(48000.0, 2048, 2);
    warmUp(chain);
    std::vector<float> result;
    for (int offset = 0; offset < 512; offset += blockSize)
    {
        juce::AudioBuffer<float> buffer(2, juce::jmin(blockSize, 512 - offset));
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto value = static_cast<float>(offset + i) / 512.0f;
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, -value);
        }
        chain.processInserts(buffer);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            result.push_back(buffer.getSample(0, i));
            require(std::abs(buffer.getSample(1, i) + buffer.getSample(0, i)) < 1.0e-7f,
                    "Downsampler mixed channels or changed stereo timing");
        }
    }
    return result;
}

void testSampleHold()
{
    const auto held = renderHeld(4.0f, 512);
    for (int i = 0; i < 512; ++i)
        require(std::abs(held[static_cast<size_t>(i)]
                         - static_cast<float>((i / 4) * 4) / 512.0f) < 1.0e-7f,
                "Downsampler did not hold samples for the requested interval");
    require(held == renderHeld(4.0f, 17), "Sample hold restarted at a block boundary");
    require(renderHeld(3.7f, 512) == renderHeld(3.7f, 17),
            "Fractional downsampling depends on block size");

    EffectsChain chain;
    chain.setDownsampler(true, 64.0f, 1.0f);
    chain.prepare(48000.0, 2048, 2);
    warmUp(chain);
    juce::AudioBuffer<float> buffer(2, 1);
    buffer.setSample(0, 0, 0.8f);
    buffer.setSample(1, 0, -0.8f);
    chain.processInserts(buffer);
    chain.reset();
    buffer.clear();
    chain.processInserts(buffer);
    require(buffer.getSample(0, 0) == 0.0f && buffer.getSample(1, 0) == 0.0f,
            "Reset leaked held audio");
}
}

int main()
{
    try
    {
        for (auto rate : { 44100.0, 48000.0, 96000.0 })
            testBypassAndUnity(rate);
        testQuantisation();
        testSampleHold();
        std::cout << "Effects tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
