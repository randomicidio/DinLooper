/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::String recParameterID   { "rec" };
    const juce::String playParameterID  { "play" };
    const juce::String stopParameterID  { "stop" };
    const juce::String cancelParameterID { "cancel" };
    const juce::String undoParameterID  { "undo" };
    const juce::String redoParameterID  { "redo" };
    const juce::String resetParameterID { "reset" };
    const juce::String rewindParameterID { "rewind" };
    const juce::String recSustainParameterID { "rec_sustain" };
    const juce::String triggerModeParameterID { "trigger_mode" };
    const juce::String thresholdParameterID   { "threshold_db" };
    const juce::String masterGainParameterID  { "master_gain_db" };
    const juce::String audioThruParameterID   { "audio_thru" };
    const juce::String recCompensationParameterID {
        "rec_compensation_ms"
    };

    juce::String layerVolumeParameterID(int layer)
    {
        return "layer_" + juce::String(layer + 1) + "_volume_db";
    }

    juce::String layerMuteParameterID(int layer)
    {
        return "layer_" + juce::String(layer + 1) + "_mute";
    }

    juce::String layerSoloParameterID(int layer)
    {
        return "layer_" + juce::String(layer + 1) + "_solo";
    }
}

//==============================================================================
DinLooperAudioProcessor::DinLooperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#else
     : AudioProcessor()
#endif
     , parameters(*this, nullptr, "DinLooperParameters", createParameterLayout())
{
    parameters.addParameterListener(recParameterID, this);
    parameters.addParameterListener(playParameterID, this);
    parameters.addParameterListener(stopParameterID, this);
    parameters.addParameterListener(cancelParameterID, this);
    parameters.addParameterListener(undoParameterID, this);
    parameters.addParameterListener(redoParameterID, this);
    parameters.addParameterListener(resetParameterID, this);
    parameters.addParameterListener(rewindParameterID, this);
    parameters.addParameterListener(recSustainParameterID, this);

    triggerModeParameter = parameters.getRawParameterValue(triggerModeParameterID);
    thresholdParameter = parameters.getRawParameterValue(thresholdParameterID);
    masterGainParameter = parameters.getRawParameterValue(masterGainParameterID);
    audioThruParameter = parameters.getRawParameterValue(audioThruParameterID);
    recCompensationParameter =
        parameters.getRawParameterValue(recCompensationParameterID);

    jassert(triggerModeParameter != nullptr);
    jassert(thresholdParameter != nullptr);
    jassert(masterGainParameter != nullptr);
    jassert(audioThruParameter != nullptr);
    jassert(recCompensationParameter != nullptr);

    for (int layer = 0; layer < LooperEngine::maximumLayers; ++layer)
    {
        const auto index = static_cast<size_t>(layer);
        layerVolumeParameters[index] =
            parameters.getRawParameterValue(layerVolumeParameterID(layer));
        layerMuteParameters[index] =
            parameters.getRawParameterValue(layerMuteParameterID(layer));
        layerSoloParameters[index] =
            parameters.getRawParameterValue(layerSoloParameterID(layer));

        jassert(layerVolumeParameters[index] != nullptr);
        jassert(layerMuteParameters[index] != nullptr);
        jassert(layerSoloParameters[index] != nullptr);
    }
}

DinLooperAudioProcessor::~DinLooperAudioProcessor()
{
    parameters.removeParameterListener(recParameterID, this);
    parameters.removeParameterListener(playParameterID, this);
    parameters.removeParameterListener(stopParameterID, this);
    parameters.removeParameterListener(cancelParameterID, this);
    parameters.removeParameterListener(undoParameterID, this);
    parameters.removeParameterListener(redoParameterID, this);
    parameters.removeParameterListener(resetParameterID, this);
    parameters.removeParameterListener(rewindParameterID, this);
    parameters.removeParameterListener(recSustainParameterID, this);
}

//==============================================================================
const juce::String DinLooperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DinLooperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DinLooperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DinLooperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DinLooperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DinLooperAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DinLooperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DinLooperAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DinLooperAudioProcessor::getProgramName (int index)
{
    return {};
}

void DinLooperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DinLooperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    looper.prepareToPlay(sampleRate, getTotalNumInputChannels());
    audioThruBuffer.setSize(getTotalNumInputChannels(),
                            samplesPerBlock,
                            false,
                            false,
                            true);
    masterGain.reset(sampleRate, 0.02);
    masterGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        masterGainParameter->load(std::memory_order_relaxed)));

    for (auto& peak : inputPeaks)
        peak = 0.0f;

    for (auto& peak : masterPeaks)
        peak = 0.0f;
}

void DinLooperAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DinLooperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DinLooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();

    const auto stateAtBlockStart = looper.getState();
    const auto isRecording =
        stateAtBlockStart == LooperEngine::State::RecordingFirstLoop
        || stateAtBlockStart == LooperEngine::State::Overdubbing;

    if (isRecording)
    {
        const auto commandsBeforeFinish =
            pendingCommands.fetch_and(~static_cast<unsigned int>(recCommand),
                                      std::memory_order_acq_rel);

        if ((commandsBeforeFinish & recCommand) != 0)
            looper.pressRecWithEndCompensation(
                recCompensationParameter->load(std::memory_order_relaxed)
                    / 1000.0);
    }

    processPendingCommands();

    const auto audioThruEnabled =
        audioThruParameter->load(std::memory_order_relaxed) >= 0.5f;

    if (audioThruEnabled)
    {
        jassert(audioThruBuffer.getNumSamples() >= numSamples);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            audioThruBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
    }

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto peak = channel < buffer.getNumChannels()
            ? buffer.getMagnitude(channel, 0, numSamples)
            : 0.0f;
        publishPeak(inputPeaks[static_cast<size_t>(channel)], peak);
    }

    for (int layer = 0; layer < LooperEngine::maximumLayers; ++layer)
    {
        const auto index = static_cast<size_t>(layer);
        looper.setLayerVolume(
            layer,
            juce::Decibels::decibelsToGain(
                layerVolumeParameters[index]->load(
                    std::memory_order_relaxed),
                -60.0f));
        looper.setLayerMuted(
            layer,
            layerMuteParameters[index]->load(
                std::memory_order_relaxed) >= 0.5f);
        looper.setLayerSoloed(
            layer,
            layerSoloParameters[index]->load(
                std::memory_order_relaxed) >= 0.5f);
    }

    const auto triggerSample = findInputTriggerSample(buffer, midiMessages);
    const auto sustainStopSample = findSustainPedalSample(midiMessages);

    if (triggerSample >= 0)
        looper.triggerRecording();

    looper.processBlock(buffer,
                        juce::jmax(0, triggerSample),
                        sustainStopSample);

    if (audioThruEnabled)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addFrom(channel,
                           0,
                           audioThruBuffer,
                           channel,
                           0,
                           numSamples);
    }

    masterGain.setTargetValue(juce::Decibels::decibelsToGain(
        masterGainParameter->load(std::memory_order_relaxed)));
    masterGain.applyGain(buffer, numSamples);

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto peak = channel < buffer.getNumChannels()
            ? buffer.getMagnitude(channel, 0, numSamples)
            : 0.0f;
        publishPeak(masterPeaks[static_cast<size_t>(channel)], peak);
    }

    midiMessages.clear();
}

//==============================================================================
bool DinLooperAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DinLooperAudioProcessor::createEditor()
{
    return new DinLooperAudioProcessorEditor (*this);
}

//==============================================================================
void DinLooperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void DinLooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary(data, sizeInBytes))
    {
        if (state->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*state));
    }
}

//==============================================================================
void DinLooperAudioProcessor::pressRec()
{
    triggerParameter(recParameterID);
}

void DinLooperAudioProcessor::pressPlay()
{
    triggerParameter(playParameterID);
}

void DinLooperAudioProcessor::pressStop()
{
    triggerParameter(stopParameterID);
}

void DinLooperAudioProcessor::pressCancel()
{
    triggerParameter(cancelParameterID);
}

void DinLooperAudioProcessor::pressUndo()
{
    triggerParameter(undoParameterID);
}

void DinLooperAudioProcessor::pressRedo()
{
    triggerParameter(redoParameterID);
}

void DinLooperAudioProcessor::pressReset()
{
    triggerParameter(resetParameterID);
}

void DinLooperAudioProcessor::pressRewind()
{
    triggerParameter(rewindParameterID);
}

void DinLooperAudioProcessor::pressRecSustain()
{
    triggerParameter(recSustainParameterID);
}

juce::String DinLooperAudioProcessor::getStateName() const
{
    return looper.getStateName();
}

LooperEngine::State DinLooperAudioProcessor::getState() const
{
    return looper.getState();
}

int DinLooperAudioProcessor::getLayerCount() const
{
    return looper.getLayerCount();
}

int DinLooperAudioProcessor::getStoredLayerCount() const
{
    return looper.getStoredLayerCount();
}

bool DinLooperAudioProcessor::isLayerActive(int layer) const
{
    return looper.isLayerActive(layer);
}

int DinLooperAudioProcessor::getLayerNumber(int layer) const
{
    return looper.getLayerNumber(layer);
}

