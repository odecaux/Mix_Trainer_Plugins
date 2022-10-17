/*
  ==============================================================================

    Processor_Host.h
    Created: 16 Sep 2022 11:53:34am
    Author:  Octave

  ==============================================================================
*/

#pragma once


#include <juce_audio_processors/juce_audio_processors.h>

static const double slider_values[] = {-100, -12, -9, -6, -3};

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

static int gain_to_slider_value(double gain)
{ 
    return db_to_slider_value(juce::Decibels::gainToDecibels(gain));
}

static double slider_value_to_gain(int value)
{
    return juce::Decibels::decibelsToGain(slider_value_to_db(value));
}

enum Action {
    
};

enum GameStep {
    Begin,
    Listening,
    Editing,
    ShowingTruth,
    ShowingAnswer
};

struct ChannelState
{
    int id;
    juce::String name;
    double edited_gain;
    double target_gain;
    float minFreq;
    float maxFreq;
    //- unused 
    double target_low_shelf_gain;
    double target_low_shelf_freq;
    double target_high_shelf_gain;
    double target_high_shelf_freq;
    
    double edited_low_shelf_gain;
    double edited_low_shelf_freq;
    double edited_high_shelf_gain;
    double edited_high_shelf_freq;
};

struct GameState
{
    std::unordered_map<int, ChannelState> channels;
    GameStep step;
    int score;
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
        broadcastAllDSP();
    }
    
    void broadcastAllDSP()
    {
        for(const auto& [_, channel] : state.channels)
        {
            ChannelDSPState dsp;
            switch(state.step)
            {
                case Begin : {
                    dsp = {0.0};
                    jassertfalse;
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
    
    void nextClicked();
    double randomGain()
    {
        int slider_value = juce::Random::getSystemRandom().nextInt() % ArraySize(slider_values);
        return slider_value_to_gain(slider_value);
    }
    void toggleInputOrTarget(bool isOn);
    
    void renameChannel(int id, juce::String newName)
    {
        state.channels[id].name = newName;
        //TODO propagate to the track
    }
    
    GameState state;
    
    private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorHost)
};
