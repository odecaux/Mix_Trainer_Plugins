#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"
#include "../shared/shared_ui.h"
#include "Game.h"
#include "Game_UI.h"
#include "Game_Mixer.h"
#include "../Plugin_Host/Main_Menu.h"
#include "../Plugin_Host/Application.h"

void mixer_game_post_event(MixerGame_IO *io, Event event)
{
    Game_Mixer_Effects effects = mixer_game_update(io->game_state, event);
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

Game_Mixer_Effects mixer_game_update(MixerGame_State state, Event event)
{
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;
    
    Game_Mixer_Effects effects = { 
        .error = 0,
        .transition = std::nullopt, 
        .dsp = std::nullopt, 
        .ui = std::nullopt, 
        .quit = false
    };
    
    bool done_listening = false;
    bool check_answer = false;
    
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
            if (state.step == GameStep_Question)
            {
                if(state.config.variant == MixerGame_Timer) return { .error = 1 };
                if (!state.can_still_listen) return { .error = 1 };
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) return { .error = 1 };
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) return { .error = 1 };
                    switch (state.config.variant)
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
            else if (state.step == GameStep_Result)
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
            else
            {
                jassertfalse;
            }
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : 
        {
            if(state.step != GameStep_Begin) return { .error = 1 };
            if(state.mix != Mix_Hidden) return { .error = 1 };
            if(state.target_slider_pos.size() != 0) return { .error = 1 };
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Timer_Tick : 
        {
            state.current_timestamp = event.value_i64;
            
            if (state.step == GameStep_Question 
                && state.config.variant == MixerGame_Timer
                && state.mix == Mix_Target)
            {
                if (state.current_timestamp >= state.timestamp_start + state.config.timeout_ms)
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
            if(state.step != GameStep_Question || state.config.variant != MixerGame_Timer) return { .error = 1 };
            done_listening = true;
        } break;
        case Event_Click_Answer : 
        {
            check_answer = true;
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
        {
            jassertfalse;
        } break;
    }
    
    if (check_answer)
    {
        if(state.step != GameStep_Question) return { .error = 1 };
        
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
    }
    
    if (done_listening)
    {
        if(state.step != GameStep_Question) return { .error = 1 };
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
            state.step = GameStep_Begin;
            state.score = 0;
            state.mix = Mix_Hidden;
            std::transform(state.config.channel_infos.begin(), state.config.channel_infos.end(), 
                           std::inserter(state.edited_slider_pos, state.edited_slider_pos.end()), 
                           [&](const auto &a) -> std::pair<int, int> {
                               return { a.first, (int)state.config.db_slider_values.size() - 2};
                           });
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : 
        {
            state.step = GameStep_Question;
            state.mix = Mix_Target;
            state.can_still_listen = true;
            for (auto& [_, channel] : state.config.channel_infos)
            {
                auto target_pos = random_uint(static_cast<int>(state.config.db_slider_values.size()));
                state.target_slider_pos[channel.id] = target_pos;
                auto edited_pos = static_cast<int>(state.config.db_slider_values.size()) - 2;
                state.edited_slider_pos[channel.id] = edited_pos;
            }
            if(state.target_slider_pos.size() != state.config.channel_infos.size()) return { .error = 1 };
            if(state.edited_slider_pos.size() != state.config.channel_infos.size()) return { .error = 1 };
            
            switch (state.config.variant)
            {
                case MixerGame_Normal : {
                } break;
                case MixerGame_Timer : {
                    state.timestamp_start = state.current_timestamp;
                } break;
                case MixerGame_Tries : {
                    state.remaining_listens = state.config.listens;
                } break;
            }
            
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            state.step = GameStep_Result;
            state.mix = Mix_Target;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        { 
            jassertfalse;
        }break;
    }
    
    std::unordered_map<uint32_t, uint32_t>* slider_pos;
    if(state.step == GameStep_Begin || state.mix == Mix_User)
        slider_pos = &state.edited_slider_pos;
    else
        slider_pos = &state.target_slider_pos;
    
    if (update_audio)
    {
        std::unordered_map < uint32_t, Channel_DSP_State > dsp;
        std::transform(slider_pos->begin(), slider_pos->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [state](const auto &a) -> std::pair<uint32_t, Channel_DSP_State> {
                           double gain_db = state.config.db_slider_values[static_cast<size_t>(a.second)];
                           return { a.first, ChannelDSP_gain_db(gain_db) };
                       });
        effects.dsp = Game_Mixer_Effect_DSP { std::move(dsp) };
    }
    
    if (update_ui)
    {
        std::optional < std::unordered_map<uint32_t, uint32_t> > slider_pos_to_display;
        
        if(state.step == GameStep_Question && state.mix == Mix_Target)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = *slider_pos;
        
        effects.ui = Game_Mixer_Effect_UI{};
        effects.ui->widget_visibility = gameStepToFaderStep(state.step, state.mix);
        effects.ui->slider_pos_to_display = std::move(slider_pos_to_display);
        
        switch (state.config.variant)
        {
            case MixerGame_Normal : {
                effects.ui->mix_toggles = state.mix;
            } break;
            case MixerGame_Timer : {
                effects.ui->mix_toggles = Mix_Hidden;
            } break;
            case MixerGame_Tries : {
                if (state.step == GameStep_Question && state.remaining_listens == 0)
                    effects.ui->mix_toggles = Mix_Hidden;
                else
                    effects.ui->mix_toggles = state.mix;
            } break;
        }
        
        switch(state.step)
        {
            case GameStep_Begin :
            {
                effects.ui->header_center_text = "Have a listen";
                effects.ui->bottom_button_text = "Begin";
                effects.ui->bottom_button_event = Event_Click_Begin;
            } break;
            case GameStep_Question :
            {
                effects.ui->header_right_text = "Score : " + std::to_string(state.score);
                
                switch (state.config.variant)
                {
                    case MixerGame_Normal : {
                        effects.ui->header_center_text = "Reproduce the target mix";
                        effects.ui->bottom_button_text = "Validate";
                        effects.ui->bottom_button_event = Event_Click_Answer;
                    } break;
                    case MixerGame_Timer : {
                        if(state.mix == Mix_Target)
                        {
                            effects.ui->header_center_text = "Listen";
                            effects.ui->bottom_button_text = "Go";
                            effects.ui->bottom_button_event = Event_Click_Done_Listening;
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->header_center_text = "Reproduce the target mix";
                            effects.ui->bottom_button_text = "Validate";
                            effects.ui->bottom_button_event = Event_Click_Answer;
                        }
                        else jassertfalse;
                    } break;
                    case MixerGame_Tries : {
                        if (state.mix == Mix_Target)
                        {
                            if(!state.can_still_listen) return { .error = 1 };
                            effects.ui->header_center_text = "remaining listens : " + std::to_string(state.remaining_listens);
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->header_center_text = "Reproduce the target mix";
                        }
                        else jassertfalse;
                        effects.ui->bottom_button_text = "Validate";
                        effects.ui->bottom_button_event = Event_Click_Answer;
                    } break;
                }
            } break;
            case GameStep_Result :
            {
                effects.ui->header_right_text = "Score : " + std::to_string(state.score);
                effects.ui->header_center_text = "Results";
                effects.ui->bottom_button_text = "Next";
                effects.ui->bottom_button_event = Event_Click_Next;
            } break;
            case GameStep_None :
            case GameStep_EndResults :
            default :
            {
                jassertfalse;
            } break;
        }
    }
    effects.new_state = state;
    return effects;
}

void mixer_game_add_observer(MixerGame_IO *io, mixer_game_observer_t new_observer)
{
    io->observers.push_back(std::move(new_observer));
}


MixerGame_State mixer_game_state_init(std::unordered_map<uint32_t, Game_Channel> &channel_infos,
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
        .config = {
            .channel_infos = channel_infos,
            .db_slider_values = db_slider_values,
            .variant = variant,
            .listens = listens,
            .timeout_ms = timeout_ms,
        },
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