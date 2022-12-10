#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "Game.h"
#include "Game_Mixer.h"
#include "../Application/MainMenu.h"
#include "../Application/Application.h"



void mixer_game_ui_transitions(MixerGameUI &ui, Effect_Transition transition)
{
    juce::ignoreUnused(ui);
    if (transition.out_transition == GameStep_Begin)
    {
    }
    if (transition.in_transition == GameStep_EndResults)
    {
    }
}

void game_ui_update(const Effect_UI &new_ui, MixerGameUI &ui)
{
    if (new_ui.slider_pos_to_display)
    {
        assert(new_ui.slider_pos_to_display->size() == ui.faders.size());
    }
    for(auto& [id, fader] : ui.faders)
    {
        int pos = new_ui.slider_pos_to_display ? new_ui.slider_pos_to_display->at(id) : -1;
        fader->update(new_ui.fader_step, pos);
    }
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
    game_ui_bottom_update(&ui.bottom, true, new_ui.button_text, new_ui.mix, new_ui.button_event);
}

void mixer_game_post_event(MixerGame_IO *io, Event event)
{
    Effects effects = mixer_game_update(io->game_state, event);
    assert(effects.error == 0);
    {
        std::lock_guard lock { io->mutex };
        io->game_state = effects.new_state;
    }
    for(auto &observer : io->observers)
        observer(effects);
    if (effects.quit)
    {
        io->on_quit();
    }
}