float DinLooperAudioProcessor::getLayerVolume(int layer) const
{
    return looper.getLayerVolume(layer);
}

bool DinLooperAudioProcessor::isLayerMuted(int layer) const
{
    return looper.isLayerMuted(layer);
}

bool DinLooperAudioProcessor::isLayerSoloed(int layer) const
{
    return looper.isLayerSoloed(layer);
}

float DinLooperAudioProcessor::consumeLayerPeak(int layer)
{
    return looper.consumeLayerPeak(layer);
}

bool DinLooperAudioProcessor::consumeMaximumLayersNotice()
{
    return looper.consumeMaximumLayersNotice();
}

float DinLooperAudioProcessor::getWaveformPeak(int index) const
{
    return looper.getWaveformPeak(index);
}

void DinLooperAudioProcessor::setLayerVolume(int layer, float gain)
{
    looper.setLayerVolume(layer, gain);
}

void DinLooperAudioProcessor::setLayerMuted(int layer, bool muted)
{
    looper.setLayerMuted(layer, muted);
}

void DinLooperAudioProcessor::setLayerSoloed(int layer, bool soloed)
{
    looper.setLayerSoloed(layer, soloed);
}

void DinLooperAudioProcessor::deleteLayer(int layer)
{
    looper.deleteLayer(layer);
}

float DinLooperAudioProcessor::getProgress() const
{
    return looper.getProgress();
}

float DinLooperAudioProcessor::getLoopLength() const
{
    return looper.getLoopLength();
}

float DinLooperAudioProcessor::getCurrentTime() const
{
    return looper.getCurrentTime();
}

float DinLooperAudioProcessor::getCropStart() const
{
    return looper.getCropStart();
}

float DinLooperAudioProcessor::getCropEnd() const
{
    return looper.getCropEnd();
}

void DinLooperAudioProcessor::setCropRange(float start, float end)
{
    looper.setCropRange(start, end);
}

juce::AudioProcessorValueTreeState& DinLooperAudioProcessor::getParameters()
{
    return parameters;
}

float DinLooperAudioProcessor::consumeInputPeak(int channel)
{
    return inputPeaks[static_cast<size_t>(juce::jlimit(0, 1, channel))]
        .exchange(0.0f, std::memory_order_acq_rel);
}

float DinLooperAudioProcessor::consumeMasterPeak(int channel)
{
    return masterPeaks[static_cast<size_t>(juce::jlimit(0, 1, channel))]
        .exchange(0.0f, std::memory_order_acq_rel);
}

juce::AudioProcessorValueTreeState::ParameterLayout DinLooperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { recParameterID, 1 }, "REC", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { playParameterID, 1 }, "PLAY", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { stopParameterID, 1 }, "STOP", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { undoParameterID, 1 }, "UNDO", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { redoParameterID, 1 }, "REDO", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { resetParameterID, 1 }, "RESET", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { rewindParameterID, 1 }, "REWIND", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { recSustainParameterID, 1 },
        "REC Pedal",
        false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { triggerModeParameterID, 1 },
        "Trigger Mode",
        juce::StringArray {
            "Instant",
            "Audio + MIDI",
            "Audio Only",
            "MIDI Only"
        },
        1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { thresholdParameterID, 1 },
        "Audio Threshold",
        juce::NormalisableRange<float> { -60.0f, 0.0f, 0.1f },
        -36.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    auto masterGainRange =
        juce::NormalisableRange<float> { -60.0f, 6.0f, 0.1f };
    masterGainRange.setSkewForCentre(-12.0f);

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { masterGainParameterID, 1 },
        "Master Volume",
        masterGainRange,
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { audioThruParameterID, 1 },
        "Audio Thru",
        false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { recCompensationParameterID, 1 },
        "REC Compensation",
        juce::NormalisableRange<float> { 0.0f, 250.0f, 1.0f },
        70.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    for (int layer = 0; layer < LooperEngine::maximumLayers; ++layer)
    {
        auto volumeRange =
            juce::NormalisableRange<float> { -60.0f, 6.0f, 0.1f };
        volumeRange.setSkewForCentre(-12.0f);
        const auto layerName = "Layer Slot " + juce::String(layer + 1);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { layerVolumeParameterID(layer), 1 },
            layerName + " Volume",
            volumeRange,
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { layerMuteParameterID(layer), 1 },
            layerName + " Mute",
            false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { layerSoloParameterID(layer), 1 },
            layerName + " Solo",
            false));
    }

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { cancelParameterID, 1 }, "CANCEL", false));

    return layout;
}

