#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../shared/shared_ui.h"
#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "../Game/Game_Mixer.h"
#include "../Game/Game_Mixer_UI.h"
#include "Main_Menu.h"
#include "Application.h"
#include "Processor_Host.h"
#include "PluginEditor_Host.h"

Application::Application(ProcessorHost &processorHost) :
    game_ui(nullptr),
    type(Panel_MainMenu),
    host(processorHost), 
    editor(nullptr)
{
    //NOTE useless
    broadcastDSP(bypassedAllChannelsDSP(multitrack_model.game_channels));
    //multitrack_model_add_observer(&multitrack_model, MultiTrack_Observers_Debug, debug_multitrack_model);
    multitrack_model_add_observer(&multitrack_model, 
                                  MultiTrack_Observers_Broadcast, 
                                  [&host = host] (auto *model) { host.broadcastChannelList(*model);});
}


void Application::toMainMenu()
{
    broadcastDSP(bypassedAllChannelsDSP(multitrack_model.game_channels));

    assert(type != Panel_MainMenu);
    type = Panel_MainMenu;
    assert(editor);
        
    auto main_menu =
        std::make_unique < MainMenu > (
            [this] { toGame(MixerGame_Normal); },
            [this] { toGame(MixerGame_Timer); },
            [this] { toGame(MixerGame_Tries); },
            [this] { toChannelSettings(); },
            [this] { toStats(); },
            [this] { toSettings(); },
            &multitrack_model
        );
    editor->changePanel(std::move(main_menu));
}

