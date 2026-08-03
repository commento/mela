#include "MainComponent.h"

#include <algorithm>

MainComponent::MainComponent()
{
    setOpaque(true);

    for (auto* component : std::array<juce::Component*, 9> {
             &playButton, &stopButton, &stopAllButton,
             &audioPageButton, &wifiPageButton, &loopPageButton,
             &keysPageButton, &effectsPageButton,
             &statusLabel })
        addAndMakeVisible(component);

    for (auto* component : std::array<juce::Component*, 19> {
             &waveform, &loadButton, &recordButton,
             &loopButton, &reverseButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        addAndMakeVisible(component);

    addAndMakeVisible(audioInfoLabel);
    addAndMakeVisible(continueButton);
    audioInfoLabel.setText(
        "Scegli l'ingresso del microfono e l'uscita audio. Il microfono non viene "
        "rimandato alle casse durante la registrazione.",
        juce::dontSendNotification);
    audioInfoLabel.setJustificationType(juce::Justification::centred);
    audioInfoLabel.setFont(juce::FontOptions(17.0f));

    for (auto* component : std::array<juce::Component*, 6> {
             &wifiInfoLabel, &wifiAddressLabel, &wifiPinLabel,
             &wifiInboxLabel, &wifiFileBox, &wifiRefreshButton })
        addAndMakeVisible(component);
    wifiInboxDirectory = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                             .getChildFile("Mela Inbox");
    wifiInboxDirectory.createDirectory();
    wifiInfoLabel.setText(
        "Dal telefono o dal Mac apri l'indirizzo qui sotto, inserisci il PIN e carica "
        "un sample. Il file apparira' in questa libreria.",
        juce::dontSendNotification);
    wifiInfoLabel.setJustificationType(juce::Justification::centred);
    wifiInfoLabel.setFont(juce::FontOptions(17.0f));
    wifiAddressLabel.setJustificationType(juce::Justification::centred);
    wifiPinLabel.setJustificationType(juce::Justification::centred);
    wifiPinLabel.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    wifiInboxLabel.setJustificationType(juce::Justification::centredLeft);
    wifiFileBox.setTextWhenNothingSelected("Nessun sample nella Inbox");
    wifiFileBox.setTextWhenNoChoicesAvailable("Nessun sample nella Inbox");
    wifiRefreshButton.onClick = [this] { refreshWifiLibrary(true); };

    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        auto& button = wifiLoadButtons[static_cast<size_t>(slot)];
        button.setButtonText("CARICA IN S" + juce::String(slot + 1));
        button.onClick = [this, slot] { loadWifiSampleIntoSlot(slot); };
        addAndMakeVisible(button);
    }

    for (auto* component : std::array<juce::Component*, 16> {
             &touchKeyboard, &octaveDownButton, &octaveUpButton,
             &rootDownButton, &rootUpButton, &rootNoteLabel,
             &instrumentModeLabel, &instrumentModeBox,
             &keyAttackSlider, &keyDecaySlider, &keySustainSlider, &keyReleaseSlider,
             &keyAttackLabel, &keyDecayLabel, &keySustainLabel, &keyReleaseLabel })
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

    for (int target = 0; target <= LoopEngine::numberOfSlots; ++target)
    {
        auto& button = effectTargetButtons[static_cast<size_t>(target)];
        button.setButtonText(target < LoopEngine::numberOfSlots
                                 ? "S" + juce::String(target + 1) : "MASTER");
        button.onClick = [this, target] { selectEffectTarget(target); };
        addAndMakeVisible(button);
    }
    for (auto* component : std::array<juce::Component*, 4> {
             &delaySendSlider, &reverbSendSlider, &delaySendLabel, &reverbSendLabel })
        addAndMakeVisible(component);
    addAndMakeVisible(dspLoadLabel);
    dspLoadLabel.setJustificationType(juce::Justification::centredRight);
    dspLoadLabel.setText("DSP 0% | XRUN 0 | ECO 32G", juce::dontSendNotification);

    clipName.setJustificationType(juce::Justification::centredLeft);
    clipName.setFont(juce::FontOptions(20.0f, juce::Font::bold));

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff267a4a));
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa96736));
    stopAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa93636));
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffa93636));
    playButton.onClick = [this] { engine.play(activeSlot); };
    stopButton.onClick = [this]
    {
        engine.stop(activeSlot);
        engine.allNotesOff(activeSlot);
        if (currentPage == Page::keys)
            touchKeyboard.releaseAll();
    };
    stopAllButton.onClick = [this]
    {
        touchKeyboard.releaseAll();
        engine.stopAll();
    };
    loadButton.onClick = [this] { chooseFile(); };
    recordButton.onClick = [this] { toggleRecording(); };
    audioPageButton.onClick = [this] { showPage(Page::audio); };
    wifiPageButton.onClick = [this] { showPage(Page::wifi); };
    loopPageButton.onClick = [this] { showPage(Page::loop); };
    keysPageButton.onClick = [this] { showPage(Page::keys); };
    effectsPageButton.onClick = [this] { showPage(Page::effects); };
    continueButton.onClick = [this] { showPage(Page::loop); };

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
    configureKnob(keyAttackSlider, keyAttackLabel, "KEY ATTACK",
                  0.0, 5.0, 0.01, 0.01, " s");
    configureKnob(keyDecaySlider, keyDecayLabel, "KEY DECAY",
                  0.0, 5.0, 0.01, 0.1, " s");
    configureKnob(keySustainSlider, keySustainLabel, "KEY SUSTAIN",
                  0.0, 1.0, 0.01, 1.0, "");
    configureKnob(keyReleaseSlider, keyReleaseLabel, "KEY RELEASE",
                  0.0, 10.0, 0.01, 0.15, " s");

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
    for (auto* slider : std::array<juce::Slider*, 4> {
             &keyAttackSlider, &keyDecaySlider, &keySustainSlider, &keyReleaseSlider })
        slider->onValueChange = [this] { updateKeyboardEnvelope(); };

    waveform.onTrimChanged = [this](double start, double end)
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.trimStart = start;
        settings.trimEnd = end;
        engine.setTrimRange(activeSlot, start, end);
    };

    touchKeyboard.onNoteOn = [this](int midiNote, float velocity)
    {
        engine.noteOn(activeSlot, midiNote, velocity);
    };
    touchKeyboard.onNoteOff = [this](int midiNote)
    {
        engine.noteOff(activeSlot, midiNote);
    };
    octaveDownButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.keyboardBaseNote = juce::jlimit(0, 104, settings.keyboardBaseNote - 12);
        touchKeyboard.setBaseMidiNote(settings.keyboardBaseNote);
    };
    octaveUpButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.keyboardBaseNote = juce::jlimit(0, 104, settings.keyboardBaseNote + 12);
        touchKeyboard.setBaseMidiNote(settings.keyboardBaseNote);
    };
    rootDownButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.instrumentRootNote = juce::jlimit(0, 127, settings.instrumentRootNote - 1);
        updateInstrumentControls();
    };
    rootUpButton.onClick = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        settings.instrumentRootNote = juce::jlimit(0, 127, settings.instrumentRootNote + 1);
        updateInstrumentControls();
    };
    instrumentModeLabel.setText("MODE", juce::dontSendNotification);
    instrumentModeLabel.setJustificationType(juce::Justification::centredRight);
    instrumentModeBox.addItem("GATE", 1);
    instrumentModeBox.addItem("ONE SHOT", 2);
    instrumentModeBox.addItem("LOOP", 3);
    instrumentModeBox.onChange = [this]
    {
        auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
        if (instrumentModeBox.getSelectedId() == 2)
            settings.instrumentMode = LoopEngine::InstrumentMode::oneShot;
        else if (instrumentModeBox.getSelectedId() == 3)
            settings.instrumentMode = LoopEngine::InstrumentMode::loop;
        else
            settings.instrumentMode = LoopEngine::InstrumentMode::gate;
        engine.setInstrumentMode(activeSlot, settings.instrumentMode);
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
    configureKnob(delaySendSlider, delaySendLabel, "DELAY SEND",
                  0.0, 1.0, 0.01, 0.15, "");
    configureKnob(reverbSendSlider, reverbSendLabel, "REVERB SEND",
                  0.0, 1.0, 0.01, 0.15, "");
    for (auto* panel : std::array<EffectPanel*, 6> {
             &distortionPanel, &granularPanel, &flangerPanel,
             &chorusPanel, &delayPanel, &reverbPanel })
        panel->onChange = [this] { updateEffects(); };
    delaySendSlider.onValueChange = [this] { updateEffects(); };
    reverbSendSlider.onValueChange = [this] { updateEffects(); };
    updateEffects();
    selectEffectTarget(0);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setText("Richiesta accesso al microfono...", juce::dontSendNotification);
    deviceManager.addAudioCallback(&engine);

    setSize(1280, 800);
    selectSlot(0);
    refreshWifiLibrary(false);
    showPage(Page::audio);
    startTimerHz(20);

    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
        [safeThis](bool granted)
        {
            if (safeThis != nullptr)
                safeThis->initialiseAudio(granted);
        });
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (engine.isRecording())
        engine.stopRecording();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff111419));
    graphics.setColour(juce::Colours::white);
    graphics.setFont(28.0f);
    const auto title = currentPage == Page::audio ? "MELA - AUDIO SETUP"
                     : currentPage == Page::wifi ? "MELA - WIFI LIBRARY"
                     : currentPage == Page::loop ? "MELA - 4 LOOP EDITOR"
                     : currentPage == Page::keys ? "MELA - SAMPLE KEYS"
                                                 : "MELA - EFFETTI";
    graphics.drawText(title,
                      24, 12, getWidth() - 550, 42, juce::Justification::centredLeft);
    graphics.setColour(juce::Colour(0xff20252c));
    graphics.fillRoundedRectangle(getLocalBounds().reduced(24).withTrimmedTop(60)
                                      .withTrimmedBottom(82).toFloat(), 14.0f);
}

