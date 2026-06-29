#include "LooperEngine.h"

LooperEngine::LooperEngine()
    : juce::Thread("DinLooper Layer Allocator")
{
    for (auto& buffer : readyLayerBuffers)
        buffer.store(nullptr, std::memory_order_relaxed);

    layerBufferSlots.fill(-1);

    for (auto& slot : spareLayerSlots)
        slot.store(-1, std::memory_order_relaxed);

    for (int layer = 0; layer < maximumLayers; ++layer)
    {
        layerActive[layer] = false;
        layerVolumes[layer] = 1.0f;
        layerMutes[layer] = false;
        layerSolos[layer] = false;
        layerPeaks[layer] = 0.0f;
        currentLayerGains[layer] = 1.0f;
    }
}

LooperEngine::~LooperEngine()
{
    stopThread(2000);
}

void LooperEngine::prepareToPlay(double sampleRate, int numChannels)
{
    constexpr double maximumLoopSeconds = 3.0 * 60.0;

    stopThread(2000);

    for (auto& buffer : readyLayerBuffers)
        buffer.store(nullptr, std::memory_order_relaxed);

    for (auto& buffer : ownedLayerBuffers)
        buffer.reset();

    currentSampleRate = sampleRate;
    preparedChannelCount = juce::jmax(1, numChannels);
    maximumLoopSamples = juce::roundToInt(sampleRate * maximumLoopSeconds);

    for (int layer = 0; layer < 3; ++layer)
    {
        ownedLayerBuffers[layer] =
            std::make_unique<juce::AudioBuffer<float>>(preparedChannelCount,
                                                       maximumLoopSamples);
        ownedLayerBuffers[layer]->clear();
        readyLayerBuffers[layer].store(ownedLayerBuffers[layer].get(),
                                       std::memory_order_release);
    }

    recordedSamples = 0;
    playbackPosition = 0;
    storedLayerCount = 0;
    recordingLayerIndex = -1;
    layerBufferSlots.fill(-1);
    layerBufferSlots[0] = 0;
    spareLayerSlots[0] = 1;
    spareLayerSlots[1] = 2;
    overdubStartRequested = false;
    sustainFinishRequested = false;
    sustainOnNextRecording = false;
    overdubSamplesWritten = 0;
    requestedLayerIndex = -1;
    undoHistorySize = 0;
    redoHistorySize = 0;
    pendingLayerDeletes = 0;
    layerCount = 0;
    progress = 0.0f;
    loopLength = 0.0f;
    currentState = State::Idle;

    for (int layer = 0; layer < maximumLayers; ++layer)
    {
        layerActive[layer] = false;
        layerVolumes[layer] = 1.0f;
        layerMutes[layer] = false;
        layerSolos[layer] = false;
        layerPeaks[layer] = 0.0f;
        currentLayerGains[layer] = 1.0f;
    }

    startThread(juce::Thread::Priority::low);
}