static void channel_dsp_log(const std::unordered_map<uint32_t, Channel_DSP_State> &dsps, 
                    const std::unordered_map<uint32_t, Game_Channel> &channels)
{
    assert(dsps.size() == channels.size());
    for (const auto& [id, channel] : channels)
    {
        assert(id == channel.id);
        assert(dsps.contains(id));
        const auto &dsp = dsps.at(id);
        juce::String db_str = juce::Decibels::toString(dsp.gain_db);
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
    assert(type == Panel_MainMenu);
    type = Panel_Game;
    assert(editor);
    assert(!game_io);
    
    MixerGame_State new_game_state = [&] {
        std::vector<Game_Channel> ordered_channels;
        {
            ordered_channels.reserve(multitrack_model.game_channels.size());
            assert(multitrack_model.game_channels.size() == multitrack_model.order.size());
            for (uint32_t id : multitrack_model.order)
            {
                ordered_channels.push_back(multitrack_model.game_channels.at(id));
            }
        }
        switch (variant)
        {
            case MixerGame_Normal : {
                return mixer_game_state_init(std::move(ordered_channels), MixerGame_Normal, -1, -1, db_slider_values);
            } break;
            case MixerGame_Timer : {
                return mixer_game_state_init(std::move(ordered_channels), MixerGame_Timer, -1, 2000, db_slider_values);
            } break;
            case MixerGame_Tries : {
                return mixer_game_state_init(std::move(ordered_channels), MixerGame_Tries, 5, -1, db_slider_values);
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
                std::vector<Game_Channel> ordered_channels;
                {
                    ordered_channels.reserve(multitrack_model.game_channels.size());
                    assert(multitrack_model.game_channels.size() == multitrack_model.order.size());
                    for (uint32_t id : multitrack_model.order)
                    {
                        ordered_channels.push_back(multitrack_model.game_channels.at(id));
                    }
                }

                auto new_game_ui = std::make_unique < MixerGameUI > (
                    ordered_channels,
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
            channel_dsp_log(effects.dsp->dsp_states, multitrack_model.game_channels); 
        }
    };

    assert(game_io == nullptr);
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

void Application::toChannelSettings()
{
    broadcastDSP(bypassedAllChannelsDSP(multitrack_model.game_channels));
    assert(type == Panel_MainMenu);
    type = Panel_Channel_Settings;
    assert(editor);
    
    auto channel_settings_menu =
        std::make_unique < ChannelSettingsMenu > (multitrack_model, [this] { toMainMenu(); });
    editor->changePanel(std::move(channel_settings_menu));
}

void Application::toStats()
{
    broadcastDSP(bypassedAllChannelsDSP(multitrack_model.game_channels));
    assert(type == Panel_MainMenu);
    type = Panel_Stats;
    assert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
    editor->changePanel(std::move(settings_menu));
}

void Application::toSettings()
{
    broadcastDSP(bypassedAllChannelsDSP(multitrack_model.game_channels));
    assert(type == Panel_MainMenu);
    type = Panel_Settings;
    assert(editor);
    
    std::unique_ptr < juce::Component > settings_menu =
        std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
    editor->changePanel(std::move(settings_menu));
   
}

void Application::onEditorDelete()
{
    assert(editor != nullptr);
    editor = nullptr;
    if (type == Panel_Game)
    {
        assert(game_io);
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
        case Panel_MainMenu : {
            panel = std::make_unique < MainMenu > (
                [this] { toGame(MixerGame_Normal); },
                [this] { toGame(MixerGame_Timer); },
                [this] { toGame(MixerGame_Tries); },
                [this] { toChannelSettings(); },
                [this] { toStats(); },
                [this] { toSettings(); },
                &multitrack_model
            );
        } break;
        case Panel_Channel_Settings : {
            std::unique_ptr<ChannelSettingsMenu> channel_settings_menu = 
                std::make_unique < ChannelSettingsMenu > (multitrack_model, [this] { toMainMenu(); });
            panel = std::move(channel_settings_menu);
        } break;
        case Panel_Settings : {
            panel = std::make_unique < SettingsMenu > ([this] { toMainMenu(); }, settings);
        } break;
        case Panel_Stats : {
            panel = std::make_unique < StatsMenu > ([this] { toMainMenu(); }, stats);
        } break;
        case Panel_Game :
        {
            assert(game_io); 
            
            std::vector<Game_Channel> ordered_channels;
            {
                ordered_channels.reserve(multitrack_model.game_channels.size());
                assert(multitrack_model.game_channels.size() == multitrack_model.order.size());
                for (uint32_t id : multitrack_model.order)
                {
                    ordered_channels.push_back(multitrack_model.game_channels.at(id));
                }
            }
            panel = std::make_unique<MixerGameUI>(
                ordered_channels,
                db_slider_values, //TODO ??
                game_io.get()
            );
            mixer_game_post_event(game_io.get(), Event { .type = Event_Create_UI });
        } break;
    }
    editor->changePanel(std::move(panel));
}

void Application::broadcastDSP(const std::unordered_map < uint32_t, Channel_DSP_State > &dsp_states)
{
    host.broadcastAllDSP(dsp_states);
}

void Application::create_daw_channel(uint32_t daw_channel_id)
{
    //DBG("action create " << daw_channel_id % 100 );
    auto [it, result] = multitrack_model.daw_channels.emplace(daw_channel_id, Daw_Channel { .id = daw_channel_id, .assigned_game_channel_id = -1  });
    assert(result);
    multitrack_model_broadcast_change(&multitrack_model);
}
    
void Application::delete_daw_channel(uint32_t daw_channel_id)
{    
    //DBG("action delete " << daw_channel_id % 100 << " " << multitrack_model.daw_channels[daw_channel_id].name);
    {
        auto erased_count = multitrack_model.daw_channels.erase(daw_channel_id);
        assert(erased_count == 1);
    }
    multitrack_model_broadcast_change(&multitrack_model);
}
    
#if 0
void Application::renameChannelFromUI(uint32_t id, juce::String new_name)
{
    juce::ignoreUnused(id);
    juce::ignoreUnused(new_name);
    multitrack_model.game_channels.at(id).name = new_name;
    
    if (game_state) {
        assert(type == Panel_Game);
    }
    host.broadcastRenameTrack(id, new_name);
    multitrack_model_broadcast_change(&multitrack_model);
}
#endif
    
void Application::rename_daw_channel(uint32_t daw_channel_id, const juce::String &new_name)
{
    //DBG("action rename " << daw_channel_id % 100 << " " << daw_channel.name << " into " << new_name);
    auto &daw_channel = multitrack_model.daw_channels.at(daw_channel_id);

    new_name.copyToUTF8(
        daw_channel.name,
        sizeof(daw_channel.name)
    );
    multitrack_model_broadcast_change(&multitrack_model);
}
    
void Application::change_frequency_range_from_daw(uint32_t daw_channel_id, uint32_t game_channel_id, float new_min, float new_max)
{
    //DBG("action frequency " << daw_channel_id % 100);
    auto &daw_channel = multitrack_model.daw_channels.at(daw_channel_id);
    assert(daw_channel.assigned_game_channel_id != -1);
    assert(daw_channel.assigned_game_channel_id == game_channel_id);
    auto &game_channel = multitrack_model.game_channels.at(game_channel_id);
    game_channel.min_freq = new_min;
    game_channel.max_freq = new_max;
    multitrack_model_broadcast_change(&multitrack_model);
}


void Application::bind_daw_channel_with_game_channel(uint32_t daw_channel_id, uint32_t game_channel_id)
{
    //DBG("action bind " << daw_channel_id % 100 << " " << game_channel_id % 100);
    assert(multitrack_model.daw_channels.contains(daw_channel_id));
    multitrack_model.daw_channels.at(daw_channel_id).assigned_game_channel_id = game_channel_id;
    if (game_channel_id != -1)
    {
        //assert(multitrack_model.game_channels.contains(game_channel_id));
    }
    multitrack_model_broadcast_change(&multitrack_model);
}

void Application::set_model(const std::vector<Game_Channel> &channels)
{
    auto size = channels.size();
    juce::ignoreUnused(size);
    assert(multitrack_model.game_channels.size() == multitrack_model.order.size());
    multitrack_model.game_channels.clear();
    multitrack_model.order.clear();
    for (const auto& channel : channels)
    {
        multitrack_model.game_channels.emplace(channel.id, channel);
        multitrack_model.order.push_back(channel.id);
    }
    DBG("SET");
    multitrack_model_broadcast_change(&multitrack_model);
}

std::vector<Game_Channel> Application::save_model()
{
    assert(multitrack_model.game_channels.size() == multitrack_model.order.size());
    std::vector<Game_Channel> out_vector{};
    for (const auto& channel_id : multitrack_model.order)
    {
        out_vector.push_back(multitrack_model.game_channels.at(channel_id));
    }
    return out_vector;
}