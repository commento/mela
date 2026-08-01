#include "MainComponent.h"

MainComponent::MainComponent()
{
    setOpaque(true);

    for (auto* component : std::array<juce::Component*, 6> {
             &playButton, &stopButton, &stopAllButton,
             &loopPageButton, &effectsPageButton, &statusLabel })
        addAndMakeVisible(component);

    for (auto* component : std::array<juce::Component*, 18> {
             &waveform, &loadButton, &loopButton, &reverseButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        addAndMakeVisible(component);

    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        auto& button = sampleButtons[static_cast<size_t>(slot)];
        button.setButtonText("SAMPLE " + juce::String(slot + 1));
        button.onClick = [this, slot] { selectSlot(slot); };
        addAndMakeVisible(button);
    }

    for (auto* panel : std::array<EffectPanel*, 6> {
             &distortionPanel, &granularPanel, &flangerPanel,
             &chorusPanel, &delayPanel, &reverbPanel })
        addAndMakeVisible(panel);

    clipName.setJustificationType(juce::Justification::centredLeft);
    clipName.setFont(juce::FontOptions(20.0f, juce::Font::bold));

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff267a4a));
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa96736));
    stopAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa93636));
    playButton.onClick = [this] { engine.play(activeSlot); };
    stopButton.onClick = [this] { engine.stop(activeSlot); };
    stopAllButton.onClick = [this] { engine.stopAll(); };
    loadButton.onClick = [this] { chooseFile(); };
    loopPageButton.onClick = [this] { showPage(Page::loop); };
    effectsPageButton.onClick = [this] { showPage(Page::effects); };

    loopButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.looping = loopButton.getToggleState();
        engine.setLooping(activeSlot, settings.looping);
    };
    reverseButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.reverse = reverseButton.getToggleState();
        engine.setReverse(activeSlot, settings.reverse);
        updateEnvelope();
    };
    envelopeCycleButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.envelopeCycle = envelopeCycleButton.getToggleState();
        engine.setEnvelopeCycle(activeSlot, settings.envelopeCycle);
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
    configureKnob(releaseSlider, releaseLabel, "RELEASE", 0.0, 10.0, 0.01, 0.0, " s");

    speedSlider.onValueChange = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.speed = speedSlider.getValue();
        engine.setPlaybackRate(activeSlot, settings.speed);
        updateEnvelope();
    };
    gainSlider.onValueChange = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.gain = gainSlider.getValue();
        engine.setGain(activeSlot, static_cast<float>(settings.gain));
    };
    for (auto* slider : std::array<juce::Slider*, 4> {
             &attackSlider, &decaySlider, &sustainSlider, &releaseSlider })
        slider->onValueChange = [this] { updateEnvelope(); };

    waveform.onTrimChanged = [this](double start, double end)
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.trimStart = start;
        settings.trimEnd = end;
        engine.setTrimRange(activeSlot, start, end);
    };

    distortionPanel.configure("DISTORSIONE", {
        { "DRIVE", 1.0, 20.0, 0.1, 2.0, "" },
        { "TONE", 800.0, 18000.0, 10.0, 12000.0, " Hz" },
        { "MIX", 0.0, 1.0, 0.01, 0.5, "" }
    });
    granularPanel.configure("GRANULARE", {
        { "SIZE", 10.0, 250.0, 1.0, 80.0, " ms" },
        { "DENSITY", 1.0, 40.0, 0.1, 12.0, " Hz" },
        { "POSITION", 0.0, 2000.0, 1.0, 250.0, " ms" },
        { "PITCH", -12.0, 12.0, 1.0, 0.0, " st" },
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
    for (auto* panel : std::array<EffectPanel*, 6> {
             &distortionPanel, &granularPanel, &flangerPanel,
             &chorusPanel, &delayPanel, &reverbPanel })
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
    selectSlot(0);
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
    graphics.drawText(currentPage == Page::loop ? "MELA - 4 LOOP EDITOR" : "MELA - EFFETTI",
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
    playButton.setBounds(footer.removeFromLeft(165).reduced(5));
    stopButton.setBounds(footer.removeFromLeft(145).reduced(5));
    stopAllButton.setBounds(footer.removeFromLeft(145).reduced(5));
    statusLabel.setBounds(footer.reduced(12, 5));

    auto content = outer.reduced(18, 12);
    if (currentPage == Page::loop)
    {
        auto selectorRow = content.removeFromTop(44);
        const auto selectorWidth = selectorRow.getWidth() / LoopEngine::numberOfSlots;
        for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
            sampleButtons[static_cast<size_t>(slot)].setBounds(
                selectorRow.removeFromLeft(selectorWidth).reduced(5, 2));

        content.removeFromTop(5);
        auto heading = content.removeFromTop(48);
        loadButton.setBounds(heading.removeFromRight(180).reduced(4));
        envelopeCycleButton.setBounds(heading.removeFromRight(170).reduced(5, 3));
        reverseButton.setBounds(heading.removeFromRight(130).reduced(5, 3));
        loopButton.setBounds(heading.removeFromRight(120).reduced(5, 3));
        clipName.setBounds(heading.reduced(4));
        content.removeFromTop(5);
        waveform.setBounds(content.removeFromTop(255));
        content.removeFromTop(7);

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
            labels[static_cast<size_t>(index)]->setBounds(control.removeFromTop(24));
            sliders[static_cast<size_t>(index)]->setBounds(control);
        }
    }
    else
    {
        auto topRow = content.removeFromTop((content.getHeight() - 10) / 2);
        content.removeFromTop(10);
        const auto topWidth = topRow.getWidth() / 3;
        distortionPanel.setBounds(topRow.removeFromLeft(topWidth).reduced(5));
        granularPanel.setBounds(topRow.removeFromLeft(topWidth).reduced(5));
        flangerPanel.setBounds(topRow.reduced(5));
        const auto bottomWidth = content.getWidth() / 3;
        chorusPanel.setBounds(content.removeFromLeft(bottomWidth).reduced(5));
        delayPanel.setBounds(content.removeFromLeft(bottomWidth).reduced(5));
        reverbPanel.setBounds(content.reduced(5));
    }
}

void MainComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Scegli un loop", juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file == juce::File {})
            return;

        juce::String error;
        if (engine.loadFile(activeSlot, file, error))
        {
            auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
            settings.trimStart = 0.0;
            settings.trimEnd = 1.0;
            waveform.setClip(engine.getClipForDisplay(activeSlot));
            clipName.setText(engine.getClipName(activeSlot), juce::dontSendNotification);
            statusLabel.setText("Sample " + juce::String(activeSlot + 1)
                                + ": " + file.getFileName(), juce::dontSendNotification);
            updateSlotButtonColours();
        }
        else
        {
            statusLabel.setText("Errore: " + error, juce::dontSendNotification);
        }
    });
}

