#include "MainComponent.h"

#include <algorithm>

namespace
{
constexpr int designWidth = 1280;
constexpr int designHeight = 800;
}

MainComponent::MainComponent()
{
    setLookAndFeel(&melaLookAndFeel);
    setOpaque(true);

    for (auto* component : std::array<juce::Component*, 10> {
             &playButton, &stopButton, &stopAllButton,
             &audioPageButton, &wifiPageButton, &loopPageButton,
             &keysPageButton, &effectsPageButton, &scenesPageButton,
             &statusLabel })
        addAndMakeVisible(component);

    for (auto* component : std::array<juce::Component*, 20> {
             &waveform, &loadButton, &deleteSampleButton, &recordButton,
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

    for (auto* component : std::array<juce::Component*, 7> {
             &wifiInfoLabel, &wifiAddressLabel, &wifiPinLabel,
             &wifiInboxLabel, &wifiFileBox, &wifiRefreshButton, &wifiDeleteButton })
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
    wifiDeleteButton.setColour(juce::TextButton::buttonColourId, MelaColours::coral);
    wifiDeleteButton.onClick = [this] { deleteSelectedWifiSample(); };

    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        auto& button = wifiLoadButtons[static_cast<size_t>(slot)];
        button.setButtonText("CARICA IN S" + juce::String(slot + 1));
        button.onClick = [this, slot] { loadWifiSampleIntoSlot(slot); };
        addAndMakeVisible(button);
    }

    scenesDirectory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("Mela/Scenes");
    scenesDirectory.createDirectory();
    for (int scene = 0; scene < numberOfScenes; ++scene)
    {
        auto& button = sceneButtons[static_cast<size_t>(scene)];
        button.onClick = [this, scene] { selectScene(scene); };
        addAndMakeVisible(button);
    }
    for (auto* component : std::array<juce::Component*, 5> {
             &sceneRecallButton, &sceneSaveButton, &sceneRenameButton,
             &sceneDeleteButton, &sceneInfoLabel })
        addAndMakeVisible(component);
    sceneInfoLabel.setJustificationType(juce::Justification::centred);
    sceneRecallButton.onClick = [this] { loadSelectedScene(); };
    sceneSaveButton.onClick = [this] { saveSelectedScene(true); };
    sceneRenameButton.onClick = [this] { renameSelectedScene(); };
    sceneDeleteButton.onClick = [this] { deleteSelectedScene(); };
    sceneSaveButton.setColour(juce::TextButton::buttonColourId, MelaColours::green);
    sceneDeleteButton.setColour(juce::TextButton::buttonColourId, MelaColours::coral);

    for (auto* component : std::array<juce::Component*, 14> {
             &touchKeyboard, &octaveDownButton, &octaveUpButton,
             &rootNoteLabel,
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

    playButton.setColour(juce::TextButton::buttonColourId, MelaColours::green);
    stopButton.setColour(juce::TextButton::buttonColourId, MelaColours::coral);
    stopAllButton.setColour(juce::TextButton::buttonColourId, MelaColours::coral.darker(0.15f));
    recordButton.setColour(juce::TextButton::buttonColourId, MelaColours::coral);
    deleteSampleButton.setColour(juce::TextButton::buttonColourId,
                                 MelaColours::coral.darker(0.18f));
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
    deleteSampleButton.onClick = [this] { deleteActiveSample(); };
    recordButton.onClick = [this] { toggleRecording(); };
    audioPageButton.onClick = [this] { showPage(Page::audio); };
    wifiPageButton.onClick = [this] { showPage(Page::wifi); };
    loopPageButton.onClick = [this] { showPage(Page::loop); };
    keysPageButton.onClick = [this] { showPage(Page::keys); };
    effectsPageButton.onClick = [this] { showPage(Page::effects); };
    scenesPageButton.onClick = [this] { showPage(Page::scenes); };
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
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 132, 39);
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

    // Keep the windowed preview aligned with the native Raspberry Pi touch display.
    // Kiosk mode will still resize the component to the active X11 display bounds.
    setSize(1920, 1200);
    selectSlot(0);
    refreshWifiLibrary(false);
    refreshSceneButtons();
    const auto autosaveFile = scenesDirectory.getChildFile("autosave.json");
    if (autosaveFile.existsAsFile())
    {
        juce::String restoreError;
        const auto restoredState = juce::JSON::parse(autosaveFile);
        restoreSceneState(restoredState, restoreError);
        lastAutosaveState = juce::JSON::toString(createSceneState("Autosave"), true);
    }
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
    saveAutosaveIfChanged();
    if (engine.isRecording())
        engine.stopRecording();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(MelaColours::aubergine);

    const auto scaleX = static_cast<float>(getWidth()) / static_cast<float>(designWidth);
    const auto scaleY = static_cast<float>(getHeight()) / static_cast<float>(designHeight);
    juce::Graphics::ScopedSaveState savedState(graphics);
    graphics.addTransform(juce::AffineTransform::scale(scaleX, scaleY));

    graphics.setColour(MelaColours::custard);
    graphics.setFont(melaLookAndFeel.getTextButtonFont(audioPageButton, 64)
                         .withHeight(30.0f));
    const auto title = currentPage == Page::audio ? "MELA - AUDIO SETUP"
                     : currentPage == Page::wifi ? "MELA - WIFI LIBRARY"
                     : currentPage == Page::loop ? "MELA - 4 LOOP EDITOR"
                     : currentPage == Page::keys ? "MELA - SAMPLE KEYS"
                     : currentPage == Page::effects ? "MELA - EFFETTI"
                                                    : "MELA - SCENE";
    graphics.drawText(title,
                      24, 12, designWidth - 650, 42, juce::Justification::centredLeft);
    const auto panel = juce::Rectangle<int>(0, 0, designWidth, designHeight)
                           .reduced(24).withTrimmedTop(60)
                           .withTrimmedBottom(82).toFloat();
    graphics.setColour(MelaColours::ink.withAlpha(0.55f));
    graphics.fillRoundedRectangle(panel.translated(0.0f, 5.0f), 20.0f);
    graphics.setColour(MelaColours::panel);
    graphics.fillRoundedRectangle(panel, 20.0f);
    graphics.setColour(MelaColours::ink);
    graphics.drawRoundedRectangle(panel, 20.0f, 3.0f);
}

void MainComponent::resized()
{
    // The UI was originally laid out on a 1280x800 design grid. All component
    // bounds are calculated on that grid and then mapped to the fixed 1920x1200
    // touch display, producing an exact 1.5x enlargement of every touch target.
    auto outer = juce::Rectangle<int>(0, 0, designWidth, designHeight).reduced(24);
    auto header = outer.removeFromTop(60);
    scenesPageButton.setBounds(header.removeFromRight(100).reduced(4));
    effectsPageButton.setBounds(header.removeFromRight(100).reduced(4));
    keysPageButton.setBounds(header.removeFromRight(100).reduced(4));
    loopPageButton.setBounds(header.removeFromRight(100).reduced(4));
    wifiPageButton.setBounds(header.removeFromRight(100).reduced(4));
    audioPageButton.setBounds(header.removeFromRight(100).reduced(4));

    auto footer = outer.removeFromBottom(76);
    if (currentPage != Page::audio && currentPage != Page::wifi
        && currentPage != Page::scenes)
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
        wifiDeleteButton.setBounds(fileRow.removeFromRight(150).reduced(5));
        wifiRefreshButton.setBounds(fileRow.removeFromRight(150).reduced(5));
        wifiFileBox.setBounds(fileRow.reduced(5, 8));
        content.removeFromTop(20);
        auto loadRow = content.removeFromTop(72);
        const auto buttonWidth = loadRow.getWidth() / LoopEngine::numberOfSlots;
        for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
            wifiLoadButtons[static_cast<size_t>(slot)].setBounds(
                loadRow.removeFromLeft(buttonWidth).reduced(6));
    }
    else if (currentPage == Page::scenes)
    {
        sceneInfoLabel.setBounds(content.removeFromTop(56).reduced(12, 3));
        content.removeFromTop(8);
        auto pads = content.removeFromTop(360);
        const auto rowHeight = pads.getHeight() / 2;
        for (int row = 0; row < 2; ++row)
        {
            auto sceneRow = pads.removeFromTop(rowHeight);
            const auto padWidth = sceneRow.getWidth() / 4;
            for (int column = 0; column < 4; ++column)
            {
                const auto scene = row * 4 + column;
                sceneButtons[static_cast<size_t>(scene)].setBounds(
                    sceneRow.removeFromLeft(padWidth).reduced(8));
            }
        }
        content.removeFromTop(18);
        auto actions = content.removeFromTop(74).reduced(70, 4);
        const auto actionWidth = actions.getWidth() / 4;
        sceneRecallButton.setBounds(actions.removeFromLeft(actionWidth).reduced(6));
        sceneSaveButton.setBounds(actions.removeFromLeft(actionWidth).reduced(6));
        sceneRenameButton.setBounds(actions.removeFromLeft(actionWidth).reduced(6));
        sceneDeleteButton.setBounds(actions.reduced(6));
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
        deleteSampleButton.setBounds(heading.removeFromRight(185).reduced(4));
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
        rootNoteLabel.setBounds(controls.removeFromLeft(190).reduced(4));
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

    const auto scaleX = static_cast<float>(getWidth()) / static_cast<float>(designWidth);
    const auto scaleY = static_cast<float>(getHeight()) / static_cast<float>(designHeight);
    for (auto* child : getChildren())
    {
        if (! child->isVisible())
            continue;

        const auto bounds = child->getBounds().toFloat();
        child->setBounds(juce::Rectangle<float>(bounds.getX() * scaleX,
                                                bounds.getY() * scaleY,
                                                bounds.getWidth() * scaleX,
                                                bounds.getHeight() * scaleY)
                             .toNearestInt());
    }
}

void MainComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Scegli un loop", juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
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
            slotSourceFiles[static_cast<size_t>(activeSlot)] = file;
            slotSourceIsRecording[static_cast<size_t>(activeSlot)] = false;
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

void MainComponent::deleteActiveSample()
{
    if (! engine.hasClip(activeSlot))
    {
        statusLabel.setText("Lo slot e' gia' vuoto", juce::dontSendNotification);
        return;
    }

    const auto slot = activeSlot;
    const auto sourceFile = slotSourceFiles[static_cast<size_t>(slot)];
    const auto isRecording = slotSourceIsRecording[static_cast<size_t>(slot)];
    const auto fileName = sourceFile.existsAsFile()
        ? sourceFile.getFileName() : engine.getClipName(slot);
    const auto message = isRecording
        ? "Svuotare SAMPLE " + juce::String(slot + 1)
              + " e cancellare definitivamente il WAV registrato \"" + fileName + "\"?"
        : "Svuotare SAMPLE " + juce::String(slot + 1)
              + "?\nIl file originale non verra' cancellato.";

    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon, "Elimina sample", message,
        "ELIMINA", "ANNULLA", this,
        juce::ModalCallbackFunction::create(
            [safeThis, slot, sourceFile, isRecording](int result)
            {
                if (result == 0 || safeThis == nullptr || safeThis->activeSlot != slot)
                    return;

                if (isRecording && sourceFile.existsAsFile())
                {
                    const auto recordingsDirectory =
                        juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                            .getChildFile("Mela Recordings");
                    if (! sourceFile.isAChildOf(recordingsDirectory)
                        || sourceFile.isSymbolicLink() || ! sourceFile.deleteFile())
                    {
                        safeThis->statusLabel.setText(
                            "Impossibile cancellare il WAV registrato",
                            juce::dontSendNotification);
                        return;
                    }
                }

                safeThis->clearActiveSlotAfterDelete();
            }));
}

void MainComponent::clearActiveSlotAfterDelete()
{
    const auto slot = activeSlot;
    engine.clearSlot(slot);
    slotSourceFiles[static_cast<size_t>(slot)] = juce::File {};
    slotSourceIsRecording[static_cast<size_t>(slot)] = false;
    auto& settings = slotSettings[static_cast<size_t>(slot)];
    settings.trimStart = 0.0;
    settings.trimEnd = 1.0;
    waveform.setClip(nullptr);
    clipName.setText("Nessun loop caricato", juce::dontSendNotification);
    statusLabel.setText("Sample " + juce::String(slot + 1) + " eliminato",
                        juce::dontSendNotification);
    updateSlotButtonColours();
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
        if (recordedFile.existsAsFile())
            recordedFile.deleteFile();
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

    slotSourceFiles[static_cast<size_t>(targetSlot)] = recordedFile;
    slotSourceIsRecording[static_cast<size_t>(targetSlot)] = true;

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
        juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
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

    slotSourceFiles[static_cast<size_t>(slotIndex)] = file;
    slotSourceIsRecording[static_cast<size_t>(slotIndex)] = false;

    auto& settings = slotSettings[static_cast<size_t>(slotIndex)];
    settings.trimStart = 0.0;
    settings.trimEnd = 1.0;
    selectSlot(slotIndex);
    showPage(Page::loop);
    statusLabel.setText("Wi-Fi -> Sample " + juce::String(slotIndex + 1)
                            + ": " + file.getFileName(),
                        juce::dontSendNotification);
}

void MainComponent::deleteSelectedWifiSample()
{
    const auto fileIndex = wifiFileBox.getSelectedId() - 1;
    if (! juce::isPositiveAndBelow(fileIndex, static_cast<int>(wifiFiles.size())))
    {
        statusLabel.setText("Scegli prima un sample da eliminare",
                            juce::dontSendNotification);
        return;
    }

    const auto file = wifiFiles[static_cast<size_t>(fileIndex)];
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Elimina sample",
        "Eliminare definitivamente \"" + file.getFileName()
            + "\" dalla Mela Inbox?\nUn sample gia' caricato continuera' a suonare fino alla chiusura.",
        "ELIMINA", "ANNULLA", this,
        juce::ModalCallbackFunction::create([safeThis, file](int result)
        {
            if (result == 0 || safeThis == nullptr)
                return;
            if (! file.isAChildOf(safeThis->wifiInboxDirectory)
                || file.isSymbolicLink() || ! file.deleteFile())
            {
                safeThis->statusLabel.setText("Impossibile eliminare " + file.getFileName(),
                                              juce::dontSendNotification);
                return;
            }
            safeThis->refreshWifiLibrary(false);
            safeThis->statusLabel.setText("Eliminato: " + file.getFileName(),
                                          juce::dontSendNotification);
        }));
}

