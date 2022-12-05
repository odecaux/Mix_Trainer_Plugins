/*
  ==============================================================================

    PluginEditor_Host.h
    Created: 16 Sep 2022 9:09:26am
    Author:  Octave

  ==============================================================================
*/

#pragma once

class EditorHost : public juce::AudioProcessorEditor
{
public:
    
    EditorHost(ProcessorHost& p, std::function<void()> &&onEditorClose) :
        AudioProcessorEditor(p),
        audio_processor(p),
        onEditorClose(std::move(onEditorClose))
    {
        setResizable(true, false);
        setSize(960, 540);
    }
    ~EditorHost() override
    {
        removeChildComponent(current_panel.get());
        onEditorClose();
    }
    
    void paint(juce::Graphics & g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        if (current_panel)
        {
            current_panel->setBounds(getLocalBounds());
        }
    }
    
    void changePanel(std::unique_ptr<juce::Component> new_panel)
    {
        if(current_panel)
            removeChildComponent(current_panel.get());
        current_panel = std::move(new_panel);
        if(current_panel)
            addAndMakeVisible(*current_panel);
        resized();
    }

public:
    std::unique_ptr<juce::Component> current_panel;
    ProcessorHost &audio_processor;
    std::function < void() > onEditorClose;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorHost)
};