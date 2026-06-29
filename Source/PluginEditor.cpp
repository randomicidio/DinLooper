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
    setSize(700, 420);
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
    statusLabel.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
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
    layersLabel.setText("Layers: " + juce::String(audioProcessor.getLayerCount()),
                        juce::dontSendNotification);

    loopProgress = audioProcessor.getProgress();
    repaint(100, 190, getWidth() - 200, 24);
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
    drawPanel({ 18.0f, 315.0f, static_cast<float>(getWidth() - 36), 58.0f });

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

    g.setColour(accentBlue.withAlpha(0.7f));
    g.fillRoundedRectangle(300.0f, 52.0f, 100.0f, 2.0f, 1.0f);

    g.setColour(secondaryText.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("LIVE LOOP STATION",
               0,
               390,
               getWidth(),
               16,
               juce::Justification::centred);
}

void DinLooperAudioProcessorEditor::resized()
{
    titleLabel.setBounds(0, 15, getWidth(), 35);
    statusLabel.setBounds(0, 55, getWidth(), 25);

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

    layersLabel.setBounds(0, 330, getWidth(), 25);
}
