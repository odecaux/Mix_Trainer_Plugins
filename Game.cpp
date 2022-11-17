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
        case GameStep_Begin :
        {
            return "Begin";
        } break;
        case GameStep_Listening :
        {
            return "Listening";
        } break;
        case GameStep_Editing :
        {
            return "Editing";
        } break;
        case GameStep_ShowingTruth :
        {
            return "ShowingTruth";
        } break;
        case GameStep_ShowingAnswer :
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

        remaining_listens_label.setJustificationType (juce::Justification::centred);
        addChildComponent(remaining_listens_label);
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

#if 0
void game_ui_wrapper_update(GameUI_Wrapper *ui, GameStep new_step, int new_score)
{
    ui->remaining_listens_label.setVisible(false);
    //hide/reveal
    switch(new_step)
    {
        case GameStep_Begin : {
            ui->target_mix_button.setEnabled(false);
            ui->user_mix_button.setEnabled(false);
            ui->target_mix_button.setVisible(false);
            ui->user_mix_button.setVisible(false);
        } break;
        case GameStep_Listening :
        case GameStep_Editing :
        case GameStep_ShowingTruth :
        case GameStep_ShowingAnswer : 
        {
            ui->target_mix_button.setEnabled(true);
            ui->user_mix_button.setEnabled(true);
            ui->target_mix_button.setVisible(true);
            ui->user_mix_button.setVisible(true);
        }break;
    };

    //ticked one
    switch(new_step)
    {
        case GameStep_Begin : break;
        case GameStep_Editing :
        case GameStep_ShowingAnswer : 
        {
            ui->target_mix_button.setToggleState(false, juce::dontSendNotification);
            ui->user_mix_button.setToggleState(true, juce::dontSendNotification);
        } break;
        case GameStep_ShowingTruth :
        case GameStep_Listening :
        {
            ui->target_mix_button.setToggleState(true, juce::dontSendNotification);
            ui->user_mix_button.setToggleState(false, juce::dontSendNotification);
        }break;
    };

    //labels
    switch(new_step)
    {
        case GameStep_Begin : 
        {
            ui->top_label.setText("Have a listen", juce::dontSendNotification);
            ui->next_button.setButtonText("Begin");
            ui->next_button.onClick = [ui] {
                ui->onNextClicked(Event_Click_Begin);
            };
        } break;
        case GameStep_Listening :
        case GameStep_Editing :
        {
            ui->top_label.setText("Reproduce the target mix", juce::dontSendNotification);
            ui->next_button.setButtonText("Validate");
            ui->next_button.onClick = [ui] {
                ui->onNextClicked(Event_Click_Answer);
            };
        }break;
        case GameStep_ShowingTruth :
        case GameStep_ShowingAnswer : 
        {
            ui->top_label.setText("Results", juce::dontSendNotification);
            ui->next_button.setButtonText("Next");            
            ui->next_button.onClick = [ui] {
                ui->onNextClicked(Event_Click_Next);
            };
        }break;
    };
    
    //score
    ui->score_label.setText(juce::String("Score : ") + juce::String(new_score), juce::dontSendNotification);
}

#endif
      
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
        remaining_listens_label.setBounds(topLabelBounds);
        //top_label.setBounds(topLabelBounds);
     
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