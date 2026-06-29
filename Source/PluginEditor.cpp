#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const auto backgroundTop = juce::Colour::fromRGB(10, 14, 22);
    const auto backgroundBottom = juce::Colour::fromRGB(18, 25, 38);
    const auto panelColour = juce::Colour::fromRGB(24, 32, 47);
    const auto panelBorder = juce::Colour::fromRGB(48, 63, 86);
    const auto primaryText = juce::Colour::fromRGB(236, 242, 252);
    const auto secondaryText = juce::Colour::fromRGB(143, 158, 181);
    const auto accentBlue = juce::Colour::fromRGB(65, 145, 255);
    const auto accentRed = juce::Colour::fromRGB(235, 75, 88);
    const auto accentGreen = juce::Colour::fromRGB(65, 205, 140);
    const auto accentAmber = juce::Colour::fromRGB(244, 176, 74);
    const auto accentPurple = juce::Colour::fromRGB(177, 116, 255);
}

//==============================================================================
DinLooperAudioProcessorEditor::DinLooperAudioProcessorEditor(DinLooperAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(designWidth, designHeight);
    setResizable(true, true);
    setResizeLimits(560, 416, 1050, 780);
    getConstrainer()->setFixedAspectRatio(
        static_cast<double>(designWidth) / designHeight);
    setOpaque(true);
    addAndMakeVisible(content);

    // ===== Title =====
    titleLabel.setText("DinLooper", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, primaryText);
    content.addAndMakeVisible(titleLabel);

    // ===== Status =====
    statusLabel.setText("IDLE", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(),
        14.0f,
        juce::Font::bold)));
    content.addAndMakeVisible(statusLabel);

    // ===== Progress =====
    progressLabel.setText("LOOP POSITION", juce::dontSendNotification);
    progressLabel.setJustificationType(juce::Justification::centred);
    progressLabel.setColour(juce::Label::textColourId, secondaryText);
    progressLabel.setFont(juce::Font(juce::FontOptions(12.0f,
                                                       juce::Font::bold)));
    content.addAndMakeVisible(progressLabel);

    // ===== Time =====
    timeLabel.setText("00.00 / 00.00", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setColour(juce::Label::textColourId, primaryText);
    content.addAndMakeVisible(timeLabel);

    // ===== Layers =====
    layersLabel.setText("Layers: 0", juce::dontSendNotification);
    layersLabel.setJustificationType(juce::Justification::centred);
    layersLabel.setColour(juce::Label::textColourId, primaryText);
    layersLabel.setFont(juce::Font(juce::FontOptions(16.0f,
                                                     juce::Font::bold)));
    content.addAndMakeVisible(layersLabel);

    // ===== Trigger =====
    triggerModeLabel.setText("Trigger", juce::dontSendNotification);
    triggerModeLabel.setJustificationType(juce::Justification::centredRight);
    triggerModeLabel.setColour(juce::Label::textColourId, secondaryText);
    content.addAndMakeVisible(triggerModeLabel);

    triggerModeBox.addItem("Instant", 1);
    triggerModeBox.addItem("Audio + MIDI", 2);
    triggerModeBox.addItem("Audio Only", 3);
    triggerModeBox.addItem("MIDI Only", 4);
    triggerModeBox.setColour(juce::ComboBox::backgroundColourId,
                             backgroundTop);
    triggerModeBox.setColour(juce::ComboBox::textColourId, primaryText);
    triggerModeBox.setColour(juce::ComboBox::outlineColourId, panelBorder);
    triggerModeBox.setColour(juce::ComboBox::arrowColourId, accentBlue);
    content.addAndMakeVisible(triggerModeBox);

    thresholdLabel.setText("THRESHOLD", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centred);
    thresholdLabel.setColour(juce::Label::textColourId, secondaryText);
    thresholdLabel.setFont(juce::Font(juce::FontOptions(9.0f,
                                                        juce::Font::bold)));
    thresholdLabel.setVisible(false);

    thresholdSlider.setSliderStyle(juce::Slider::LinearVertical);
    thresholdSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider.setColour(juce::Slider::backgroundColourId,
                              juce::Colours::transparentBlack);
    thresholdSlider.setColour(juce::Slider::trackColourId,
                              juce::Colours::transparentBlack);
    thresholdSlider.setColour(juce::Slider::thumbColourId,
                              juce::Colours::transparentBlack);
    thresholdSlider.onValueChange = [this]
    {
        repaint();
    };
    content.addAndMakeVisible(thresholdSlider);

    triggerModeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getParameters(), "trigger_mode", triggerModeBox);
    thresholdAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getParameters(), "threshold_db", thresholdSlider);

    audioThruButton.setColour(juce::ToggleButton::textColourId, primaryText);
    audioThruButton.setColour(juce::ToggleButton::tickColourId, accentBlue);
    audioThruButton.setColour(juce::ToggleButton::tickDisabledColourId,
                              secondaryText);
    content.addAndMakeVisible(audioThruButton);
    audioThruAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getParameters(), "audio_thru", audioThruButton);

    // ===== Metering and master volume =====
    inputMeterLabel.setText("INPUT", juce::dontSendNotification);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    inputMeterLabel.setColour(juce::Label::textColourId, secondaryText);
    inputMeterLabel.setFont(juce::Font(juce::FontOptions(11.0f,
                                                        juce::Font::bold)));
    content.addAndMakeVisible(inputMeterLabel);

    masterMeterLabel.setText("OUT", juce::dontSendNotification);
    masterMeterLabel.setJustificationType(juce::Justification::centred);
    masterMeterLabel.setColour(juce::Label::textColourId, secondaryText);
    masterMeterLabel.setFont(juce::Font(juce::FontOptions(11.0f,
                                                         juce::Font::bold)));
    content.addAndMakeVisible(masterMeterLabel);

    masterVolumeLabel.setText("VOLUME", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType(juce::Justification::centred);
    masterVolumeLabel.setColour(juce::Label::textColourId, secondaryText);
    content.addAndMakeVisible(masterVolumeLabel);

    masterVolumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow,
                                       false,
                                       54,
                                       20);
    masterVolumeSlider.setTextValueSuffix(" dB");
    masterVolumeSlider.setColour(juce::Slider::backgroundColourId,
                                 backgroundTop);
    masterVolumeSlider.setColour(juce::Slider::trackColourId, accentGreen);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, primaryText);
    masterVolumeSlider.setColour(juce::Slider::textBoxTextColourId,
                                 primaryText);
    masterVolumeSlider.setColour(juce::Slider::textBoxBackgroundColourId,
                                 backgroundTop);
    masterVolumeSlider.setColour(juce::Slider::textBoxOutlineColourId,
                                 panelBorder);
    content.addAndMakeVisible(masterVolumeSlider);

    masterVolumeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getParameters(),
            "master_gain_db",
            masterVolumeSlider);

    // ===== Individual layer controls =====
    layerViewport.setViewedComponent(&layerControls, false);
    layerViewport.setScrollBarsShown(true, false);
    layerViewport.setColour(juce::ScrollBar::thumbColourId,
                            panelBorder.brighter(0.35f));
    layerViewport.setColour(juce::ScrollBar::trackColourId,
                            backgroundTop.withAlpha(0.45f));
    content.addAndMakeVisible(layerViewport);

    for (int layer = 0; layer < LooperEngine::maximumLayers; ++layer)
    {
        auto& label = layerLabels[static_cast<size_t>(layer)];
        label.setText("L" + juce::String(layer + 1),
                      juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, primaryText);
        label.setFont(juce::Font(juce::FontOptions(11.0f,
                                                   juce::Font::bold)));
        layerControls.addAndMakeVisible(label);

        auto& meter = layerMeters[static_cast<size_t>(layer)];
        meter.setSliderStyle(juce::Slider::LinearBar);
        meter.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        meter.setRange(0.0, 1.0);
        meter.setInterceptsMouseClicks(false, false);
        meter.setColour(juce::Slider::backgroundColourId, backgroundTop);
        meter.setColour(juce::Slider::trackColourId, accentGreen);
        layerControls.addAndMakeVisible(meter);

        auto& volume = layerVolumeSliders[static_cast<size_t>(layer)];
        volume.setSliderStyle(juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle(juce::Slider::TextBoxRight,
                               false, 49, 18);
        volume.setRange(-60.0, 6.0, 0.1);
        volume.setSkewFactorFromMidPoint(-12.0);
        volume.setValue(0.0, juce::dontSendNotification);
        volume.setTextValueSuffix(" dB");
        volume.setColour(juce::Slider::backgroundColourId, backgroundTop);
        volume.setColour(juce::Slider::trackColourId, accentBlue);
        volume.setColour(juce::Slider::thumbColourId, primaryText);
        volume.setColour(juce::Slider::textBoxTextColourId, primaryText);
        volume.setColour(juce::Slider::textBoxBackgroundColourId,
                         backgroundTop);
        volume.setColour(juce::Slider::textBoxOutlineColourId, panelBorder);
        volume.onValueChange = [this, layer]
        {
            audioProcessor.setLayerVolume(
                layer,
                juce::Decibels::decibelsToGain(
                    static_cast<float>(
                        layerVolumeSliders[static_cast<size_t>(layer)]
                            .getValue()),
                    -60.0f));
        };
        layerControls.addAndMakeVisible(volume);

        auto& mute = layerMuteButtons[static_cast<size_t>(layer)];
        mute.setButtonText("M");
        mute.setColour(juce::ToggleButton::textColourId, primaryText);
        mute.setColour(juce::ToggleButton::tickColourId, accentAmber);
        mute.onClick = [this, layer]
        {
            audioProcessor.setLayerMuted(
                layer,
                layerMuteButtons[static_cast<size_t>(layer)]
                    .getToggleState());
        };
        layerControls.addAndMakeVisible(mute);

        auto& solo = layerSoloButtons[static_cast<size_t>(layer)];
        solo.setButtonText("S");
        solo.setColour(juce::ToggleButton::textColourId, primaryText);
        solo.setColour(juce::ToggleButton::tickColourId, accentGreen);
        solo.onClick = [this, layer]
        {
            audioProcessor.setLayerSoloed(
                layer,
                layerSoloButtons[static_cast<size_t>(layer)]
                    .getToggleState());
        };
        layerControls.addAndMakeVisible(solo);

        auto& remove = layerDeleteButtons[static_cast<size_t>(layer)];
        remove.setButtonText("X");
        remove.setColour(juce::TextButton::buttonColourId,
                         accentRed.withMultipliedBrightness(0.45f));
        remove.setColour(juce::TextButton::textColourOffId, primaryText);
        remove.onClick = [this, layer]
        {
            audioProcessor.deleteLayer(layer);
        };
        layerControls.addAndMakeVisible(remove);
    }

    // ===== Buttons =====
    content.addAndMakeVisible(recButton);
    content.addAndMakeVisible(playButton);
    content.addAndMakeVisible(stopButton);
    content.addAndMakeVisible(undoButton);
    content.addAndMakeVisible(redoButton);
    content.addAndMakeVisible(resetButton);
    content.addAndMakeVisible(rewindButton);
    content.addAndMakeVisible(recSustainButton);

    const auto styleButton = [](juce::TextButton& button,
                                juce::Colour colour)
    {
        button.setColour(juce::TextButton::buttonColourId,
                         colour.withMultipliedBrightness(0.55f));
        button.setColour(juce::TextButton::buttonOnColourId, colour);
        button.setColour(juce::TextButton::textColourOffId, primaryText);
        button.setColour(juce::TextButton::textColourOnId,
                         juce::Colours::white);
    };

    styleButton(recButton, accentRed);
    styleButton(recSustainButton, accentPurple);
    styleButton(playButton, accentGreen);
    styleButton(stopButton, accentAmber);
    styleButton(rewindButton, accentBlue);
    styleButton(undoButton, accentBlue);
    styleButton(redoButton, accentBlue);
    styleButton(resetButton, juce::Colour::fromRGB(102, 116, 139));

    recButton.onClick = [this] { audioProcessor.pressRec(); };
    playButton.onClick = [this] { audioProcessor.pressPlay(); };
    stopButton.onClick = [this] { audioProcessor.pressStop(); };
    undoButton.onClick = [this] { audioProcessor.pressUndo(); };
    redoButton.onClick = [this] { audioProcessor.pressRedo(); };
    resetButton.onClick = [this] { audioProcessor.pressReset(); };
    rewindButton.onClick = [this] { audioProcessor.pressRewind(); };
    recSustainButton.onClick = [this] { audioProcessor.pressRecSustain(); };

    updateLooperStatus();
    startTimerHz(20);
}

