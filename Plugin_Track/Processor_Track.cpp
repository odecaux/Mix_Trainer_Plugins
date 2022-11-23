/*
  ==============================================================================

    Processor_Track.cpp
    Created: 16 Sep 2022 11:54:42am
    Author:  Octave

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "shared.h"
#include "Processor_Track.h"
#include "PluginEditor_Track.h"

//==============================================================================
ProcessorTrack::ProcessorTrack()
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
    id { juce::Random().nextInt() },
    name { id },
    gain { 0.0f },
    minFrequency { 20 },
    maxFrequency { 20000 }
{
    juce::MessageManager::getInstance()->registerBroadcastListener(this);
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("create ") + juce::String(id));
    broadcastFrequencies();
}

ProcessorTrack::~ProcessorTrack()
{
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("delete ") + juce::String(id));
    juce::MessageManager::getInstance()->deregisterBroadcastListener(this);
}

//==============================================================================
const juce::String ProcessorTrack::getName() const
{
    return JucePlugin_Name;
}

bool ProcessorTrack::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ProcessorTrack::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ProcessorTrack::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ProcessorTrack::getTailLengthSeconds() const
{
    return 0.0;
}

int ProcessorTrack::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int ProcessorTrack::getCurrentProgram()
{
    return 0;
}

void ProcessorTrack::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ProcessorTrack::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ProcessorTrack::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void ProcessorTrack::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void ProcessorTrack::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProcessorTrack::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void ProcessorTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    //auto totalNumInputChannels = getTotalNumInputChannels();
    //auto totalNumOutputChannels = getTotalNumOutputChannels();
    buffer.applyGain(static_cast<float>(gain));
}

//==============================================================================
bool ProcessorTrack::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ProcessorTrack::createEditor()
{
    return new EditorTrack(*this, id, name, minFrequency, maxFrequency);
}

//==============================================================================
void ProcessorTrack::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream out (destData, false);
    out.writeFloat(minFrequency);
    out.writeFloat(maxFrequency);
    out.writeInt(0);
}

void ProcessorTrack::setStateInformation(const void* data, int sizeInBytes)
{
    if(sizeInBytes > 0)
    {
        jassert(sizeInBytes == 12);
        minFrequency = ((const float*)data)[0];
        maxFrequency = ((const float*)data)[1];
    }
}

void ProcessorTrack::updateTrackProperties(const TrackProperties& properties)
{
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("name_from_track ") + juce::String(id) + juce::String(" ") + properties.name);
    name = properties.name;
    if (auto *editor = (EditorTrack*)getActiveEditor())
    {
        editor->renameTrack(name);
    }
}


void ProcessorTrack::actionListenerCallback(const juce::String& message)
{
    juce::StringArray tokens = juce::StringArray::fromTokens(message, " ", "\"");
    
    jassert(tokens.size() >= 2);
    int message_id = tokens[1].getIntValue();
    if(message_id != id)
        return;
    
    if(tokens[0] == "dsp")
    {
        auto blob = juce::MemoryBlock{};
        blob.loadFromHexString(tokens[2]);
        size_t blob_size = blob.getSize();
        size_t dsp_state_size = sizeof(ChannelDSPState);
        jassert(blob_size == dsp_state_size);
        ChannelDSPState state = *(ChannelDSPState*)blob.getData();
        gain = state.gain;
    }
    else if(tokens[0] == "name_from_ui")
    {
        name = tokens[2];
        if (auto *editor = (EditorTrack*)getActiveEditor())
        {
            editor->renameTrack(name);
        }
    }
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorTrack();
}
