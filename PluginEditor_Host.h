/*
  ==============================================================================

    PluginEditor_Host.h
    Created: 16 Sep 2022 9:09:26am
    Author:  Octave

  ==============================================================================
*/

#pragma once

class MainMenu : public juce::Component
{
public :
    MainMenu(std::function<void()> onMainButtonClick) : 
        onMainButtonClick(std::move(onMainButtonClick))
    {
        button.setSize(40, 40);
        button.onClick = [this] {
            this->onMainButtonClick();
        };
        addAndMakeVisible(button);
    }
    
    void paint(juce::Graphics &g) override
    {
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        button.setCentrePosition(bounds.getCentre());
    }

private :
    juce::TextButton button;
    std::function < void() > onMainButtonClick;
};


class EditorHost : public juce::AudioProcessorEditor
{
public:
    
    EditorHost(ProcessorHost& p,
               juce::Component *mainPanel) :
        AudioProcessorEditor(p),
        audio_processor(p),
        mainPanel(mainPanel)
    {
        setResizable(true, false);
        setSize(960, 540);
        addAndMakeVisible(*this->mainPanel);
    }
    ~EditorHost() override
    {
        removeChildComponent(mainPanel);
        mainPanel = nullptr;
        audio_processor.uiIsBeingDestroyed();
    }
    
    void paint(juce::Graphics & g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        mainPanel->setBounds(getLocalBounds());
    }
    
public:
    juce::Component *mainPanel;
    ProcessorHost &audio_processor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorHost)
};