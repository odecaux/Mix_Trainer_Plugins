#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "Game.h"
#include "Game_Mixer.h"
#include "../Application/MainMenu.h"
#include "../Application/Application.h"

Effects mixer_game_tries_update(MixerGame_State *state, Event event)
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
                if (state->mix == Mix_User)
                {
                    jassert(event.value_b);
                    jassert(state->remaining_listens >= 0);
                    state->mix = Mix_Target;
                }
                else if (state->mix == Mix_Target)
                {
                    jassert(!event.value_b);
                    jassert(state->remaining_listens > 0);
                    state->remaining_listens--;
                    state->mix = Mix_User;
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
            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            jassert(state->target_slider_pos.size() == state->channel_infos.size());
            jassert(state->edited_slider_pos.size() == state->channel_infos.size());

            state->remaining_listens = state->listens;
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

        Mix mix;
        if(step == GameStep_Question && state->remaining_listens == 0)
            mix = Mix_Hidden;
        else 
            mix = state->mix;

        effects.ui = Effect_UI {
            .fader_step = gameStepToFaderStep(state->step, state->mix),
            .score = state->score,
            .slider_pos_to_display = std::move(slider_pos_to_display),
            .remaining_listens = state->remaining_listens,
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
                if (state->remaining_listens >= 0)
                    effects.ui->header_text = juce::String("remaining listens : ") + juce::String(state->remaining_listens);
                else
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


std::unique_ptr<MixerGame_State> mixer_game_init_tries(
    std::unordered_map<int, ChannelInfos> &channel_infos,
    int tries,
    std::vector<double> db_slider_values,
    Application *app)
{
    MixerGame_State state = {
        .channel_infos = channel_infos,
        .listens = tries,
        .db_slider_values = db_slider_values,
        .app = app
    };
    return std::make_unique < MixerGame_State > (std::move(state));
}