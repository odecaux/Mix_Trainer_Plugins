/*
  ==============================================================================

    Processor_Host.cpp
    Created: 16 Sep 2022 11:54:32am
    Author:  Octave

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "../shared/shared.h"


#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "../Game/Game_Mixer.h"
#include "Main_Menu.h"
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

static const char out_message[] = "hello world";
//==============================================================================
void ProcessorHost::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream out (destData, false);
    out.write(out_message, sizeof(out_message));

    auto game_channels = app.save_model();
    int size = checked_cast<int>(game_channels.size());
    out.writeInt(checked_cast<int>(size));
    out.write(game_channels.data(), size * sizeof(Game_Channel));
    out.writeInt(0);
}

void ProcessorHost::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream in { data, checked_cast<size_t>(sizeInBytes), false };
    if (sizeInBytes > 0)
    {
        //assert(sizeInBytes == (size * sizeof(Game_Channel) + sizeof(int) * 2));
        char in_message[sizeof(out_message) / sizeof(*out_message)] = {};
        in.read(in_message, sizeof(in_message));

        int size = in.readInt();
        std::vector<Game_Channel> game_channels{};
        game_channels.resize(size);
        in.read(game_channels.data(), size * sizeof(Game_Channel));
        app.set_model(game_channels);
    }
}

void ProcessorHost::actionListenerCallback(const juce::String& message) {
    juce::StringArray tokens = juce::StringArray::fromTokens(message, " ", "\"");
    

    assert(tokens.size() >= 2);
    int daw_channel_id = tokens[1].getIntValue();
    
    if (tokens[0] == "create") 
    {
        app.create_daw_channel(daw_channel_id);
    }
    else if (tokens[0] == "delete") 
    {
       app.delete_daw_channel(daw_channel_id);
    }
    else if (tokens[0] == "name_from_track")
    {
        app.rename_daw_channel(daw_channel_id, tokens[2]);
    }
    else if (tokens[0] == "frequency_range")
    {
        auto game_channel_id = tokens[2].getIntValue();
        auto minFreq = tokens[3].getFloatValue();
        auto maxFreq = tokens[4].getFloatValue();
        app.change_frequency_range_from_daw(daw_channel_id, game_channel_id, minFreq, maxFreq);
    }
    else if (tokens[0] == "select_game_channel")
    {
        auto game_channel_id = tokens[2].getIntValue();
        app.bind_daw_channel_with_game_channel(daw_channel_id, game_channel_id);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorHost();
}