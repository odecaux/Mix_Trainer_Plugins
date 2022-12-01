#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "Game.h"
#include "Game_Mixer.h"
#include "../Application/MainMenu.h"
#include "../Application/Application.h"


void game_ui_update(Effect_UI &new_ui, MixerGameUI &ui)
{
    if (new_ui.slider_pos_to_display)
    {
        jassert(new_ui.slider_pos_to_display->size() == ui.faders.size());
    }
    for(auto& [id, fader] : ui.faders)
    {
        int pos = new_ui.slider_pos_to_display ? new_ui.slider_pos_to_display->at(id) : -1;
        fader->update(new_ui.fader_step, pos);
    }
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
    game_ui_bottom_update(&ui.bottom, true, new_ui.button_text, new_ui.mix, new_ui.button_event);
}

void mixer_game_post_event(MixerGame_State *state, Event event)
{
    Effects effects = state->update_fn(state, event);
    if (effects.dsp)
    {
        for(auto &observer : state->observers_audio)
            observer(*effects.dsp);
    }
    if (effects.ui)
    {
        for(auto &observer : state->observers_ui)
            observer(*effects.ui);
    }
    if (effects.timer)
    {
        jassert(!state->timer.isTimerRunning());
        state->timer.callback = std::move(effects.timer->callback); 
        state->timer.startTimer(effects.timer->timeout_ms);
    }
    if (effects.rename)
    {
        state->app->renameChannelFromUI(effects.rename->id, effects.rename->new_name);
    }
    if (effects.quit)
    {
        state->timer.stopTimer();
        state->app->quitGame();
    }
}

