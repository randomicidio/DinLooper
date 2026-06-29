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
    int getStoredLayerCount() const;
    bool isLayerActive(int layer) const;
    int getLayerNumber(int layer) const;
    float getLayerVolume(int layer) const;
    bool isLayerMuted(int layer) const;
    bool isLayerSoloed(int layer) const;
    float consumeLayerPeak(int layer);
    void setLayerVolume(int layer, float gain);
    void setLayerMuted(int layer, bool muted);
    void setLayerSoloed(int layer, bool soloed);
    void deleteLayer(int layer);

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
    void activateLayer(int layer);
    void deactivateLayer(int layer);
    void pushHistory(int layer, bool activated);
    void discardHistoryForLayer(int layer);
    void processPendingLayerDeletes();
    static void publishPeak(std::atomic<float>& destination, float peak);

public:
    static constexpr int maximumLayers = 16;

private:
    static constexpr int maximumBufferSlots = maximumLayers + 2;

    struct LayerAction
    {
        int layer = -1;
        bool activated = false;
    };

    std::atomic<State> currentState{ State::Idle };

    std::atomic<int> layerCount{ 0 };

    std::atomic<float> progress{ 0.0f };

    std::atomic<float> loopLength{ 0.0f };

    std::array<std::unique_ptr<juce::AudioBuffer<float>>, maximumBufferSlots>
        ownedLayerBuffers;
    std::array<std::atomic<juce::AudioBuffer<float>*>, maximumBufferSlots>
        readyLayerBuffers;
    std::array<int, maximumLayers> layerBufferSlots{};
    std::array<std::atomic<bool>, maximumLayers> layerActive{};
    std::array<std::atomic<int>, maximumLayers> layerNumbers{};
    std::array<std::atomic<float>, maximumLayers> layerVolumes{};
    std::array<std::atomic<bool>, maximumLayers> layerMutes{};
    std::array<std::atomic<bool>, maximumLayers> layerSolos{};
    std::array<std::atomic<float>, maximumLayers> layerPeaks{};
    std::array<float, maximumLayers> currentLayerGains{};
    std::array<LayerAction, 128> undoHistory{};
    std::array<LayerAction, 128> redoHistory{};
    int undoHistorySize = 0;
    int redoHistorySize = 0;
    std::atomic<unsigned int> pendingLayerDeletes{ 0 };
    int nextLayerNumber = 1;

    double currentSampleRate = 44100.0;
    int recordedSamples = 0;
    int playbackPosition = 0;
    int maximumLoopSamples = 0;
    std::atomic<int> storedLayerCount{ 0 };
    int recordingLayerIndex = -1;
    int recordingLogicalLayerIndex = -1;
    std::array<std::atomic<int>, 2> spareLayerSlots;
    std::atomic<bool> overdubStartRequested{ false };
    std::atomic<bool> sustainFinishRequested{ false };
    std::atomic<bool> sustainOnNextRecording{ false };
    int overdubSamplesWritten = 0;
    std::atomic<int> requestedLayerIndex{ -1 };
    std::atomic<int> allocationLengthSamples{ 0 };
    int preparedChannelCount = 0;
};
