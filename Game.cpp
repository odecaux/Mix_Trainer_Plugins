#if 0

#include <juce_audio_processors/juce_audio_processors.h>
#include "shared.h"
#include "Game.h"
#include "MainMenu.h"
#include "PluginEditor_Host.h"
#include "Application.h"
#include "Processor_Host.h"

double randomGain()
{
    int slider_value = juce::Random::getSystemRandom().nextInt() % ArraySize(slider_values);
    return slider_value_to_gain(slider_value);
}

    
void GameImplementation::toggleInputOrTarget(bool isOn)
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
    
void GameImplementation::nextClicked(){
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

void GameImplementation::backClicked()
{
    app.toMainMenu();
}


UIImplementation::UIImplementation(std::function < void() > && onNextClicked,
                                   std::function < void(bool) > && onToggleClicked,
                                   std::function < void() > && onBackClicked)
:
    onNextClicked(std::move(onNextClicked)),
    onToggleClicked(std::move(onToggleClicked)),
    onBackClicked(std::move(onBackClicked))
{
    {
        topLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(topLabel);
        backButton.setButtonText("Back");
        backButton.onClick = [this] {
            this->onBackClicked();
        };
        addAndMakeVisible(backButton);
        addAndMakeVisible(scoreLabel);
    }
    {
        nextButton.onClick = [this] {
            this->onNextClicked();
        };
        addAndMakeVisible(nextButton);
    }
    
    {
        targetMixButton.setButtonText("Target mix");
        targetMixButton.onClick = [this] {
            this->onToggleClicked(true);
        };
        addAndMakeVisible(targetMixButton);
            
        userMixButton.setButtonText("My mix");
        userMixButton.onClick = [this] {
            this->onToggleClicked(false);
        };
        addAndMakeVisible(userMixButton);
            
        targetMixButton.setRadioGroupId (1000);
        userMixButton.setRadioGroupId (1000);
    }
}
        
void UIImplementation::updateGameUI_Generic(const GameState &new_state)
{
    targetMixButton.setToggleState(new_state.step == Listening || 
                                   new_state.step == ShowingTruth, 
                                   juce::dontSendNotification);

    if (new_state.step == Begin)
    {
        targetMixButton.setEnabled(false);
        userMixButton.setEnabled(false);
        targetMixButton.setVisible(false);
        userMixButton.setVisible(false);
    }
    else
    {
        targetMixButton.setEnabled(true);
        userMixButton.setEnabled(true);
        targetMixButton.setVisible(true);
        userMixButton.setVisible(true);
    }

    switch(new_state.step)
    {
        case Begin : {
            topLabel.setText("Have a listen", juce::dontSendNotification);
            nextButton.setButtonText("Begin");
        } break;
        case Listening :
        case Editing :
        {
            topLabel.setText("Reproduce the target mix", juce::dontSendNotification);
            nextButton.setButtonText("Validate");
        }break;
        case ShowingTruth :
        case ShowingAnswer : 
        {
            topLabel.setText("Results", juce::dontSendNotification);
            nextButton.setButtonText("Next");
        }break;
    };
    
    scoreLabel.setText(juce::String("Score : ") + juce::String(new_state.score), juce::dontSendNotification);
}
        
void UIImplementation::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}
       
void UIImplementation::resized()
{
    auto bottom_height = 50;
    auto top_height = 20;
    auto r = getLocalBounds();
            
    auto topBounds = r.withHeight(top_height);
            
    {
        auto topLabelBounds = topBounds.withTrimmedLeft(90).withTrimmedRight(90);
        topLabel.setBounds(topLabelBounds);
     
        auto backButtonBounds = topBounds.withWidth(90);
        backButton.setBounds(backButtonBounds);

        auto scoreLabelBounds = topBounds.withTrimmedLeft(topBounds.getWidth() - 90);
        scoreLabel.setBounds(scoreLabelBounds);
    }

    auto gameBounds = r.withTrimmedBottom(bottom_height).withTrimmedTop(top_height);
    resized_child(gameBounds);
    
    {
        auto bottomStripBounds = r.withTrimmedTop(r.getHeight() - bottom_height);
        auto center = bottomStripBounds.getCentre();
    
        auto button_temp = juce::Rectangle(100, bottom_height - 10);
        auto nextBounds = button_temp.withCentre(center);
        nextButton.setBounds(nextBounds);
            
        auto toggleBounds = bottomStripBounds.withWidth(100);
        auto targetMixBounds = toggleBounds.withHeight(bottom_height / 2);
        auto userMixBounds = targetMixBounds.translated(0, bottom_height / 2);
        targetMixButton.setBounds(targetMixBounds);
        userMixButton.setBounds(userMixBounds);
    }
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
    
std::unique_ptr<UIImplementation> MixerGame::createUI()
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

MixerUI::MixerUI(GameState &state,
           std::function<void()> &&onNextClicked,
           std::function<void(bool)> &&onToggleClicked,
           std::function<void()> &&onBackClicked,
           std::function<void(int, double)> &&onFaderMoved,
           std::function<void(int, const juce::String&)> &&onEditedName)
:
    UIImplementation(std::move(onNextClicked), std::move(onToggleClicked), std::move(onBackClicked)),
    fadersRow(faderComponents),
    onFaderMoved(std::move(onFaderMoved)),
    onEditedName(std::move(onEditedName))
{
    for(const auto& [_, channel] : state.channels)
    {
        createChannelUI(channel, state.step);
    }
    addAndMakeVisible(fadersViewport);
    fadersViewport.setScrollBarsShown(false, true);
    fadersViewport.setViewedComponent(&fadersRow, false);
        
    updateGameUI(state);
}

void MixerUI::resized_child(juce::Rectangle<int> bounds)
{
    fadersViewport.setBounds(bounds);
    fadersRow.setSize(fadersRow.getWidth(),
                      fadersViewport.getHeight() - fadersViewport.getScrollBarThickness());
}

void MixerUI::createChannelUI(const ChannelState& channel, GameStep step)
{
    {
        auto assertFader = faderComponents.find(channel.id);
        jassert(assertFader == faderComponents.end());
    }
    
    auto faderMoved = [this, id = channel.id] (double gain) {
        onFaderMoved(id, gain);
    };
    auto nameChanged = [this, id = channel.id] (juce::String newName){
        onEditedName(id, newName);
    };
    
    auto newFader = std::make_unique<FaderComponent>(channel, 
        step,
        faderMoved,
        nameChanged);
    fadersRow.addAndMakeVisible(newFader.get());
    faderComponents[channel.id] = std::move(newFader);
    
    fadersRow.adjustWidth();
    resized();
}

void MixerUI::deleteChannelUI(int channelToRemoveId)
{
    auto fader = faderComponents.find(channelToRemoveId);
    jassert(fader != faderComponents.end());
    faderComponents.erase(fader);
    fadersRow.adjustWidth();
    resized();
}

void MixerUI::renameChannel(int id, const juce::String &newName)
{
    auto &fader = faderComponents[id];
    fader->setName(newName);
}
    
void MixerUI::updateGameUI(const GameState &new_state)
{
    for(auto& [id, fader] : faderComponents){
        const ChannelState &channel = new_state.channels.at(id);
        fader->updateStep(new_state.step, channel);
    }
    
    updateGameUI_Generic(new_state);
}


#endif 