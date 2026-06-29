/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LooperEngine.h"

//==============================================================================
class DinLooperAudioProcessor : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    DinLooperAudioProcessor();
    ~DinLooperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Looper API
    //==========================================================================

    void pressRec();
    void pressPlay();
    void pressStop();
    void pressCancel();
    void pressUndo();
    void pressRedo();
    void pressReset();
    void pressRewind();
    void pressRecSustain();

    juce::String getStateName() const;

    LooperEngine::State getState() const;

    int getLayerCount() const;
    int getStoredLayerCount() const;
    bool isLayerActive(int layer) const;
    int getLayerNumber(int layer) const;
    float getLayerVolume(int layer) const;
    bool isLayerMuted(int layer) const;
    bool isLayerSoloed(int layer) const;
    float consumeLayerPeak(int layer);
    bool consumeMaximumLayersNotice();
    float getWaveformPeak(int index) const;
    void setLayerVolume(int layer, float gain);
    void setLayerMuted(int layer, bool muted);
    void setLayerSoloed(int layer, bool soloed);
    void deleteLayer(int layer);

    float getProgress() const;
    float getLoopLength() const;
    float getCurrentTime() const;
    float getCropStart() const;
    float getCropEnd() const;
    void setCropRange(float start, float end);

    juce::AudioProcessorValueTreeState& getParameters();
    float consumeInputPeak(int channel);
    float consumeMasterPeak(int channel);

private:
    enum Command : unsigned int
    {
        recCommand   = 1u << 0,
        playCommand  = 1u << 1,
        stopCommand  = 1u << 2,
        undoCommand  = 1u << 3,
        redoCommand  = 1u << 4,
        resetCommand  = 1u << 5,
        rewindCommand    = 1u << 6,
        recSustainCommand = 1u << 7,
        cancelCommand = 1u << 8
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void triggerParameter(const juce::String& parameterID);
    void processPendingCommands();
    int findInputTriggerSample(const juce::AudioBuffer<float>&,
                               const juce::MidiBuffer&) const;
    int findSustainPedalSample(const juce::MidiBuffer&) const;
    static void publishPeak(std::atomic<float>& destination, float peak);

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* triggerModeParameter = nullptr;
    std::atomic<float>* thresholdParameter = nullptr;
    std::atomic<float>* masterGainParameter = nullptr;
    std::atomic<float>* audioThruParameter = nullptr;
    std::atomic<float>* recCompensationParameter = nullptr;
    std::array<std::atomic<float>*, LooperEngine::maximumLayers>
        layerVolumeParameters{};
    std::array<std::atomic<float>*, LooperEngine::maximumLayers>
        layerMuteParameters{};
    std::array<std::atomic<float>*, LooperEngine::maximumLayers>
        layerSoloParameters{};
    juce::SmoothedValue<float> masterGain;
    juce::AudioBuffer<float> audioThruBuffer;
    std::array<std::atomic<float>, 2> inputPeaks{};
    std::array<std::atomic<float>, 2> masterPeaks{};
    std::atomic<unsigned int> pendingCommands{ 0 };

    LooperEngine looper;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DinLooperAudioProcessor)
};
