#include "MainComponent.h"

MainComponent::MainComponent()
{
    setOpaque(true);

    for (auto* component : std::array<juce::Component*, 5> {
             &playButton, &stopButton, &loopPageButton, &effectsPageButton, &statusLabel })
        addAndMakeVisible(component);

    for (auto* component : std::array<juce::Component*, 17> {
             &waveform, &loadButton, &loopButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        addAndMakeVisible(component);

    for (auto* panel : std::array<EffectPanel*, 5> {
             &distortionPanel, &flangerPanel, &chorusPanel, &delayPanel, &reverbPanel })
        addAndMakeVisible(panel);

    clipName.setText("Nessun loop caricato", juce::dontSendNotification);
    clipName.setJustificationType(juce::Justification::centredLeft);
    clipName.setFont(juce::FontOptions(22.0f, juce::Font::bold));

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff267a4a));
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa93636));
    playButton.onClick = [this] { engine.play(); };
    stopButton.onClick = [this] { engine.stop(); };
    loadButton.onClick = [this] { chooseFile(); };
    loopPageButton.onClick = [this] { showPage(Page::loop); };
    effectsPageButton.onClick = [this] { showPage(Page::effects); };

    loopButton.setToggleState(true, juce::dontSendNotification);
    loopButton.onClick = [this] { engine.setLooping(loopButton.getToggleState()); };

    envelopeCycleButton.setToggleState(true, juce::dontSendNotification);
    envelopeCycleButton.onClick = [this]
    {
        engine.setEnvelopeCycle(envelopeCycleButton.getToggleState());
        updateEnvelope();
    };

    const auto configureKnob = [](juce::Slider& slider, juce::Label& label,
                                  const juce::String& name, double minimum,
                                  double maximum, double step, double initialValue,
                                  const juce::String& suffix)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 26);
        slider.setMouseDragSensitivity(300);
        slider.setRange(minimum, maximum, step);
        slider.setValue(initialValue, juce::dontSendNotification);
        slider.setDoubleClickReturnValue(true, initialValue);
        slider.setTextValueSuffix(suffix);
    };

    configureKnob(speedSlider, speedLabel, "VELOCITA", 0.25, 1.5, 0.01, 1.0, " x");
    configureKnob(gainSlider, gainLabel, "VOLUME", 0.0, 1.0, 0.01, 0.8, "");
    configureKnob(attackSlider, attackLabel, "ATTACK", 0.0, 5.0, 0.01, 0.02, " s");
    configureKnob(decaySlider, decayLabel, "DECAY", 0.0, 5.0, 0.01, 0.1, " s");
    configureKnob(sustainSlider, sustainLabel, "SUSTAIN", 0.0, 1.0, 0.01, 1.0, "");
    configureKnob(releaseSlider, releaseLabel, "RELEASE", 0.0, 10.0, 0.01, 0.3, " s");

    speedSlider.onValueChange = [this]
    {
        engine.setPlaybackRate(speedSlider.getValue());
        updateEnvelope();
    };
    gainSlider.onValueChange = [this]
    {
        engine.setGain(static_cast<float>(gainSlider.getValue()));
    };
    for (auto* slider : std::array<juce::Slider*, 4> {
             &attackSlider, &decaySlider, &sustainSlider, &releaseSlider })
        slider->onValueChange = [this] { updateEnvelope(); };

    waveform.onTrimChanged = [this](double start, double end)
    {
        engine.setTrimRange(start, end);
    };
    updateEnvelope();

    distortionPanel.configure("DISTORSIONE", {
        { "DRIVE", 1.0, 20.0, 0.1, 2.0, "" },
        { "TONE", 800.0, 18000.0, 10.0, 12000.0, " Hz" },
        { "MIX", 0.0, 1.0, 0.01, 0.5, "" }
    });
    flangerPanel.configure("FLANGER", {
        { "RATE", 0.05, 5.0, 0.01, 0.25, " Hz" },
        { "DEPTH", 0.0, 1.0, 0.01, 0.5, "" },
        { "FEEDBACK", -0.9, 0.9, 0.01, 0.2, "" },
        { "MIX", 0.0, 1.0, 0.01, 0.35, "" }
    });
    chorusPanel.configure("CHORUS", {
        { "RATE", 0.05, 5.0, 0.01, 0.8, " Hz" },
        { "DEPTH", 0.0, 1.0, 0.01, 0.35, "" },
        { "MIX", 0.0, 1.0, 0.01, 0.35, "" }
    });
    delayPanel.configure("DELAY", {
        { "TIME", 1.0, 1500.0, 1.0, 350.0, " ms" },
        { "FEEDBACK", 0.0, 0.95, 0.01, 0.35, "" },
        { "MIX", 0.0, 1.0, 0.01, 0.3, "" }
    });
    reverbPanel.configure("RIVERBERO", {
        { "SIZE", 0.0, 1.0, 0.01, 0.5, "" },
        { "DAMPING", 0.0, 1.0, 0.01, 0.5, "" },
        { "MIX", 0.0, 1.0, 0.01, 0.25, "" }
    });

    for (auto* panel : std::array<EffectPanel*, 5> {
             &distortionPanel, &flangerPanel, &chorusPanel, &delayPanel, &reverbPanel })
        panel->onChange = [this] { updateEffects(); };
    updateEffects();

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
    showPage(Page::loop);
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
    graphics.drawText(currentPage == Page::loop ? "MELA - LOOP EDITOR" : "MELA - EFFETTI",
                      24, 12, getWidth() - 430, 42, juce::Justification::centredLeft);

    graphics.setColour(juce::Colour(0xff20252c));
    graphics.fillRoundedRectangle(getLocalBounds().reduced(24).withTrimmedTop(60)
                                      .withTrimmedBottom(82).toFloat(), 14.0f);
}

