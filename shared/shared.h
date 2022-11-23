/* date = October 14th 2022 5:21 pm */

#ifndef SHARED_H
#define SHARED_H

#include <vector>
#include <algorithm>
#include <functional>

//#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

struct ChannelDSPState{
    double gain;
    double low_shelf_gain;
    double low_shelf_freq;
    double high_shelf_gain;
    double high_shelf_freq;
};

static ChannelDSPState ChannelDSP_on()
{
    return {
        .gain = 1.0,
        .low_shelf_gain = 1.0,
        .high_shelf_gain = 1.0,
    };
}

static ChannelDSPState ChannelDSP_off()
{
    return {
        .gain = 0.0,
    };
}


static ChannelDSPState ChannelDSP_gain(double gain)
{
    return {
        .gain = gain,
        .low_shelf_gain = 1.0,
        .high_shelf_gain = 1.0,
    };
}

struct Settings{
    float difficulty;
};

struct Stats {
    int total_score;
};


static double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

static int db_to_slider_pos(double db, const std::vector<double> &db_values)
{
    for(auto i = 0; i < db_values.size(); i++)
    {
        if(equal_double(db, db_values[i], 0.001)) return i;
    }
    jassertfalse;
    return -1;
}

static int gain_to_slider_pos(double gain, const std::vector<double> &db_values)
{ 
    return db_to_slider_pos(juce::Decibels::gainToDecibels(gain), db_values);
}

static double slider_pos_to_gain(int pos, const std::vector<double> &db_values)
{
    return juce::Decibels::decibelsToGain(db_values[pos]);
}

class DecibelSlider : public juce::Slider
{
public:
    explicit DecibelSlider(const std::vector < double > &db_values)
    : db_values(db_values)
    {
    }
    
    juce::String getTextFromValue(double pos) override
    {
        double db = db_values[(int)pos];
        if (db == -100.0) 
            return "Muted";
        else
            return juce::Decibels::toString(db, 0);
    }

    const std::vector < double > &db_values;
};

enum FaderStep {
    FaderStep_Editing,
    FaderStep_Hiding,
    FaderStep_Showing
};

class FaderComponent : public juce::Component
{
public:
    explicit FaderComponent(const std::vector<double> &db_values,
                            const juce::String &name,
                            std::function<void(int)> &&onFaderMove,
                            std::function<void(const juce::String&)> &&onEditedName)
    :   fader(db_values)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        label.setEditable(true);
        label.onTextChange = [this, onEdited = std::move(onEditedName)] {
            onEdited(label.getText());
        };
        label.onEditorShow = [&] {
            auto *editor = label.getCurrentTextEditor();
            jassert(editor != nullptr);
            editor->setJustification(juce::Justification::centred);
        };
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, true, fader.getTextBoxWidth(), 40);
        fader.setRange(0.0, (double)(db_values.size() - 1), 1.0);
        fader.setScrollWheelEnabled(true);
        
        fader.onValueChange = [this, onMove = std::move(onFaderMove)] {
            jassert(this->step == FaderStep_Editing);
            onMove((int)fader.getValue());
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
    
    void setName(const juce::String& new_name) //TODO rename, ça override un truc de component
    {
        label.setText(new_name, juce::dontSendNotification);
    }
    
    void update(FaderStep new_step, int new_pos)
    {
        switch (new_step)
        {
            case FaderStep_Editing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case FaderStep_Hiding :
            {
                jassert(new_pos == -1);
                //TODO ?
                //fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            } break;
            case FaderStep_Showing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            } break;
        }

        step = new_step;
        repaint();
    }
    
private:
    juce::Label label;
    DecibelSlider fader;
    FaderStep step;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& faders)
    : faders(faders)
    {
    }
    
    void adjustWidth() 
    {
        setSize(fader_width * (int)faders.size(), getHeight());
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
        auto top_left = bounds.getTopLeft();
        //draw in order
        for (auto& [_, fader] : faders)
        {
            fader->setTopLeftPosition(top_left + juce::Point<int>(x_offset, 0));
            fader->setSize(fader_width, bounds.getHeight());
            x_offset += fader_width;
        }
        adjustWidth();
    }
    
private:
    const static int fader_width = 60;
    std::unordered_map<int, std::unique_ptr<FaderComponent>> &faders;
};



#endif //SHARED_H