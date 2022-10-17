/*
  ==============================================================================

    PluginEditor_Host.h
    Created: 16 Sep 2022 9:09:26am
    Author:  Octave

  ==============================================================================
*/

#pragma once
#include <vector>
#include <algorithm>
#include <functional>

class DecibelSlider : public juce::Slider
{
    public:
    DecibelSlider() = default;
    
    juce::String getTextFromValue(double value) override
    {
        return juce::Decibels::toString(slider_value_to_db((int)value));
    }
    
    private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecibelSlider)
};


//TODO don't use the slider's textbox, that way I can hide the value when locked

class FaderComponent : public juce::Component
{
    public:
    FaderComponent(const ChannelState &state,
                   GameStep step,
                   std::function<void(double)> onFaderChange,
                   std::function<void(const juce::String&)> onNameChange)
        : onFaderChange(onFaderChange),
    onNameChange(onNameChange)
    {
        label.setText(state.name, juce::NotificationType::dontSendNotification);
        label.setEditable(true);
        label.onTextChange = [this] {
            this->onNameChange(label.getText());
        };
        label.onEditorShow = [&] {
            auto *editor = label.getCurrentTextEditor();
            jassert(editor != nullptr);
            editor->setJustification(juce::Justification::centred);
        };
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxAbove, true, getWidth(), 20);
        fader.setRange(0.0f, (double)(ArraySize(slider_values) - 1), 1.0);
        fader.setScrollWheelEnabled(false);
        
        fader.onValueChange = [this] {
            jassert(this->step == Editing);
            auto newGain = slider_value_to_gain((int)fader.getValue());
            this->onFaderChange(newGain);
        };
        addAndMakeVisible(fader);
        
        updateStep(step, state);
    }
    
    
    void resized() override
    {
        auto r = getLocalBounds();
        auto labelBounds = r.withBottom(50);
        label.setBounds(labelBounds);
        
        auto faderBounds = r.withTrimmedTop(50);
        fader.setBounds(faderBounds);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(r.toFloat(), 5.0f, 2.0f);
    }
    
    void setName(const juce::String& newName)
    {
        label.setText(newName, juce::dontSendNotification);
    }
    
    void updateStep(GameStep newStep, const ChannelState &state)
    {
        switch(newStep)
        {
            case Begin : jassertfalse; break;
            case Listening :
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            }break;
            case Editing :
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            }break;
            case ShowingTruth :
            {
                fader.setValue(gain_to_slider_value(state.target_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            }break;
            case ShowingAnswer : 
            {
                fader.setValue(gain_to_slider_value(state.edited_gain), juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            }break;
        };
        step = newStep;
        repaint();
    }
    
    private:
    juce::Label label;
    DecibelSlider fader;
    std::function<void(double)> onFaderChange;
    std::function<void(const juce::String&)> onNameChange;
    GameStep step;
    //double targetValue;
    //double smoothing;
};


