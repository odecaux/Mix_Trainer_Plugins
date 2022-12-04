#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "../Game/Game.h"
#include "../Game/Game_Mixer.h"
#include "MainMenu.h"
#include "Application.h"
#include "../Plugin_Host/Processor_Host.h"
#include "../Plugin_Host/PluginEditor_Host.h"

Application::Application(ProcessorHost &host) :
    type(PanelType::MainMenu),
    host(host), 
    editor(nullptr)
{
    //NOTE useless
    broadcastDSP(bypassedAllChannelsDSP(channels));
}


void Application::toMainMenu()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));

    assert(type != PanelType::MainMenu);
    type = PanelType::MainMenu;
    assert(editor);
        
    auto main_menu =
        std::make_unique < MainMenu > (
            [this] { toGame(MixerGame_Normal); },
            [this] { toGame(MixerGame_Timer); },
            [this] { toGame(MixerGame_Tries); },
            [this] { toStats(); },
            [this] { toSettings(); }
        );
    editor->changePanel(std::move(main_menu));
}

static void channel_dsp_log(const std::unordered_map<int, Channel_DSP_State> &dsps, 
                    const std::unordered_map<int, ChannelInfos> &channels)
{
    assert(dsps.size() == channels.size());
    for (const auto& [id, channel] : channels)
    {
        assert(id == channel.id);
        assert(dsps.contains(id));
        const auto &dsp = dsps.at(id);
        double db = juce::Decibels::gainToDecibels(dsp.gain);
        juce::String db_str = juce::Decibels::toString(db);
        juce::String output_str = juce::String(id) + " " + channel.name + " " + db_str;
#if 0
        printf("%s", output_str);
#endif
#if 1
        DBG(output_str);
#endif
    }
}

void Application::toGame(MixerGame_Variant variant)
{
    assert(type == PanelType::MainMenu);
    type = PanelType::Game;
    assert(editor);
    assert(!game_state);
    
    switch (variant)
    {
        case MixerGame_Normal : {
            game_state = mixer_game_init(channels, MixerGame_Normal, -1, -1, std::vector<double> { -100.0, -12.0, -9.0, -6.0, -3.0 }, this);
        } break;
        case MixerGame_Timer : {
            game_state = mixer_game_init(channels, MixerGame_Timer, -1, 2000, std::vector<double> { -100.0, -12.0, -9.0, -6.0, -3.0 }, this);
        } break;
        case MixerGame_Tries : {
            game_state = mixer_game_init(channels, MixerGame_Tries, 5, -1, std::vector<double> { -100.0, -12.0, -9.0, -6.0, -3.0 }, this);
        } break;
    }
    mixer_game_add_audio_observer(
        game_state.get(), 
        [this] (auto &&effect) { 
            broadcastDSP(effect.dsp_states); 
        }
    );
    mixer_game_add_audio_observer(
        game_state.get(), 
        [this] (auto &&effect){ 
            channel_dsp_log(effect.dsp_states, channels); 
        }
    );
    mixer_game_post_event(game_state.get(), Event { .type = Event_Init });

    auto game_panel = std::make_unique<MixerGameUI>(
        channels,
        game_state->db_slider_values,
        game_state.get()
    );
    game_ui = game_panel.get();        

    mixer_game_add_ui_observer(
        game_state.get(), 
        [this] (auto &&effect){ 
            game_ui_update(effect, *game_ui); 
        }
    );      
    mixer_game_post_event(game_state.get(), Event { .type = Event_Create_UI, .value_ptr = game_ui });

    editor->changePanel(std::move(game_panel));
}

void Application::toStats()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));
    assert(type == PanelType::MainMenu);
    type = PanelType::Stats;
    assert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
    editor->changePanel(std::move(settings_menu));
}

void Application::toSettings()
{
    broadcastDSP(bypassedAllChannelsDSP(channels));
    assert(type == PanelType::MainMenu);
    type = PanelType::Settings;
    assert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
    editor->changePanel(std::move(settings_menu));
   
}

void Application::quitGame()
{
    game_state.reset();  
    toMainMenu();
}

void Application::onEditorDelete()
{
    assert(editor != nullptr);
    editor = nullptr;
    if (type == PanelType::Game)
    {
        assert(game_state);
        mixer_game_post_event(game_state.get(), Event { .type = Event_Destroy_UI });
        game_ui = nullptr;
    }
}


void Application::initialiseEditorUI(EditorHost *new_editor)
{
    assert(editor == nullptr);
    editor = new_editor;
    std::unique_ptr < juce::Component > panel;
    switch (type)
    {
        case PanelType::MainMenu : {
            panel = std::make_unique < MainMenu > (
                [this] { toGame(MixerGame_Normal); },
                [this] { toGame(MixerGame_Timer); },
                [this] { toGame(MixerGame_Tries); },
                [this] { toStats(); },
                [this] { toSettings(); }
            );
        } break;
        case PanelType::Settings : {
            panel = std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
        } break;
        case PanelType::Stats : {
            panel = std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
        } break;
        case PanelType::Game :
        {
            assert(game_state); 
            auto game_ui_temp = std::make_unique<MixerGameUI>(
                channels,
                game_state->db_slider_values,
                game_state.get()
            );
            game_ui = game_ui_temp.get();
            mixer_game_post_event(game_state.get(), Event { .type = Event_Create_UI, .value_ptr = game_ui });
            panel = std::move(game_ui_temp);
        } break;
    }
    editor->changePanel(std::move(panel));
}

void Application::broadcastDSP(const std::unordered_map < int, Channel_DSP_State > &dsp_states)
{
    host.broadcastAllDSP(dsp_states);
}

void Application::createChannel(int id)
{
    {
        auto assertChannel = channels.find(id);
        assert(assertChannel == channels.end());
    }
    
    channels[id] = ChannelInfos { .id = id };

    if (game_state) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Create,
            .id = id
        };
        mixer_game_post_event(game_state.get(), event);
    }
}
    
void Application::deleteChannel(int id)
{
    auto channel = channels.find(id);
    assert(channel != channels.end());

    if (game_state) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Delete,
            .id = id
        };
        mixer_game_post_event(game_state.get(), event);
    }
    channels.erase(channel);
}
    
void Application::renameChannelFromUI(int id, juce::String new_name)
{
    channels[id].name = new_name;
    
    if (game_state) {
        assert(type == PanelType::Game);
    }
    host.broadcastRenameTrack(id, new_name);
}
    
void Application::renameChannelFromTrack(int id, const juce::String &new_name)
{
    auto &channel = channels[id];
    channel.name = new_name;
        
    if (game_state) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Rename_From_Track,
            .id = id,
            .value_js = new_name //copy
        };
        mixer_game_post_event(game_state.get(), event);
    }
}
    
void Application::changeFrequencyRange(int id, float new_min, float new_max)
{
    auto &channel = channels[id];
    channel.min_freq = new_min;
    channel.max_freq = new_max;

    if (game_state) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Change_Frequency_Range,
            .id = id,
            .value_f = new_min,
            .value_f_2 = new_max
        };
        mixer_game_post_event(game_state.get(), event);
    }
}