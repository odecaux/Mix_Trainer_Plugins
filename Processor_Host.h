/*
  ==============================================================================

    Processor_Host.h
    Created: 16 Sep 2022 11:53:34am
    Author:  Octave

  ==============================================================================
*/

#pragma once


#include <juce_audio_processors/juce_audio_processors.h>

static const double slider_values[] = {-100, -24, -16, -10, -4};

#define DISCRETE_VALUE_COUNT 5
#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))


double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

static double slider_value_to_db(int value)
{
    jassert(value < ArraySize(slider_values));
    return slider_values[value];
}


static int db_to_slider_value(double db)
{
    for(int i = 0; i < ArraySize(slider_values); i++)
    {
        if(equal_double(db, slider_values[i], 0.001)) return i;
    }
    jassertfalse;
    return -1;
}

static double slider_value_to_gain(int value)
{
    return juce::Decibels::decibelsToGain(slider_value_to_db(value));
}

enum Listening {
    Target,
    User_Input
};

struct ChannelState
{
    int id;
    juce::String name;
    double edited_gain;
    double target_gain;
    float minFrequency;
    float maxFrequency;
};
/*
enum GameStep {
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};
*/
struct GameState
{
    std::unordered_map<int, ChannelState> channels;
    Listening listening;
    //GameStep step;
};

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
    
    void setUserGain(int id, double  newGain)
    {
        auto channel = state.channels.find(id);
        jassert(channel != state.channels.end());
        channel->second.edited_gain = newGain;
        sendGainToTracks();
    }
    
    void sendGainToTracks()
    {
        for(const auto& [_, channel] : state.channels)
        {
            double gainToSend = state.listening == User_Input ? channel.edited_gain : channel.target_gain;
            /*switch(state.step)
            {
                case Listening :
                gainToSend = channel.target_gain;
                break;
                case Editing :
                gainToSend = channel.target_gain;
                break;
                case ShowingTruth : 
                gainToSend = channel.target_gain;
                break;
                case ShowingAnswer :
                gainToSend = channel.edited_gain;
                break;
            }*/
            auto message = juce::String("setGain ") + juce::String(channel.id) + " " + juce::String(gainToSend);
            juce::MessageManager::getInstance()->broadcastMessage(message);
        }
    }
    
    void toggleListeningState(bool isToggled);
    void randomizeGains();
    GameState state;
    
    private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorHost)
};
