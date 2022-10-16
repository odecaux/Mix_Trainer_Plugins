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

class ChannelComponent : public juce::Component
{
    public:
    ChannelComponent(const juce::String &name, std::function<void(double)> onFaderChange)
        : onFaderChange(onFaderChange)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxAbove, true, getWidth(), 20);
        fader.setRange(0.0f, (double)(ArraySize(slider_values) - 1), 1.0);
        fader.setScrollWheelEnabled(false);
        
        fader.onValueChange = [this] {
            //jassert(locked == false);
            targetValue = slider_value_to_gain((int)fader.getValue());
            this->onFaderChange(targetValue);
        };
        
        addAndMakeVisible(fader);
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
    
    void setGain(double newGain)
    {
        auto db = juce::Decibels::gainToDecibels(newGain);
        int slider_value = db_to_slider_value(db);
        fader.setValue(slider_value, juce::dontSendNotification);
    }
    /*
    void lockTo(double value)
    {
        jassert(locked == false);
        locked = true;
        targetValue = value;
        fader.setValue(value, juce::dontSendNotification);
        fader.setEnabled(false);
    }
    
    void unlock(double value)
    {
        jassert(locked == true);
        locked = false;
        targetValue = value;
        fader.setValue(value, juce::dontSendNotification);
        fader.setEnabled(true);
    }
    */
    private:
    juce::Label label;
    DecibelSlider fader;
    std::function<void(double)> onFaderChange;
    bool locked;
    double targetValue;
    double smoothing;
};


class FaderRowComponent : public juce::Component
{
    public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<ChannelComponent>>& channelComponents)
        : channelComponents(channelComponents)
    {
    }
    
    void adjustWidth() 
    {
        setSize(fader_width * (int)channelComponents.size(), getHeight());
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
        for (auto& channelPair : channelComponents)
        {
            auto& channel = channelPair.second;
            channel->setTopLeftPosition(topLeft + juce::Point<int>(x_offset, 0));
            channel->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
    private:
    const static int fader_width = 60;
    std::unordered_map<int, std::unique_ptr<ChannelComponent>> &channelComponents;
};


class MixerPanel : public juce::Component
{
    public:
    MixerPanel(std::unordered_map<int, ChannelState> channels,
               Listening listening,
               std::function<void(int, double)> onFaderMoved,
               std::function<void()> onRandomizeClicked,
               std::function<void(bool)> onToggleClicked) : 
    faders(channelComponents),
    onFaderMoved(std::move(onFaderMoved)),
    onRandomizeClicked(std::move(onRandomizeClicked)),
    onToggleClicked(onToggleClicked)
    {
        for(const auto& [_, channel] : channels)
        {
            createChannel(channel.id);
            renameChannel(channel.id, channel.name);
            setChannelGain(channel.id, channel.edited_gain);
        }
        addAndMakeVisible(fadersViewport);
        fadersViewport.setScrollBarsShown(false, true);
        fadersViewport.setViewedComponent(&faders, false);
        
        randomizeButton.setButtonText("Randomize");
        randomizeButton.onClick = [this] {
            this->onRandomizeClicked();
        };
        addAndMakeVisible(randomizeButton);
        
        targetOrUserToggle.setToggleState(listening == Target ? true : false, juce::dontSendNotification);
        targetOrUserToggle.setButtonText("Listen to target");
        targetOrUserToggle.onStateChange = [this] {
            this->onToggleClicked(targetOrUserToggle.getToggleState());
        };
        addAndMakeVisible(targetOrUserToggle);
        
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    void resized() override
    {
        auto bottom_height = 80;
        
        auto r = getLocalBounds();
        auto fileListBounds = r.withTrimmedBottom(bottom_height);
        fadersViewport.setBounds(fileListBounds);
        faders.setSize(faders.getWidth(),
                       fadersViewport.getHeight() - fadersViewport.getScrollBarThickness());
        
        auto bottomStripBounds = r.withTrimmedTop(r.getHeight() - bottom_height);
        auto center = bottomStripBounds.getCentre();
        
        auto button_temp = juce::Rectangle(100, 50);
        auto randomizeBounds = button_temp.withCentre(center + juce::Point(-50, 0)); 
        
        auto toggleBounds = bottomStripBounds.withWidth(200);
        targetOrUserToggle.setBounds(toggleBounds);
        randomizeButton.setBounds(randomizeBounds);
    }
    
    void createChannel(int newChannelId)
    {
        {
            auto assertChannel = channelComponents.find(newChannelId);
            jassert(assertChannel == channelComponents.end());
        }
        
        auto callback = [this, newChannelId] (double gain) {
            onFaderMoved(newChannelId, gain);
        };
        
        auto newChannel = std::make_unique<ChannelComponent>(juce::String(newChannelId), callback);
        faders.addAndMakeVisible(newChannel.get());
        channelComponents[newChannelId] = std::move(newChannel);
        
        faders.adjustWidth();
        resized();
    }
    
    void removeChannel(int channelToRemoveId)
    {
        
        auto channel = channelComponents.find(channelToRemoveId);
        jassert(channel != channelComponents.end());
        channelComponents.erase(channel);
        faders.adjustWidth();
        resized();
    }
    
    void renameChannel(int id, const juce::String& newName)
    {
        auto &channel = channelComponents[id];
        channel->setName(newName);
    }
    
    
    void setChannelGain(int id, double newGain)
    {
        auto &channel = channelComponents[id];
        channel->setGain(newGain);
    }
    
    std::unordered_map<int, std::unique_ptr<ChannelComponent>> channelComponents = {};
    FaderRowComponent faders;
    std::function<void(int, double)> onFaderMoved;
    std::function<void()> onRandomizeClicked;
    std::function<void(bool)> onToggleClicked;
    juce::Viewport fadersViewport;
    juce::TextButton playStopButton;
    juce::TextButton randomizeButton;
    juce::ToggleButton targetOrUserToggle;
    
    private:
};



class EditorHost : public juce::AudioProcessorEditor
{
    public:
    
    EditorHost(ProcessorHost& p)
        : audioProcessor(p), 
    AudioProcessorEditor(p),
    mixerPanel(p.state.channels, 
               p.state.listening,
               [this](int id, double gain){audioProcessor.setUserGain(id, gain);},
               [this](){audioProcessor.randomizeGains();},
               [this](bool isToggled){audioProcessor.toggleListeningState(isToggled);})
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