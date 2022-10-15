/*
  ==============================================================================

    Processor_Slave.cpp
    Created: 16 Sep 2022 11:54:42am
    Author:  Octave

  ==============================================================================
*/

#include "Processor_Slave.h"
#include "PluginEditor_Slave.h"

//==============================================================================
ProcessorSlave::ProcessorSlave()
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
id{juce::Random().nextInt()},
gain{0.0f}
{
    juce::MessageManager::getInstance()->registerBroadcastListener(this);
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("create ") + juce::String(id));
}

ProcessorSlave::~ProcessorSlave()
{
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("delete ") + juce::String(id));
}

//==============================================================================
const juce::String ProcessorSlave::getName() const
{
    return JucePlugin_Name;
}

bool ProcessorSlave::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ProcessorSlave::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ProcessorSlave::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ProcessorSlave::getTailLengthSeconds() const
{
    return 0.0;
}

int ProcessorSlave::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int ProcessorSlave::getCurrentProgram()
{
    return 0;
}

void ProcessorSlave::setCurrentProgram(int index)
{
}

const juce::String ProcessorSlave::getProgramName(int index)
{
    return {};
}

void ProcessorSlave::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void ProcessorSlave::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void ProcessorSlave::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProcessorSlave::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void ProcessorSlave::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    buffer.applyGain(gain);
}

//==============================================================================
bool ProcessorSlave::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ProcessorSlave::createEditor()
{
    return new EditorSlave(*this, id);
}

//==============================================================================
void ProcessorSlave::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ProcessorSlave::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void ProcessorSlave::updateTrackProperties(const TrackProperties& properties)
{
    juce::MessageManager::getInstance()->broadcastMessage(juce::String("name ") + juce::String(id) + juce::String(" ") + properties.name);
}


void ProcessorSlave::actionListenerCallback(const juce::String& message)
{
    juce::StringArray tokens = juce::StringArray::fromTokens(message, " ", "\"");
    
    jassert(tokens.size() >= 2);
    int message_id = tokens[1].getIntValue();
    if(message_id != id)
        return;
    
    if(tokens[0] == "setGain")
    {
        gain = tokens[2].getDoubleValue();
        if(auto* editor = (EditorSlave*)getActiveEditor())
        {
            editor->setText(juce::String(gain));
        }
    }
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorSlave();
}