Effects mixer_game_update(MixerGame_State state, Event event)
{
    GameStep step = state.step;
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
        .error = 0,
        .transition = std::nullopt, 
        .dsp = std::nullopt, 
        .ui = std::nullopt, 
        .rename = std::nullopt, 
        .quit = false
    };
    
    bool done_listening = false;

    switch (event.type)
    {
        case Event_Init : 
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Slider : 
        {
            state.edited_slider_pos[event.id] = event.value_i;
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : 
        {
            if (step == GameStep_Question)
            {
                if(state.variant == MixerGame_Timer) return { .error = 1 };
                if (!state.can_still_listen) return { .error = 1 };
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) return { .error = 1 };
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) return { .error = 1 };
                    switch (state.variant)
                    {
                        case MixerGame_Normal : 
                        {
                            state.mix = Mix_User;
                        } break;
                        case MixerGame_Timer : 
                        {
                            jassertfalse;
                        } break;
                        case MixerGame_Tries : 
                        {
                            state.remaining_listens--;
                            if(state.remaining_listens == 0)
                                done_listening = true;
                            else 
                                state.mix = Mix_User;
                        } break;
                    }
                }
                else jassertfalse;
            }
            else if (step == GameStep_Result)
            {
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) return { .error = 1 };
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) return { .error = 1 };
                    state.mix = Mix_User;
                }
                else jassertfalse;
            }
            else jassertfalse;
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : 
        {
            if(step != GameStep_Begin) return { .error = 1 };
            if(state.mix != Mix_Hidden) return { .error = 1 };
            if(state.target_slider_pos.size() != 0) return { .error = 1 };
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Timer_Tick : 
        {
            state.current_timestamp = event.value_i64;

            if (step == GameStep_Question 
                && state.variant == MixerGame_Timer
                && state.mix == Mix_Target)
            {
                if (state.current_timestamp >= state.timestamp_start + state.timeout_ms)
                {
                    done_listening = true;
                }
            }
            else
            {
                if(state.timestamp_start != -1) return { .error = 1 };
            }
        } break;
        case Event_Click_Done_Listening : 
        {
            if(step != GameStep_Question || state.variant != MixerGame_Timer) return { .error = 1 };
            done_listening = true;
        } break;
        case Event_Click_Answer : 
        {
            if(step != GameStep_Question) return { .error = 1 };

            int points_awarded = 0;
            for (auto& [id, edited] : state.edited_slider_pos)
            {
                if(edited == state.target_slider_pos[id])
                {
                    points_awarded++;
                }
            }
            state.score += points_awarded;
            out_transition = GameStep_Question;
            in_transition = GameStep_Result;
        } break;
        case Event_Click_Next : 
        {
            out_transition = GameStep_Result;
            in_transition = GameStep_Question;
        } break;
        case Event_Click_Back : 
        {
            effects.quit = true;
        } break;
        case Event_Channel_Create : 
        {
            auto [edited, edited_result] = state.edited_slider_pos.emplace(event.id, true);
            juce::ignoreUnused(edited);
            if(!edited_result) return { .error = 1 };
            juce::ignoreUnused(edited);
            auto [target, target_result] = state.target_slider_pos.emplace(event.id, true);
            if(!target_result) return { .error = 1 };
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Delete : 
        {
            {
                const auto channel_to_remove = state.edited_slider_pos.find(event.id);
                if(channel_to_remove == state.edited_slider_pos.end()) return { .error = 1 };
                state.edited_slider_pos.erase(channel_to_remove);
            }
        
            {
                const auto channel_to_remove = state.target_slider_pos.find(event.id);
                if(channel_to_remove == state.target_slider_pos.end()) return { .error = 1 };
                state.target_slider_pos.erase(channel_to_remove);
            }
            
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Rename_From_Track : 
        {
            //update channel infos ?
        } break;
        case Event_Channel_Rename_From_UI : 
        {
            effects.rename = Effect_Rename { event.id, event.value_str };
        } break;
        case Event_Create_UI :
        {
            update_ui = true;
        } break;
        case Event_Destroy_UI :
        {
        } break;
        case Event_Click_Quit : 
        case Event_Click_Track : 
        case Event_Click_Frequency :
        case Event_Toggle_Track : 
        case Event_Change_Frequency_Range : 
        {
            jassertfalse;
        } break;
    }
    
    if (done_listening)
    {
        if(step != GameStep_Question) return { .error = 1 };
        if(state.mix != Mix_Target) return { .error = 1 };
        state.mix = Mix_User;
        state.timestamp_start = -1;

        state.can_still_listen = false;
        update_audio = true;
        update_ui = true;
    }

    if (in_transition != GameStep_None || out_transition != GameStep_None)
    {
        effects.transition = {
            .in_transition = in_transition,
            .out_transition = out_transition
        };
    }
    switch (out_transition)
    {
        case GameStep_None :
        {
        } break;
        case GameStep_Begin :
        {
        } break;
        case GameStep_Question :
        {
        } break;
        case GameStep_Result :
        {
        } break;
        case GameStep_EndResults :
        {
            jassertfalse;
        } break;
    }

    switch (in_transition)
    {
        case GameStep_None : 
        {
        }break;
        case GameStep_Begin : 
        {
            step = GameStep_Begin;
            state.score = 0;
            state.mix = Mix_Hidden;
            std::transform(state.channel_infos.begin(), state.channel_infos.end(), 
                           std::inserter(state.edited_slider_pos, state.edited_slider_pos.end()), 
                           [&](const auto &a) -> std::pair<int, int> {
                           return { a.first, (int)state.db_slider_values.size() - 2};
            });
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : 
        {
            step = GameStep_Question;
            state.mix = Mix_Target;
            state.can_still_listen = true;
            for (auto& [_, channel] : state.channel_infos)
            {
                //I want to keep the value positive, it's an index !!!
                auto seed = juce::Random::getSystemRandom().nextInt();
                auto mod = static_cast<int>(state.db_slider_values.size());
                auto target_pos = ((seed % mod) + mod) % mod;

                state.target_slider_pos[channel.id] = target_pos;
                auto edited_pos = static_cast<int>(state.db_slider_values.size()) - 2;
                state.edited_slider_pos[channel.id] = edited_pos;
            }
            if(state.target_slider_pos.size() != state.channel_infos.size()) return { .error = 1 };
            if(state.edited_slider_pos.size() != state.channel_infos.size()) return { .error = 1 };
            
            switch (state.variant)
            {
                case MixerGame_Normal : {
                } break;
                case MixerGame_Timer : {
                    state.timestamp_start = state.current_timestamp;
                } break;
                case MixerGame_Tries : {
                    state.remaining_listens = state.listens;
                } break;
            }

            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            step = GameStep_Result;
            state.mix = Mix_Target;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        { 
            jassertfalse;
        }break;
    }
    
    std::unordered_map<int, int>* edit_or_target;
    if(step == GameStep_Begin || state.mix == Mix_User)
        edit_or_target = &state.edited_slider_pos;
    else
        edit_or_target = &state.target_slider_pos;

    if (update_audio)
    {
        std::unordered_map < int, Channel_DSP_State > dsp;
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [state](const auto &a) -> std::pair<int, Channel_DSP_State> {
                       double gain = slider_pos_to_gain(static_cast<size_t>(a.second), state.db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
        });
        effects.dsp = Effect_DSP { std::move(dsp) };
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == GameStep_Question && state.mix == Mix_Target)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = *edit_or_target;
        
        Mix mix;
        switch (state.variant)
        {
            case MixerGame_Normal : {
                mix = state.mix;
            } break;
            case MixerGame_Timer : {
                mix = Mix_Hidden;
            } break;
            case MixerGame_Tries : {
                if (step == GameStep_Question && state.remaining_listens == 0)
                    mix = Mix_Hidden;
                else
                    mix = state.mix;
            } break;
            default:
            {
                jassertfalse;
                mix = {};
            } break;
        }
        
        effects.ui = Effect_UI {
            .fader_step = gameStepToFaderStep(step, state.mix),
            .score = state.score,
            .slider_pos_to_display = std::move(slider_pos_to_display),
            .mix = mix
        };

        switch(step)
        {
            case GameStep_Begin :
            {
                effects.ui->header_text = "Have a listen";
                effects.ui->button_text = "Begin";
                effects.ui->button_event = Event_Click_Begin;
            } break;
            case GameStep_Question :
            {
                
                switch (state.variant)
                {
                    case MixerGame_Normal : {
                        effects.ui->header_text = "Reproduce the target mix";
                        effects.ui->button_text = "Validate";
                        effects.ui->button_event = Event_Click_Answer;
                    } break;
                    case MixerGame_Timer : {
                        if(state.mix == Mix_Target)
                        {
                            effects.ui->header_text = "Listen";
                            effects.ui->button_text = "Go";
                            effects.ui->button_event = Event_Click_Done_Listening;
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->header_text = "Reproduce the target mix";
                            effects.ui->button_text = "Validate";
                            effects.ui->button_event = Event_Click_Answer;
                        }
                        else jassertfalse;
                    } break;
                    case MixerGame_Tries : {
                        if (state.mix == Mix_Target)
                        {
                            if(!state.can_still_listen) return { .error = 1 };
                            effects.ui->header_text = juce::String("remaining listens : ") + juce::String(state.remaining_listens);
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->header_text = "Reproduce the target mix";
                        }
                        else jassertfalse;
                        effects.ui->button_text = "Validate";
                        effects.ui->button_event = Event_Click_Answer;
                    } break;
                }
            } break;
            case GameStep_Result :
            {
                effects.ui->header_text = "Results";
                effects.ui->button_text = "Next";
                effects.ui->button_event = Event_Click_Next;
            } break;
            case GameStep_None :
            case GameStep_EndResults :
            default :
            {
                jassertfalse;
            } break;
        }
    }

    state.step = step;
    effects.new_state = state;
    return effects;
}

void mixer_game_add_observer(MixerGame_IO *io, observer_t new_observer)
{
    io->observers.push_back(std::move(new_observer));
}


MixerGame_State mixer_game_state_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    MixerGame_Variant variant,
    int listens,
    int timeout_ms,
    std::vector<double> db_slider_values)
{
    if (variant != MixerGame_Tries)
        assert(listens == -1);
    if (variant != MixerGame_Timer)
        assert(timeout_ms == -1);

    MixerGame_State state = {
        .channel_infos = channel_infos,
        .variant = variant,
        .listens = listens,
        .timeout_ms = timeout_ms,
        .db_slider_values = db_slider_values,
        .timestamp_start = -1
    };
    return state;
}


std::unique_ptr<MixerGame_IO> mixer_game_io_init(MixerGame_State state)
{
    auto io = std::make_unique<MixerGame_IO>();
    io->game_state = state;
    return io;
}