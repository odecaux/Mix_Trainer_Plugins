/*
  ==============================================================================

    Processor_Host.cpp
    Created: 16 Sep 2022 11:54:32am
    Author:  Octave

  ==============================================================================
*/


#include "Processor_Host.h"
#include "PluginEditor_Host.h"

//==============================================================================
ProcessorHost::ProcessorHost()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                 )
#endif
{
    juce::MessageManager::getInstance()->registerBroadcastListener(this);
    state.listening = User_Input;
}

ProcessorHost::~ProcessorHost()
{
}

//==============================================================================
const juce::String ProcessorHost::getName() const
{
    return JucePlugin_Name;
}

bool ProcessorHost::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ProcessorHost::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ProcessorHost::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ProcessorHost::getTailLengthSeconds() const
{
    return 0.0;
}

int ProcessorHost::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int ProcessorHost::getCurrentProgram()
{
    return 0;
}

void ProcessorHost::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ProcessorHost::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ProcessorHost::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void ProcessorHost::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void ProcessorHost::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProcessorHost::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    
    return true;
#endif
}
#endif

void ProcessorHost::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(buffer, midiMessages);
    juce::ScopedNoDenormals noDenormals;
    //auto totalNumInputChannels = getTotalNumInputChannels();
    //auto totalNumOutputChannels = getTotalNumOutputChannels();
}

//==============================================================================
bool ProcessorHost::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ProcessorHost::createEditor()
{
    return new EditorHost(*this);
}

//==============================================================================
void ProcessorHost::getStateInformation(juce::MemoryBlock& destData)
{
    
}

void ProcessorHost::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}
void ProcessorHost::actionListenerCallback(const juce::String& message) {
    juce::StringArray tokens = juce::StringArray::fromTokens(message, " ", "\"");
    
    jassert(tokens.size() >= 2);
    int message_id = tokens[1].getIntValue();
    
    auto* editor = (EditorHost*)getActiveEditor();
    if (tokens[0] == "create") {
        {
            auto assertChannel = state.channels.find(message_id);
            jassert(assertChannel == state.channels.end());
        }
        
        auto slider_value = 0;
        auto gain = slider_value_to_gain(slider_value);
        
        state.channels[message_id] = ChannelState{message_id, "", gain, gain};
        //3) set random value
        if(editor)
        {
            editor->mixerPanel.createChannel(message_id);
        }
    }
    else if (tokens[0] == "delete") {
        auto channel = state.channels.find(message_id);
        jassert(channel != state.channels.end());
        
        if(editor)
        {
            editor->mixerPanel.removeChannel(message_id);
        }
        state.channels.erase(channel);
    }
    else if (tokens[0] == "name")
    {
        auto &channel = state.channels[message_id];
        channel.name = tokens[2];
        
        if(editor)
        {
            editor->mixerPanel.renameChannel(message_id, tokens[2]);
        }
    }
    else if (tokens[0] == "frequencyRange")
    {
        auto &channel = state.channels[message_id];
        channel.minFrequency = tokens[2].getFloatValue();
        channel.maxFrequency = tokens[3].getFloatValue();
    }
}

void ProcessorHost::toggleListeningState(bool isToggled)
{
    auto old_listening = state.listening;
    if(isToggled)
    {
        //jassert(state.listening == User_Input);
        state.listening = Target;
    }
    else
    {
        //jassert(state.listening == Target);
        state.listening = User_Input;
    }
    if(old_listening != state.listening)
        sendGainToTracks();
}

void ProcessorHost::randomizeGains() 
{
    //auto* editor = (EditorHost*)getActiveEditor();
    for (auto& [_, channel] : state.channels)
    {
        auto slider_value = juce::Random::getSystemRandom().nextInt() % ArraySize(slider_values);
        auto gain = slider_value_to_gain(slider_value);
        channel.target_gain = gain;
    }
    if(state.listening == Target)
        sendGainToTracks();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorHost();
}