void LooperEngine::processBlock(juce::AudioBuffer<float>& buffer,
                                int recordingStartSample,
                                int recordingStopSample)
{
    const auto numSamples = buffer.getNumSamples();
    processPendingLayerDeletes();
    const auto hasRecordingStop = recordingStopSample >= 0
                                  && recordingStopSample < numSamples;
    const auto stopSample = hasRecordingStop
        ? juce::jlimit(0, numSamples, recordingStopSample)
        : numSamples;
    startPendingOverdubIfReady();

    if (recordedSamples > 0)
        requestNextLayerBuffer();

    const auto state = currentState.load(std::memory_order_relaxed);

    if (state == State::RecordingFirstLoop)
    {
        const auto firstSample = juce::jlimit(0, numSamples, recordingStartSample);
        const auto availableSamples = maximumLoopSamples - recordedSamples;
        const auto samplesToRecord = juce::jmin(juce::jmax(0,
                                                           stopSample - firstSample),
                                                availableSamples);
        auto* firstLayer = readyLayerBuffers[0].load(std::memory_order_acquire);
        const auto channelsToRecord = juce::jmin(buffer.getNumChannels(),
                                                 firstLayer->getNumChannels());

        for (int channel = 0; channel < channelsToRecord; ++channel)
        {
            firstLayer->copyFrom(channel,
                                 recordedSamples,
                                 buffer,
                                 channel,
                                 firstSample,
                                 samplesToRecord);
        }

        recordedSamples += samplesToRecord;
        loopLength = static_cast<float>(recordedSamples / currentSampleRate);
        buffer.clear();

        if (recordedSamples >= maximumLoopSamples)
        {
            applyBoundaryFade(*firstLayer, recordedSamples);
            storedLayerCount = 1;
            activateLayer(0);
            pushHistory(0, true);
            playbackPosition = 0;
            progress = 0.0f;
            currentState = State::Playing;
            requestNextLayerBuffer();
        }
        else if (hasRecordingStop && recordedSamples > 0)
        {
            sustainFinishRequested = false;
            pressRec();
        }

        return;
    }

    if ((state == State::Playing || state == State::Overdubbing)
        && recordedSamples > 0)
    {
        const auto storedLayers =
            storedLayerCount.load(std::memory_order_relaxed);
        const auto blockPlaybackStart = playbackPosition;
        auto* recordingLayer = recordingLayerIndex >= 0
            ? readyLayerBuffers[recordingLayerIndex].load(std::memory_order_acquire)
            : nullptr;

        bool anySolo = false;
        for (int layer = 0; layer < storedLayers; ++layer)
            anySolo = anySolo
                      || (layerActive[layer].load(std::memory_order_relaxed)
                          && layerSolos[layer].load(
                              std::memory_order_relaxed));

        std::array<float, maximumLayers> targetGains{};
        std::array<float, maximumLayers> gainSteps{};
        std::array<float, maximumLayers> blockPeaks{};

        for (int layer = 0; layer < storedLayers; ++layer)
        {
            const auto audible =
                layerActive[layer].load(std::memory_order_relaxed)
                && !layerMutes[layer].load(std::memory_order_relaxed)
                && (!anySolo
                    || layerSolos[layer].load(std::memory_order_relaxed));
            targetGains[layer] = audible
                ? layerVolumes[layer].load(std::memory_order_relaxed)
                : 0.0f;
            gainSteps[layer] =
                (targetGains[layer] - currentLayerGains[layer])
                / static_cast<float>(juce::jmax(1, numSamples));
        }

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* output = buffer.getWritePointer(channel);
            auto position = playbackPosition;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto input = output[sample];
                float mixedSample = 0.0f;

                for (int layer = 0; layer < storedLayers; ++layer)
                {
                    const auto slot = layerBufferSlots[layer];

                    if (slot >= 0)
                    {
                        if (auto* layerBuffer = readyLayerBuffers[slot].load(
                                std::memory_order_acquire))
                        {
                            if (channel < layerBuffer->getNumChannels())
                            {
                                const auto gain =
                                    currentLayerGains[layer]
                                    + gainSteps[layer]
                                          * static_cast<float>(sample + 1);
                                const auto layerSample =
                                    layerBuffer->getSample(channel, position)
                                    * gain;
                                mixedSample += layerSample;
                                blockPeaks[layer] =
                                    juce::jmax(blockPeaks[layer],
                                               std::abs(layerSample));
                            }
                        }
                    }
                }

                if (state == State::Overdubbing
                    && recordingLayer != nullptr
                    && channel < recordingLayer->getNumChannels()
                    && sample < stopSample)
                {
                    const auto positionWasRecorded =
                        overdubSamplesWritten + sample >= recordedSamples;

                    if (positionWasRecorded)
                        mixedSample += recordingLayer->getSample(channel, position);

                    recordingLayer->addSample(channel, position, input);
                }

                output[sample] = mixedSample;

                if (++position >= recordedSamples)
                    position = 0;
            }
        }

        for (int layer = 0; layer < storedLayers; ++layer)
        {
            currentLayerGains[layer] = targetGains[layer];
            publishPeak(layerPeaks[layer], blockPeaks[layer]);
        }

        if (state == State::Overdubbing)
            overdubSamplesWritten += stopSample;

        playbackPosition = (playbackPosition + numSamples) % recordedSamples;
        progress = static_cast<float>(playbackPosition)
                   / static_cast<float>(recordedSamples);

        if (state == State::Overdubbing && hasRecordingStop)
        {
            const auto continuingPlaybackPosition = playbackPosition;
            playbackPosition = (blockPlaybackStart + stopSample)
                               % recordedSamples;
            sustainFinishRequested = false;
            pressRec();
            playbackPosition = continuingPlaybackPosition;
            progress = static_cast<float>(playbackPosition)
                       / static_cast<float>(recordedSamples);
        }

        return;
    }

    buffer.clear();
}

