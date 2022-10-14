/*
  ==============================================================================

    PluginEditor_Master.h
    Created: 16 Sep 2022 9:09:26am
    Author:  Octave

  ==============================================================================
*/

#pragma once
#include <vector>
#include <algorithm>
#include <functional>

#define DISCRETE_VALUE_COUNT 5

static double slider_value_to_db(int value)
{
    switch (value) {
        case 0:
        return -100.0;
        case 1:
        return -24;
        case 2:
        return -16;
        case 3:
        return -10;
        case 4:
        return -4;
        default: {
            jassertfalse;
            return 0;
        }
    }
}

static double slider_value_to_db(double value)
{
    int int_value = (int)value;
    jassert((double)value == value);
    return slider_value_to_db(int_value);
}

class DecibelSlider : public juce::Slider
{
    public:
    DecibelSlider() = default;
    
    juce::String getTextFromValue(double value) override
    {
        return juce::Decibels::toString(slider_value_to_db(value));
    }
    
    private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecibelSlider)
};


//TODO don't use the slider's textbox, that way I can hide the value when locked

class ChannelComponent : public juce::Component
{
    public:
    ChannelComponent(const juce::String &name, std::function<void(float)> onFaderChange)
        : onFaderChange(onFaderChange)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxAbove, true, getWidth(), 20);
        fader.setRange(0.0f, (double)(DISCRETE_VALUE_COUNT - 1), 1.0);
        fader.setScrollWheelEnabled(false);
        
        fader.onValueChange = [this] {
            //jassert(locked == false);
            targetValue = fader.getValue();
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
    
    void lockTo(float value)
    {
        jassert(locked == false);
        locked = true;
        targetValue = value;
        fader.setValue(value, juce::dontSendNotification);
        fader.setEnabled(false);
    }
    
    void unlock(float value)
    {
        jassert(locked == true);
        locked = false;
        targetValue = value;
        fader.setValue(value, juce::dontSendNotification);
        fader.setEnabled(true);
    }
    
    private:
    juce::Label label;
    DecibelSlider fader;
    std::function<void(float)> onFaderChange;
    bool locked;
    float targetValue;
    float smoothing;
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
    MixerPanel(std::function<void(int, float)> onFaderMoved) : 
    faders(channelComponents),
    onFaderMoved(std::move(onFaderMoved))
    {
        addAndMakeVisible(fadersViewport);
        fadersViewport.setScrollBarsShown(false, true);
        fadersViewport.setViewedComponent(&faders, false);
        
        playStopButton.setButtonText("Play/Stop");
        playStopButton.onClick = [this] {
        };
        addAndMakeVisible(playStopButton);
        
        randomizeButton.setButtonText("Randomize");
        randomizeButton.onClick = [this] {
            //for (auto& channel : state.channels)
            //    channel.targetValue = juce::Random::getSystemRandom().nextFloat();
        };
        addAndMakeVisible(randomizeButton);
        
        targetVersusCurrentButton.setButtonText("Listen to target");
        targetVersusCurrentButton.onStateChange = [this] {
            //TODO does this need additionnal plumbing ?
            //state.isTargetPlaying = targetVersusCurrentButton.getToggleState();
        };
        addAndMakeVisible(targetVersusCurrentButton);
        
    }
    
    //==============================================================================
    
    void paint(juce::Graphics& g) override
    {
        juce::ignoreUnused(g);
    }
    
    void resized() override
    {
        auto r = getLocalBounds();
        auto fileListBounds = r.withTrimmedBottom(100);
        fadersViewport.setBounds(fileListBounds);
        faders.setSize(
                       faders.getWidth(),
                       fadersViewport.getHeight() - fadersViewport.getScrollBarThickness()
                       );
        
        auto bottomStripBounds = r.withTrimmedTop(r.getHeight() - 100);
        auto center_x = bottomStripBounds.getCentreX();
        
        auto playStopBounds = bottomStripBounds
            .withRight(center_x)
            .withLeft(center_x - 200);
        auto randomizeBounds = bottomStripBounds
            .withLeft(center_x)
            .withRight(center_x + 200);
        
        auto switchBounds = bottomStripBounds.withWidth(200);
        targetVersusCurrentButton.setBounds(switchBounds);
        playStopButton.setBounds(playStopBounds);
        randomizeButton.setBounds(randomizeBounds);
    }
    
    void createChannel(int newChannelId)
    {
        {
            auto assertChannel = channelComponents.find(newChannelId);
            jassert(assertChannel == channelComponents.end());
        }
        
        auto callback = [this, newChannelId] (float gain) {
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
        resized();
    }
    
    std::unordered_map<int, std::unique_ptr<ChannelComponent>> channelComponents = {};
    FaderRowComponent faders;
    std::function<void(int, float)> onFaderMoved;
    juce::Viewport fadersViewport;
    juce::TextButton playStopButton;
    juce::TextButton randomizeButton;
    juce::ToggleButton targetVersusCurrentButton; //TODO rename
    
    private:
};



class EditorMaster : public juce::AudioProcessorEditor
{
    public:
    
    EditorMaster(ProcessorMaster& p)
        : audioProcessor(p), 
    AudioProcessorEditor(p),
    mixerPanel([this](int id, float gain){audioProcessor.setGain(id, gain);})
    {
        addAndMakeVisible(mixerPanel);
        setSize(400, 300);
        setResizable(true, false);
    }
    ~EditorMaster() override {}
    
    void paint(juce::Graphics & g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        mixerPanel.setBounds(getLocalBounds());
    }
    
    MixerPanel mixerPanel;
    
    private:
    ProcessorMaster& audioProcessor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorMaster)
};