Effects mixer_game_update(MixerGame_State *state, Event event)
{
    GameStep step = state->step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
        .dsp = std::nullopt, 
        .ui = std::nullopt, 
        .rename = std::nullopt, 
        .quit = false, 
        .timer = std::nullopt 
    };
    
    bool done_listening = false;

    switch (event.type)
    {
        case Event_Init : {
            transition = Transition_To_Begin;
        } break;
        case Event_Click_Track : {
            jassertfalse;
        } break;
        case Event_Toggle_Track : {
            jassertfalse;
        } break;
        case Event_Slider : {
            state->edited_slider_pos[event.id] = event.value_i;
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : {
            if (step == GameStep_Question)
            {
                jassert(state->variant != MixerGame_Timer);
                jassert(state->can_still_listen);
                if (state->mix == Mix_User)
                {
                    jassert(event.value_b);
                    state->mix = Mix_Target;
                }
                else if (state->mix == Mix_Target)
                {
                    jassert(!event.value_b);
                    switch (state->variant)
                    {
                        case MixerGame_Normal : {
                            state->mix = Mix_User;
                        } break;
                        case MixerGame_Timer : {
                            jassertfalse;
                        } break;
                        case MixerGame_Tries : {
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
                    jassert(event.value_b);
                    state->mix = Mix_Target;
                }
                else if (state->mix == Mix_Target)
                {
                    jassert(!event.value_b);
                    state->mix = Mix_User;
                }
                else jassertfalse;
            }
            else jassertfalse;
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : {
            jassert(step == GameStep_Begin);
            jassert(state->mix == Mix_Hidden);
            jassert(state->target_slider_pos.size() == 0);
            transition = Transition_To_Exercice;
        } break;
        case Event_Timeout : {
            done_listening = true;
        } break;
        case Event_Click_Done_Listening : {
            jassert(state->timer.isTimerRunning());
            state->timer.stopTimer();
            done_listening = true;
        } break;
        case Event_Click_Answer : {
            jassert(step == GameStep_Question);

            int points_awarded = 0;
            for (auto& [id, edited] : state->edited_slider_pos)
            {
                if(edited == state->target_slider_pos[id])
                {
                    points_awarded++;
                }
            }
            state->score += points_awarded;
            transition = Transition_To_Answer;
        } break;
        case Event_Click_Next : {
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Back : {
            effects.quit = true;
        } break;
        case Event_Click_Quit : {
            jassertfalse;
        } break;
        case Event_Channel_Create : {
            auto [edited, edited_result] = state->edited_slider_pos.emplace(event.id, true);
            jassert(edited_result);
            auto [target, target_result] = state->target_slider_pos.emplace(event.id, true);
            jassert(target_result);
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Delete : {
            {
                const auto channel_to_remove = state->edited_slider_pos.find(event.id);
                jassert(channel_to_remove != state->edited_slider_pos.end());
                state->edited_slider_pos.erase(channel_to_remove);
            }
        
            {
                const auto channel_to_remove = state->target_slider_pos.find(event.id);
                jassert(channel_to_remove != state->target_slider_pos.end());
                state->target_slider_pos.erase(channel_to_remove);
            }
            
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Channel_Rename_From_Track : {
            //update channel infos ?
        } break;
        case Event_Channel_Rename_From_UI : {
            effects.rename = Effect_Rename { event.id, event.value_str };
        } break;
        case Event_Change_Frequency_Range : {
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
        jassert(step == GameStep_Question);
        jassert(state->mix == Mix_Target);
        state->mix = Mix_User;
        state->can_still_listen = false;
        update_audio = true;
        update_ui = true;
    }

    switch (transition)
    {
        case Transition_None : {
        }break;
        case Transition_To_Begin : {
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
        case Transition_To_Exercice : {
            step = GameStep_Question;
            state->mix = Mix_Target;
            state->can_still_listen = true;
            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            jassert(state->target_slider_pos.size() == state->channel_infos.size());
            jassert(state->edited_slider_pos.size() == state->channel_infos.size());
            
            switch (state->variant)
            {
                case MixerGame_Normal : {
                } break;
                case MixerGame_Timer : {
                    effects.timer = Effect_Timer {
                        .timeout_ms = state->timeout_ms ,
                        .callback = [state] {
                            mixer_game_post_event(state, Event { .type = Event_Timeout });
                        }
                    };
                } break;
                case MixerGame_Tries : {
                    state->remaining_listens = state->listens;
                } break;
            }

            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Answer : {
            step = GameStep_Result;
            state->mix = Mix_Target;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_End_Result : {
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

        
        effects.ui = Effect_UI {
            .fader_step = gameStepToFaderStep(step, state->mix),
            .score = state->score,
            .slider_pos_to_display = std::move(slider_pos_to_display),
            .mix = state->mix
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
                effects.ui->header_text = "Reproduce the target mix";
                effects.ui->button_text = "Validate";
                effects.ui->button_event = Event_Click_Answer;
            }break;
            case GameStep_Result : 
            {
                effects.ui->header_text = "Results";
                effects.ui->button_text = "Next";
                effects.ui->button_event = Event_Click_Next;
            }break;
        }
    }

    state->step = step;
    return effects;
}


void mixer_game_add_ui_observer(MixerGame_State *state, ui_observer_t &&observer)
{
    state->observers_ui.push_back(std::move(observer));
}

void mixer_game_add_audio_observer(MixerGame_State *state, audio_observer_t &&observer)
{
    state->observers_audio.push_back(std::move(observer));
}


std::unique_ptr<MixerGame_State> mixer_game_init(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    MixerGame_Variant variant,
    int listens,
    int timeout_ms,
    std::vector<double> db_slider_values,
    Application *app)
{
    if(variant != MixerGame_Tries)
        jassert(listens == -1);
    if(variant != MixerGame_Timer)
        jassert(timeout_ms == -1);

    mixer_game_update_t update_fn = nullptr;
    
    switch (variant)
    {
        case MixerGame_Normal : {
            update_fn = &mixer_game_update;
        } break;
        case MixerGame_Timer : {
            update_fn = &mixer_game_timer_update;
        } break;
        case MixerGame_Tries : {
            update_fn = &mixer_game_tries_update;
        } break;
    }

    MixerGame_State state = {
        .channel_infos = channel_infos,
        .variant = variant,
        .listens = listens,
        .timeout_ms = timeout_ms,
        .db_slider_values = db_slider_values,
        .update_fn = update_fn,
        .app = app
    };
    return std::make_unique < MixerGame_State > (std::move(state));
}