void LooperEngine::run()
{
    while (!threadShouldExit())
    {
        auto layer = requestedLayerIndex.load(std::memory_order_acquire);

        if (layer > 0 && layer < maximumLayers)
        {
            const auto samples = allocationLengthSamples.load(std::memory_order_acquire);
            try
            {
                auto buffer = std::make_unique<juce::AudioBuffer<float>>(
                    preparedChannelCount, samples);
                buffer->clear();

                if (requestedLayerIndex.load(std::memory_order_acquire) == layer)
                {
                    auto* rawBuffer = buffer.get();
                    ownedLayerBuffers[layer] = std::move(buffer);
                    readyLayerBuffers[layer].store(rawBuffer,
                                                   std::memory_order_release);
                    requestedLayerIndex.compare_exchange_strong(
                        layer, -1, std::memory_order_release);
                }
            }
            catch (const std::bad_alloc&)
            {
                requestedLayerIndex.compare_exchange_strong(
                    layer, -1, std::memory_order_release);
                wait(100);
            }
        }
        else
        {
            wait(10);
        }
    }
}

void LooperEngine::requestNextLayerBuffer()
{
    const auto storedLayers = storedLayerCount.load(std::memory_order_acquire);

    if (storedLayers >= maximumLayers)
    {
        for (auto& slot : spareLayerSlots)
            slot.store(-1, std::memory_order_release);

        return;
    }

    for (auto& spare : spareLayerSlots)
    {
        auto spareSlot = spare.load(std::memory_order_acquire);

        if (spareSlot < 0)
        {
            for (int candidate = 1; candidate < maximumLayers; ++candidate)
            {
                bool isUsed = candidate == recordingLayerIndex;

                for (int layer = 0; layer < storedLayers; ++layer)
                    isUsed = isUsed || layerBufferSlots[layer] == candidate;

                for (const auto& otherSpare : spareLayerSlots)
                    isUsed = isUsed
                             || otherSpare.load(std::memory_order_acquire)
                                    == candidate;

                if (!isUsed)
                {
                    spareSlot = candidate;
                    spare.store(candidate, std::memory_order_release);
                    break;
                }
            }
        }

        if (spareSlot >= 0
            && readyLayerBuffers[spareSlot].load(std::memory_order_acquire)
                   == nullptr)
        {
            const auto state = currentState.load(std::memory_order_acquire);
            allocationLengthSamples =
                state == State::RecordingFirstLoop
                    ? maximumLoopSamples
                    : recordedSamples;
            auto noRequest = -1;

            requestedLayerIndex.compare_exchange_strong(
                noRequest, spareSlot, std::memory_order_release);
        }
    }
}

void LooperEngine::startPendingOverdubIfReady()
{
    if (!overdubStartRequested.load(std::memory_order_acquire)
        || currentState.load(std::memory_order_relaxed) != State::Playing)
    {
        return;
    }

    for (int reserve = 0; reserve < static_cast<int>(spareLayerSlots.size());
         ++reserve)
    {
        const auto spareSlot =
            spareLayerSlots[reserve].load(std::memory_order_acquire);

        auto* spareBuffer = spareSlot >= 0
            ? readyLayerBuffers[spareSlot].load(std::memory_order_acquire)
            : nullptr;

        if (spareBuffer != nullptr
            && spareBuffer->getNumSamples() < recordedSamples)
        {
            readyLayerBuffers[spareSlot].store(nullptr,
                                               std::memory_order_release);
            spareBuffer = nullptr;
        }

        if (spareSlot >= 0 && spareBuffer != nullptr)
        {
            recordingLayerIndex = spareSlot;

            if (reserve == 0)
            {
                spareLayerSlots[0] =
                    spareLayerSlots[1].load(std::memory_order_acquire);
            }

            spareLayerSlots[1] = -1;
            overdubSamplesWritten = 0;
            overdubStartRequested = false;
            sustainFinishRequested =
                sustainOnNextRecording.exchange(false,
                                                std::memory_order_acq_rel);
            currentState = State::Overdubbing;
            requestNextLayerBuffer();
            return;
        }
    }
}

