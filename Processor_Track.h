/*
  ==============================================================================

    Processor_Track.h
    Created: 16 Sep 2022 11:53:19am
    Author:  Octave

  ==============================================================================
*/

#pragma once


#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
*/
class ProcessorTrack : public juce::AudioProcessor, public juce::ActionListener
#if JucePlugin_Enable_ARA
, public juce::AudioProcessorARAExtension
#endif
{
    public:
    // Inherited via ActionListener
    void actionListenerCallback(const juce::String& message) override;
    //==============================================================================
    ProcessorTrack();
    ~ProcessorTrack() override;
    
    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    void updateTrackProperties(const TrackProperties& properties) override;
    
    private:
    //==============================================================================
    int id;
    double gain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorTrack)
};
