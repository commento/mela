#include "MainComponent.h"

MainComponent::MainComponent()
{
    setOpaque(true);

    for (auto* component : std::array<juce::Component*, 20> {
             &waveform, &loadButton, &playButton, &stopButton, &loopButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &speedLabel, &gainLabel,
             &attackSlider, &decaySlider, &sustainSlider, &releaseSlider,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel,
             &clipName, &statusLabel })
        addAndMakeVisible(component);

    clipName.setText("Nessun loop caricato", juce::dontSendNotification);
    clipName.setJustificationType(juce::Justification::centredLeft);
    clipName.setFont(juce::FontOptions(22.0f, juce::Font::bold));

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff267a4a));
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa93636));
    playButton.onClick = [this] { engine.play(); };
    stopButton.onClick = [this] { engine.stop(); };
    loadButton.onClick = [this] { chooseFile(); };

    loopButton.setToggleState(true, juce::dontSendNotification);
    loopButton.onClick = [this] { engine.setLooping(loopButton.getToggleState()); };

    envelopeCycleButton.setToggleState(true, juce::dontSendNotification);
    envelopeCycleButton.onClick = [this]
    {
        engine.setEnvelopeCycle(envelopeCycleButton.getToggleState());
        updateEnvelope();
    };

    speedLabel.setText("VELOCITA", juce::dontSendNotification);
    speedLabel.setJustificationType(juce::Justification::centred);
    speedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    speedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 28);
    speedSlider.setMouseDragSensitivity(300);
    speedSlider.setRange(0.25, 1.5, 0.01);
    speedSlider.setValue(1.0, juce::dontSendNotification);
    speedSlider.setDoubleClickReturnValue(true, 1.0);
    speedSlider.setTextValueSuffix(" x");
    speedSlider.onValueChange = [this]
    {
        engine.setPlaybackRate(speedSlider.getValue());
        updateEnvelope();
    };

    gainLabel.setText("VOLUME", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 28);
    gainSlider.setMouseDragSensitivity(300);
    gainSlider.setRange(0.0, 1.0, 0.01);
    gainSlider.setValue(0.8, juce::dontSendNotification);
    gainSlider.setDoubleClickReturnValue(true, 0.8);
    gainSlider.onValueChange = [this]
    {
        engine.setGain(static_cast<float>(gainSlider.getValue()));
    };

    const auto configureEnvelopeSlider = [this](juce::Slider& slider,
                                                 juce::Label& label,
                                                 const juce::String& name,
                                                 double maximum,
                                                 double initialValue,
                                                 const juce::String& suffix)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 25);
        slider.setMouseDragSensitivity(300);
        slider.setRange(0.0, maximum, 0.01);
        slider.setValue(initialValue, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, initialValue);
        slider.setTextValueSuffix(suffix);
        slider.onValueChange = [this] { updateEnvelope(); };
    };

    configureEnvelopeSlider(attackSlider, attackLabel, "ATTACK", 5.0, 0.02, " s");
    configureEnvelopeSlider(decaySlider, decayLabel, "DECAY", 5.0, 0.1, " s");
    configureEnvelopeSlider(sustainSlider, sustainLabel, "SUSTAIN", 1.0, 1.0, "");
    configureEnvelopeSlider(releaseSlider, releaseLabel, "RELEASE", 10.0, 0.3, " s");
    updateEnvelope();

    waveform.onTrimChanged = [this](double start, double end)
    {
        engine.setTrimRange(start, end);
    };

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Inizializzazione audio...", juce::dontSendNotification);

    const auto error = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (error.isEmpty())
    {
        deviceManager.addAudioCallback(&engine);
        statusLabel.setText("Audio pronto", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Errore audio: " + error, juce::dontSendNotification);
    }

    setSize(1280, 800);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff111419));
    graphics.setColour(juce::Colours::white);
    graphics.setFont(28.0f);
    graphics.drawText("MELA - LOOP EDITOR", 24, 12, getWidth() - 48, 42,
                      juce::Justification::centredLeft);

    graphics.setColour(juce::Colour(0xff282d35));
    graphics.fillRoundedRectangle(getLocalBounds().reduced(24).withTrimmedTop(64)
                                      .withTrimmedBottom(78).toFloat(), 14.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(64);
    auto footer = area.removeFromBottom(70);
    statusLabel.setBounds(footer.reduced(6));

    area.reduce(18, 16);
    auto heading = area.removeFromTop(54);
    loadButton.setBounds(heading.removeFromRight(210).reduced(4));
    clipName.setBounds(heading.reduced(4));
    area.removeFromTop(10);

    waveform.setBounds(area.removeFromTop(280));
    area.removeFromTop(12);

    auto transport = area.removeFromTop(110);
    playButton.setBounds(transport.removeFromLeft(190).reduced(5));
    stopButton.setBounds(transport.removeFromLeft(135).reduced(5));
    loopButton.setBounds(transport.removeFromLeft(150).reduced(8, 5));
    envelopeCycleButton.setBounds(transport.removeFromLeft(190).reduced(8, 5));

    auto gainArea = transport.removeFromRight(215).reduced(8, 0);
    gainLabel.setBounds(gainArea.removeFromTop(28));
    gainSlider.setBounds(gainArea);

    auto speedArea = transport.reduced(8, 0);
    speedLabel.setBounds(speedArea.removeFromTop(28));
    speedSlider.setBounds(speedArea);

    area.removeFromTop(8);
    const auto envelopeWidth = area.getWidth() / 4;
    std::array<juce::Slider*, 4> envelopeSliders {
        &attackSlider, &decaySlider, &sustainSlider, &releaseSlider
    };
    std::array<juce::Label*, 4> envelopeLabels {
        &attackLabel, &decayLabel, &sustainLabel, &releaseLabel
    };

    for (int index = 0; index < 4; ++index)
    {
        auto envelopeArea = area.withTrimmedLeft(index * envelopeWidth)
                                .withWidth(envelopeWidth).reduced(7, 0);
        envelopeLabels[static_cast<size_t>(index)]->setBounds(envelopeArea.removeFromTop(25));
        envelopeSliders[static_cast<size_t>(index)]->setBounds(envelopeArea);
    }
}

void MainComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Scegli un loop",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file == juce::File {})
            return;

        juce::String error;
        if (engine.loadFile(file, error))
        {
            waveform.setClip(engine.getClipForDisplay());
            clipName.setText(engine.getClipName(), juce::dontSendNotification);
            statusLabel.setText("Caricato: " + file.getFileName(), juce::dontSendNotification);
        }
        else
        {
            statusLabel.setText("Errore: " + error, juce::dontSendNotification);
        }
    });
}

void MainComponent::timerCallback()
{
    playButton.setButtonText(engine.isPlaying() ? "IN PLAY..." : "PLAY");
    waveform.setPlayhead(engine.getPlayheadNormalised());
}

void MainComponent::updateEnvelope()
{
    engine.setEnvelope(attackSlider.getValue(),
                       decaySlider.getValue(),
                       static_cast<float>(sustainSlider.getValue()),
                       releaseSlider.getValue());
    waveform.setEnvelope(attackSlider.getValue(),
                         decaySlider.getValue(),
                         static_cast<float>(sustainSlider.getValue()),
                         releaseSlider.getValue(),
                         speedSlider.getValue(),
                         envelopeCycleButton.getToggleState());
}
