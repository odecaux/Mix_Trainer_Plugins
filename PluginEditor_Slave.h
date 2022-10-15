/*
  ==============================================================================

    PluginEditor_Slave.h
    Created: 16 Sep 2022 9:09:15am
    Author:  Octave

  ==============================================================================
*/

#pragma once
class EditorSlave : public juce::AudioProcessorEditor
{
    public:
    
    EditorSlave(ProcessorSlave& p, int id)
        : audioProcessor(p), AudioProcessorEditor(p), id(id)
    {
        // Make sure that before the constructor has finished, you've set the
        // editor's size to whatever you need it to be.
        label.setSize(120, 80);
        label.setText("Slave", juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        setSize(400, 300);
        setResizable(true, false);
    }
    ~EditorSlave() override {
    }
    
    void paint(juce::Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized()
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();
        label.setCentrePosition(center.x, center.y);
        // This is generally where you'll want to lay out the positions of any
        // subcomponents in your editor..
    }
    
    void setText(const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        repaint(); //????
    }
    
    
    private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ProcessorSlave& audioProcessor;
    juce::Label label;
    int id;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorSlave)
        
        
};

