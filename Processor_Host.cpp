/*
  ==============================================================================

    Processor_Host.cpp
    Created: 16 Sep 2022 11:54:32am
    Author:  Octave

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "shared.h"


#include "Game.h"
#include "Game_Mixer.h"
#include "MainMenu.h"
#include "Application.h"
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
,
    app(*this)
{
    juce::MessageManager::getInstance()->registerBroadcastListener(this);
}

ProcessorHost::~ProcessorHost()
{
    juce::MessageManager::getInstance()->deregisterBroadcastListener(this);
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
    auto *editor = new EditorHost(*this, [&] { app.onEditorDelete(); });
    app.initialiseEditorUI(editor);
    return editor;
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
    
    if (tokens[0] == "create") 
    {
        app.createChannel(message_id);
    }
    else if (tokens[0] == "delete") 
    {
       app.deleteChannel(message_id);
    }
    else if (tokens[0] == "name_from_track")
    {
        app.renameChannelFromTrack(message_id, tokens[2]);
    }
    //TODO more infos from the channel ?
    else if (tokens[0] == "frequency_range")
    {
        auto minFreq = tokens[2].getFloatValue();
        auto maxFreq = tokens[3].getFloatValue();
        app.changeFrequencyRange(message_id, minFreq, maxFreq);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorHost();
}