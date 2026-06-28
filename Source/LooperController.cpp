#include "LooperController.h"

LooperController::LooperController()
{
}

void LooperController::pressRec()
{
    engine.pressRec();
}

void LooperController::pressPlay()
{
    engine.pressPlay();
}

void LooperController::pressStop()
{
    engine.pressStop();
}

void LooperController::pressUndo()
{
    engine.pressUndo();
}

void LooperController::pressRedo()
{
    engine.pressRedo();
}

void LooperController::pressReset()
{
    engine.pressReset();
}

juce::String LooperController::getStateName() const
{
    return engine.getStateName();
}

int LooperController::getLayerCount() const
{
    return engine.getLayerCount();
}

float LooperController::getProgress() const
{
    return engine.getProgress();
}