#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
#include "Game_Mixer.h"
#include "MainMenu.h"
#include "Application.h"
#include "Processor_Host.h"
#include "PluginEditor_Host.h"

juce::String step_to_str(GameStep step)
{
    switch (step)
    {
        case Begin :
        {
            return "Begin";
        } break;
        case Listening :
        {
            return "Listening";
        } break;
        case Editing :
        {
            return "Editing";
        } break;
        case ShowingTruth :
        {
            return "ShowingTruth";
        } break;
        case ShowingAnswer :
        {
            return "ShowingAnswer";
        } break;
    }
}


//------------------------------------------------------------------------

GameUI_Wrapper::GameUI_Wrapper(juce::Component *game_ui) :
    game_ui(game_ui)
{
    {
        top_label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(top_label);
        back_button.setButtonText("Back");
        back_button.onClick = [this] {
            onBackClicked();
        };
        addAndMakeVisible(back_button);
        addAndMakeVisible(score_label);
    }
    {
        addAndMakeVisible(next_button);
    }
    
    {
        target_mix_button.setButtonText("Target mix");
        target_mix_button.onClick = [this] {
            juce::String str = target_mix_button.getToggleState() ? juce::String{ "on" } : juce::String{ "off" };
            
            if(target_mix_button.getToggleState())
                onToggleClicked(true);
        };
        addAndMakeVisible(target_mix_button);
            
        user_mix_button.setButtonText("My mix");
        
        user_mix_button.onClick = [this] {
            juce::String str = user_mix_button.getToggleState() ?  juce::String{ "on" } :  juce::String{ "off" };
            
            if(user_mix_button.getToggleState())
                onToggleClicked(false);
         };
         
        addAndMakeVisible(user_mix_button);
            
        target_mix_button.setRadioGroupId (1000);
        user_mix_button.setRadioGroupId (1000);
    }

    addAndMakeVisible(this->game_ui);
}
 
void GameUI_Wrapper::update(GameStep new_step, int new_score)
{
    //hide/reveal
    switch(new_step)
    {
        case Begin : {
            target_mix_button.setEnabled(false);
            user_mix_button.setEnabled(false);
            target_mix_button.setVisible(false);
            user_mix_button.setVisible(false);
        } break;
        case Listening :
        case Editing :
        case ShowingTruth :
        case ShowingAnswer : 
        {
            target_mix_button.setEnabled(true);
            user_mix_button.setEnabled(true);
            target_mix_button.setVisible(true);
            user_mix_button.setVisible(true);
        }break;
    };

    //ticked one
    switch(new_step)
    {
        case Begin : break;
        case Editing :
        case ShowingAnswer : 
        {
            target_mix_button.setToggleState(false, juce::dontSendNotification);
            user_mix_button.setToggleState(true, juce::dontSendNotification);
        } break;
        case ShowingTruth :
        case Listening :
        {
            target_mix_button.setToggleState(true, juce::dontSendNotification);
            user_mix_button.setToggleState(false, juce::dontSendNotification);
        }break;
    };

    //labels
    switch(new_step)
    {
        case Begin : 
        {
            top_label.setText("Have a listen", juce::dontSendNotification);
            next_button.setButtonText("Begin");
            next_button.onClick = [this] {
                onNextClicked(Event_Click_Begin);
            };
        } break;
        case Listening :
        case Editing :
        {
            top_label.setText("Reproduce the target mix", juce::dontSendNotification);
            next_button.setButtonText("Validate");
            next_button.onClick = [this] {
                onNextClicked(Event_Click_Answer);
            };
        }break;
        case ShowingTruth :
        case ShowingAnswer : 
        {
            top_label.setText("Results", juce::dontSendNotification);
            next_button.setButtonText("Next");            
            next_button.onClick = [this] {
                onNextClicked(Event_Click_Next);
            };
        }break;
    };
    
    //score
    score_label.setText(juce::String("Score : ") + juce::String(new_score), juce::dontSendNotification);
}

      
void GameUI_Wrapper::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}
       
