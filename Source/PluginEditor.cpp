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
    setSize(700, 520);
    setOpaque(true);

    // ===== Title =====
    titleLabel.setText("DinLooper", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, primaryText);
    addAndMakeVisible(titleLabel);

    // ===== Status =====
    statusLabel.setText("IDLE", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(juce::FontOptions(
        juce::Font::getDefaultMonospacedFontName(),
        14.0f,
        juce::Font::bold)));
    addAndMakeVisible(statusLabel);

    // ===== Progress =====
    progressLabel.setText("LOOP POSITION", juce::dontSendNotification);
    progressLabel.setJustificationType(juce::Justification::centred);
    progressLabel.setColour(juce::Label::textColourId, secondaryText);
    progressLabel.setFont(juce::Font(juce::FontOptions(12.0f,
                                                       juce::Font::bold)));
    addAndMakeVisible(progressLabel);

    // ===== Time =====
    timeLabel.setText("00.00 / 00.00", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setColour(juce::Label::textColourId, primaryText);
    addAndMakeVisible(timeLabel);

    // ===== Layers =====
    layersLabel.setText("Layers: 0", juce::dontSendNotification);
    layersLabel.setJustificationType(juce::Justification::centred);
    layersLabel.setColour(juce::Label::textColourId, primaryText);
    layersLabel.setFont(juce::Font(juce::FontOptions(16.0f,
                                                     juce::Font::bold)));
    addAndMakeVisible(layersLabel);

    // ===== Trigger =====
    triggerModeLabel.setText("Trigger", juce::dontSendNotification);
    triggerModeLabel.setJustificationType(juce::Justification::centredRight);
    triggerModeLabel.setColour(juce::Label::textColourId, secondaryText);
    addAndMakeVisible(triggerModeLabel);

    triggerModeBox.addItem("Instant", 1);
    triggerModeBox.addItem("Audio + MIDI", 2);
    triggerModeBox.addItem("Audio Only", 3);
    triggerModeBox.addItem("MIDI Only", 4);
    triggerModeBox.setColour(juce::ComboBox::backgroundColourId,
                             backgroundTop);
    triggerModeBox.setColour(juce::ComboBox::textColourId, primaryText);
    triggerModeBox.setColour(juce::ComboBox::outlineColourId, panelBorder);
    triggerModeBox.setColour(juce::ComboBox::arrowColourId, accentBlue);
    addAndMakeVisible(triggerModeBox);

    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centredRight);
    thresholdLabel.setColour(juce::Label::textColourId, secondaryText);
    addAndMakeVisible(thresholdLabel);

    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    thresholdSlider.setTextValueSuffix(" dB");
    thresholdSlider.setColour(juce::Slider::backgroundColourId,
                              backgroundTop);
    thresholdSlider.setColour(juce::Slider::trackColourId, accentBlue);
    thresholdSlider.setColour(juce::Slider::thumbColourId, primaryText);
    thresholdSlider.setColour(juce::Slider::textBoxTextColourId, primaryText);
    thresholdSlider.setColour(juce::Slider::textBoxBackgroundColourId,
                              backgroundTop);
    thresholdSlider.setColour(juce::Slider::textBoxOutlineColourId,
                              panelBorder);
    addAndMakeVisible(thresholdSlider);

    triggerModeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getParameters(), "trigger_mode", triggerModeBox);
    thresholdAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getParameters(), "threshold_db", thresholdSlider);

    // ===== Metering and master volume =====
    inputMeterLabel.setText("INPUT LEVEL", juce::dontSendNotification);
    inputMeterLabel.setJustificationType(juce::Justification::centred);
    inputMeterLabel.setColour(juce::Label::textColourId, secondaryText);
    inputMeterLabel.setFont(juce::Font(juce::FontOptions(11.0f,
                                                        juce::Font::bold)));
    addAndMakeVisible(inputMeterLabel);

    masterMeterLabel.setText("MASTER OUTPUT", juce::dontSendNotification);
    masterMeterLabel.setJustificationType(juce::Justification::centred);
    masterMeterLabel.setColour(juce::Label::textColourId, secondaryText);
    masterMeterLabel.setFont(juce::Font(juce::FontOptions(11.0f,
                                                         juce::Font::bold)));
    addAndMakeVisible(masterMeterLabel);

    masterVolumeLabel.setText("MASTER VOLUME", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType(juce::Justification::centred);
    masterVolumeLabel.setColour(juce::Label::textColourId, secondaryText);
    addAndMakeVisible(masterVolumeLabel);

    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight,
                                       false,
                                       68,
                                       22);
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
    addAndMakeVisible(masterVolumeSlider);

    masterVolumeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getParameters(),
            "master_gain_db",
            masterVolumeSlider);

    // ===== Buttons =====
    addAndMakeVisible(recButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(rewindButton);
    addAndMakeVisible(recSustainButton);

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
    repaint((getWidth() - 230) / 2, 57, 230, 26);
    layersLabel.setText("Layers: " + juce::String(audioProcessor.getLayerCount()),
                        juce::dontSendNotification);

    loopProgress = audioProcessor.getProgress();
    repaint(100, 190, getWidth() - 200, 24);

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

    repaint(18, 302, getWidth() - 36, 112);
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

//==============================================================================
void DinLooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(backgroundTop,
                                           0.0f,
                                           0.0f,
                                           backgroundBottom,
                                           0.0f,
                                           static_cast<float>(getHeight()),
                                           false));
    g.fillAll();

    const auto drawPanel = [&g](juce::Rectangle<float> bounds)
    {
        g.setColour(panelColour);
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(panelBorder);
        g.drawRoundedRectangle(bounds, 10.0f, 1.0f);
    };

    drawPanel({ 18.0f, 86.0f, static_cast<float>(getWidth() - 36), 54.0f });
    drawPanel({ 18.0f, 150.0f, static_cast<float>(getWidth() - 36), 96.0f });
    drawPanel({ 18.0f, 252.0f, static_cast<float>(getWidth() - 36), 43.0f });
    drawPanel({ 18.0f, 305.0f, static_cast<float>(getWidth() - 36), 110.0f });
    drawPanel({ 18.0f, 423.0f, static_cast<float>(getWidth() - 36), 58.0f });

    const auto statusBounds = juce::Rectangle<float>(
        static_cast<float>((getWidth() - 230) / 2),
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
        100.0f, 190.0f, static_cast<float>(getWidth() - 200), 24.0f);
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

    const auto drawMeter = [&g](float x,
                                float y,
                                float width,
                                float levelDb,
                                const juce::String& channelName)
    {
        const auto normalisedLevel = juce::jlimit(
            0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
        const auto barBounds = juce::Rectangle<float>(x + 18.0f,
                                                       y,
                                                       width,
                                                       14.0f);

        g.setColour(secondaryText);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(channelName,
                   static_cast<int>(x),
                   static_cast<int>(y),
                   14,
                   14,
                   juce::Justification::centred);

        g.setColour(backgroundTop);
        g.fillRoundedRectangle(barBounds, 7.0f);

        auto meterColour = accentGreen;

        if (levelDb > -3.0f)
            meterColour = accentRed;
        else if (levelDb > -12.0f)
            meterColour = accentAmber;

        if (normalisedLevel > 0.0f)
        {
            auto levelBounds = barBounds;
            levelBounds.setWidth(barBounds.getWidth() * normalisedLevel);
            g.setColour(meterColour);
            g.fillRoundedRectangle(levelBounds, 7.0f);
        }

        g.setColour(levelDb > -0.1f ? accentRed : primaryText);
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawText(juce::String(levelDb, 1) + " dB",
                   static_cast<int>(x + width + 25.0f),
                   static_cast<int>(y),
                   52,
                   14,
                   juce::Justification::centredRight);
    };

    drawMeter(34.0f, 339.0f, 130.0f, inputMeterDb[0], "L");
    drawMeter(34.0f, 365.0f, 130.0f, inputMeterDb[1], "R");
    drawMeter(254.0f, 339.0f, 130.0f, masterMeterDb[0], "L");
    drawMeter(254.0f, 365.0f, 130.0f, masterMeterDb[1], "R");

    g.setColour(accentBlue.withAlpha(0.7f));
    g.fillRoundedRectangle(300.0f, 52.0f, 100.0f, 2.0f, 1.0f);

    g.setColour(secondaryText.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("LIVE LOOP STATION",
               0,
               495,
               getWidth(),
               16,
               juce::Justification::centred);
}

void DinLooperAudioProcessorEditor::resized()
{
    titleLabel.setBounds(0, 15, getWidth(), 35);
    statusLabel.setBounds((getWidth() - 196) / 2 + 12, 57, 196, 26);

    const int buttonW = 76;
    const int buttonH = 35;
    const int gap = 6;

    int totalWidth = buttonW * 8 + gap * 7;
    int x = (getWidth() - totalWidth) / 2;
    int y = 95;

    recButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    recSustainButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    playButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    stopButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    rewindButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    undoButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    redoButton.setBounds(x, y, buttonW, buttonH); x += buttonW + gap;
    resetButton.setBounds(x, y, buttonW, buttonH);

    progressLabel.setBounds(0, 160, getWidth(), 25);

    triggerModeLabel.setBounds(90, 260, 110, 25);
    triggerModeBox.setBounds(205, 260, 150, 25);
    thresholdLabel.setBounds(365, 260, 100, 25);
    thresholdSlider.setBounds(470, 260, 150, 25);

    timeLabel.setBounds(0, 220, getWidth(), 25);

    inputMeterLabel.setBounds(34, 312, 205, 18);
    masterMeterLabel.setBounds(254, 312, 205, 18);
    masterVolumeLabel.setBounds(470, 312, 190, 18);
    masterVolumeSlider.setBounds(475, 350, 180, 28);

    layersLabel.setBounds(0, 440, getWidth(), 25);
}
