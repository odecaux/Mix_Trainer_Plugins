/*
  ==============================================================================

    Processor_Host.h
    Created: 16 Sep 2022 11:53:34am
    Author:  Octave

  ==============================================================================
*/

#pragma once

//==============================================================================

class ProcessorHost : public juce::AudioProcessor, public juce::ActionListener
#if JucePlugin_Enable_ARA
, public juce::AudioProcessorARAExtension
#endif
{
    public:
    void actionListenerCallback(const juce::String& message) override;
    
    //==============================================================================
    ProcessorHost();
    ~ProcessorHost() override;
    
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
    
    
    void broadcastAllDSP()
    {
        for(const auto& [_, channel] : state.channels)
        {
            ChannelDSPState dsp;
            switch(state.step)
            {
                case Begin : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                case Listening : {
                    dsp = { .gain = channel.target_gain };
                } break;
                case Editing : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                case ShowingTruth : { 
                    dsp = { .gain = channel.target_gain };
                } break;
                case ShowingAnswer : {
                    dsp = { .gain = channel.edited_gain };
                } break;
                default : {
                    dsp = { .gain = 0.0 };
                    jassertfalse;
                } break;
            }
            sendDSPMessage(channel.id, dsp);
        }
    }
      
    void sendDSPMessage(int id, ChannelDSPState dsp)
    {
        auto message = juce::String("setDSP ") + juce::String(id) + " " + juce::String::toHexString((void*)&dsp, sizeof(dsp), 0);
        juce::MessageManager::getInstance()->broadcastMessage(message);
    }
    
    void uiIsBeingDestroyed()
    {
        jassert(game);
        game->deleteUI();
    }

    void backToMainMenu()
    {
    }

    GameState state;
    std::unique_ptr<GameImplementation> game;
    
    private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorHost)
};
