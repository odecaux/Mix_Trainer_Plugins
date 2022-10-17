/*
  ==============================================================================

    Processor_Host.cpp
    Created: 16 Sep 2022 11:54:32am
    Author:  Octave

  ==============================================================================
*/

#include "shared.h"
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
    state.step = Listening;
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
    juce::ignoreUnused(destData);
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
        
        auto target_gain = randomGain();
        auto edit_gain = slider_value_to_gain(0);
        
        state.channels[message_id] = ChannelState{message_id, "", edit_gain, target_gain};
        //3) set random value
        if(editor)
        {
            editor->mixerPanel.createFader(state.channels[message_id], state.step);
        }
        broadcastAllDSP();
        
    }
    else if (tokens[0] == "delete") {
        auto channel = state.channels.find(message_id);
        jassert(channel != state.channels.end());
        
        if(editor)
        {
            editor->mixerPanel.removeFader(message_id);
        }
        state.channels.erase(channel);
    }
    else if (tokens[0] == "name")
    {
        auto &channel = state.channels[message_id];
        channel.name = tokens[2];
        
        if(editor)
        {
            editor->mixerPanel.renameFader(message_id, tokens[2]);
        }
    }
    else if (tokens[0] == "frequencyRange")
    {
        auto &channel = state.channels[message_id];
        channel.minFreq = tokens[2].getFloatValue();
        channel.maxFreq = tokens[3].getFloatValue();
    }
}

void ProcessorHost::toggleInputOrTarget(bool isOn) //TODO rename isOn
{
    jassert(state.step != Begin);
    auto old_step = state.step;
    
    if(isOn && state.step == Editing)
    {
        state.step = Listening;
    }
    else if(isOn && state.step == ShowingAnswer)
    {
        state.step = ShowingTruth;
    }
    else if(!isOn && state.step == Listening)
    {
        state.step = Editing;
    }
    else if(!isOn && state.step == ShowingTruth){
        state.step = ShowingAnswer;
    }
    
    auto* editor = (EditorHost*)getActiveEditor();
    if(old_step != state.step)
    {
        broadcastAllDSP();
        if(editor)
            editor->mixerPanel.updateGameStep(state.step, state.channels);
    }
}

void ProcessorHost::nextClicked(){
    switch(state.step)
    {
        case Begin :
        jassertfalse; //not implemented yet
        break;
        case Listening :
        case Editing :
        {
            state.step = ShowingTruth;
        }break;
        case ShowingTruth : 
        case ShowingAnswer :
        {
            for (auto& [_, channel] : state.channels)
            {
                channel.target_gain = randomGain();
                channel.edited_gain = 0.0;
            }
            state.step = Listening;
        }break;
    }
    broadcastAllDSP();
    auto* editor = (EditorHost*)getActiveEditor();
    if(editor)
    {
        editor->mixerPanel.updateGameStep(state.step, state.channels);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorHost();
}


