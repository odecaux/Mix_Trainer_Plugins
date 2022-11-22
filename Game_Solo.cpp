#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
#include "Game_Mixer_Timer.h"
#include "Game_Solo.h"
#include "MainMenu.h"
#include "Application.h"


void solo_game_ui_header_update(GameUI_Header *ui, GameStep new_step, int new_score)
{
    switch(new_step)
    {
        case GameStep_Begin : 
        {
            ui->header_label.setText("Have a listen", juce::dontSendNotification);
        } break;
        case GameStep_Listening :
        case GameStep_Editing :
        {
            ui->header_label.setText("Reproduce the target mix", juce::dontSendNotification);
        } break;
        case GameStep_ShowingTruth : {
            jassertfalse;
        } break;
        case GameStep_ShowingAnswer : 
        {
            ui->header_label.setText("Results", juce::dontSendNotification);
        } break;
    };
    
    //score
    ui->score_label.setText(juce::String("Score : ") + juce::String(new_score), juce::dontSendNotification);
}

void solo_game_post_event(SoloGame_State *state, Event event)
{
    Effects effects = solo_game_update(state, event);
    if (effects.dsp)
    {
        for(auto &observer : state->observers_audio)
            observer(*effects.dsp);
    }
    if (effects.ui && state->ui)
    {
        state->ui->updateGameUI(effects.ui->new_step, effects.ui->new_score, effects.ui->slider_pos_to_display);
    }
    if (effects.rename)
    {
        state->app->renameChannelFromUI(effects.rename->id, effects.rename->new_name);
    }
    if (effects.quit)
    {
        state->app->quitGame();
    }
}

Effects solo_game_update(SoloGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
        .dsp = std::nullopt, 
        .ui = std::nullopt, 
        .rename = std::nullopt, 
        .quit = false 
    };

    switch (event.type)
    {
        case Event_Init : {
            transition = Transition_To_Begin;
        } break;
        case Event_Click_Track : {
            IMPLMEMENT();
        } break;
        case Event_Toggle_Track : {
            jassertfalse;
        } break;
        case Event_Slider : {
            jassertfalse;
        } break;
        case Event_Toggle_Input_Target : {
            jassertfalse;
        } break;
        case Event_Click_Begin : {
            jassert(step == GameStep_Begin);
            jassert(state->target_slider_pos.size() == 0);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Answer : {
            jassert(step == GameStep_Listening || step == GameStep_Editing);

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
            jassert(state->ui == nullptr);
            state->ui = (SoloGameUI*) event.value_ptr;
            update_ui = true;
        } break;
        case Event_Destroy_UI :
        {
            jassert(state->ui != nullptr);
            state->ui = nullptr;
        } break;
    }

    switch (transition)
    {
        case Transition_None : {
        }break;
        case Transition_To_Begin : {
            step = GameStep_Begin;
            state->score = 0;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Exercice : {
            step = GameStep_Listening;
            int random_id = -1;
            {
                int random_idx = juce::Random::getSystemRandom().nextInt() % state->channel_infos.size();
                int count = 0;
                for (auto& [id, channel] : state->channel_infos)
                {
                    if (count++ == random_idx)
                    {
                        random_id = id;
                        break;
                    }
                }
                jassert(random_id != -1);
            }
            state->target_solo = random_id;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Answer : {
            step = GameStep_ShowingTruth;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_End_Result : {
        }break;
    }
    
    std::unordered_map<int, int>* edit_or_target;
    if(step == GameStep_Begin || step == GameStep_Editing || step == GameStep_ShowingAnswer)
        edit_or_target = &state->edited_slider_pos;
    else
        edit_or_target = &state->target_slider_pos;

    if (update_audio)
    {
        std::unordered_map < int, ChannelDSPState > dsp;
        std::transform(
            edit_or_target->begin(), edit_or_target->end(), 
            std::inserter(dsp, dsp.end()), 
            [state](const auto &a) -> std::pair<int, ChannelDSPState> {
                if (GameStep == GameStep_Begin)
                {
                    return { a.first, ChannelDSP_gain(1.0f);
                }
                else 
                {
                    int gain = a.first == state->target_solo ? 1.0f : 0.0f;
                    return { a.first, ChannelDSP_gain(gain) };
                }
            }
        );
        effects.dsp = Effect_DSP { std::move(dsp) };
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == GameStep_Listening)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = *edit_or_target;

        effects.ui = Effect_UI {
            step,
            state->score,
            std::move(slider_pos_to_display)
        };
    }

    state->step = step;
    return effects;
}