void GameUI_Wrapper::resized()
{
    auto bottom_height = 50;
    auto top_height = 20;
    auto r = getLocalBounds();
            
    auto topBounds = r.withHeight(top_height);
            
    {
        auto topLabelBounds = topBounds.withTrimmedLeft(90).withTrimmedRight(90);
        top_label.setBounds(topLabelBounds);
     
        auto back_buttonBounds = topBounds.withWidth(90);
        back_button.setBounds(back_buttonBounds);

        auto score_labelBounds = topBounds.withTrimmedLeft(topBounds.getWidth() - 90);
        score_label.setBounds(score_labelBounds);
    }

    auto gameBounds = r.withTrimmedBottom(bottom_height).withTrimmedTop(top_height);
    game_ui->setBounds(gameBounds);

    {
        auto bottomStripBounds = r.withTrimmedTop(r.getHeight() - bottom_height);
        auto center = bottomStripBounds.getCentre();
    
        auto button_temp = juce::Rectangle(100, bottom_height - 10);
        auto nextBounds = button_temp.withCentre(center);
        next_button.setBounds(nextBounds);
            
        auto toggleBounds = bottomStripBounds.withWidth(100);
        auto targetMixBounds = toggleBounds.withHeight(bottom_height / 2);
        auto userMixBounds = targetMixBounds.translated(0, bottom_height / 2);
        target_mix_button.setBounds(targetMixBounds);
        user_mix_button.setBounds(userMixBounds);
    }
}



//------------------------------------------------------------------------


void mixer_game_post_event(MixerGame_State *state, Event event, MixerGameUI_2 *ui)
{
    Effects effects = mixer_game_update(state, event);
    if (effects.dsp)
    {
        state->app->broadcastDSP(effects.dsp->dsp_states);
    }
    if (effects.ui && ui)
    {
        ui->updateGameUI(effects.ui->new_step, effects.ui->new_score, effects.ui->slider_pos_to_display);
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


Effects mixer_game_update(MixerGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { std::nullopt, std::nullopt, std::nullopt, false };

    switch (event.type)
    {
        case Event_Init : {
            jassertfalse;
            //update_audio = true;
            //update_ui = true;
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
            jassert(old_step != Begin);
            if(event.value_b && step == Editing)
            {
                step = Listening;
            }
            else if(event.value_b && step == ShowingAnswer)
            {
                step = ShowingTruth;
            }
            else if(!event.value_b && step == Listening)
            {
                step = Editing;
            }
            else if(!event.value_b && step == ShowingTruth){
                step = ShowingAnswer;
            }
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Click_Begin : {
            jassert(step == Begin);
            jassert(state->target_slider_pos.size() == 0);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Answer : {
            jassert(step == Listening || step == Editing);

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
            effects.rename = std::make_optional < Effect_Rename > (event.id, event.value_str);
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
        case Transition_To_Exercice : {

            for (auto& [_, channel] : state->channel_infos)
            {
                state->target_slider_pos[channel.id] = juce::Random::getSystemRandom().nextInt() % state->db_slider_values.size();//;
                state->edited_slider_pos[channel.id] = (int)state->db_slider_values.size() - 2;//true;
            }
            jassert(state->target_slider_pos.size() == state->channel_infos.size());
            jassert(state->edited_slider_pos.size() == state->channel_infos.size());

            step = Listening;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Answer : {
            step = ShowingTruth;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_End_Result : {
        }break;
    }
    
    std::unordered_map<int, int>* edit_or_target;
    if(step == Begin || step == Editing || step == ShowingAnswer)
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
        effects.dsp = std::make_optional < Effect_DSP > (std::move(dsp));
    }

    if (update_ui)
    {
        std::optional < std::unordered_map<int, int> > slider_pos_to_display;
        
        if(step == Listening)
            slider_pos_to_display = std::nullopt; 
        else
            slider_pos_to_display = std::make_optional < std::unordered_map<int, int> > (*edit_or_target);

        effects.ui = std::make_optional< Effect_UI> (
            step,
            state->score,
            std::move(slider_pos_to_display)
        );
    }

    state->step = step;
    return effects;
}