DinLooperAudioProcessorEditor::~DinLooperAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void DinLooperAudioProcessorEditor::timerCallback()
{
    updateLooperStatus();
    updateLayerControls();
}

void DinLooperAudioProcessorEditor::updateLooperStatus()
{
    const auto stateName = audioProcessor.getStateName();
    statusLabel.setText(stateName, juce::dontSendNotification);

    auto statusColour = secondaryText;

    if (stateName.contains("RECORD") || stateName.contains("SUSTAIN"))
        statusColour = accentRed;
    else if (stateName.contains("OVERDUB"))
        statusColour = accentPurple;
    else if (stateName.contains("PLAY"))
        statusColour = accentGreen;
    else if (stateName.contains("WAIT") || stateName.contains("PREPAR"))
        statusColour = accentAmber;

    statusLabel.setColour(juce::Label::textColourId, statusColour);
    repaint();
    layersLabel.setText("Layers: " + juce::String(audioProcessor.getLayerCount()),
                        juce::dontSendNotification);

    loopProgress = audioProcessor.getProgress();

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto inputPeakDb = juce::Decibels::gainToDecibels(
            audioProcessor.consumeInputPeak(channel), -60.0f);
        const auto masterPeakDb = juce::Decibels::gainToDecibels(
            audioProcessor.consumeMasterPeak(channel), -60.0f);

        inputMeterDb[static_cast<size_t>(channel)] =
            juce::jmax(inputPeakDb,
                       inputMeterDb[static_cast<size_t>(channel)] - 1.5f);
        masterMeterDb[static_cast<size_t>(channel)] =
            juce::jmax(masterPeakDb,
                       masterMeterDb[static_cast<size_t>(channel)] - 1.5f);
    }

    timeLabel.setText(juce::String(audioProcessor.getCurrentTime(), 2)
                          + " / "
                          + juce::String(audioProcessor.getLoopLength(), 2)
                          + " s",
                      juce::dontSendNotification);

    const auto selectedTriggerMode = triggerModeBox.getSelectedId();
    const auto usesInputTrigger = selectedTriggerMode == 2
                                  || selectedTriggerMode == 3;
    thresholdLabel.setEnabled(usesInputTrigger);
    thresholdSlider.setEnabled(usesInputTrigger);
}

