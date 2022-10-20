#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"

#include "Game.h"
#include "MainMenu.h"
#include "Application.h"
#include "Processor_Host.h"
#include "PluginEditor_Host.h"
#if 0
double randomGain()
{
    int slider_value = juce::Random::getSystemRandom().nextInt() % ArraySize(slider_values);
    return slider_value_to_gain(slider_value);
}

    
void Game::toggleInputOrTarget(bool isOn)
{
    jassert(state.step != Begin);
    auto old_step = state.step;
        
    //TODO virtual affects score ?
        
    if(isOn && state.step == Editing)
    {
        state.step = Listening;
    }
    else if(isOn && state.step == ShowingAnswer)
    {
        state.step = ShowingTruth;
    }
    else if(!isOn && state.step == Listening)
    {
        state.step = Editing;
    }
    else if(!isOn && state.step == ShowingTruth){
        state.step = ShowingAnswer;
    }

    if(old_step != state.step)
    {
        app.broadcastAllDSP();
        //virtual
        if(game_ui)
            game_ui->updateGameUI(state);
    }
} 
    
void Game::nextClicked(){
    switch(state.step)
    {
        case Begin :
        {
            generateNewRound();
            state.step = Listening;
        } break;
        case Listening :
        case Editing :
        {
            state.score += awardPoints();
            state.step = ShowingTruth;
        }break;
        case ShowingTruth : 
        case ShowingAnswer :
        {
            generateNewRound();
            state.step = Listening;
        }break;
    }
    app.broadcastAllDSP();
    if(game_ui)
        game_ui->updateGameUI(state);
}

void Game::backClicked()
{
    app.toMainMenu();
}

#endif 
GameUI_Panel::GameUI_Panel(std::function < void() > && onNextClicked,
                                   std::function < void(bool) > && onToggleClicked,
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
        target_mix_button.onClick = [this, toggle = std::move(onToggleClicked)] {
            toggle(true);
        };
        addAndMakeVisible(target_mix_button);
            
        user_mix_button.setButtonText("My mix");
        user_mix_button.onClick = [this, toggle = std::move(onToggleClicked)] {
            toggle(false);
        };
        addAndMakeVisible(user_mix_button);
            
        target_mix_button.setRadioGroupId (1000);
        user_mix_button.setRadioGroupId (1000);
    }

    addAndMakeVisible(this->game_ui.get());
}
 
