/*
  ==============================================================================

    PluginEditor_Track.h
    Created: 16 Sep 2022 9:09:15am
    Author:  Octave

  ==============================================================================
*/

float denormalize_frequency(float value)
{
    float a = std::powf(value, 2.0f);
    float b = a * (20000.0f - 20.0f);
    return b + 20.0f;
}


float normalize_frequency(float frequency)
{
    return std::sqrt((frequency - 20.0f) / (20000.0f - 20.0f));
}

#pragma once
class EditorTrack : public juce::AudioProcessorEditor
{
    public:
    
    EditorTrack(ProcessorTrack& p, int id, float minFrequency, float maxFrequency)
        : AudioProcessorEditor(p), 
    audioProcessor(p), 
    id(id)
    {
        minLabel.setSize(120, 30);
        minLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(minLabel);
        
        maxLabel.setSize(120, 30);
        maxLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(maxLabel);
        
        frequencyRangeSlider.setSize(200, 30);
        frequencyRangeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        frequencyRangeSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
        frequencyRangeSlider.setRange(0.0f, 1.0f);
        frequencyRangeSlider.onValueChange = [this]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float newMinFrequency = denormalize_frequency(minValue);
            float newMaxFrequency = denormalize_frequency(maxValue);
            minLabel.setText(juce::String((int)newMinFrequency), juce::dontSendNotification);
            maxLabel.setText(juce::String((int)newMaxFrequency), juce::dontSendNotification);
            audioProcessor.frequencyRangeChanged(newMinFrequency, newMaxFrequency);
        };
        addAndMakeVisible(frequencyRangeSlider);
        frequencyRangeSlider.setMinAndMaxValues(normalize_frequency(minFrequency), normalize_frequency(maxFrequency));
        
        setSize(400, 300);
        setResizable(true, false);
    }
    ~EditorTrack() override {
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
        frequencyRangeSlider.setCentrePosition(center.x, center.y);
        maxLabel.setCentrePosition(center.x, center.y - 40);
        minLabel.setCentrePosition(center.x, center.y + 40);
    }
    
    private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ProcessorTrack& audioProcessor;
    juce::Label minLabel;
    juce::Label maxLabel;
    juce::Slider frequencyRangeSlider;
    int id;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorTrack)
};