void DinLooperAudioProcessorEditor::updateLayerControls()
{
    constexpr int rowHeight = 36;
    constexpr int contentWidth = 410;
    int visibleRow = 0;
    const auto storedLayers = audioProcessor.getStoredLayerCount();

    for (int layer = 0; layer < LooperEngine::maximumLayers; ++layer)
    {
        const auto index = static_cast<size_t>(layer);
        const auto active = layer < storedLayers
                            && audioProcessor.isLayerActive(layer);

        layerLabels[index].setVisible(active);
        layerMeters[index].setVisible(active);
        layerVolumeSliders[index].setVisible(active);
        layerMuteButtons[index].setVisible(active);
        layerSoloButtons[index].setVisible(active);
        layerDeleteButtons[index].setVisible(active);

        if (!active)
        {
            layerMeterLevels[index] = 0.0f;
            continue;
        }

        const auto y = visibleRow * rowHeight;
        layerLabels[index].setBounds(0, y + 4, 30, 26);
        layerMeters[index].setBounds(32, y + 9, 55, 16);
        layerVolumeSliders[index].setBounds(92, y + 4, 190, 26);
        layerMuteButtons[index].setBounds(287, y + 4, 38, 26);
        layerSoloButtons[index].setBounds(326, y + 4, 38, 26);
        layerDeleteButtons[index].setBounds(372, y + 5, 28, 24);

        const auto peak = audioProcessor.consumeLayerPeak(layer);
        layerMeterLevels[index] =
            juce::jmax(peak, layerMeterLevels[index] * 0.82f);
        layerMeters[index].setValue(layerMeterLevels[index],
                                    juce::dontSendNotification);
        layerVolumeSliders[index].setValue(
            juce::Decibels::gainToDecibels(
                audioProcessor.getLayerVolume(layer), -60.0f),
            juce::dontSendNotification);
        layerMuteButtons[index].setToggleState(
            audioProcessor.isLayerMuted(layer),
            juce::dontSendNotification);
        layerSoloButtons[index].setToggleState(
            audioProcessor.isLayerSoloed(layer),
            juce::dontSendNotification);
        ++visibleRow;
    }

    layerControls.setSize(contentWidth,
                          juce::jmax(130, visibleRow * rowHeight));
}