void LooperEngine::applyBoundaryFade(juce::AudioBuffer<float>& buffer,
                                     int numSamples)
{
    const auto fadeSamples = juce::jmin(
        numSamples / 2,
        juce::jmax(1, juce::roundToInt(currentSampleRate * 0.005)));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.applyGainRamp(channel,
                             numSamples - fadeSamples,
                             fadeSamples,
                             1.0f,
                             0.0f);
    }
}

LooperEngine::State LooperEngine::getState() const
{
    return currentState.load(std::memory_order_acquire);
}

juce::String LooperEngine::getStateName() const
{
    switch (currentState.load(std::memory_order_acquire))
    {
    case State::Idle:
        return "IDLE";

    case State::WaitingForInput:
        return "WAITING FOR INPUT";

    case State::RecordingFirstLoop:
        if (sustainFinishRequested.load(std::memory_order_acquire))
            return "WAITING FOR SUSTAIN";

        return "RECORDING";

    case State::Playing:
    {
        if (overdubStartRequested.load(std::memory_order_acquire))
            return "OVERDUB ARMED";

        bool hasReadyBuffer = false;
        bool hasReservedBuffer = false;

        for (const auto& spare : spareLayerSlots)
        {
            const auto slot = spare.load(std::memory_order_acquire);
            hasReservedBuffer = hasReservedBuffer || slot >= 0;
            hasReadyBuffer = hasReadyBuffer
                             || (slot >= 0
                                 && readyLayerBuffers[slot].load(
                                        std::memory_order_acquire)
                                        != nullptr);
        }

        if (hasReservedBuffer && !hasReadyBuffer)
            return "PREPARING LAYER";

        return "PLAYING";
    }

    case State::Overdubbing:
        if (sustainFinishRequested.load(std::memory_order_acquire))
            return "WAITING FOR SUSTAIN";

        return "OVERDUB";

    case State::Stopped:
        return "STOPPED";
    }

    return "";
}