void MainComponent::timerCallback()
{
    deleteSampleButton.setEnabled(engine.hasClip(activeSlot) && ! engine.isRecording());
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
    if (++autosaveTicks >= 100)
    {
        autosaveTicks = 0;
        saveAutosaveIfChanged();
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
            target == effectTarget ? MelaColours::sky
                                   : MelaColours::panelDark);
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
    const auto showScenes = currentPage == Page::scenes;
    for (auto* component : std::array<juce::Component*, 20> {
             &waveform, &loadButton, &deleteSampleButton, &recordButton,
             &loopButton, &reverseButton, &envelopeCycleButton,
             &speedSlider, &gainSlider, &attackSlider, &decaySlider,
             &sustainSlider, &releaseSlider, &speedLabel, &gainLabel,
             &attackLabel, &decayLabel, &sustainLabel, &releaseLabel, &clipName })
        component->setVisible(showLoop);
    audioInfoLabel.setVisible(showAudio);
    continueButton.setVisible(showAudio);
    if (audioDeviceSelector != nullptr)
        audioDeviceSelector->setVisible(showAudio);
    for (auto* component : std::array<juce::Component*, 7> {
             &wifiInfoLabel, &wifiAddressLabel, &wifiPinLabel,
             &wifiInboxLabel, &wifiFileBox, &wifiRefreshButton, &wifiDeleteButton })
        component->setVisible(showWifi);
    for (auto& button : wifiLoadButtons)
        button.setVisible(showWifi);
    if (showWifi)
    {
        wifiRefreshTicks = 0;
        refreshWifiLibrary(false);
    }
    clipName.setVisible(showLoop || showKeys);
    for (auto* component : std::array<juce::Component*, 14> {
             &touchKeyboard, &octaveDownButton, &octaveUpButton,
             &rootNoteLabel,
             &instrumentModeLabel, &instrumentModeBox,
             &keyAttackSlider, &keyDecaySlider, &keySustainSlider, &keyReleaseSlider,
             &keyAttackLabel, &keyDecayLabel, &keySustainLabel, &keyReleaseLabel })
        component->setVisible(showKeys);
    for (auto& button : sampleButtons)
        button.setVisible(showLoop || showKeys);
    for (auto& button : sceneButtons)
        button.setVisible(showScenes);
    for (auto* component : std::array<juce::Component*, 5> {
             &sceneRecallButton, &sceneSaveButton, &sceneRenameButton,
             &sceneDeleteButton, &sceneInfoLabel })
        component->setVisible(showScenes);
    updateEffectPageVisibility();

    const auto showTransport = ! showAudio && ! showWifi && ! showScenes;
    playButton.setVisible(showTransport);
    stopButton.setVisible(showTransport);
    stopAllButton.setVisible(showTransport);
    audioPageButton.setColour(juce::TextButton::buttonColourId,
        showAudio ? MelaColours::sky : MelaColours::panelDark);
    wifiPageButton.setColour(juce::TextButton::buttonColourId,
        showWifi ? MelaColours::sky : MelaColours::panelDark);
    loopPageButton.setColour(juce::TextButton::buttonColourId,
        showLoop ? MelaColours::sky : MelaColours::panelDark);
    keysPageButton.setColour(juce::TextButton::buttonColourId,
        showKeys ? MelaColours::sky : MelaColours::panelDark);
    effectsPageButton.setColour(juce::TextButton::buttonColourId,
        currentPage == Page::effects ? MelaColours::sky
                                     : MelaColours::panelDark);
    scenesPageButton.setColour(juce::TextButton::buttonColourId,
        showScenes ? MelaColours::sky : MelaColours::panelDark);
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
        auto colour = engine.hasClip(slot) ? MelaColours::green.darker(0.18f)
                                           : MelaColours::panelDark;
        if (engine.isPlaying(slot))
            colour = MelaColours::green;
        if (slot == activeSlot)
            colour = MelaColours::custard;
        sampleButtons[static_cast<size_t>(slot)].setColour(
            juce::TextButton::buttonColourId, colour);
        sampleButtons[static_cast<size_t>(slot)].setColour(
            juce::TextButton::textColourOffId,
            slot == activeSlot ? MelaColours::ink : MelaColours::cream);
    }
}

