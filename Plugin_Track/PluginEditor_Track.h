/*
  ==============================================================================

    PluginEditor_Track.h
    Created: 16 Sep 2022 9:09:15am
    Author:  Octave

  ==============================================================================
*/

#pragma once
class EditorTrack : public juce::AudioProcessorEditor
{
    public:
    
    EditorTrack(ProcessorTrack& p, int id, const juce::String &name, float minFrequency, float maxFrequency)
        : AudioProcessorEditor(p), 
    audioProcessor(p), 
    id(id),
    frequency_bounds_widget(minFrequency, maxFrequency)
    {
        
        track_name_label.setSize(200, 30);
        track_name_label.setJustificationType(juce::Justification::centred);
        track_name_label.setText(name, juce::dontSendNotification);
        addAndMakeVisible(track_name_label);

        frequency_bounds_widget.setSize(200, 100);
        frequency_bounds_widget.on_mix_max_changed = [&] 
            (float new_min_frequency, float new_max_frequency, bool done_dragging) 
            {
                if(done_dragging)
                    audioProcessor.frequencyRangeChanged(new_min_frequency, new_max_frequency);
            };
        addAndMakeVisible(frequency_bounds_widget);

        setSize(400, 300);
        setResizable(true, false);
    }
    
    void paint(juce::Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();
        track_name_label.setCentrePosition(center.x, 40);
        frequency_bounds_widget.setCentrePosition(center.x, center.y);
    }

    void renameTrack(const juce::String &new_name)
    {
        track_name_label.setText(new_name, juce::dontSendNotification);
    }
    
    private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ProcessorTrack& audioProcessor;
    juce::Label track_name_label;
    int id;
    Frequency_Bounds_Widget frequency_bounds_widget;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorTrack)
};