void LooperEngine::pressRec()
{
    sustainFinishRequested = false;
    sustainOnNextRecording = false;

    switch (currentState.load(std::memory_order_relaxed))
    {
    case State::Idle:
        if (storedLayerCount.load(std::memory_order_relaxed) > 0)
        {
            requestedLayerIndex = -1;
            storedLayerCount = 0;
            undoHistorySize = 0;
            redoHistorySize = 0;
            layerBufferSlots.fill(-1);
            layerBufferSlots[0] = 0;

            for (auto& slot : spareLayerSlots)
                slot.store(-1, std::memory_order_release);

            for (int layer = 1; layer < maximumLayers; ++layer)
                readyLayerBuffers[layer].store(nullptr,
                                               std::memory_order_release);

            for (auto& active : layerActive)
                active.store(false, std::memory_order_release);

            layerCount = 0;
        }

        currentState = State::WaitingForInput;
        break;

    case State::WaitingForInput:
        break;

    case State::RecordingFirstLoop:
        if (recordedSamples > 0)
        {
            auto* firstLayer = readyLayerBuffers[0].load(std::memory_order_acquire);
            applyBoundaryFade(*firstLayer, recordedSamples);
            currentState = State::Playing;
            storedLayerCount = 1;
            activateLayer(0);
            pushHistory(0, true);
            playbackPosition = 0;
            progress = 0.0f;
            requestNextLayerBuffer();
        }
        break;

    case State::Playing:
    {
        if (overdubStartRequested.load(std::memory_order_relaxed))
        {
            overdubStartRequested = false;
            break;
        }

        if (storedLayerCount.load(std::memory_order_relaxed) < maximumLayers)
        {
            overdubStartRequested = true;
            startPendingOverdubIfReady();
        }

        break;
    }

    case State::Overdubbing:
        if (recordingLayerIndex >= 0 && overdubSamplesWritten > 0)
        {
            auto* layer = readyLayerBuffers[recordingLayerIndex].load(
                std::memory_order_acquire);
            const auto fadeSamples = juce::jmin(
                overdubSamplesWritten,
                juce::jmax(1, juce::roundToInt(currentSampleRate * 0.005)));

            for (int channel = 0; channel < layer->getNumChannels(); ++channel)
            {
                for (int sample = 0; sample < fadeSamples; ++sample)
                {
                    const auto position = (playbackPosition - 1 - sample
                                           + recordedSamples)
                                          % recordedSamples;
                    const auto gain = static_cast<float>(sample)
                                      / static_cast<float>(fadeSamples);
                    layer->setSample(channel,
                                     position,
                                     layer->getSample(channel, position) * gain);
                }
            }

            const auto targetLayer =
                storedLayerCount.load(std::memory_order_relaxed);
            const auto previousStoredLayers =
                storedLayerCount.load(std::memory_order_relaxed);

            for (int layerIndex = targetLayer;
                 layerIndex < previousStoredLayers;
                 ++layerIndex)
            {
                const auto discardedSlot = layerBufferSlots[layerIndex];

                if (discardedSlot >= 0 && discardedSlot != recordingLayerIndex)
                    readyLayerBuffers[discardedSlot].store(
                        nullptr, std::memory_order_release);

                layerBufferSlots[layerIndex] = -1;
            }

            layerBufferSlots[targetLayer] = recordingLayerIndex;
            storedLayerCount = targetLayer + 1;
            layerVolumes[targetLayer] = 1.0f;
            layerMutes[targetLayer] = false;
            layerSolos[targetLayer] = false;
            currentLayerGains[targetLayer] = 1.0f;
            activateLayer(targetLayer);
            pushHistory(targetLayer, true);
            recordingLayerIndex = -1;
            requestNextLayerBuffer();
        }

        currentState = State::Playing;
        break;

    default:
        break;
    }
}

void LooperEngine::pressRecSustain()
{
    const auto state = currentState.load(std::memory_order_relaxed);

    if (state == State::RecordingFirstLoop || state == State::Overdubbing)
    {
        sustainFinishRequested = true;
        return;
    }

    if (state == State::WaitingForInput)
    {
        sustainOnNextRecording = true;
        return;
    }

    if (state == State::Idle || state == State::Playing)
    {
        const auto wasWaitingForOverdub =
            overdubStartRequested.load(std::memory_order_relaxed);
        pressRec();

        if (wasWaitingForOverdub)
            return;

        sustainOnNextRecording = true;

        if (currentState.load(std::memory_order_relaxed) == State::Overdubbing)
        {
            sustainFinishRequested = true;
            sustainOnNextRecording = false;
        }
    }
}

void LooperEngine::triggerRecording()
{
    if (currentState.load(std::memory_order_relaxed) == State::WaitingForInput)
    {
        sustainFinishRequested =
            sustainOnNextRecording.exchange(false, std::memory_order_acq_rel);
        currentState = State::RecordingFirstLoop;
    }
}

void LooperEngine::pressPlay()
{
    if (currentState == State::Stopped)
        currentState = State::Playing;
}

void LooperEngine::pressStop()
{
    overdubStartRequested = false;
    sustainFinishRequested = false;
    sustainOnNextRecording = false;

    if (currentState == State::WaitingForInput)
    {
        currentState = State::Idle;
        return;
    }

    if (currentState == State::RecordingFirstLoop)
    {
        recordedSamples = 0;
        playbackPosition = 0;
        layerCount = 0;
        storedLayerCount = 0;
        undoHistorySize = 0;
        redoHistorySize = 0;
        layerActive[0] = false;
        progress = 0.0f;
        loopLength = 0.0f;
        currentState = State::Idle;
        return;
    }

    if (currentState == State::Overdubbing)
    {
        if (recordingLayerIndex >= 0)
        {
            readyLayerBuffers[recordingLayerIndex].store(
                nullptr, std::memory_order_release);
        }

        recordingLayerIndex = -1;
        overdubSamplesWritten = 0;
        currentState = State::Stopped;
        requestNextLayerBuffer();
        return;
    }

    if (currentState == State::Playing ||
        currentState == State::Overdubbing)
    {
        currentState = State::Stopped;
    }
}