juce::var MainComponent::createSceneState(const juce::String& sceneName) const
{
    const auto valuesToVar = [](const auto& values)
    {
        juce::Array<juce::var> result;
        for (const auto value : values)
            result.add(value);
        return juce::var(result);
    };

    auto root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("name", sceneName);
    root->setProperty("activeSlot", activeSlot);

    juce::Array<juce::var> slots;
    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        const auto index = static_cast<size_t>(slot);
        const auto& settings = slotSettings[index];
        const auto& effects = slotEffectSettings[index];
        auto item = new juce::DynamicObject();
        item->setProperty("samplePath", slotSourceFiles[index].getFullPathName());
        item->setProperty("recordedByMela", slotSourceIsRecording[index]);
        item->setProperty("looping", settings.looping);
        item->setProperty("reverse", settings.reverse);
        item->setProperty("envelopeCycle", settings.envelopeCycle);
        item->setProperty("speed", settings.speed);
        item->setProperty("gain", settings.gain);
        item->setProperty("attack", settings.attack);
        item->setProperty("decay", settings.decay);
        item->setProperty("sustain", settings.sustain);
        item->setProperty("release", settings.release);
        item->setProperty("trimStart", settings.trimStart);
        item->setProperty("trimEnd", settings.trimEnd);
        item->setProperty("instrumentRootNote", settings.instrumentRootNote);
        item->setProperty("keyboardBaseNote", settings.keyboardBaseNote);
        item->setProperty("instrumentMode", static_cast<int>(settings.instrumentMode));
        item->setProperty("keyAttack", settings.keyAttack);
        item->setProperty("keyDecay", settings.keyDecay);
        item->setProperty("keySustain", settings.keySustain);
        item->setProperty("keyRelease", settings.keyRelease);
        item->setProperty("distortionEnabled", effects.distortionEnabled);
        item->setProperty("distortion", valuesToVar(effects.distortion));
        item->setProperty("granularEnabled", effects.granularEnabled);
        item->setProperty("granular", valuesToVar(effects.granular));
        item->setProperty("flangerEnabled", effects.flangerEnabled);
        item->setProperty("flanger", valuesToVar(effects.flanger));
        item->setProperty("chorusEnabled", effects.chorusEnabled);
        item->setProperty("chorus", valuesToVar(effects.chorus));
        item->setProperty("delaySend", effects.delaySend);
        item->setProperty("reverbSend", effects.reverbSend);
        slots.add(juce::var(item));
    }
    root->setProperty("slots", juce::var(slots));

    auto master = new juce::DynamicObject();
    master->setProperty("delayEnabled", masterEffectSettings.delayEnabled);
    master->setProperty("delay", valuesToVar(masterEffectSettings.delay));
    master->setProperty("reverbEnabled", masterEffectSettings.reverbEnabled);
    master->setProperty("reverb", valuesToVar(masterEffectSettings.reverb));
    root->setProperty("master", juce::var(master));
    return juce::var(root);
}

