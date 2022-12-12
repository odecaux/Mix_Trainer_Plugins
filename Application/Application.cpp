#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "../Game/Game.h"
#include "../Game/Game_Mixer.h"
#include "MainMenu.h"
#include "Application.h"
#include "../Plugin_Host/Processor_Host.h"
#include "../Plugin_Host/PluginEditor_Host.h"

Application::Application(ProcessorHost &processorHost) :
    game_ui(nullptr),
    type(PanelType::MainMenu),
    host(processorHost), 
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
    assert(!game_io);
    
    MixerGame_State new_game_state = [&] {
        switch (variant)
        {
            case MixerGame_Normal : {
                return mixer_game_state_init(channels, MixerGame_Normal, -1, -1, db_slider_values);
            } break;
            case MixerGame_Timer : {
                return mixer_game_state_init(channels, MixerGame_Timer, -1, 2000, db_slider_values);
            } break;
            case MixerGame_Tries : {
                return mixer_game_state_init(channels, MixerGame_Tries, 5, -1, db_slider_values);
            } break;
        }
        assert(false);
        return MixerGame_State{};
    }();

    editor->changePanel(nullptr);

    auto observer = [this] (const Game_Mixer_Effects &effects)
    {
        if (effects.transition)
        {
            if (effects.transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < MixerGameUI > (
                    channels,
                    db_slider_values,
                    game_io.get()
                );
                game_ui = new_game_ui.get();
                editor->changePanel(std::move(new_game_ui));
            }
            if(game_ui)
                mixer_game_ui_transitions(*game_ui, *effects.transition);

        }
        if (effects.dsp)
        {
            broadcastDSP(effects.dsp->dsp_states); 
        }
        if (effects.ui)
        {
            assert(game_ui);
            game_ui_update(*effects.ui, *game_ui); 
        }
    };

    auto debug_observer = [this] (const Game_Mixer_Effects &effects)
    {
        if (effects.dsp)
        {
            channel_dsp_log(effects.dsp->dsp_states, channels); 
        }
    };

    assert(!game_io);
    game_io = mixer_game_io_init(new_game_state);
    
    auto on_quit = [this] { 
        game_io->timer.stopTimer();
        game_io.reset();  
        toMainMenu();
    };

    game_io->on_quit = std::move(on_quit);

    mixer_game_add_observer(game_io.get(), std::move(observer));
    mixer_game_add_observer(game_io.get(), std::move(debug_observer));
    
    game_io->timer.callback = [io = game_io.get()] (juce::int64 timestamp) {
        mixer_game_post_event(io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
    };
    game_io->timer.startTimerHz(60);

    mixer_game_post_event(game_io.get(), Event { .type = Event_Init });
    mixer_game_post_event(game_io.get(), Event { .type = Event_Create_UI });
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

void Application::onEditorDelete()
{
    assert(editor != nullptr);
    editor = nullptr;
    if (type == PanelType::Game)
    {
        mixer_game_post_event(game_io.get(), Event { .type = Event_Destroy_UI });
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
            assert(game_io); 
            panel = std::make_unique<MixerGameUI>(
                channels,
                db_slider_values,
                game_io.get()
            );
            mixer_game_post_event(game_io.get(), Event { .type = Event_Create_UI });
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

    if (game_io) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Create,
            .id = id
        };
        mixer_game_post_event(game_io.get(), event);
    }
}
    
void Application::deleteChannel(int id)
{
    auto channel = channels.find(id);
    assert(channel != channels.end());

    if (game_io) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Delete,
            .id = id
        };
        mixer_game_post_event(game_io.get(), event);
    }
    channels.erase(channel);
}
    
void Application::renameChannelFromUI(int id, juce::String new_name)
{
    channels[id].name = new_name;
    
    if (game_io) {
        assert(type == PanelType::Game);
    }
    host.broadcastRenameTrack(id, new_name);
}
    
void Application::renameChannelFromTrack(int id, const juce::String &new_name)
{
    auto &channel = channels[id];
    channel.name = new_name;
        
    if (game_io) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Channel_Rename_From_Track,
            .id = id,
            .value_js = new_name //copy
        };
        mixer_game_post_event(game_io.get(), event);
    }
}
    
void Application::changeFrequencyRange(int id, float new_min, float new_max)
{
    auto &channel = channels[id];
    channel.min_freq = new_min;
    channel.max_freq = new_max;

    if (game_io) {
        assert(type == PanelType::Game);
        Event event = {
            .type = Event_Change_Frequency_Range,
            .id = id,
            .value_f = new_min,
            .value_f_2 = new_max
        };
        mixer_game_post_event(game_io.get(), event);
    }
}