void MainComponent::resized()
{
    auto outer = getLocalBounds().reduced(24);
    auto header = outer.removeFromTop(60);
    effectsPageButton.setBounds(header.removeFromRight(100).reduced(4));
    keysPageButton.setBounds(header.removeFromRight(100).reduced(4));
    loopPageButton.setBounds(header.removeFromRight(100).reduced(4));
    wifiPageButton.setBounds(header.removeFromRight(100).reduced(4));
    audioPageButton.setBounds(header.removeFromRight(100).reduced(4));

    auto footer = outer.removeFromBottom(76);
    if (currentPage != Page::audio && currentPage != Page::wifi)
    {
        playButton.setBounds(footer.removeFromLeft(165).reduced(5));
        stopButton.setBounds(footer.removeFromLeft(145).reduced(5));
        stopAllButton.setBounds(footer.removeFromLeft(145).reduced(5));
    }
    statusLabel.setBounds(footer.reduced(12, 5));

    auto content = outer.reduced(18, 12);
    if (currentPage == Page::audio)
    {
        audioInfoLabel.setBounds(content.removeFromTop(62).reduced(12, 4));
        content.removeFromTop(8);
        continueButton.setBounds(content.removeFromBottom(58).removeFromRight(220).reduced(5));
        if (audioDeviceSelector != nullptr)
            audioDeviceSelector->setBounds(content.reduced(35, 8));
    }
    else if (currentPage == Page::wifi)
    {
        wifiInfoLabel.setBounds(content.removeFromTop(66).reduced(18, 4));
        wifiAddressLabel.setBounds(content.removeFromTop(48).reduced(12, 3));
        wifiPinLabel.setBounds(content.removeFromTop(52).reduced(12, 3));
        content.removeFromTop(12);
        wifiInboxLabel.setBounds(content.removeFromTop(34).reduced(8, 2));
        auto fileRow = content.removeFromTop(62);
        wifiRefreshButton.setBounds(fileRow.removeFromRight(170).reduced(5));
        wifiFileBox.setBounds(fileRow.reduced(5, 8));
        content.removeFromTop(20);
        auto loadRow = content.removeFromTop(72);
        const auto buttonWidth = loadRow.getWidth() / LoopEngine::numberOfSlots;
        for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
            wifiLoadButtons[static_cast<size_t>(slot)].setBounds(
                loadRow.removeFromLeft(buttonWidth).reduced(6));
    }
    else if (currentPage == Page::loop)
    {
        auto selectorRow = content.removeFromTop(44);
        const auto selectorWidth = selectorRow.getWidth() / LoopEngine::numberOfSlots;
        for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
            sampleButtons[static_cast<size_t>(slot)].setBounds(
                selectorRow.removeFromLeft(selectorWidth).reduced(5, 2));

        content.removeFromTop(5);
        auto heading = content.removeFromTop(48);
        loadButton.setBounds(heading.removeFromRight(165).reduced(4));
        recordButton.setBounds(heading.removeFromRight(145).reduced(4));
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
    else if (currentPage == Page::keys)
    {
        auto selectorRow = content.removeFromTop(44);
        const auto selectorWidth = selectorRow.getWidth() / LoopEngine::numberOfSlots;
        for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
            sampleButtons[static_cast<size_t>(slot)].setBounds(
                selectorRow.removeFromLeft(selectorWidth).reduced(5, 2));

        content.removeFromTop(8);
        clipName.setBounds(content.removeFromTop(42).reduced(5));
        content.removeFromTop(8);
        auto controls = content.removeFromTop(70);
        octaveDownButton.setBounds(controls.removeFromLeft(105).reduced(4));
        octaveUpButton.setBounds(controls.removeFromLeft(105).reduced(4));
        rootDownButton.setBounds(controls.removeFromLeft(105).reduced(4));
        rootNoteLabel.setBounds(controls.removeFromLeft(170).reduced(4));
        rootUpButton.setBounds(controls.removeFromLeft(105).reduced(4));
        instrumentModeLabel.setBounds(controls.removeFromLeft(80).reduced(2));
        instrumentModeBox.setBounds(controls.removeFromLeft(190).reduced(5, 12));
        content.removeFromTop(8);
        auto envelopeArea = content.removeFromTop(120);
        std::array<juce::Slider*, 4> keySliders {
            &keyAttackSlider, &keyDecaySlider, &keySustainSlider, &keyReleaseSlider
        };
        std::array<juce::Label*, 4> keyLabels {
            &keyAttackLabel, &keyDecayLabel, &keySustainLabel, &keyReleaseLabel
        };
        const auto keyControlWidth = envelopeArea.getWidth() / 4;
        for (int index = 0; index < 4; ++index)
        {
            auto keyControl = envelopeArea.withTrimmedLeft(index * keyControlWidth)
                                          .withWidth(keyControlWidth).reduced(6, 0);
            keyLabels[static_cast<size_t>(index)]->setBounds(keyControl.removeFromTop(22));
            keySliders[static_cast<size_t>(index)]->setBounds(keyControl);
        }
        content.removeFromTop(8);
        touchKeyboard.setBounds(content.reduced(5));
    }
    else if (currentPage == Page::effects)
    {
        auto targetRow = content.removeFromTop(46);
        dspLoadLabel.setBounds(targetRow.removeFromRight(260).reduced(6, 2));
        const auto targetWidth = targetRow.getWidth()
                               / static_cast<int>(effectTargetButtons.size());
        for (auto& button : effectTargetButtons)
            button.setBounds(targetRow.removeFromLeft(targetWidth).reduced(5, 2));
        content.removeFromTop(8);

        if (effectTarget < LoopEngine::numberOfSlots)
        {
            auto sends = content.removeFromRight(230).reduced(8);
            auto delayArea = sends.removeFromTop(sends.getHeight() / 2).reduced(6);
            delaySendLabel.setBounds(delayArea.removeFromTop(25));
            delaySendSlider.setBounds(delayArea);
            auto reverbArea = sends.reduced(6);
            reverbSendLabel.setBounds(reverbArea.removeFromTop(25));
            reverbSendSlider.setBounds(reverbArea);

            auto topRow = content.removeFromTop((content.getHeight() - 10) / 2);
            content.removeFromTop(10);
            const auto topWidth = topRow.getWidth() / 2;
            distortionPanel.setBounds(topRow.removeFromLeft(topWidth).reduced(5));
            granularPanel.setBounds(topRow.reduced(5));
            const auto bottomWidth = content.getWidth() / 2;
            flangerPanel.setBounds(content.removeFromLeft(bottomWidth).reduced(5));
            chorusPanel.setBounds(content.reduced(5));
        }
        else
        {
            const auto masterWidth = content.getWidth() / 2;
            delayPanel.setBounds(content.removeFromLeft(masterWidth).reduced(8));
            reverbPanel.setBounds(content.reduced(8));
        }
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

void MainComponent::toggleRecording()
{
    if (engine.isRecording())
    {
        stopAndLoadRecording();
        return;
    }

    const auto recordingsDirectory =
        juce::File::getSpecialLocation(juce::File::userMusicDirectory)
            .getChildFile("Mela Recordings");
    const auto directoryResult = recordingsDirectory.createDirectory();
    if (directoryResult.failed())
    {
        statusLabel.setText("Errore: " + directoryResult.getErrorMessage(),
                            juce::dontSendNotification);
        return;
    }

    const auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
    const auto destination = recordingsDirectory.getNonexistentChildFile(
        "Mela-S" + juce::String(activeSlot + 1) + "-" + timestamp, ".wav", false);

    juce::String error;
    if (! engine.startRecording(destination, error))
    {
        statusLabel.setText("Errore REC: " + error, juce::dontSendNotification);
        return;
    }

    recordingSlot = activeSlot;
    engine.stop(recordingSlot);
    engine.allNotesOff(recordingSlot);
    recordButton.setButtonText("STOP REC S" + juce::String(recordingSlot + 1));
    statusLabel.setText("Registrazione sample " + juce::String(recordingSlot + 1)
                            + "...",
                        juce::dontSendNotification);
}

void MainComponent::stopAndLoadRecording()
{
    if (! engine.isRecording())
        return;

    const auto targetSlot = recordingSlot;
    const auto recordedFile = engine.stopRecording();
    recordingSlot = -1;
    recordButton.setButtonText("REC");

    if (! juce::isPositiveAndBelow(targetSlot, LoopEngine::numberOfSlots)
        || ! recordedFile.existsAsFile() || recordedFile.getSize() <= 44)
    {
        statusLabel.setText("Registrazione vuota", juce::dontSendNotification);
        return;
    }

    juce::String error;
    if (! engine.loadFile(targetSlot, recordedFile, error))
    {
        statusLabel.setText("Errore registrazione: " + error,
                            juce::dontSendNotification);
        return;
    }

    auto& settings = slotSettings[static_cast<size_t>(targetSlot)];
    settings.trimStart = 0.0;
    settings.trimEnd = 1.0;
    if (targetSlot == activeSlot)
    {
        waveform.setClip(engine.getClipForDisplay(activeSlot));
        waveform.setTrimRange(0.0, 1.0);
        clipName.setText(engine.getClipName(activeSlot), juce::dontSendNotification);
    }
    statusLabel.setText("Sample " + juce::String(targetSlot + 1)
                            + " registrato: " + recordedFile.getFileName(),
                        juce::dontSendNotification);
    updateSlotButtonColours();
}

void MainComponent::initialiseAudio(bool microphonePermissionGranted)
{
    const auto error = deviceManager.initialiseWithDefaultDevices(
        microphonePermissionGranted ? 1 : 0, 2);

    audioDeviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager, 0, 2, 1, 2, false, false, false, false);
    audioDeviceSelector->setItemHeight(34);
    addAndMakeVisible(*audioDeviceSelector);
    audioDeviceSelector->setVisible(currentPage == Page::audio);

    if (! error.isEmpty())
        statusLabel.setText("Errore audio: " + error, juce::dontSendNotification);
    else if (! microphonePermissionGranted)
        statusLabel.setText("Audio pronto, ma il permesso microfono e' negato",
                            juce::dontSendNotification);
    else
        statusLabel.setText("Audio pronto - scegli ingresso e uscita",
                            juce::dontSendNotification);

    resized();
}

void MainComponent::refreshWifiLibrary(bool announceResult)
{
    const auto directoryResult = wifiInboxDirectory.createDirectory();
    if (directoryResult.failed())
    {
        if (announceResult)
            statusLabel.setText("Errore Inbox: " + directoryResult.getErrorMessage(),
                                juce::dontSendNotification);
        return;
    }

    const auto previouslySelected = wifiFileBox.getText();
    const auto foundFiles = wifiInboxDirectory.findChildFiles(
        juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    wifiFiles.assign(foundFiles.begin(), foundFiles.end());
    std::sort(wifiFiles.begin(), wifiFiles.end(), [](const auto& first, const auto& second)
    {
        return first.getLastModificationTime() > second.getLastModificationTime();
    });

    wifiFileBox.clear(juce::dontSendNotification);
    int selectedId = 0;
    for (size_t index = 0; index < wifiFiles.size(); ++index)
    {
        const auto itemId = static_cast<int>(index) + 1;
        wifiFileBox.addItem(wifiFiles[index].getFileName(), itemId);
        if (wifiFiles[index].getFileName() == previouslySelected)
            selectedId = itemId;
    }
    if (selectedId == 0 && ! wifiFiles.empty())
        selectedId = 1;
    wifiFileBox.setSelectedId(selectedId, juce::dontSendNotification);

    auto hostName = juce::SystemStats::getComputerName().trim().toLowerCase();
    hostName = hostName.replaceCharacters(" ", "-");
    if (hostName.isEmpty())
        hostName = "mela";
    const auto localAddress = juce::IPAddress::getLocalAddress(false).toString();
    wifiAddressLabel.setText("APRI:  http://" + hostName + ".local:8080     oppure     http://"
                                     + localAddress + ":8080",
                                 juce::dontSendNotification);

    const auto pinFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                             .getChildFile(".config/mela/upload-pin.txt");
    const auto pin = pinFile.existsAsFile() ? pinFile.loadFileAsString().trim() : juce::String {};
    wifiPinLabel.setText(pin.length() == 6 ? "PIN:  " + pin
                                           : "PIN: servizio mela-upload non ancora avviato",
                         juce::dontSendNotification);
    wifiInboxLabel.setText("MELA INBOX - " + juce::String(wifiFiles.size())
                               + " sample - " + wifiInboxDirectory.getFullPathName(),
                           juce::dontSendNotification);

    if (announceResult)
        statusLabel.setText(juce::String(wifiFiles.size()) + " sample trovati nella Inbox",
                            juce::dontSendNotification);
}

void MainComponent::loadWifiSampleIntoSlot(int slotIndex)
{
    const auto fileIndex = wifiFileBox.getSelectedId() - 1;
    if (! juce::isPositiveAndBelow(fileIndex, static_cast<int>(wifiFiles.size())))
    {
        statusLabel.setText("Scegli prima un sample dalla Inbox",
                            juce::dontSendNotification);
        return;
    }

    const auto file = wifiFiles[static_cast<size_t>(fileIndex)];
    juce::String error;
    if (! engine.loadFile(slotIndex, file, error))
    {
        statusLabel.setText("Errore sample Wi-Fi: " + error,
                            juce::dontSendNotification);
        return;
    }

    auto& settings = slotSettings[static_cast<size_t>(slotIndex)];
    settings.trimStart = 0.0;
    settings.trimEnd = 1.0;
    selectSlot(slotIndex);
    showPage(Page::loop);
    statusLabel.setText("Wi-Fi -> Sample " + juce::String(slotIndex + 1)
                            + ": " + file.getFileName(),
                        juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    playButton.setButtonText(engine.isPlaying(activeSlot) ? "LOOP PLAYING..." : "PLAY LOOP");
    if (engine.isRecording())
    {
        recordButton.setButtonText("STOP REC S" + juce::String(recordingSlot + 1));
        statusLabel.setText("REC sample " + juce::String(recordingSlot + 1) + "  "
                                + juce::String(engine.getRecordingDurationSeconds(), 1) + " s",
                            juce::dontSendNotification);
    }
    waveform.setPlayhead(engine.getPlayheadNormalised(activeSlot));
    updateSlotButtonColours();
    if (currentPage == Page::wifi && ++wifiRefreshTicks >= 40)
    {
        wifiRefreshTicks = 0;
        refreshWifiLibrary(false);
    }
    const auto cpuPercent = juce::roundToInt(deviceManager.getCpuUsage() * 100.0);
    const auto xRuns = deviceManager.getXRunCount();
    dspLoadLabel.setText("DSP " + juce::String(cpuPercent) + "% | XRUN "
                             + juce::String(xRuns) + " | ECO 32G",
                         juce::dontSendNotification);
    dspLoadLabel.setColour(juce::Label::textColourId,
        cpuPercent >= 85 ? juce::Colour(0xffff6565)
                         : cpuPercent >= 65 ? juce::Colour(0xffffcf4a)
                                            : juce::Colour(0xff8de3b5));
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
    if (effectTarget < LoopEngine::numberOfSlots)
    {
        auto& settings = slotEffectSettings[static_cast<size_t>(effectTarget)];
        settings.distortionEnabled = distortionPanel.isEnabled();
        for (int index = 0; index < 3; ++index)
            settings.distortion[static_cast<size_t>(index)] = distortionPanel.value(index);
        settings.granularEnabled = granularPanel.isEnabled();
        for (int index = 0; index < 5; ++index)
            settings.granular[static_cast<size_t>(index)] = granularPanel.value(index);
        settings.flangerEnabled = flangerPanel.isEnabled();
        for (int index = 0; index < 4; ++index)
            settings.flanger[static_cast<size_t>(index)] = flangerPanel.value(index);
        settings.chorusEnabled = chorusPanel.isEnabled();
        for (int index = 0; index < 3; ++index)
            settings.chorus[static_cast<size_t>(index)] = chorusPanel.value(index);
        settings.delaySend = delaySendSlider.getValue();
        settings.reverbSend = reverbSendSlider.getValue();

        engine.setDistortion(effectTarget, settings.distortionEnabled,
            static_cast<float>(settings.distortion[0]),
            static_cast<float>(settings.distortion[1]),
            static_cast<float>(settings.distortion[2]));
        engine.setGranular(effectTarget, settings.granularEnabled,
            static_cast<float>(settings.granular[0]),
            static_cast<float>(settings.granular[1]),
            static_cast<float>(settings.granular[2]),
            static_cast<float>(settings.granular[3]),
            static_cast<float>(settings.granular[4]));
        engine.setFlanger(effectTarget, settings.flangerEnabled,
            static_cast<float>(settings.flanger[0]),
            static_cast<float>(settings.flanger[1]),
            static_cast<float>(settings.flanger[2]),
            static_cast<float>(settings.flanger[3]));
        engine.setChorus(effectTarget, settings.chorusEnabled,
            static_cast<float>(settings.chorus[0]),
            static_cast<float>(settings.chorus[1]),
            static_cast<float>(settings.chorus[2]));
        engine.setDelaySend(effectTarget, static_cast<float>(settings.delaySend));
        engine.setReverbSend(effectTarget, static_cast<float>(settings.reverbSend));
        return;
    }

    masterEffectSettings.delayEnabled = delayPanel.isEnabled();
    masterEffectSettings.reverbEnabled = reverbPanel.isEnabled();
    for (int index = 0; index < 3; ++index)
    {
        masterEffectSettings.delay[static_cast<size_t>(index)] = delayPanel.value(index);
        masterEffectSettings.reverb[static_cast<size_t>(index)] = reverbPanel.value(index);
    }
    engine.setDelay(masterEffectSettings.delayEnabled,
                    static_cast<float>(masterEffectSettings.delay[0]),
                    static_cast<float>(masterEffectSettings.delay[1]),
                    static_cast<float>(masterEffectSettings.delay[2]));
    engine.setReverb(masterEffectSettings.reverbEnabled,
                     static_cast<float>(masterEffectSettings.reverb[0]),
                     static_cast<float>(masterEffectSettings.reverb[1]),
                     static_cast<float>(masterEffectSettings.reverb[2]));
}

void MainComponent::selectEffectTarget(int targetIndex)
{
    if (! juce::isPositiveAndBelow(targetIndex, LoopEngine::numberOfSlots + 1))
        return;
    effectTarget = targetIndex;

    if (effectTarget < LoopEngine::numberOfSlots)
    {
        const auto& settings = slotEffectSettings[static_cast<size_t>(effectTarget)];
        distortionPanel.setEnabled(settings.distortionEnabled);
        granularPanel.setEnabled(settings.granularEnabled);
        flangerPanel.setEnabled(settings.flangerEnabled);
        chorusPanel.setEnabled(settings.chorusEnabled);
        for (int index = 0; index < 3; ++index)
        {
            distortionPanel.setValue(index, settings.distortion[static_cast<size_t>(index)]);
            chorusPanel.setValue(index, settings.chorus[static_cast<size_t>(index)]);
        }
        for (int index = 0; index < 5; ++index)
            granularPanel.setValue(index, settings.granular[static_cast<size_t>(index)]);
        for (int index = 0; index < 4; ++index)
            flangerPanel.setValue(index, settings.flanger[static_cast<size_t>(index)]);
        delaySendSlider.setValue(settings.delaySend, juce::dontSendNotification);
        reverbSendSlider.setValue(settings.reverbSend, juce::dontSendNotification);
    }
    else
    {
        delayPanel.setEnabled(masterEffectSettings.delayEnabled);
        reverbPanel.setEnabled(masterEffectSettings.reverbEnabled);
        for (int index = 0; index < 3; ++index)
        {
            delayPanel.setValue(index, masterEffectSettings.delay[static_cast<size_t>(index)]);
            reverbPanel.setValue(index, masterEffectSettings.reverb[static_cast<size_t>(index)]);
        }
    }

    for (int target = 0; target <= LoopEngine::numberOfSlots; ++target)
        effectTargetButtons[static_cast<size_t>(target)].setColour(
            juce::TextButton::buttonColourId,
            target == effectTarget ? juce::Colour(0xff3b6f91)
                                   : juce::Colour(0xff30343b));
    updateEffectPageVisibility();
    resized();
}

void MainComponent::updateEffectPageVisibility()
{
    const auto showEffects = currentPage == Page::effects;
    const auto showSlot = showEffects && effectTarget < LoopEngine::numberOfSlots;
    for (auto& button : effectTargetButtons)
        button.setVisible(showEffects);
    dspLoadLabel.setVisible(showEffects);
    for (auto* panel : std::array<EffectPanel*, 4> {
             &distortionPanel, &granularPanel, &flangerPanel, &chorusPanel })
        panel->setVisible(showSlot);
    delayPanel.setVisible(showEffects && ! showSlot);
    reverbPanel.setVisible(showEffects && ! showSlot);
    for (auto* component : std::array<juce::Component*, 4> {
             &delaySendSlider, &reverbSendSlider, &delaySendLabel, &reverbSendLabel })
        component->setVisible(showSlot);
}

void MainComponent::selectSlot(int slotIndex)
{
    if (! juce::isPositiveAndBelow(slotIndex, LoopEngine::numberOfSlots))
        return;

    touchKeyboard.releaseAll();
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
    keyAttackSlider.setValue(settings.keyAttack, juce::dontSendNotification);
    keyDecaySlider.setValue(settings.keyDecay, juce::dontSendNotification);
    keySustainSlider.setValue(settings.keySustain, juce::dontSendNotification);
    keyReleaseSlider.setValue(settings.keyRelease, juce::dontSendNotification);
    waveform.setClip(engine.getClipForDisplay(activeSlot));
    waveform.setTrimRange(settings.trimStart, settings.trimEnd);
    clipName.setText(engine.hasClip(activeSlot) ? engine.getClipName(activeSlot)
                                               : "Nessun loop caricato",
                     juce::dontSendNotification);
    updateEnvelope();
    updateInstrumentControls();
    updateKeyboardEnvelope();
    updateSlotButtonColours();
}

void MainComponent::showPage(Page pageToShow)
{
    if (pageToShow != Page::loop && engine.isRecording())
        stopAndLoadRecording();
    if (currentPage == Page::keys && pageToShow != Page::keys)
        touchKeyboard.releaseAll();
    currentPage = pageToShow;
    const auto showAudio = currentPage == Page::audio;
    const auto showWifi = currentPage == Page::wifi;
    const auto showLoop = currentPage == Page::loop;
    const auto showKeys = currentPage == Page::keys;
    for (auto* component : std::array<juce::Component*, 19> {
             &waveform, &loadButton, &recordButton,
             &loopButton, &reverseButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        component->setVisible(showLoop);
    audioInfoLabel.setVisible(showAudio);
    continueButton.setVisible(showAudio);
    if (audioDeviceSelector != nullptr)
        audioDeviceSelector->setVisible(showAudio);
    for (auto* component : std::array<juce::Component*, 6> {
             &wifiInfoLabel, &wifiAddressLabel, &wifiPinLabel,
             &wifiInboxLabel, &wifiFileBox, &wifiRefreshButton })
        component->setVisible(showWifi);
    for (auto& button : wifiLoadButtons)
        button.setVisible(showWifi);
    if (showWifi)
    {
        wifiRefreshTicks = 0;
        refreshWifiLibrary(false);
    }
    clipName.setVisible(showLoop || showKeys);
    for (auto* component : std::array<juce::Component*, 16> {
             &touchKeyboard, &octaveDownButton, &octaveUpButton,
             &rootDownButton, &rootUpButton, &rootNoteLabel,
             &instrumentModeLabel, &instrumentModeBox,
             &keyAttackSlider, &keyDecaySlider, &keySustainSlider, &keyReleaseSlider,
             &keyAttackLabel, &keyDecayLabel, &keySustainLabel, &keyReleaseLabel })
        component->setVisible(showKeys);
    for (auto& button : sampleButtons)
        button.setVisible(showLoop || showKeys);
    updateEffectPageVisibility();

    const auto showTransport = ! showAudio && ! showWifi;
    playButton.setVisible(showTransport);
    stopButton.setVisible(showTransport);
    stopAllButton.setVisible(showTransport);
    audioPageButton.setColour(juce::TextButton::buttonColourId,
        showAudio ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    wifiPageButton.setColour(juce::TextButton::buttonColourId,
        showWifi ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    loopPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    keysPageButton.setColour(juce::TextButton::buttonColourId,
        showKeys ? juce::Colour(0xff3b6f91) : juce::Colour(0xff30343b));
    effectsPageButton.setColour(juce::TextButton::buttonColourId,
        currentPage == Page::effects ? juce::Colour(0xff3b6f91)
                                     : juce::Colour(0xff30343b));
    resized();
    repaint();
}

void MainComponent::updateInstrumentControls()
{
    const auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
    touchKeyboard.setBaseMidiNote(settings.keyboardBaseNote);
    rootNoteLabel.setText("ROOT: " + juce::MidiMessage::getMidiNoteName(
                              settings.instrumentRootNote, true, true, 4),
                          juce::dontSendNotification);
    rootNoteLabel.setJustificationType(juce::Justification::centred);
    const auto modeId = settings.instrumentMode == LoopEngine::InstrumentMode::oneShot ? 2
                      : settings.instrumentMode == LoopEngine::InstrumentMode::loop ? 3 : 1;
    instrumentModeBox.setSelectedId(modeId, juce::dontSendNotification);
    engine.setInstrumentRootNote(activeSlot, settings.instrumentRootNote);
    engine.setInstrumentMode(activeSlot, settings.instrumentMode);
}

void MainComponent::updateKeyboardEnvelope()
{
    auto& settings = slotSettings[static_cast<size_t>(activeSlot)];
    settings.keyAttack = keyAttackSlider.getValue();
    settings.keyDecay = keyDecaySlider.getValue();
    settings.keySustain = keySustainSlider.getValue();
    settings.keyRelease = keyReleaseSlider.getValue();
    engine.setInstrumentEnvelope(activeSlot, settings.keyAttack, settings.keyDecay,
                                 static_cast<float>(settings.keySustain),
                                 settings.keyRelease);
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