bool MainComponent::restoreSceneState(const juce::var& state, juce::String& errorMessage)
{
    auto* root = state.getDynamicObject();
    if (root == nullptr || ! root->hasProperty("slots"))
    {
        errorMessage = "File scena non valido";
        return false;
    }

    auto slotsValue = root->getProperty("slots");
    auto* slots = slotsValue.getArray();
    if (slots == nullptr || slots->size() != LoopEngine::numberOfSlots)
    {
        errorMessage = "La scena non contiene quattro slot";
        return false;
    }

    const auto number = [](juce::DynamicObject* object, const char* key, double fallback)
    {
        const juce::Identifier property(key);
        return object->hasProperty(property) ? static_cast<double>(object->getProperty(property))
                                             : fallback;
    };
    const auto integer = [](juce::DynamicObject* object, const char* key, int fallback)
    {
        const juce::Identifier property(key);
        return object->hasProperty(property) ? static_cast<int>(object->getProperty(property))
                                             : fallback;
    };
    const auto boolean = [](juce::DynamicObject* object, const char* key, bool fallback)
    {
        const juce::Identifier property(key);
        return object->hasProperty(property) ? static_cast<bool>(object->getProperty(property))
                                             : fallback;
    };
    const auto readArray = [](juce::DynamicObject* object, const char* key, auto& destination)
    {
        auto value = object->getProperty(juce::Identifier(key));
        if (auto* source = value.getArray())
            for (int index = 0; index < juce::jmin(source->size(),
                                                   static_cast<int>(destination.size())); ++index)
                destination[static_cast<size_t>(index)] = static_cast<double>((*source)[index]);
    };

    touchKeyboard.releaseAll();
    engine.stopAll();
    int missingFiles = 0;
    juce::String loadWarnings;
    const auto recordingsDirectory =
        juce::File::getSpecialLocation(juce::File::userMusicDirectory)
            .getChildFile("Mela Recordings");

    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        const auto index = static_cast<size_t>(slot);
        auto* item = (*slots)[slot].getDynamicObject();
        if (item == nullptr)
        {
            errorMessage = "Dati slot non validi";
            return false;
        }

        auto& settings = slotSettings[index];
        settings.looping = boolean(item, "looping", settings.looping);
        settings.reverse = boolean(item, "reverse", settings.reverse);
        settings.envelopeCycle = boolean(item, "envelopeCycle", settings.envelopeCycle);
        settings.speed = number(item, "speed", settings.speed);
        settings.gain = number(item, "gain", settings.gain);
        settings.attack = number(item, "attack", settings.attack);
        settings.decay = number(item, "decay", settings.decay);
        settings.sustain = number(item, "sustain", settings.sustain);
        settings.release = number(item, "release", settings.release);
        settings.trimStart = number(item, "trimStart", settings.trimStart);
        settings.trimEnd = number(item, "trimEnd", settings.trimEnd);
        settings.instrumentRootNote = integer(item, "instrumentRootNote",
                                              settings.instrumentRootNote);
        settings.keyboardBaseNote = integer(item, "keyboardBaseNote",
                                             settings.keyboardBaseNote);
        settings.instrumentMode = static_cast<LoopEngine::InstrumentMode>(
            juce::jlimit(0, 2, integer(item, "instrumentMode", 0)));
        settings.keyAttack = number(item, "keyAttack", settings.keyAttack);
        settings.keyDecay = number(item, "keyDecay", settings.keyDecay);
        settings.keySustain = number(item, "keySustain", settings.keySustain);
        settings.keyRelease = number(item, "keyRelease", settings.keyRelease);

        auto& effects = slotEffectSettings[index];
        effects.distortionEnabled = boolean(item, "distortionEnabled", false);
        effects.granularEnabled = boolean(item, "granularEnabled", false);
        effects.flangerEnabled = boolean(item, "flangerEnabled", false);
        effects.chorusEnabled = boolean(item, "chorusEnabled", false);
        readArray(item, "distortion", effects.distortion);
        readArray(item, "granular", effects.granular);
        readArray(item, "flanger", effects.flanger);
        readArray(item, "chorus", effects.chorus);
        effects.delaySend = number(item, "delaySend", effects.delaySend);
        effects.reverbSend = number(item, "reverbSend", effects.reverbSend);

        engine.clearSlot(slot);
        const juce::File sourceFile(item->getProperty("samplePath").toString());
        slotSourceFiles[index] = sourceFile;
        slotSourceIsRecording[index] = boolean(item, "recordedByMela", false)
                                       && sourceFile.isAChildOf(recordingsDirectory);
        if (sourceFile.getFullPathName().isNotEmpty())
        {
            if (sourceFile.existsAsFile())
            {
                juce::String loadError;
                if (engine.loadFile(slot, sourceFile, loadError))
                {
                }
                else
                {
                    ++missingFiles;
                    loadWarnings << "S" << (slot + 1) << " non leggibile; ";
                }
            }
            else
            {
                ++missingFiles;
                loadWarnings << "S" << (slot + 1) << " mancante; ";
            }
        }
    }

    if (auto* master = root->getProperty("master").getDynamicObject())
    {
        masterEffectSettings.delayEnabled = boolean(master, "delayEnabled", false);
        masterEffectSettings.reverbEnabled = boolean(master, "reverbEnabled", false);
        readArray(master, "delay", masterEffectSettings.delay);
        readArray(master, "reverb", masterEffectSettings.reverb);
    }

    applyAllSettingsToEngine();
    activeSlot = juce::jlimit(0, LoopEngine::numberOfSlots - 1,
                              integer(root, "activeSlot", 0));
    selectSlot(activeSlot);
    selectEffectTarget(juce::jlimit(0, LoopEngine::numberOfSlots, effectTarget));
    errorMessage = missingFiles > 0 ? loadWarnings.trimEnd() : juce::String {};
    return true;
}