#if 0
void GameUI::updateGameUI_Generic(const GameState &new_state)
{
    target_mix_button.setToggleState(new_state.step == Listening || 
                                   new_state.step == ShowingTruth, 
                                   juce::dontSendNotification);

    if (new_state.step == Begin)
    {
        target_mix_button.setEnabled(false);
        user_mix_button.setEnabled(false);
        target_mix_button.setVisible(false);
        user_mix_button.setVisible(false);
    }
    else
    {
        target_mix_button.setEnabled(true);
        user_mix_button.setEnabled(true);
        target_mix_button.setVisible(true);
        user_mix_button.setVisible(true);
    }

    switch(new_state.step)
    {
        case Begin : {
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
    
    score_label.setText(juce::String("Score : ") + juce::String(new_state.score), juce::dontSendNotification);
}
#endif
      
  
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
#if 0
void GameUI::deleteUI() {
    jassert(game_ui != nullptr);
    game_ui = nullptr;
}

MixerGame::MixerGame(Application &app, GameState &state)
: GameImplementation(app, state)
{}
    
ChannelState MixerGame::generateChannel(int id)
{
    auto target_gain = randomGain();
    auto edit_gain = slider_value_to_gain(0);
        
    return ChannelState{id, "", edit_gain, target_gain};
}
    
int MixerGame::awardPoints()
{
    int points_awarded = 0;
    for (auto& [_, channel] : state.channels)
    {
            
        if(channel.target_gain == channel.edited_gain)
        {
            points_awarded++;
        }
    }
    return points_awarded;
}
void MixerGame::generateNewRound() 
{
    for (auto& [_, channel] : state.channels)
    {
        channel.target_gain = randomGain();
        channel.edited_gain = slider_value_to_gain(2);
    }
};
    
void MixerGame::editGain(int id, double new_gain)
{
    //virtual
    auto channel = state.channels.find(id);
    jassert(channel != state.channels.end());
    channel->second.edited_gain = new_gain;
    app.broadcastAllDSP();
}
    
std::unique_ptr<GameUI> MixerGame::createUI()
{
    jassert(game_ui == nullptr);
    auto new_game_ui = std::make_unique<MixerUI>(
        state,
        [this]() { nextClicked(); /* TODO insert state assert debug*/ },
        [this](bool isToggled) { toggleInputOrTarget(isToggled); },
        [this]() { backClicked(); },
        [this](int id, double gain) { editGain(id, gain); },
        [this](int id, const juce::String &newName) { renameChannelFromUI(id, newName); }
    );
    game_ui = new_game_ui.get();
    return new_game_ui;
}

MixerUI::MixerUI(MixerGame::GameState &state,
           std::function<void()> &&onNextClicked,
           std::function<void(bool)> &&onToggleClicked,
           std::function<void()> &&onBackClicked,
           std::function<void(int, double)> &&onFaderMoved,
           std::function<void(int, const juce::String&)> &&onEditedName)
:
    GameUI(std::move(onNextClicked), std::move(onToggleClicked), std::move(onBackClicked)),
    fader_row(faders),
    onFaderMoved(std::move(onFaderMoved)),
    onEditedName(std::move(onEditedName))
{
    for(const auto& [_, channel] : state.channels)
    {
        createChannelUI(channel, state.step);
    }
    addAndMakeVisible(fader_viewport);
    fader_viewport.setScrollBarsShown(false, true);
    fader_viewport.setViewedComponent(&fader_row, false);
        
    updateGameUI(state);
}

void MixerUI::resizedChild(juce::Rectangle<int> bounds)
{
    fader_viewport.setBounds(bounds);
    fader_row.setSize(fader_row.getWidth(),
                      fader_viewport.getHeight() - fader_viewport.getScrollBarThickness());
}

void MixerUI::createChannelUI(const ChannelInfos& channel_infos,
                              GameStep step)
{
    {
        auto assertFader = faders.find(channel_infos.id);
        jassert(assertFader == faders.end());
    }
    
    auto faderMoved = [this, id = channel_infos.id] (double gain) {
        onFaderMoved(id, gain);
    };
    auto nameChanged = [this, id = channel_infos.id] (juce::String newName){
        onEditedName(id, newName);
    };

    auto fader_step = gameStepToFaderStep(step);

    auto new_fader = std::make_unique<FaderComponent>(fader_gain, 
        fader_step,
        channel_infos.name
        faderMoved,
        nameChanged);
    fader_row.addAndMakeVisible(new_fader.get());
    faders[channel_infos.id] = std::move(new_fader);
    
    fader_row.adjustWidth();
    resized();
}

void MixerUI::deleteChannelUI(int channelToRemoveId)
{
    auto fader = faders.find(channelToRemoveId);
    jassert(fader != faders.end());
    faders.erase(fader);
    fader_row.adjustWidth();
    resized();
}

void MixerUI::renameChannel(int id, const juce::String &newName)
{
    auto &fader = faders[id];
    fader->setName(newName);
}
    
void MixerUI::updateGameUI(const GameStep step)
{
    for(auto& [id, fader] : faders)
    {
        const ChannelState &channel = new_state.channels.at(id);

        FaderStep fader_step;
        double fader_gain = 0.0;
        switch(step)
        {
            case Begin :
            {
                fader_step = FaderStep_Editing;
            } break;
            case Listening :
            {
                fader_step = FaderStep_Hiding;
            } break;
            case Editing :
            {
                fader_step = FaderStep_Editing;
            }break;
            case ShowingTruth :
            {
                fader_step = FaderStep_Showing;
            } break;
            case ShowingAnswer :
            {
                fader_step = FaderStep_Showing;
            }break;
        }

        fader->update(fader_step, fader_gain);
    }
    
    updateGameUI_Generic(new_state);
}

#endif