#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
#include "MainMenu.h"
#include "Application.h"
#include "Game_2.h"
#include "Processor_Host.h"
#include "PluginEditor_Host.h"

#if 0
void Game::

void Game::backClicked()
{
    app.toMainMenu();
}

#endif 
GameUI_Panel::GameUI_Panel(std::function < void() > && onNextClicked,
                                   std::function < void(bool) > &&onToggleClicked,
                                   std::function < void() > && onBackClicked,
                                   std::unique_ptr < GameUI > game_ui) :
    game_ui(std::move(game_ui))
{
    {
        top_label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(top_label);
        back_button.setButtonText("Back");
        back_button.onClick = [this, back = std::move(onBackClicked)] {
            back();
        };
        addAndMakeVisible(back_button);
        addAndMakeVisible(score_label);
    }
    {
        next_button.onClick = [this, next = std::move(onNextClicked)] {
            next();
        };
        addAndMakeVisible(next_button);
    }
    
    {
        target_mix_button.setButtonText("Target mix");
        target_mix_button.onClick = [this, toggle = onToggleClicked] {
            juce::String str = target_mix_button.getToggleState() ? juce::String{ "on" } : juce::String{ "off" };
            
            if(target_mix_button.getToggleState())
                toggle(true);
        };
        addAndMakeVisible(target_mix_button);
            
        user_mix_button.setButtonText("My mix");
        
        user_mix_button.onClick = [this, toggle = onToggleClicked] {
            juce::String str = user_mix_button.getToggleState() ?  juce::String{ "on" } :  juce::String{ "off" };
            
            if(user_mix_button.getToggleState())
                toggle(false);
         };
         
        addAndMakeVisible(user_mix_button);
            
        target_mix_button.setRadioGroupId (1000);
        user_mix_button.setRadioGroupId (1000);
    }

    this->game_ui->attachParentPanel(this);
    addAndMakeVisible(this->game_ui.get());
}
 
void GameUI_Panel::updateGameUI_Generic(GameStep new_step, int new_score)
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
        } break;
        case Listening :
        case Editing :
        {
            top_label.setText("Reproduce the target mix", juce::dontSendNotification);
            next_button.setButtonText("Validate");
        }break;
        case ShowingTruth :
        case ShowingAnswer : 
        {
            top_label.setText("Results", juce::dontSendNotification);
            next_button.setButtonText("Next");
        }break;
    };
    
    //score
    score_label.setText(juce::String("Score : ") + juce::String(new_score), juce::dontSendNotification);
}

      
void GameUI_Panel::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}
       
void GameUI_Panel::resized()
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
void Game::onUIDelete() {
    jassert(game_ui != nullptr);
    game_ui = nullptr;
}


















//------------------------------------------------------------------------


GameUI_Panel2::GameUI_Panel2(std::function < void() > && onNextClicked,
                           std::function < void(bool) > &&onToggleClicked,
                           std::function < void() > && onBackClicked,
                           juce::Component *game_ui) :
    game_ui(game_ui)
{
    {
        top_label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(top_label);
        back_button.setButtonText("Back");
        back_button.onClick = [this, back = std::move(onBackClicked)] {
            back();
        };
        addAndMakeVisible(back_button);
        addAndMakeVisible(score_label);
    }
    {
        next_button.onClick = [this, next = std::move(onNextClicked)] {
            next();
        };
        addAndMakeVisible(next_button);
    }
    
    {
        target_mix_button.setButtonText("Target mix");
        target_mix_button.onClick = [this, toggle = onToggleClicked] {
            juce::String str = target_mix_button.getToggleState() ? juce::String{ "on" } : juce::String{ "off" };
            
            if(target_mix_button.getToggleState())
                toggle(true);
        };
        addAndMakeVisible(target_mix_button);
            
        user_mix_button.setButtonText("My mix");
        
        user_mix_button.onClick = [this, toggle = onToggleClicked] {
            juce::String str = user_mix_button.getToggleState() ?  juce::String{ "on" } :  juce::String{ "off" };
            
            if(user_mix_button.getToggleState())
                toggle(false);
         };
         
        addAndMakeVisible(user_mix_button);
            
        target_mix_button.setRadioGroupId (1000);
        user_mix_button.setRadioGroupId (1000);
    }

    addAndMakeVisible(this->game_ui);
}
 
void GameUI_Panel2::updateGameUI_Generic(GameStep new_step, int new_score)
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
        } break;
        case Listening :
        case Editing :
        {
            top_label.setText("Reproduce the target mix", juce::dontSendNotification);
            next_button.setButtonText("Validate");
        }break;
        case ShowingTruth :
        case ShowingAnswer : 
        {
            top_label.setText("Results", juce::dontSendNotification);
            next_button.setButtonText("Next");
        }break;
    };
    
    //score
    score_label.setText(juce::String("Score : ") + juce::String(new_score), juce::dontSendNotification);
}

      
void GameUI_Panel2::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}
       
void GameUI_Panel2::resized()
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
