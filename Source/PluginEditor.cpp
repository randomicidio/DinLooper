#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DinLooperAudioProcessorEditor::DinLooperAudioProcessorEditor(DinLooperAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(700, 420);

    // ===== Title =====
    titleLabel.setText("DinLooper", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(26.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    // ===== Status =====
    statusLabel.setText("IDLE", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // ===== Progress =====
    progressLabel.setText("Loop Progress", juce::dontSendNotification);
    progressLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(progressLabel);
    addAndMakeVisible(loopProgressBar);

    // ===== Time =====
    timeLabel.setText("00.00 / 00.00", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeLabel);

    // ===== Layers =====
    layersLabel.setText("Layers: 0", juce::dontSendNotification);
    layersLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(layersLabel);

    // ===== Trigger =====
    triggerModeLabel.setText("Trigger", juce::dontSendNotification);
    triggerModeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(triggerModeLabel);

    triggerModeBox.addItem("Instant", 1);
    triggerModeBox.addItem("Audio + MIDI", 2);
    triggerModeBox.addItem("Audio Only", 3);
    triggerModeBox.addItem("MIDI Only", 4);
    addAndMakeVisible(triggerModeBox);

    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(thresholdLabel);

    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
    thresholdSlider.setTextValueSuffix(" dB");
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
    statusLabel.setText(audioProcessor.getStateName(), juce::dontSendNotification);
    layersLabel.setText("Layers: " + juce::String(audioProcessor.getLayerCount()),
                        juce::dontSendNotification);

    loopProgress = audioProcessor.getProgress();
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
    g.fillAll(juce::Colour(25, 25, 25));

    g.setColour(juce::Colours::darkgrey);

    g.drawRect(getLocalBounds(), 2);

    g.drawLine(20, 150, getWidth() - 20, 150);
    g.drawLine(20, 300, getWidth() - 20, 300);
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
    loopProgressBar.setBounds(100, 190, getWidth() - 200, 24);

    triggerModeLabel.setBounds(90, 260, 110, 25);
    triggerModeBox.setBounds(205, 260, 150, 25);
    thresholdLabel.setBounds(365, 260, 100, 25);
    thresholdSlider.setBounds(470, 260, 150, 25);

    timeLabel.setBounds(0, 220, getWidth(), 25);

    layersLabel.setBounds(0, 330, getWidth(), 25);
}
