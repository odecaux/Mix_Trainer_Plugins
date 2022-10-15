/*
  ==============================================================================

    Processor_Master.cpp
    Created: 16 Sep 2022 11:54:32am
    Author:  Octave

  ==============================================================================
*/


#include "Processor_Master.h"
#include "PluginEditor_Master.h"

//==============================================================================
ProcessorMaster::ProcessorMaster()
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
}

ProcessorMaster::~ProcessorMaster()
{
}

//==============================================================================
const juce::String ProcessorMaster::getName() const
{
    return JucePlugin_Name;
}

bool ProcessorMaster::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ProcessorMaster::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ProcessorMaster::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ProcessorMaster::getTailLengthSeconds() const
{
    return 0.0;
}

int ProcessorMaster::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int ProcessorMaster::getCurrentProgram()
{
    return 0;
}

void ProcessorMaster::setCurrentProgram(int index)
{
}

const juce::String ProcessorMaster::getProgramName(int index)
{
    return {};
}

void ProcessorMaster::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void ProcessorMaster::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void ProcessorMaster::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProcessorMaster::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void ProcessorMaster::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
}

//==============================================================================
bool ProcessorMaster::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ProcessorMaster::createEditor()
{
    return new EditorMaster(*this);
}

//==============================================================================
void ProcessorMaster::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ProcessorMaster::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
void ProcessorMaster::actionListenerCallback(const juce::String& message) {
    juce::StringArray tokens = juce::StringArray::fromTokens(message, " ", "\"");
    
    jassert(tokens.size() >= 2);
    int message_id = tokens[1].getIntValue();
    
    auto* editor = (EditorMaster*)getActiveEditor();
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
        auto channel = state.channels.find(message_id);
        jassert(channel != state.channels.end());
        channel->second.name = tokens[2];
        if(editor)
        {
            editor->mixerPanel.renameChannel(message_id, tokens[2]);
        }
    }
}
void ProcessorMaster::randomizeGains() 
{
    auto* editor = (EditorMaster*)getActiveEditor();
    for (auto& [_, channel] : state.channels)
    {
        auto slider_value = juce::Random::getSystemRandom().nextInt() % ArraySize(slider_values);
        auto gain = slider_value_to_gain(slider_value);
        channel.edited_gain = gain;
        
        if(editor)
        {
            editor->mixerPanel.setChannelGain(channel.id, gain);
        }
    }
    sendGainToSlaves();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorMaster();
}