void MainComponent::timerCallback()
{
    playButton.setButtonText(engine.isPlaying(activeSlot) ? "IN PLAY..." : "PLAY SLOT");
    waveform.setPlayhead(engine.getPlayheadNormalised(activeSlot));
    updateSlotButtonColours();
}

void MainComponent::updateEnvelope()
{
    auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
    settings.attack = attackSlider.getValue();
    settings.decay = decaySlider.getValue();
    settings.sustain = sustainSlider.getValue();
    settings.release = releaseSlider.getValue();
    engine.setEnvelope(activeSlot, settings.attack, settings.decay,
                       static_cast<float>(settings.sustain), settings.release);
    waveform.setEnvelope(settings.attack, settings.decay,
                         static_cast<float>(settings.sustain), settings.release,
                         settings.speed, settings.envelopeCycle, settings.reverse);
}

void MainComponent::updateEffects()
{
    engine.setDistortion(distortionPanel.isEnabled(),
                         static_cast<float>(distortionPanel.value(0)),
                         static_cast<float>(distortionPanel.value(1)),
                         static_cast<float>(distortionPanel.value(2)));
    engine.setGranular(granularPanel.isEnabled(),
                       static_cast<float>(granularPanel.value(0)),
                       static_cast<float>(granularPanel.value(1)),
                       static_cast<float>(granularPanel.value(2)),
                       static_cast<float>(granularPanel.value(3)),
                       static_cast<float>(granularPanel.value(4)));
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

void MainComponent::selectSlot(int slotIndex)
{
    if (! juce::isPositiveAndBelow(slotIndex, LoopEngine::numberOfSlots))
        return;

    activeSlot = slotIndex;
    const auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
    loopButton.setToggleState(settings.looping, juce::dontSendNotification);
    reverseButton.setToggleState(settings.reverse, juce::dontSendNotification);
    envelopeCycleButton.setToggleState(settings.envelopeCycle, juce::dontSendNotification);
    speedSlider.setValue(settings.speed, juce::dontSendNotification);
    gainSlider.setValue(settings.gain, juce::dontSendNotification);
    attackSlider.setValue(settings.attack, juce::dontSendNotification);
    decaySlider.setValue(settings.decay, juce::dontSendNotification);
    sustainSlider.setValue(settings.sustain, juce::dontSendNotification);
    releaseSlider.setValue(settings.release, juce::dontSendNotification);
    waveform.setClip(engine.getClipForDisplay(activeSlot));
    waveform.setTrimRange(settings.trimStart, settings.trimEnd);
    clipName.setText(engine.hasClip(activeSlot) ? engine.getClipName(activeSlot)
                                               : "Nessun loop caricato",
                     juce::dontSendNotification);
    updateEnvelope();
    updateSlotButtonColours();
}

void MainComponent::showPage(Page pageToShow)
{
    currentPage = pageToShow;
    const auto showLoop = currentPage == Page::loop;
    for (auto* component : std::array<juce::Component*, 18> {
             &waveform, &loadButton, &loopButton, &reverseButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        component->setVisible(showLoop);
    for (auto& button : sampleButtons)
        button.setVisible(showLoop);
    for (auto* panel : std::array<EffectPanel*, 6> {
             &distortionPanel, &granularPanel, &flangerPanel,
             &chorusPanel, &delayPanel, &reverbPanel })
        panel->setVisible(! showLoop);

    loopPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    effectsPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? juce::Colour(0xff30343b) : juce::Colour(0xff3b6f91));
    resized();
    repaint();
}

void MainComponent::updateSlotButtonColours()
{
    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        auto colour = engine.hasClip(slot) ? juce::Colour(0xff365445)
                                           : juce::Colour(0xff30343b);
        if (engine.isPlaying(slot))
            colour = juce::Colour(0xff267a4a);
        if (slot == activeSlot)
            colour = colour.brighter(0.35f);
        sampleButtons[static_cast<size_t>(slot)].setColour(
            juce::TextButton::buttonColourId, colour);
    }
}
