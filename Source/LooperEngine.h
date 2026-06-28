#pragma once

#include <JuceHeader.h>

class LooperEngine : private juce::Thread
{
public:

    enum class State
    {
        Idle,
        WaitingForInput,
        RecordingFirstLoop,
        Playing,
        Overdubbing,
        Stopped
    };

    LooperEngine();
    ~LooperEngine() override;

    void prepareToPlay(double sampleRate, int numChannels);
    void processBlock(juce::AudioBuffer<float>& buffer,
                      int recordingStartSample,
                      int recordingStopSample);

    // Botões
    void pressRec();
    void pressPlay();
    void pressStop();
    void pressUndo();
    void pressRedo();
    void pressReset();
    void pressRewind();
    void pressRecSustain();
    void triggerRecording();

    // Consulta
    State getState() const;
    juce::String getStateName() const;

    int getLayerCount() const;

    float getProgress() const;
    void setProgress(float p);

    float getLoopLength() const;
    void setLoopLength(float seconds);
    float getCurrentTime() const;
    bool isWaitingForSustain() const;

private:
    void run() override;
    void requestNextLayerBuffer();
    void startPendingOverdubIfReady();
    void applyBoundaryFade(juce::AudioBuffer<float>&, int numSamples);

    static constexpr int maximumLayers = 16;


    std::atomic<State> currentState{ State::Idle };

    std::atomic<int> layerCount{ 0 };

    int undoCount = 0;

    std::atomic<float> progress{ 0.0f };

    std::atomic<float> loopLength{ 0.0f };

    std::array<std::unique_ptr<juce::AudioBuffer<float>>, maximumLayers>
        ownedLayerBuffers;
    std::array<std::atomic<juce::AudioBuffer<float>*>, maximumLayers>
        readyLayerBuffers;
    std::array<int, maximumLayers> layerBufferSlots{};

    double currentSampleRate = 44100.0;
    int recordedSamples = 0;
    int playbackPosition = 0;
    int maximumLoopSamples = 0;
    std::atomic<int> storedLayerCount{ 0 };
    int recordingLayerIndex = -1;
    std::array<std::atomic<int>, 2> spareLayerSlots;
    std::atomic<bool> overdubStartRequested{ false };
    std::atomic<bool> sustainFinishRequested{ false };
    std::atomic<bool> sustainOnNextRecording{ false };
    int overdubSamplesWritten = 0;
    std::atomic<int> requestedLayerIndex{ -1 };
    std::atomic<int> allocationLengthSamples{ 0 };
    int preparedChannelCount = 0;
};
