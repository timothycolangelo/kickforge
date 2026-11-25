#include "PluginEditor.h"

KickforgeEditor::KickforgeEditor (KickforgeProcessor& p)
    : AudioProcessorEditor (p), processor (p), generic (p)
{
    audition.onClick = [this] { processor.triggerAudition(); };
    addAndMakeVisible (audition);
    addAndMakeVisible (generic);

    setResizable (true, false);
    setSize (560, 720);
}

void KickforgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void KickforgeEditor::resized()
{
    auto r = getLocalBounds();
    audition.setBounds (r.removeFromTop (40).reduced (8, 6));
    generic.setBounds (r);
}
