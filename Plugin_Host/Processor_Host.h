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
    

    void broadcastAllDSP(const std::unordered_map<uint32_t, Channel_DSP_State> &dsp_states)
    {
        for (const auto &[id, state] : dsp_states)
        {
            auto message = juce::String("dsp ") + juce::String(id) + " " + juce::String::toHexString((void*)&state, sizeof(state), 0);
            juce::MessageManager::getInstance()->broadcastMessage(message);
        }
    }

    void broadcastChannelList(const MuliTrack_Model& model)
    {
        juce::ignoreUnused(model);
        Game_Channel_Broadcast_Message out = {};
        assert(model.game_channels.size() == model.order.size());
        //assert(model.game_channels.size() == model.assigned_daw_track_count.size());
        out.channel_count = static_cast<int>(model.game_channels.size());
        for (auto i = 0; i < model.game_channels.size(); i++)
        {
            auto id = model.order.at(i);
            auto track = model.game_channels.at(id);
            out.channels[i] = track;
        }
        auto message = juce::String("new_track_list ") + juce::String::toHexString((void*)&out, sizeof(out), 0);
        juce::MessageManager::getInstance()->broadcastMessage(message);
    }

    
    Application app;
    private:
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorHost)
};
