#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
#include "Game_Mixer.h"
#include "MainMenu.h"
#include "Application.h"

void game_ui_bottom_update_timer(GameUI_Bottom *bottom, GameStep new_step)
{
    switch(new_step)
    {
        default : {
            bottom->target_mix_button.setEnabled(false);
            bottom->user_mix_button.setEnabled(false);
            bottom->target_mix_button.setVisible(false);
            bottom->user_mix_button.setVisible(false);
        } break;
    };

    switch (new_step)
    {
        case GameStep_Begin :
        {
            bottom->next_button.setButtonText("Begin");
            bottom->next_button.onClick = [bottom] {
                bottom->onNextClicked(Event_Click_Begin);
            };
        } break;
        case GameStep_Listening :
        {
            bottom->next_button.setButtonText("Go");
            bottom->next_button.onClick = [bottom] {
                bottom->onNextClicked(Event_Click_Start_Answering_RENAME);
            };
        } break;
        case GameStep_Editing :
        {
            bottom->next_button.setButtonText("Validate");
            bottom->next_button.onClick = [bottom] {
                bottom->onNextClicked(Event_Click_Answer);
            };
        } break;
        case GameStep_ShowingTruth :
        {
            bottom->next_button.setButtonText("Next");
            bottom->next_button.onClick = [bottom] {
                bottom->onNextClicked(Event_Click_Next);
            };
        } break;
        case GameStep_ShowingAnswer : 
        {
            jassertfalse;
        } break;
    };
}

void game_ui_update_timer(Effect_UI &new_ui, MixerGameUI &ui)
{
    if (new_ui.slider_pos_to_display)
    {
        jassert(new_ui.slider_pos_to_display->size() == ui.faders.size());
    }
    for(auto& [id, fader] : ui.faders)
    {
        auto fader_step = gameStepToFaderStep(new_ui.step);
        int pos = new_ui.slider_pos_to_display ? new_ui.slider_pos_to_display->at(id) : -1;
        fader->update(fader_step, pos);
    }
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
    game_ui_bottom_update_timer(&ui.bottom, new_ui.step);
}

Effects mixer_game_timer_update(MixerGame_State *state, Event event)
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
        .quit = false, 
        .timer  = std::nullopt 
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
            jassertfalse;
        } break;
        case Event_Timeout : {
            jassert(step == GameStep_Listening);
            step = GameStep_Editing;
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Click_Begin : {
            jassert(step == GameStep_Begin);
            jassert(state->target_slider_pos.size() == 0);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Start_Answering_RENAME : {
            jassert(state->timer.isTimerRunning());
            state->timer.stopTimer();
            jassert(step == GameStep_Listening);
            step = GameStep_Editing;
            update_ui = true;
            update_audio = true;
        } break;
        case Event_Click_Answer : {
            jassert(step == GameStep_Editing);

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
            std::transform(state->channel_infos.begin(), state->channel_infos.end(), 
                           std::inserter(state->edited_slider_pos, state->edited_slider_pos.end()), 
                           [&](const auto &a) -> std::pair<int, int> {
                           return { a.first, (int)state->db_slider_values.size() - 2};
            });
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Exercice : {
            step = GameStep_Listening;
            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            jassert(state->target_slider_pos.size() == state->channel_infos.size());
            jassert(state->edited_slider_pos.size() == state->channel_infos.size());
            
            effects.timer = Effect_Timer {
                .timeout_ms = state->timeout_ms ,
                .callback = [state] {
                    mixer_game_post_event(state, Event { .type = Event_Timeout });
                }
            };
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
        std::transform(edit_or_target->begin(), edit_or_target->end(), 
                       std::inserter(dsp, dsp.end()), 
                       [state](const auto &a) -> std::pair<int, ChannelDSPState> {
                       double gain = slider_pos_to_gain(a.second, state->db_slider_values);
                       return { a.first, ChannelDSP_gain(gain) };
        });
        effects.dsp = Effect_DSP { std::move(dsp) };
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == GameStep_Listening)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = *edit_or_target;

        juce::String header_text;
        switch (step)
        {
            case GameStep_Begin :
            {
                header_text = "Have a listen";
            } break;
            case GameStep_Listening :
            {
                header_text = "Listen";
            } break;
            case GameStep_Editing :
            {
                header_text = "Reproduce the target mix";
            } break;
            case GameStep_ShowingTruth :
            {
                header_text = "Results";
            } break;
            case GameStep_ShowingAnswer : 
            {
                jassertfalse;
            } break;
        }

        effects.ui = Effect_UI {
            .step = step,
            .header_text = std::move(header_text),
            .score = state->score,
            .slider_pos_to_display = std::move(slider_pos_to_display)
        };
    }

    state->step = step;
    return effects;
}
