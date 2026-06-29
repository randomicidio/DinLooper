#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DinLooperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    DinLooperAudioProcessorEditor(DinLooperAudioProcessor&);
    ~DinLooperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    static constexpr int designWidth = 700;
    static constexpr int designHeight = 520;
    static constexpr int waveformPoints = 256;

    void timerCallback() override;
    void updateLooperStatus();
    void updateLayerControls();

    DinLooperAudioProcessor& audioProcessor;
    juce::Component content;

    // ===== Botões =====
    juce::TextButton recButton{ "REC" };
    juce::TextButton playButton{ "PLAY" };
    juce::TextButton stopButton{ "STOP" };
    juce::TextButton cancelButton{ "CANCEL" };
    juce::TextButton undoButton{ "UNDO" };
    juce::TextButton redoButton{ "REDO" };
    juce::TextButton resetButton{ "RESET" };
    juce::TextButton rewindButton{ "REWIND" };
    juce::TextButton recSustainButton{ "REC PEDAL" };

    // ===== Labels =====
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label progressLabel;
    juce::Label timeLabel;
    juce::Label layersLabel;
    juce::Label triggerModeLabel;
    juce::Label thresholdLabel;
    juce::Label inputMeterLabel;
    juce::Label masterMeterLabel;
    juce::Label masterVolumeLabel;
    juce::Label pitchLabel;
    juce::Label recCompensationLabel;

    double loopProgress = 0.0;
    float cropStart = 0.0f;
    float cropEnd = 1.0f;
    enum class CropHandle { none, start, end };
    CropHandle draggedCropHandle = CropHandle::none;
    std::uint32_t maximumLayersNoticeUntil = 0;

    juce::ComboBox triggerModeBox;
    juce::ToggleButton audioThruButton{ "Audio Thru" };
    juce::ToggleButton pitchEnabledButton{ "ON" };
    juce::Slider thresholdSlider;
    juce::Slider masterVolumeSlider;
    juce::Slider pitchSlider;
    juce::Slider recCompensationSlider;
    juce::Viewport layerViewport;
    juce::Component layerControls;
    std::array<juce::Label, LooperEngine::maximumLayers> layerLabels;
    std::array<juce::Slider, LooperEngine::maximumLayers> layerMeters;
    std::array<juce::Slider, LooperEngine::maximumLayers> layerVolumeSliders;
    std::array<juce::ToggleButton, LooperEngine::maximumLayers> layerMuteButtons;
    std::array<juce::ToggleButton, LooperEngine::maximumLayers> layerSoloButtons;
    std::array<juce::TextButton, LooperEngine::maximumLayers> layerDeleteButtons;
    std::array<float, LooperEngine::maximumLayers> layerMeterLevels{};

    std::array<float, 2> inputMeterDb{ -60.0f, -60.0f };
    std::array<float, 2> masterMeterDb{ -60.0f, -60.0f };
    std::array<float, waveformPoints> waveformPeaks{};

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        triggerModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        masterVolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        pitchEnabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        recCompensationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        audioThruAttachment;
    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        LooperEngine::maximumLayers> layerVolumeAttachments;
    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,
        LooperEngine::maximumLayers> layerMuteAttachments;
    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,
        LooperEngine::maximumLayers> layerSoloAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DinLooperAudioProcessorEditor)
};
