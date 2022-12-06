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

void mixer_game_post_event(MixerGame_State *state, Event event, std::mutex* mutex)
{
    Effects effects;
    {
        if(mutex)
        std::lock_guard lock { *mutex };
        effects = mixer_game_update(state, event);
    }
    for(auto &observer : state->observers)
        observer(effects);

#if 0
    if (effects.rename)
    {
        state->app->renameChannelFromUI(effects.rename->id, effects.rename->new_name);
    }
#endif
    if (effects.quit)
    {
        state->on_quit();
    }
}

Effects mixer_game_update(MixerGame_State *state, Event event)
{
    GameStep step = state->step;
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
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
        case Event_Click_Track : 
        {
            jassertfalse;
        } break;
        case Event_Toggle_Track : 
        {
            jassertfalse;
        } break;
        case Event_Slider : 
        {
            state->edited_slider_pos[event.id] = event.value_i;
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : 
        {
            if (step == GameStep_Question)
            {
                assert(state->variant != MixerGame_Timer);
                assert(state->can_still_listen);
                if (state->mix == Mix_User)
                {
                    assert(event.value_b);
                    state->mix = Mix_Target;
                }
                else if (state->mix == Mix_Target)
                {
                    assert(!event.value_b);
                    switch (state->variant)
                    {
                        case MixerGame_Normal : 
                        {
                            state->mix = Mix_User;
                        } break;
                        case MixerGame_Timer : 
                        {
                            jassertfalse;
                        } break;
                        case MixerGame_Tries : 
                        {
                            state->remaining_listens--;
                            if(state->remaining_listens == 0)
                                done_listening = true;
                            else 
                                state->mix = Mix_User;
                        } break;
                    }
                }
                else jassertfalse;
            }
            else if (step == GameStep_Result)
            {
                if (state->mix == Mix_User)
                {
                    assert(event.value_b);
                    state->mix = Mix_Target;
                }
                else if (state->mix == Mix_Target)
                {
                    assert(!event.value_b);
                    state->mix = Mix_User;
                }
                else jassertfalse;
            }
            else jassertfalse;
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : 
        {
            assert(step == GameStep_Begin);
            assert(state->mix == Mix_Hidden);
            assert(state->target_slider_pos.size() == 0);
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Timer_Tick : 
        {
            state->current_timestamp = event.value_i64;

            if (step == GameStep_Question 
                && state->variant == MixerGame_Timer
                && state->mix == Mix_Target)
            {
                if (state->current_timestamp >= state->timestamp_start + state->timeout_ms)
                {
                    done_listening = true;
                }
            }
            else
            {
                assert(state->timestamp_start == -1);
            }
        } break;
        case Event_Click_Done_Listening : 
        {
            assert(step == GameStep_Question && state->variant == MixerGame_Timer);
            done_listening = true;
        } break;
        case Event_Click_Answer : 
        {
            assert(step == GameStep_Question);

            int points_awarded = 0;
            for (auto& [id, edited] : state->edited_slider_pos)
            {
                if(edited == state->target_slider_pos[id])
                {
                    points_awarded++;
                }
            }
            state->score += points_awarded;
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
        case Event_Click_Quit : 
        {
            jassertfalse;
        } break;
        case Event_Channel_Create : 
        {
            auto [edited, edited_result] = state->edited_slider_pos.emplace(event.id, true);
            juce::ignoreUnused(edited);
            assert(edited_result);
            juce::ignoreUnused(edited);
            auto [target, target_result] = state->target_slider_pos.emplace(event.id, true);
            assert(target_result);
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Delete : 
        {
            {
                const auto channel_to_remove = state->edited_slider_pos.find(event.id);
                assert(channel_to_remove != state->edited_slider_pos.end());
                state->edited_slider_pos.erase(channel_to_remove);
            }
        
            {
                const auto channel_to_remove = state->target_slider_pos.find(event.id);
                assert(channel_to_remove != state->target_slider_pos.end());
                state->target_slider_pos.erase(channel_to_remove);
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
        case Event_Change_Frequency_Range : 
        {
            jassertfalse;
        } break;
        case Event_Create_UI :
        {
            update_ui = true;
        } break;
        case Event_Destroy_UI :
        {
        } break;
    }
    
    if (done_listening)
    {
        assert(step == GameStep_Question);
        assert(state->mix == Mix_Target);
        state->mix = Mix_User;
        state->timestamp_start = -1;

        state->can_still_listen = false;
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
            state->score = 0;
            state->mix = Mix_Hidden;
            std::transform(state->channel_infos.begin(), state->channel_infos.end(), 
                           std::inserter(state->edited_slider_pos, state->edited_slider_pos.end()), 
                           [&](const auto &a) -> std::pair<int, int> {
                           return { a.first, (int)state->db_slider_values.size() - 2};
            });
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : 
        {
            step = GameStep_Question;
            state->mix = Mix_Target;
            state->can_still_listen = true;
            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            assert(state->target_slider_pos.size() == state->channel_infos.size());
            assert(state->edited_slider_pos.size() == state->channel_infos.size());
            
            switch (state->variant)
            {
                case MixerGame_Normal : {
                } break;
                case MixerGame_Timer : {
                    state->timestamp_start = state->current_timestamp;
                } break;
                case MixerGame_Tries : {
                    state->remaining_listens = state->listens;
                } break;
            }

            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            step = GameStep_Result;
            state->mix = Mix_Target;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        { 
            jassertfalse;
        }break;
    }
    
    std::unordered_map<int, int>* edit_or_target;
    if(step == GameStep_Begin || state->mix == Mix_User)
        edit_or_target = &state->edited_slider_pos;
    else
        edit_or_target = &state->target_slider_pos;

    if (update_audio)
    {
        std::unordered_map < int, Channel_DSP_State > dsp;
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [state](const auto &a) -> std::pair<int, Channel_DSP_State> {
                       double gain = slider_pos_to_gain(a.second, state->db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
        });
        effects.dsp = Effect_DSP { std::move(dsp) };
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == GameStep_Question && state->mix == Mix_Target)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = *edit_or_target;
        
        Mix mix;
        switch (state->variant)
        {
            case MixerGame_Normal : {
                mix = state->mix;
            } break;
            case MixerGame_Timer : {
                mix = Mix_Hidden;
            } break;
            case MixerGame_Tries : {
                if (step == GameStep_Question && state->remaining_listens == 0)
                    mix = Mix_Hidden;
                else
                    mix = state->mix;
            } break;
            default:
            {
                jassertfalse;
                mix = {};
            } break;
        }
        
        effects.ui = Effect_UI {
            .fader_step = gameStepToFaderStep(step, state->mix),
            .score = state->score,
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
                
                switch (state->variant)
                {
                    case MixerGame_Normal : {
                        effects.ui->header_text = "Reproduce the target mix";
                        effects.ui->button_text = "Validate";
                        effects.ui->button_event = Event_Click_Answer;
                    } break;
                    case MixerGame_Timer : {
                        if(state->mix == Mix_Target)
                        {
                            effects.ui->header_text = "Listen";
                            effects.ui->button_text = "Go";
                            effects.ui->button_event = Event_Click_Done_Listening;
                        }
                        else if (state->mix == Mix_User)
                        {
                            effects.ui->header_text = "Reproduce the target mix";
                            effects.ui->button_text = "Validate";
                            effects.ui->button_event = Event_Click_Answer;
                        }
                        else jassertfalse;
                    } break;
                    case MixerGame_Tries : {
                        if (state->mix == Mix_Target)
                        {
                            assert(state->can_still_listen);
                            effects.ui->header_text = juce::String("remaining listens : ") + juce::String(state->remaining_listens);
                        }
                        else if (state->mix == Mix_User)
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
            default:
            {
                jassertfalse;
            } break;
        }
    }

    state->step = step;
    return effects;
}

void mixer_game_add_observer(MixerGame_State *state, observer_t &&observer)
{
    state->observers.push_back(std::move(observer));
}


std::unique_ptr < MixerGame_State > mixer_game_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    MixerGame_Variant variant,
    int listens,
    int timeout_ms,
    std::vector<double> db_slider_values,
    std::function<void()> on_quit)
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
        .timestamp_start = -1,
        .on_quit = std::move(on_quit)
    };
    return std::make_unique < MixerGame_State > (std::move(state));
}