void MainComponent::applyAllSettingsToEngine()
{
    for (int slot = 0; slot < LoopEngine::numberOfSlots; ++slot)
    {
        const auto index = static_cast<size_t>(slot);
        const auto& settings = slotSettings[index];
        const auto& effects = slotEffectSettings[index];
        engine.setLooping(slot, settings.looping);
        engine.setReverse(slot, settings.reverse);
        engine.setGain(slot, static_cast<float>(settings.gain));
        engine.setPlaybackRate(slot, settings.speed);
        engine.setTrimRange(slot, settings.trimStart, settings.trimEnd);
        engine.setEnvelope(slot, settings.attack, settings.decay,
                           static_cast<float>(settings.sustain), settings.release);
        engine.setEnvelopeCycle(slot, settings.envelopeCycle);
        engine.setInstrumentRootNote(slot, settings.instrumentRootNote);
        engine.setInstrumentMode(slot, settings.instrumentMode);
        engine.setInstrumentEnvelope(slot, settings.keyAttack, settings.keyDecay,
                                     static_cast<float>(settings.keySustain),
                                     settings.keyRelease);
        engine.setDistortion(slot, effects.distortionEnabled,
            static_cast<float>(effects.distortion[0]), static_cast<float>(effects.distortion[1]),
            static_cast<float>(effects.distortion[2]));
        engine.setGranular(slot, effects.granularEnabled,
            static_cast<float>(effects.granular[0]), static_cast<float>(effects.granular[1]),
            static_cast<float>(effects.granular[2]), static_cast<float>(effects.granular[3]),
            static_cast<float>(effects.granular[4]));
        engine.setFlanger(slot, effects.flangerEnabled,
            static_cast<float>(effects.flanger[0]), static_cast<float>(effects.flanger[1]),
            static_cast<float>(effects.flanger[2]), static_cast<float>(effects.flanger[3]));
        engine.setChorus(slot, effects.chorusEnabled,
            static_cast<float>(effects.chorus[0]), static_cast<float>(effects.chorus[1]),
            static_cast<float>(effects.chorus[2]));
        engine.setDelaySend(slot, static_cast<float>(effects.delaySend));
        engine.setReverbSend(slot, static_cast<float>(effects.reverbSend));
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

juce::File MainComponent::sceneFile(int sceneIndex) const
{
    return scenesDirectory.getChildFile("scene-" + juce::String(sceneIndex + 1) + ".json");
}

bool MainComponent::writeSceneFile(const juce::File& file, const juce::var& state) const
{
    juce::TemporaryFile temporary(file);
    if (! temporary.getFile().replaceWithText(juce::JSON::toString(state, true)))
        return false;
    return temporary.overwriteTargetFileWithTemporary();
}

void MainComponent::selectScene(int sceneIndex)
{
    selectedScene = juce::jlimit(0, numberOfScenes - 1, sceneIndex);
    refreshSceneButtons();
}

void MainComponent::saveSelectedScene(bool askBeforeOverwrite)
{
    const auto file = sceneFile(selectedScene);
    if (askBeforeOverwrite && file.existsAsFile())
    {
        juce::Component::SafePointer<MainComponent> safeThis(this);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::QuestionIcon, "Sovrascrivi scena",
            "Sostituire la scena " + juce::String(selectedScene + 1) + " con lo stato attuale?",
            "SOVRASCRIVI", "ANNULLA", this,
            juce::ModalCallbackFunction::create([safeThis](int result)
            {
                if (result != 0 && safeThis != nullptr)
                    safeThis->saveSelectedScene(false);
            }));
        return;
    }

    juce::String name = "SCENA " + juce::String(selectedScene + 1);
    if (file.existsAsFile())
    {
        const auto existingState = juce::JSON::parse(file);
        if (auto* existing = existingState.getDynamicObject())
            name = existing->getProperty("name").toString();
    }
    if (! writeSceneFile(file, createSceneState(name)))
    {
        statusLabel.setText("Impossibile salvare la scena", juce::dontSendNotification);
        return;
    }
    refreshSceneButtons();
    statusLabel.setText(name + " salvata", juce::dontSendNotification);
}

void MainComponent::loadSelectedScene()
{
    const auto file = sceneFile(selectedScene);
    if (! file.existsAsFile())
    {
        statusLabel.setText("La scena selezionata e' vuota", juce::dontSendNotification);
        return;
    }
    juce::String error;
    const auto state = juce::JSON::parse(file);
    if (! restoreSceneState(state, error))
    {
        statusLabel.setText(error, juce::dontSendNotification);
        return;
    }
    const auto name = state.getDynamicObject()->getProperty("name").toString();
    statusLabel.setText(error.isEmpty() ? name + " richiamata"
                                        : name + " richiamata - " + error,
                        juce::dontSendNotification);
}

void MainComponent::renameSelectedScene()
{
    const auto file = sceneFile(selectedScene);
    if (! file.existsAsFile())
    {
        statusLabel.setText("Prima salva la scena", juce::dontSendNotification);
        return;
    }
    auto state = juce::JSON::parse(file);
    auto* object = state.getDynamicObject();
    if (object == nullptr)
        return;

    auto* dialog = new juce::AlertWindow("Rinomina scena", "Inserisci il nuovo nome",
                                         juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", object->getProperty("name").toString(), "NOME");
    dialog->addButton("SALVA", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("ANNULLA", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<MainComponent> safeThis(this);
    dialog->enterModalState(true,
        juce::ModalCallbackFunction::create([safeThis, dialog, file, state](int result) mutable
        {
            if (result != 0 && safeThis != nullptr)
            {
                auto name = dialog->getTextEditorContents("name").trim().substring(0, 24);
                if (name.isEmpty())
                    name = "SCENA " + juce::String(safeThis->selectedScene + 1);
                state.getDynamicObject()->setProperty("name", name);
                if (safeThis->writeSceneFile(file, state))
                    safeThis->statusLabel.setText("Scena rinominata: " + name,
                                                  juce::dontSendNotification);
                safeThis->refreshSceneButtons();
            }
            delete dialog;
        }), false);
}

void MainComponent::deleteSelectedScene()
{
    const auto file = sceneFile(selectedScene);
    if (! file.existsAsFile())
        return;
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon, "Elimina scena",
        "Eliminare la scena " + juce::String(selectedScene + 1)
            + "? I sample audio non verranno cancellati.",
        "ELIMINA", "ANNULLA", this,
        juce::ModalCallbackFunction::create([safeThis, file](int result)
        {
            if (result == 0 || safeThis == nullptr)
                return;
            if (file.deleteFile())
            {
                safeThis->refreshSceneButtons();
                safeThis->statusLabel.setText("Scena eliminata",
                                              juce::dontSendNotification);
            }
        }));
}

void MainComponent::refreshSceneButtons()
{
    for (int scene = 0; scene < numberOfScenes; ++scene)
    {
        const auto file = sceneFile(scene);
        auto name = "SCENA " + juce::String(scene + 1) + " - VUOTA";
        if (file.existsAsFile())
        {
            const auto state = juce::JSON::parse(file);
            if (auto* object = state.getDynamicObject())
                name = object->getProperty("name").toString();
        }
        auto& button = sceneButtons[static_cast<size_t>(scene)];
        button.setButtonText(name);
        button.setColour(juce::TextButton::buttonColourId,
                         scene == selectedScene ? MelaColours::custard
                                                : file.existsAsFile()
                                                    ? MelaColours::green.darker(0.18f)
                                                    : MelaColours::panelDark);
        button.setColour(juce::TextButton::textColourOffId,
                         scene == selectedScene ? MelaColours::ink : MelaColours::cream);
    }
    const auto exists = sceneFile(selectedScene).existsAsFile();
    sceneRecallButton.setEnabled(exists);
    sceneRenameButton.setEnabled(exists);
    sceneDeleteButton.setEnabled(exists);
    sceneInfoLabel.setText(
        "Seleziona una memoria, poi usa RICHIAMA o SALVA. I file audio restano separati.",
        juce::dontSendNotification);
}

void MainComponent::saveAutosaveIfChanged()
{
    if (scenesDirectory == juce::File {})
        return;
    const auto state = createSceneState("Autosave");
    const auto json = juce::JSON::toString(state, true);
    if (json == lastAutosaveState)
        return;
    if (writeSceneFile(scenesDirectory.getChildFile("autosave.json"), state))
        lastAutosaveState = json;
}
