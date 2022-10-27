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
    //NOTE useless
    broadcastDSP(bypassedAllChannelsDSP(channels));
}


void Application::toMainMenu()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));

    jassert(type != PanelType::MainMenu);
    type = PanelType::MainMenu;
    jassert(editor);
        
    auto main_menu =
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
    
#if 0
    game = std::make_unique < ChannelNamesDemo > (
        *this, 
        channels, 
        [this](const auto& dsp_states) { broadcastDSP(dsp_states); }
    );
#endif
    game = std::make_unique < MixerGame > (
        *this, 
        channels, 
        std::vector<double>{ -100.0, -12.0, -9.0, -6.0, -3.0},
        [this](const auto& dsp_states) { broadcastDSP(dsp_states); }
    );
    printf("toGame\n");
    auto game_ui = game->createUI();
    
    auto game_panel = std::make_unique < GameUI_Panel > (
        [this] { game->nextClicked();  },
            [this] (bool was_target) { game->toggleInputOrTarget(was_target); },
            [this] { 
                game.reset();  
                toMainMenu(); //NOTE unclear lifetime, while rewinding the stack all the references will be invalid
            },
            std::move(game_ui)
        );

    game->finishUIInitialization();
    editor->changePanel(std::move(game_panel));
}

void Application::toStats()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));
    jassert(type == PanelType::MainMenu);
    type = PanelType::Stats;
    jassert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
    editor->changePanel(std::move(settings_menu));
}

void Application::toSettings()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));
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
                [this] { toSettings(); });
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
            printf("create\n");
            auto game_ui = game->createUI();
            panel = std::make_unique < GameUI_Panel > (
                [this] { game->nextClicked();  },
                [this] (bool was_target) { game->toggleInputOrTarget(was_target); },
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

void Application::broadcastDSP(const std::unordered_map < int, ChannelDSPState > &dsp_states)
{
    host.broadcastAllDSP(dsp_states);
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
}
    
void Application::deleteChannel(int id)
{
    auto channel = channels.find(id);
    jassert(channel != channels.end());

    if (game) {
        jassert(type == PanelType::Game);
        game->onChannelDelete(id);
    }
    channels.erase(channel);
}
    
void Application::renameChannelFromUI(int id, juce::String newName)
{
    channels[id].name = newName;
    
    if (game) {
        jassert(type == PanelType::Game);
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