#pragma once

#include <JuceHeader.h>
#include "LooperEngine.h"

class LooperController
{
public:

    LooperController();

    // Botões
    void pressRec();
    void pressPlay();
    void pressStop();
    void pressUndo();
    void pressRedo();
    void pressReset();

    // Informações para a interface
    juce::String getStateName() const;

    int getLayerCount() const;

    float getProgress() const;

private:

    LooperEngine engine;
};