class FaderRowComponent : public juce::Component
{
    public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& faderComponents)
        : faderComponents(faderComponents)
    {
    }
    
    void adjustWidth() 
    {
        setSize(fader_width * (int)faderComponents.size(), getHeight());
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    
    void resized() override
    {
        int x_offset = 0;
        auto bounds = getLocalBounds();
        auto topLeft = bounds.getTopLeft();
        //draw in order
        for (auto& [_, fader] : faderComponents)
        {
            fader->setTopLeftPosition(topLeft + juce::Point<int>(x_offset, 0));
            fader->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
    private:
    const static int fader_width = 60;
    std::unordered_map<int, std::unique_ptr<FaderComponent>> &faderComponents;
};


class MixerPanel : public juce::Component
{
    public:
    MixerPanel(const GameState &state,
               std::function<void(int, double)> onFaderMoved,
               std::function<void(int, const juce::String&)> onNameChanged,
               std::function<void()> onNextClicked,
               std::function<void(bool)> onToggleClicked) : 
    fadersRow(faderComponents),
    onFaderMoved(std::move(onFaderMoved)),
    onNameChanged(std::move(onNameChanged)),
    onNextClicked(std::move(onNextClicked)),
    onToggleClicked(onToggleClicked)
    {
        topLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible(topLabel);
        
        for(const auto& [_, channel] : state.channels)
        {
            createFader(channel, state.step);
        }
        addAndMakeVisible(fadersViewport);
        fadersViewport.setScrollBarsShown(false, true);
        fadersViewport.setViewedComponent(&fadersRow, false);
        
        nextButton.onClick = [this] {
            this->onNextClicked();
        };
        addAndMakeVisible(nextButton);
        
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
        
        addAndMakeVisible(scoreLabel);
        
        updateGameUI(state);
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    void resized() override
    {
        auto bottom_height = 50;
        auto top_height = 20;
        auto r = getLocalBounds();
        
        auto topBounds = r.withHeight(top_height);
        
        auto topLabelBounds = topBounds;
        topLabel.setBounds(topLabelBounds);
        
        auto scoreLabelBounds = topBounds.withTrimmedLeft(topBounds.getWidth() - 90);
        scoreLabel.setBounds(scoreLabelBounds);
        
        auto fadersBounds = r.withTrimmedBottom(bottom_height).withTrimmedTop(top_height);
        fadersViewport.setBounds(fadersBounds);
        fadersRow.setSize(fadersRow.getWidth(),
                          fadersViewport.getHeight() - fadersViewport.getScrollBarThickness());
        
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
    
    void createFader(const ChannelState &channel, GameStep step)
    {
        {
            auto assertFader = faderComponents.find(channel.id);
            jassert(assertFader == faderComponents.end());
        }
        
        auto faderMoved = [this, id = channel.id] (double gain) {
            onFaderMoved(id, gain);
        };
        auto nameChanged = [this, id = channel.id] (juce::String newName){
            onNameChanged(id, newName);
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
    
    void removeFader(int channelToRemoveId)
    {
        
        auto fader = faderComponents.find(channelToRemoveId);
        jassert(fader != faderComponents.end());
        faderComponents.erase(fader);
        fadersRow.adjustWidth();
        resized();
    }
    
    void renameFader(int id, const juce::String& newName)
    {
        auto &fader = faderComponents[id];
        fader->setName(newName);
    }
    
    void updateGameUI(const GameState& new_state)
    {
        targetMixButton.setToggleState(new_state.step == Listening || 
                                       new_state.step == ShowingTruth, 
                                       juce::dontSendNotification);
        
        for(auto& [id, fader] : faderComponents){
            const ChannelState &channel = new_state.channels.at(id);
            fader->updateStep(new_state.step, channel);
        }
        
        switch(new_state.step)
        {
            case Begin : jassertfalse; break;
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
    
    private:
    std::unordered_map<int, std::unique_ptr<FaderComponent>> faderComponents = {};
    FaderRowComponent fadersRow;
    juce::Label topLabel;
    juce::Label scoreLabel;
    juce::Viewport fadersViewport;
    juce::TextButton nextButton;
    
    juce::ToggleButton targetMixButton;
    juce::ToggleButton userMixButton;
    
    std::function<void(int, double)> onFaderMoved;
    std::function<void(int, const juce::String&)> onNameChanged;
    std::function<void()> onNextClicked;
    std::function<void(bool)> onToggleClicked;
};



class EditorHost : public juce::AudioProcessorEditor
{
    public:
    
    EditorHost(ProcessorHost& p)
        : audioProcessor(p), 
    AudioProcessorEditor(p),
    mixerPanel(p.state,
               [this](int id, double gain){audioProcessor.setUserGain(id, gain);},
               [this](int id, const juce::String &newName){audioProcessor.renameChannel(id, newName);},
               [this](){ audioProcessor.nextClicked(); /* TODO insert state assert debug*/ },
               [this](bool isToggled){audioProcessor.toggleInputOrTarget(isToggled);})
    {
        addAndMakeVisible(mixerPanel);
        setSize(400, 300);
        setResizable(true, false);
    }
    ~EditorHost() override {}
    
    void paint(juce::Graphics & g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        mixerPanel.setBounds(getLocalBounds());
    }
    
    
    private:
    ProcessorHost& audioProcessor;
    public:
    MixerPanel mixerPanel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorHost)
};