void LooperEngine::pressUndo()
{
    overdubStartRequested = false;

    const auto state = currentState.load(std::memory_order_relaxed);

    if ((state == State::Playing || state == State::Stopped
         || state == State::Idle)
        && undoHistorySize > 0)
    {
        const auto action = undoHistory[--undoHistorySize];

        if (action.activated)
            deactivateLayer(action.layer);
        else
            activateLayer(action.layer);

        if (redoHistorySize < static_cast<int>(redoHistory.size()))
            redoHistory[redoHistorySize++] = action;

        if (layerCount.load(std::memory_order_relaxed) == 0)
        {
            currentState = State::Idle;
            progress = 0.0f;
        }
    }
}

void LooperEngine::pressRedo()
{
    if (redoHistorySize > 0)
    {
        const auto action = redoHistory[--redoHistorySize];

        if (action.activated)
            activateLayer(action.layer);
        else
            deactivateLayer(action.layer);

        if (undoHistorySize < static_cast<int>(undoHistory.size()))
            undoHistory[undoHistorySize++] = action;

        if (layerCount.load(std::memory_order_relaxed) > 0
            && currentState.load(std::memory_order_relaxed) == State::Idle)
            currentState = State::Playing;
    }
}

void LooperEngine::pressReset()
{
    currentState = State::Idle;

    layerCount = 0;
    undoHistorySize = 0;
    redoHistorySize = 0;
    pendingLayerDeletes = 0;

    progress = 0.0f;
    loopLength = 0.0f;
    recordedSamples = 0;
    playbackPosition = 0;
    storedLayerCount = 0;
    recordingLayerIndex = -1;
    requestedLayerIndex = -1;
    for (auto& slot : spareLayerSlots)
        slot.store(-1, std::memory_order_release);
    overdubStartRequested = false;
    sustainFinishRequested = false;
    sustainOnNextRecording = false;
    overdubSamplesWritten = 0;
    layerBufferSlots.fill(-1);
    layerBufferSlots[0] = 0;

    for (int layer = 1; layer < maximumLayers; ++layer)
        readyLayerBuffers[layer].store(nullptr, std::memory_order_release);

    for (int layer = 0; layer < maximumLayers; ++layer)
    {
        layerActive[layer] = false;
        layerVolumes[layer] = 1.0f;
        layerMutes[layer] = false;
        layerSolos[layer] = false;
        layerPeaks[layer] = 0.0f;
        currentLayerGains[layer] = 1.0f;
    }
}

void LooperEngine::pressRewind()
{
    const auto state = currentState.load(std::memory_order_relaxed);

    if (recordedSamples > 0
        && (state == State::Playing || state == State::Stopped))
    {
        playbackPosition = 0;
        progress = 0.0f;
    }
}

int LooperEngine::getLayerCount() const
{
    return layerCount.load(std::memory_order_acquire);
}

int LooperEngine::getStoredLayerCount() const
{
    return storedLayerCount.load(std::memory_order_acquire);
}

bool LooperEngine::isLayerActive(int layer) const
{
    return layer >= 0 && layer < maximumLayers
           && layerActive[layer].load(std::memory_order_acquire);
}

float LooperEngine::getLayerVolume(int layer) const
{
    return layer >= 0 && layer < maximumLayers
        ? layerVolumes[layer].load(std::memory_order_acquire)
        : 0.0f;
}

bool LooperEngine::isLayerMuted(int layer) const
{
    return layer >= 0 && layer < maximumLayers
           && layerMutes[layer].load(std::memory_order_acquire);
}