void MainComponent::resized()
{
    auto outer = getLocalBounds().reduced(24);
    auto header = outer.removeFromTop(60);
    effectsPageButton.setBounds(header.removeFromRight(150).reduced(4));
    loopPageButton.setBounds(header.removeFromRight(150).reduced(4));

    auto footer = outer.removeFromBottom(76);
    playButton.setBounds(footer.removeFromLeft(190).reduced(5));
    stopButton.setBounds(footer.removeFromLeft(140).reduced(5));
    statusLabel.setBounds(footer.reduced(12, 5));

    auto content = outer.reduced(18, 12);
    if (currentPage == Page::loop)
    {
        auto heading = content.removeFromTop(52);
        loadButton.setBounds(heading.removeFromRight(200).reduced(4));
        envelopeCycleButton.setBounds(heading.removeFromRight(180).reduced(6, 4));
        loopButton.setBounds(heading.removeFromRight(145).reduced(6, 4));
        clipName.setBounds(heading.reduced(4));
        content.removeFromTop(8);
        waveform.setBounds(content.removeFromTop(300));
        content.removeFromTop(8);

        std::array<juce::Slider*, 6> sliders {
            &speedSlider, &gainSlider, &attackSlider, &decaySlider, &sustainSlider, &releaseSlider
        };
        std::array<juce::Label*, 6> labels {
            &speedLabel, &gainLabel, &attackLabel, &decayLabel, &sustainLabel, &releaseLabel
        };
        const auto controlWidth = content.getWidth() / 6;
        for (int index = 0; index < 6; ++index)
        {
            auto control = content.withTrimmedLeft(index * controlWidth)
                                  .withWidth(controlWidth).reduced(6, 0);
            labels[static_cast<size_t>(index)]->setBounds(control.removeFromTop(25));
            sliders[static_cast<size_t>(index)]->setBounds(control);
        }
    }
    else
    {
        auto topRow = content.removeFromTop((content.getHeight() - 10) / 2);
        content.removeFromTop(10);
        const auto topWidth = topRow.getWidth() / 3;
        distortionPanel.setBounds(topRow.removeFromLeft(topWidth).reduced(5));
        flangerPanel.setBounds(topRow.removeFromLeft(topWidth).reduced(5));
        chorusPanel.setBounds(topRow.reduced(5));

        const auto bottomWidth = content.getWidth() / 2;
        delayPanel.setBounds(content.removeFromLeft(bottomWidth).reduced(5));
        reverbPanel.setBounds(content.reduced(5));
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
    engine.setEnvelope(attackSlider.getValue(), decaySlider.getValue(),
                       static_cast<float>(sustainSlider.getValue()), releaseSlider.getValue());
    waveform.setEnvelope(attackSlider.getValue(), decaySlider.getValue(),
                         static_cast<float>(sustainSlider.getValue()), releaseSlider.getValue(),
                         speedSlider.getValue(), envelopeCycleButton.getToggleState());
}

void MainComponent::updateEffects()
{
    engine.setDistortion(distortionPanel.isEnabled(),
                         static_cast<float>(distortionPanel.value(0)),
                         static_cast<float>(distortionPanel.value(1)),
                         static_cast<float>(distortionPanel.value(2)));
    engine.setFlanger(flangerPanel.isEnabled(),
                      static_cast<float>(flangerPanel.value(0)),
                      static_cast<float>(flangerPanel.value(1)),
                      static_cast<float>(flangerPanel.value(2)),
                      static_cast<float>(flangerPanel.value(3)));
    engine.setChorus(chorusPanel.isEnabled(),
                     static_cast<float>(chorusPanel.value(0)),
                     static_cast<float>(chorusPanel.value(1)),
                     static_cast<float>(chorusPanel.value(2)));
    engine.setDelay(delayPanel.isEnabled(),
                    static_cast<float>(delayPanel.value(0)),
                    static_cast<float>(delayPanel.value(1)),
                    static_cast<float>(delayPanel.value(2)));
    engine.setReverb(reverbPanel.isEnabled(),
                     static_cast<float>(reverbPanel.value(0)),
                     static_cast<float>(reverbPanel.value(1)),
                     static_cast<float>(reverbPanel.value(2)));
}

void MainComponent::showPage(Page pageToShow)
{
    currentPage = pageToShow;
    const auto showLoop = currentPage == Page::loop;

    for (auto* component : std::array<juce::Component*, 17> {
             &waveform, &loadButton, &loopButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        component->setVisible(showLoop);

    for (auto* panel : std::array<EffectPanel*, 5> {
             &distortionPanel, &flangerPanel, &chorusPanel, &delayPanel, &reverbPanel })
        panel->setVisible(! showLoop);

    loopPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    effectsPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? juce::Colour(0xff30343b) : juce::Colour(0xff3b6f91));
    resized();
    repaint();
}
