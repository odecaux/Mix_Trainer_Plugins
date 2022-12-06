/* date = October 14th 2022 5:21 pm */

#ifndef SHARED_H
#define SHARED_H

#include <vector>
#include <algorithm>
#include <functional>
#undef NDEBUG
#include <assert.h>

static float denormalize_frequency(float value)
{
    float a = powf(value, 2.0f);
    float b = a * (20000.0f - 20.0f);
    return b + 20.0f;
}

static float normalize_frequency(float frequency)
{
    return std::sqrt((frequency - 20.0f) / (20000.0f - 20.0f));
}

//#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

struct Timer: public juce::Timer
{
    void timerCallback() override {
        callback(juce::Time::currentTimeMillis());
    }
    std::function<void(juce::int64)> callback;
};

enum DSP_Filter_Type
{
    Filter_None = 0,
    Filter_HighPass,
    Filter_HighPass1st,
    Filter_LowShelf,
    Filter_BandPass,
    Filter_AllPass,
    Filter_AllPass1st,
    Filter_Notch,
    Filter_Peak,
    Filter_HighShelf,
    Filter_LowPass1st,
    Filter_Low_Pass,
    Filter_LastID
};

struct DSP_EQ_Band {
    DSP_Filter_Type type;
    float frequency;
    float quality;
    float gain;
};


struct Channel_DSP_State{
    double gain;
    DSP_EQ_Band bands[1];
};

Channel_DSP_State ChannelDSP_on();
Channel_DSP_State ChannelDSP_off();
Channel_DSP_State ChannelDSP_gain(double gain);

struct Settings{
    float difficulty;
};

struct Stats {
    int total_score;
};

bool equal_double(double a, double b, double theta);
size_t db_to_slider_pos(double db, const std::vector<double> &db_values);
size_t gain_to_slider_pos(double gain, const std::vector<double> &db_values);
double slider_pos_to_gain(size_t pos, const std::vector<double> &db_values);

class DecibelSlider : public juce::Slider
{
public:
    explicit DecibelSlider(const std::vector < double > &db_values)
    : db_values(db_values)
    {
    }
    
    juce::String getTextFromValue(double pos) override
    {
        double db = db_values[(size_t)pos];
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
            assert(editor != nullptr);
            editor->setJustification(juce::Justification::centred);
        };
        addAndMakeVisible(label);
        
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, true, fader.getTextBoxWidth(), 40);
        fader.setRange(0.0, (double)(db_values.size() - 1), 1.0);
        fader.setScrollWheelEnabled(true);
        
        fader.onValueChange = [this, onMove = std::move(onFaderMove)] {
            assert(this->step == FaderStep_Editing);
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
    
    void setTrackName(const juce::String& new_name)
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
                assert(new_pos == -1);
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


enum Transport_Step
{
    Transport_Stopped,
    Transport_Playing,
    Transport_Paused,
};

struct Transport_State
{
    Transport_Step step;
};

enum Audio_Command_Type
{
    Audio_Command_Play,
    Audio_Command_Pause,
    Audio_Command_Stop,
    Audio_Command_Seek,
    Audio_Command_Load,
};

struct Audio_Command
{
    Audio_Command_Type type;
    float value_f;
    juce::File value_file;
};

struct Return_Value
{
    bool value_b;
};


struct Frequency_Bounds_Widget : juce::Component
{
public:
    Frequency_Bounds_Widget(float min_frequency, float max_frequency)
    {
        
        minLabel.setSize(200, 30);
        minLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(minLabel);
        
        maxLabel.setSize(200, 30);
        maxLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(maxLabel);
        
        frequencyRangeSlider.setSize(200, 30);
        frequencyRangeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        frequencyRangeSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
        frequencyRangeSlider.setRange(0.0f, 1.0f);
        frequencyRangeSlider.onValueChange = [this]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_frequency(minValue);
            float new_max_frequency = denormalize_frequency(maxValue);
            minLabel.setText(juce::String((int)new_min_frequency), juce::dontSendNotification);
            maxLabel.setText(juce::String((int)new_max_frequency), juce::dontSendNotification);
            on_mix_max_changed(new_min_frequency, new_max_frequency, false);
        };

        frequencyRangeSlider.onDragEnd = [this]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_frequency(minValue);
            float new_max_frequency = denormalize_frequency(maxValue);
            maxLabel.setText(juce::String((int)new_max_frequency), juce::dontSendNotification);
            on_mix_max_changed(new_min_frequency, new_max_frequency, true);
        };
        addAndMakeVisible(frequencyRangeSlider);
        frequencyRangeSlider.setMinAndMaxValues(normalize_frequency(min_frequency), normalize_frequency(max_frequency));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();
        frequencyRangeSlider.setCentrePosition(center.x, center.y);
        maxLabel.setCentrePosition(center.x, center.y - 40);
        minLabel.setCentrePosition(center.x, center.y + 40);
    }
    std::function < void(float, float, bool) > on_mix_max_changed;

private:
    juce::Label minLabel;
    juce::Label maxLabel;
    juce::Slider frequencyRangeSlider;
};

#endif //SHARED_H
