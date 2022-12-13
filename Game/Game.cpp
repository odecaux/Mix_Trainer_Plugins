#include <juce_audio_processors/juce_audio_processors.h>
#include "../shared/shared.h"

#include "Game.h"

juce::String step_to_str(GameStep step)
{
    switch (step)
    {
        case GameStep_Begin :
        {
            return "Begin";
        } break;
        case GameStep_Question :
        {
            return "Question";
        } break;
        case GameStep_Result :
        {
            return "Result";
        } break;
        case GameStep_EndResults :
        {
            return "End Results";
        } break;
        case GameStep_None :
        default :
        {
            jassertfalse;
            return "";
        };
    }
}

//TODO helloooooooo tu vas mourir toi
Widget_Interaction_Type gameStepToFaderStep(GameStep game_step, Mix mix)
{
    switch (game_step)
    {
        case GameStep_Begin :
        {
            assert(mix == Mix_Hidden);
            return Widget_Editing;
        } break;
        case GameStep_Question :
        {
            if (mix == Mix_User)
            {
                return Widget_Editing;
            }
            else if (mix == Mix_Target)
            {
                return Widget_Hiding;
            }
            else
            {
                jassertfalse;
                return {};
            }
        } break;
        case GameStep_Result :
        {
            assert(mix != Mix_Hidden);
            return Widget_Showing;
        } break;
        case GameStep_EndResults :
        case GameStep_None :
        default :
        {
            jassertfalse;
            return {};
        };
    }
}


//------------------------------------------------------------------------

GameUI_Header::GameUI_Header()
{
    {
        center_label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(center_label);
        back_button.setButtonText("Back");
        back_button.onClick = [this] {
            onBackClicked();
        };
        addAndMakeVisible(back_button);
        addAndMakeVisible(right_label);
    }
}


GameUI_Bottom::GameUI_Bottom()
{
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
}
       
void GameUI_Header::resized()
{
    auto bounds = getLocalBounds();
            
    auto headerLabelBounds = bounds.withTrimmedLeft(90).withTrimmedRight(90);
    center_label.setBounds(headerLabelBounds);

    auto back_buttonBounds = bounds.withWidth(90);
    back_button.setBounds(back_buttonBounds);

    auto score_labelBounds = bounds.withTrimmedLeft(bounds.getWidth() - 90);
    right_label.setBounds(score_labelBounds);
}


void GameUI_Bottom::resized()
{
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    auto button_temp = juce::Rectangle(100, bounds.getHeight() - 10);
    auto nextBounds = button_temp.withCentre(center);
    next_button.setBounds(nextBounds);
        
    auto toggleBounds = bounds.withWidth(100);
    auto targetMixBounds = toggleBounds.withHeight(bounds.getHeight() / 2);
    auto userMixBounds = targetMixBounds.translated(0, bounds.getHeight() / 2);
    target_mix_button.setBounds(targetMixBounds);
    user_mix_button.setBounds(userMixBounds);
}



void game_ui_header_update(GameUI_Header *header, juce::String center_text, juce::String right_text)
{
    header->center_label.setText(center_text, juce::dontSendNotification);
    header->right_label.setText(right_text, juce::dontSendNotification);
}

void game_ui_bottom_update(GameUI_Bottom *bottom, bool show_button, juce::String button_text, Mix mix, Event_Type event)
{   
    if (mix == Mix_Hidden)
    {
        bottom->target_mix_button.setEnabled(false);
        bottom->user_mix_button.setEnabled(false);
        bottom->target_mix_button.setVisible(false);
        bottom->user_mix_button.setVisible(false);
    }
    else
    {
        bottom->target_mix_button.setEnabled(true);
        bottom->user_mix_button.setEnabled(true);
        bottom->target_mix_button.setVisible(true);
        bottom->user_mix_button.setVisible(true);

        if (mix == Mix_User)
        {
            bottom->target_mix_button.setToggleState(false, juce::dontSendNotification);
            bottom->user_mix_button.setToggleState(true, juce::dontSendNotification);
        }
        else if (mix == Mix_Target)
        {
            bottom->target_mix_button.setToggleState(true, juce::dontSendNotification);
            bottom->user_mix_button.setToggleState(false, juce::dontSendNotification);
        }
    }

    bottom->next_button.setVisible(show_button);
    bottom->next_button.setButtonText(button_text);

    bottom->next_button.onClick = [bottom, event] {
        bottom->onNextClicked(event);
    };
}