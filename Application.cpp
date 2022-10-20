#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
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
    jassert(type == PanelType::MainMenu);
    type = PanelType::Game;
    jassert(editor);
    jassert(!game);

    game = std::make_unique < DemoGame > (*this, channels);
    auto game_ui = game->createUI();
    
    std::unique_ptr < juce::Component > game_panel =
        std::make_unique < GameUI_Panel > (
            [] {  },
            [] (bool _){},
            [this] { 
                game.reset();  
                toMainMenu(); //NOTE unclear lifetime, while rewinding the stack all the references will be invalid
            },
            std::move(game_ui)
        );
    editor->changePanel(std::move(game_panel));
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
    if (type == PanelType::Game)
    {
        jassert(game);
        game->onUIDelete();
    }
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
        case PanelType::Game :
        {
            jassert(game);
            auto game_ui = game->createUI();
            panel = std::make_unique < GameUI_Panel > (
                [] {  },
                    [] (bool _){},
                        [this] { 
                        game.reset();  
                        toMainMenu(); //NOTE unclear lifetime, while rewinding the stack all the references will be invalid
            },
                std::move(game_ui)
            );
        } break;
    }
    editor->changePanel(std::move(panel));
}

void Application::createChannel(int id)
{
    {
        auto assertChannel = channels.find(id);
        jassert(assertChannel == channels.end());
    }
    
    channels[id] = ChannelInfos { .id = id };

    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelCreate(id);
    }
    
#if 0    
    
    audioProcessor.broadcastAllDSP();
#endif
}
    
void Application::deleteChannel(int id)
{
    auto channel = channels.find(id);
    jassert(channel != channels.end());

    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelDelete(id);
    }
#if 0
    //TODO virtual
    state.channels.erase(channel);
        
#endif
    channels.erase(channel);
}
    
void Application::renameChannelFromUI(int id, juce::String newName)
{
    channels[id].name = newName;
    
    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelDelete(id);
    }
    //TODO propagate to the track
}
    
void Application::renameChannelFromTrack(int id, const juce::String &new_name)
{
    auto &channel = channels[id];
    channel.name = new_name;
        
    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelRenamedFromTrack(id, new_name);
    }
}
    
void Application::changeFrequencyRange(int id, float new_min, float new_max)
{
    auto &channel = channels[id];
    channel.min_freq = new_min;
    channel.max_freq = new_max;

    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelFrequenciesChanged(id);
    }
}