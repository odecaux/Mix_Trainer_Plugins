#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"
#if 0
#include "Game.h"
#endif
#include "MainMenu.h"
#include "Application.h"
#include "Processor_Host.h"
#include "PluginEditor_Host.h"

Application::Application(ProcessorHost &host) : 
    host(host), 
    editor(nullptr),
    type(PanelType::MainMenu)
{
#if 0
    state.step = Begin;
    state.score = 0;
    //game = std::make_unique<MixerGame>(*this, state);
#endif
}


void Application::toMainMenu()
{
    jassert(type != PanelType::MainMenu);
    type = PanelType::MainMenu;
    jassert(editor);
        
    std::unique_ptr < juce::Component > main_menu =
        std::make_unique < MainMenu > (
        [this] { toGame(); },
            [this] { toStats(); },
                [this ] { toSettings(); });
    editor->changePanel(std::move(main_menu));
}


void Application::toGame()
{
}

void Application::toStats()
{
    jassert(type == PanelType::MainMenu);
    type = PanelType::Stats;
    jassert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
    editor->changePanel(std::move(settings_menu));
}

void Application::toSettings()
{
    jassert(type == PanelType::MainMenu);
    type = PanelType::Settings;
    jassert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
    editor->changePanel(std::move(settings_menu));
   
}


void Application::onEditorDelete()
{
    jassert(editor != nullptr);
    editor = nullptr;
}


void Application::initialiseEditorUI(EditorHost *new_editor)
{
    jassert(editor == nullptr);
    editor = new_editor;
    std::unique_ptr < juce::Component > panel;
    switch (type)
    {
        case PanelType::MainMenu : {
            panel = std::make_unique < MainMenu > (
                [this] { toGame(); },
                [this] { toStats(); },
                [this ] { toSettings(); });
        } break;
        case PanelType::Settings : {
            panel = std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
        } break;
        case PanelType::Stats : {
            panel = std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
        } break;
    }
    editor->changePanel(std::move(panel));
}

#if 0

void Application::createChannel(int id)
{
    {
        auto assertChannel = state.channels.find(id);
        jassert(assertChannel == state.channels.end());
    }
    
    state.channels[id] = generateChannel(id);
        
    audioProcessor.broadcastAllDSP();
    if(game_ui)
        game_ui->createChannelUI(state.channels[id], state.step);
}
    
void Application::deleteChannel(int id)
{
    auto channel = state.channels.find(id);
    jassert(channel != state.channels.end());
        
    if(game_ui)
    {
        game_ui->deleteChannelUI(id);
    }
        
    //TODO virtual
    state.channels.erase(channel);
}
    
void Application::renameChannelFromUI(int id, juce::String newName)
{
    state.channels[id].name = newName;
    //TODO propagate to the track
}
    
void Application::renameChannelFromTrack(int id, const juce::String &new_name)
{
    auto &channel = state.channels[id];
    channel.name = new_name;
        
    if(game_ui)
        game_ui->renameChannel(id, new_name);
}
    
void Application::changeFrequencyRange(int id, float new_min, float new_max)
{
    auto &channel = state.channels[id];
    channel.minFreq = new_min;
    channel.maxFreq = new_max;
}

#endif