void DinLooperAudioProcessor::parameterChanged(const juce::String& parameterID,
                                               float newValue)
{
    if (newValue < 0.5f)
        return;

    unsigned int command = 0;

    if (parameterID == recParameterID)          command = recCommand;
    else if (parameterID == playParameterID)    command = playCommand;
    else if (parameterID == stopParameterID)    command = stopCommand;
    else if (parameterID == cancelParameterID)  command = cancelCommand;
    else if (parameterID == undoParameterID)    command = undoCommand;
    else if (parameterID == redoParameterID)    command = redoCommand;
    else if (parameterID == resetParameterID)   command = resetCommand;
    else if (parameterID == rewindParameterID)  command = rewindCommand;
    else if (parameterID == recSustainParameterID)
        command = recSustainCommand;

    pendingCommands.fetch_or(command, std::memory_order_release);
}

void DinLooperAudioProcessor::triggerParameter(const juce::String& parameterID)
{
    if (auto* parameter = parameters.getParameter(parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(1.0f);
        parameter->setValueNotifyingHost(0.0f);
        parameter->endChangeGesture();
    }
}

void DinLooperAudioProcessor::processPendingCommands()
{
    const auto commands = pendingCommands.exchange(0, std::memory_order_acquire);

    if ((commands & resetCommand) != 0) looper.pressReset();
    if ((commands & stopCommand) != 0)  looper.pressStop();
    if ((commands & cancelCommand) != 0) looper.pressCancel();
    if ((commands & undoCommand) != 0)  looper.pressUndo();
    if ((commands & redoCommand) != 0)  looper.pressRedo();
    if ((commands & rewindCommand) != 0) looper.pressRewind();
    if ((commands & playCommand) != 0)  looper.pressPlay();
    if ((commands & recSustainCommand) != 0)
    {
        const auto wasIdle = looper.getState() == LooperEngine::State::Idle;
        looper.pressRecSustain();

        if (wasIdle && triggerModeParameter->load(std::memory_order_relaxed) < 0.5f)
            looper.triggerRecording();
    }
    if ((commands & recCommand) != 0)
    {
        const auto wasIdle = looper.getState() == LooperEngine::State::Idle;

        if (looper.getState() == LooperEngine::State::RecordingFirstLoop)
            looper.pressRecWithEndCompensation(
                recCompensationParameter->load(std::memory_order_relaxed)
                    / 1000.0);
        else
            looper.pressRec();

        if (wasIdle && triggerModeParameter->load(std::memory_order_relaxed) < 0.5f)
            looper.triggerRecording();
    }
}

int DinLooperAudioProcessor::findInputTriggerSample(
    const juce::AudioBuffer<float>& buffer,
    const juce::MidiBuffer& midiMessages) const
{
    if (looper.getState() != LooperEngine::State::WaitingForInput
        || triggerModeParameter->load(std::memory_order_relaxed) < 0.5f)
    {
        return -1;
    }

    auto firstTriggerSample = buffer.getNumSamples();
    const auto triggerMode = juce::roundToInt(
        triggerModeParameter->load(std::memory_order_relaxed));
    const auto usesAudio = triggerMode == 1 || triggerMode == 2;
    const auto usesMidi = triggerMode == 1 || triggerMode == 3;
    const auto threshold = juce::Decibels::decibelsToGain(
        thresholdParameter->load(std::memory_order_relaxed));

    if (usesAudio)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto* samples = buffer.getReadPointer(channel);

            for (int sample = 0; sample < firstTriggerSample; ++sample)
            {
                if (std::abs(samples[sample]) >= threshold)
                {
                    firstTriggerSample = sample;
                    break;
                }
            }
        }
    }

    if (usesMidi)
    {
        for (const auto metadata : midiMessages)
        {
            if (metadata.getMessage().isNoteOn())
                firstTriggerSample = juce::jmin(firstTriggerSample,
                                                metadata.samplePosition);
        }
    }

    return firstTriggerSample < buffer.getNumSamples() ? firstTriggerSample : -1;
}

int DinLooperAudioProcessor::findSustainPedalSample(
    const juce::MidiBuffer& midiMessages) const
{
    if (!looper.isWaitingForSustain())
        return -1;

    for (const auto metadata : midiMessages)
    {
        const auto& message = metadata.getMessage();

        if (message.isController()
            && message.getControllerNumber() == 64)
        {
            return metadata.samplePosition;
        }
    }

    return -1;
}

void DinLooperAudioProcessor::publishPeak(std::atomic<float>& destination,
                                          float peak)
{
    auto currentPeak = destination.load(std::memory_order_relaxed);

    while (peak > currentPeak
           && !destination.compare_exchange_weak(currentPeak,
                                                 peak,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed))
    {
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DinLooperAudioProcessor();
}