//==============================================================================
void DinLooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto scale = static_cast<float>(getWidth()) / designWidth;
    g.addTransform(juce::AffineTransform::scale(scale));

    g.setGradientFill(juce::ColourGradient(backgroundTop,
                                           0.0f,
                                           0.0f,
                                           backgroundBottom,
                                           0.0f,
                                           static_cast<float>(designHeight),
                                           false));
    g.fillAll();

    const auto drawPanel = [&g](juce::Rectangle<float> bounds)
    {
        g.setColour(panelColour);
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(panelBorder);
        g.drawRoundedRectangle(bounds, 10.0f, 1.0f);
    };

    drawPanel({ 18.0f, 86.0f, static_cast<float>(designWidth - 36), 54.0f });
    drawPanel({ 105.0f, 150.0f, 430.0f, 96.0f });
    drawPanel({ 105.0f, 252.0f, 430.0f, 43.0f });
    drawPanel({ 105.0f, 305.0f, 430.0f, 176.0f });
    drawPanel({ 18.0f, 150.0f, 76.0f, 331.0f });
    drawPanel({ 548.0f, 150.0f, 134.0f, 331.0f });

    const auto statusBounds = juce::Rectangle<float>(
        static_cast<float>((designWidth - 230) / 2),
        57.0f,
        230.0f,
        26.0f);
    const auto statusColour =
        statusLabel.findColour(juce::Label::textColourId);

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillRoundedRectangle(statusBounds.translated(0.0f, 2.0f), 6.0f);
    g.setColour(backgroundTop);
    g.fillRoundedRectangle(statusBounds, 6.0f);
    g.setColour(statusColour.withAlpha(0.65f));
    g.drawRoundedRectangle(statusBounds, 6.0f, 1.0f);
    g.setColour(statusColour.withAlpha(0.18f));
    g.fillEllipse(statusBounds.getX() + 10.0f,
                  statusBounds.getCentreY() - 6.0f,
                  12.0f,
                  12.0f);
    g.setColour(statusColour);
    g.fillEllipse(statusBounds.getX() + 13.0f,
                  statusBounds.getCentreY() - 3.0f,
                  6.0f,
                  6.0f);

    const auto progressBounds = juce::Rectangle<float>(
        125.0f, 190.0f, 390.0f, 24.0f);
    const auto progressAmount = static_cast<float>(
        juce::jlimit(0.0, 1.0, loopProgress));

    g.setColour(backgroundTop);
    g.fillRoundedRectangle(progressBounds, 12.0f);

    if (progressAmount > 0.0f)
    {
        auto filledBounds = progressBounds;
        filledBounds.setWidth(progressBounds.getWidth() * progressAmount);
        g.setColour(accentBlue);
        g.fillRoundedRectangle(filledBounds, 12.0f);
    }

    const auto percentageBounds =
        progressBounds.withSizeKeepingCentre(54.0f, 18.0f);
    g.setColour(backgroundTop.withAlpha(0.82f));
    g.fillRoundedRectangle(percentageBounds, 9.0f);
    g.setColour(primaryText);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText(juce::String(juce::roundToInt(progressAmount * 100.0f)) + "%",
               percentageBounds,
               juce::Justification::centred);

    const auto drawVerticalMeter = [&g](juce::Rectangle<float> barBounds,
                                        float levelDb)
    {
        const auto normalisedLevel = juce::jlimit(
            0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

        g.setColour(backgroundTop);
        g.fillRoundedRectangle(barBounds, 3.0f);

        auto meterColour = accentGreen;

        if (levelDb > -3.0f)
            meterColour = accentRed;
        else if (levelDb > -12.0f)
            meterColour = accentAmber;

        if (normalisedLevel > 0.0f)
        {
            auto levelBounds = barBounds;
            const auto levelHeight = barBounds.getHeight() * normalisedLevel;
            levelBounds.setY(barBounds.getBottom() - levelHeight);
            levelBounds.setHeight(levelHeight);
            g.setColour(meterColour);
            g.fillRoundedRectangle(levelBounds, 3.0f);
        }
    };

    const juce::Rectangle<float> inputLeft(35.0f, 181.0f, 17.0f, 247.0f);
    const juce::Rectangle<float> inputRight(54.0f, 181.0f, 17.0f, 247.0f);
    const juce::Rectangle<float> outputLeft(619.0f, 181.0f, 17.0f, 247.0f);
    const juce::Rectangle<float> outputRight(638.0f, 181.0f, 17.0f, 247.0f);

    drawVerticalMeter(inputLeft, inputMeterDb[0]);
    drawVerticalMeter(inputRight, inputMeterDb[1]);
    drawVerticalMeter(outputLeft, masterMeterDb[0]);
    drawVerticalMeter(outputRight, masterMeterDb[1]);

    const auto thresholdDb = static_cast<float>(thresholdSlider.getValue());
    const auto thresholdAmount = juce::jlimit(
        0.0f, 1.0f, (thresholdDb + 60.0f) / 60.0f);
    const auto thresholdY = inputLeft.getBottom()
                            - inputLeft.getHeight() * thresholdAmount;

    if (thresholdSlider.isEnabled())
    {
        g.setColour(accentBlue);
        g.fillRoundedRectangle(29.0f, thresholdY - 1.5f, 48.0f, 3.0f, 1.5f);
        juce::Path thresholdMarker;
        thresholdMarker.addTriangle(76.0f, thresholdY,
                                    81.0f, thresholdY - 4.0f,
                                    81.0f, thresholdY + 4.0f);
        g.fillPath(thresholdMarker);
    }

    g.setColour(secondaryText);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("L R", 34, 432, 38, 14, juce::Justification::centred);
    g.drawText("L R", 618, 432, 38, 14, juce::Justification::centred);
    g.setColour(thresholdSlider.isEnabled() ? accentBlue : secondaryText);
    g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
    g.drawText("THRESHOLD",
               23, 445, 66, 12, juce::Justification::centred);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText(juce::String(thresholdDb, 1) + " dB",
               23, 457, 66, 14, juce::Justification::centred);

    g.setColour(accentBlue.withAlpha(0.7f));
    g.fillRoundedRectangle(300.0f, 52.0f, 100.0f, 2.0f, 1.0f);

    g.setColour(secondaryText.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("LIVE LOOP STATION",
               0,
               495,
               designWidth,
               16,
               juce::Justification::centred);
}

void DinLooperAudioProcessorEditor::resized()
{
    content.setTransform(juce::AffineTransform());
    content.setBounds(0, 0, designWidth, designHeight);
    const auto scale = static_cast<float>(getWidth()) / designWidth;
    content.setTransform(juce::AffineTransform::scale(scale));

    titleLabel.setBounds(0, 15, designWidth, 35);
    statusLabel.setBounds((designWidth - 196) / 2 + 12, 57, 196, 26);

    const int buttonW = 76;
    const int buttonH = 35;
    const int gap = 6;

    int totalWidth = buttonW * 8 + gap * 7;
    int x = (designWidth - totalWidth) / 2;
    int y = 95;

    recButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    recSustainButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    playButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    stopButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    rewindButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    undoButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    redoButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    resetButton.setBounds(x, y, buttonW, buttonH);

    progressLabel.setBounds(105, 160, 430, 25);

    triggerModeLabel.setBounds(120, 260, 85, 25);
    triggerModeBox.setBounds(210, 260, 145, 25);
    audioThruButton.setBounds(385, 260, 125, 25);
    thresholdLabel.setBounds(0, 0, 0, 0);
    thresholdSlider.setBounds(24, 177, 58, 255);

    timeLabel.setBounds(105, 220, 430, 25);

    inputMeterLabel.setBounds(22, 158, 68, 16);
    masterMeterLabel.setBounds(612, 158, 50, 16);
    masterVolumeLabel.setBounds(553, 158, 58, 16);
    masterVolumeSlider.setBounds(553, 177, 58, 291);

    layersLabel.setBounds(105, 309, 430, 22);
    layerViewport.setBounds(112, 333, 416, 140);
    updateLayerControls();
}