bool LooperEngine::isLayerSoloed(int layer) const
{
    return layer >= 0 && layer < maximumLayers
           && layerSolos[layer].load(std::memory_order_acquire);
}

float LooperEngine::consumeLayerPeak(int layer)
{
    if (layer < 0 || layer >= maximumLayers)
        return 0.0f;

    return layerPeaks[layer].exchange(0.0f, std::memory_order_acq_rel);
}

void LooperEngine::setLayerVolume(int layer, float gain)
{
    if (layer >= 0 && layer < maximumLayers)
        layerVolumes[layer] = juce::jlimit(0.0f, 2.0f, gain);
}

void LooperEngine::setLayerMuted(int layer, bool muted)
{
    if (layer >= 0 && layer < maximumLayers)
        layerMutes[layer] = muted;
}

void LooperEngine::setLayerSoloed(int layer, bool soloed)
{
    if (layer >= 0 && layer < maximumLayers)
        layerSolos[layer] = soloed;
}

void LooperEngine::deleteLayer(int layer)
{
    if (layer >= 0 && layer < maximumLayers)
        pendingLayerDeletes.fetch_or(1u << static_cast<unsigned int>(layer),
                                     std::memory_order_release);
}

void LooperEngine::activateLayer(int layer)
{
    if (layer < 0 || layer >= maximumLayers
        || layerActive[layer].exchange(true, std::memory_order_acq_rel))
    {
        return;
    }

    layerCount.fetch_add(1, std::memory_order_release);
}

void LooperEngine::deactivateLayer(int layer)
{
    if (layer < 0 || layer >= maximumLayers
        || !layerActive[layer].exchange(false, std::memory_order_acq_rel))
    {
        return;
    }

    layerCount.fetch_sub(1, std::memory_order_release);
}

void LooperEngine::pushHistory(int layer, bool activated)
{
    if (undoHistorySize == static_cast<int>(undoHistory.size()))
    {
        std::move(undoHistory.begin() + 1,
                  undoHistory.end(),
                  undoHistory.begin());
        --undoHistorySize;
    }

    undoHistory[undoHistorySize++] = { layer, activated };
    redoHistorySize = 0;
}

void LooperEngine::processPendingLayerDeletes()
{
    auto deletes =
        pendingLayerDeletes.exchange(0, std::memory_order_acquire);

    for (int layer = 0; layer < maximumLayers; ++layer)
    {
        if ((deletes & (1u << static_cast<unsigned int>(layer))) != 0
            && layerActive[layer].load(std::memory_order_relaxed))
        {
            deactivateLayer(layer);
            pushHistory(layer, false);
        }
    }

    const auto state = currentState.load(std::memory_order_relaxed);

    if (layerCount.load(std::memory_order_relaxed) == 0
        && storedLayerCount.load(std::memory_order_relaxed) > 0
        && (state == State::Playing || state == State::Stopped))
    {
        currentState = State::Idle;
        progress = 0.0f;
    }
}

void LooperEngine::publishPeak(std::atomic<float>& destination, float peak)
{
    auto current = destination.load(std::memory_order_relaxed);

    while (current < peak
           && !destination.compare_exchange_weak(
               current, peak, std::memory_order_release,
               std::memory_order_relaxed))
    {
    }
}

float LooperEngine::getProgress() const
{
    return progress.load(std::memory_order_acquire);
}

void LooperEngine::setProgress(float p)
{
    progress = juce::jlimit(0.0f, 1.0f, p);
}

float LooperEngine::getLoopLength() const
{
    return loopLength.load(std::memory_order_acquire);
}

void LooperEngine::setLoopLength(float seconds)
{
    loopLength = seconds;
}

float LooperEngine::getCurrentTime() const
{
    if (currentState.load(std::memory_order_acquire) == State::RecordingFirstLoop)
        return loopLength.load(std::memory_order_acquire);

    return progress.load(std::memory_order_acquire)
           * loopLength.load(std::memory_order_acquire);
}

bool LooperEngine::isWaitingForSustain() const
{
    return sustainFinishRequested.load(std::memory_order_acquire);
}
