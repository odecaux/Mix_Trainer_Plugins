/* date = October 14th 2022 5:21 pm */

#ifndef SHARED_H
#define SHARED_H

#include <vector>
#include <algorithm>
#include <functional>
#undef NDEBUG
#include <assert.h>

template<class to, class from>
inline to checked_cast(const from& from_value) {
    to to_value{static_cast<to>(from_value)};
#if _DEBUG
    DBG("test");
    from round_trip{static_cast<from>(to_value)};
    if (round_trip != from_value)
        throw std::runtime_error("Naughty cast");
#endif
    return to_value;
}
//#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

static int random_int(int max)
{
    //I want to keep the value positive, it's an index !!!
    auto seed = juce::Random::getSystemRandom().nextInt();
    return ((seed % max) + max) % max;
}

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

struct Compressor_DSP_State {
    bool is_on;
    float threshold_gain;
    float ratio;
    float attack;
    float release;
    float makeup_gain;
};

struct Channel_DSP_State {
    double gain;
    DSP_EQ_Band eq_bands[1];
    Compressor_DSP_State comp;
};


struct Audio_File
{
    juce::File file;
    juce::String title;
    juce::Range<juce::int64> loop_bounds;
    juce::Range<int> freq_bounds;
    float max_gain;
};

Channel_DSP_State ChannelDSP_on();
Channel_DSP_State ChannelDSP_off();
Channel_DSP_State ChannelDSP_gain(double gain);
DSP_EQ_Band eq_band_peak(float frequency, float quality, float gain);


juce::dsp::IIR::Coefficients < float > ::Ptr make_coefficients(DSP_EQ_Band band, double sample_rate);

//------------------------------------------------------------------------
struct Channel_DSP_Callback : public juce::AudioSource
{
    Channel_DSP_Callback(juce::AudioSource* inputSource) : input_source(inputSource)
    {
    }
    
    virtual ~Channel_DSP_Callback() override = default;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        assert(bufferToFill.buffer != nullptr);
        input_source->getNextAudioBlock (bufferToFill);

        juce::ScopedNoDenormals noDenormals;

        juce::dsp::AudioBlock<float> block(*bufferToFill.buffer, (size_t) bufferToFill.startSample);
        juce::dsp::ProcessContextReplacing<float> context (block);
        {
            juce::ScopedLock processLock (lock);
            dsp_chain.process (context);
        }
    }
    
    void prepareToPlay (int blockSize, double sampleRate) override
    {
        sample_rate = sampleRate;
        input_source->prepareToPlay (blockSize, sampleRate);
        dsp_chain.prepare ({ sampleRate, (juce::uint32) blockSize, 2 }); //TODO always stereo ?
        updateDspChain();
    }

    void releaseResources() override 
    {
        input_source->releaseResources();
    }

    void push_new_dsp_state(Channel_DSP_State new_state)
    {
        state = new_state;
        if(sample_rate != -1.0)
            updateDspChain();
    }

    Channel_DSP_State state;

    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain       = juce::dsp::Gain<float>;
    using Compressor = juce::dsp::Compressor<float>;
    juce::dsp::ProcessorChain<FilterBand, Compressor, Gain, Gain> dsp_chain;
    double sample_rate = -1.0;
    juce::CriticalSection lock;
    juce::AudioSource *input_source;

private:
    void updateDspChain()
    {
        for (auto i = 0; i < 1 /* une seule bande pour l'instant */; ++i) {
            auto new_coefficients = make_coefficients(state.eq_bands[i], sample_rate);
            assert(new_coefficients);
            {
                juce::ScopedLock processLock (lock);
                if (i == 0)
                    *dsp_chain.get<0>().state = *new_coefficients;
            }
        }
        //compressor 
        dsp_chain.setBypassed<1>(!state.comp.is_on);
        if (state.comp.is_on)
        {
            dsp_chain.get<1>().setThreshold(state.comp.threshold_gain);
            dsp_chain.get<1>().setRatio(state.comp.ratio);
            dsp_chain.get<1>().setAttack(state.comp.attack);
            dsp_chain.get<1>().setRelease(state.comp.release);
            //makeup gain
            dsp_chain.setBypassed<2>(!state.comp.is_on);
            dsp_chain.get<2>().setGainLinear ((float)state.gain);
        }
        //gain
        dsp_chain.get<3>().setGainLinear ((float)state.gain);
    }
};
struct Game_Channel
{
    int id;
    juce::String name;
    float min_freq;
    float max_freq;
};

struct Daw_Channel
{
    int id;
    int assigned_game_channel_id;
    juce::String name;
};

struct MuliTrack_Model;

using multitrack_observer_t = std::function < void(MuliTrack_Model*) >;

enum MultiTrack_Observers_ID : int
{
    MultiTrack_Observers_Debug
};

struct MuliTrack_Model
{
    std::unordered_map<int, Game_Channel> game_channels;
    std::vector<int> order;
    std::unordered_map<int, Daw_Channel> daw_channels;
    std::unordered_map<int, multitrack_observer_t> observers;
    //std::unordered_map<int, int> assigned_daw_track_count;
};

static void multitrack_model_broadcast_change(MuliTrack_Model *model, int observer_id_to_skip = -1)
{
    for (auto &[id, observer] : model->observers)
    {
        if(id != observer_id_to_skip)
            observer(model);
    }
}

static void multitrack_model_add_observer(MuliTrack_Model *model, int observer_id, multitrack_observer_t observer)
{
    assert(!model->observers.contains(observer_id));
    auto [it, result] = model->observers.emplace(observer_id, std::move(observer));
    assert(result);
}
static void multitrack_model_remove_observer(MuliTrack_Model *model, int observer_id)
{
    assert(model->observers.contains(observer_id));
    auto count = model->observers.erase(observer_id);
    assert(count == 1);
}

struct Game_Channel_Broadcast_Message {
    Game_Channel channels[256];
    int channel_count;
};

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

struct TextSlider : public juce::Slider
{
    TextSlider() : Slider(){}
    juce::String getTextFromValue(double pos) override { return get_text_from_value(pos); }
    std::function < juce::String(double) > get_text_from_value = [] (double){ return ""; };
};

enum FaderStep {
    FaderStep_Editing,
    FaderStep_Hiding,
    FaderStep_Showing
};

class FaderComponent : public juce::Component
{
public:
    explicit FaderComponent(const std::vector<double> &dbValues,
                            const juce::String &name,
                            std::function<void(int)> &&onFaderMove)
    : db_values(dbValues)
    {
        label.setText(name, juce::NotificationType::dontSendNotification);
        label.setEditable(false);
        addAndMakeVisible(label);
        
        fader.get_text_from_value = [&] (double pos) -> juce::String
        {
            double db = db_values[static_cast<size_t>(pos)];
            if (db == -100.0) 
                return "Muted";
            else
                return juce::Decibels::toString(db, 0);
        };
        
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
        auto labelBounds = r.removeFromTop(50);
        label.setBounds(labelBounds);
        
        auto faderBounds = r;
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
    TextSlider fader;
    FaderStep step;
    const std::vector<double> &db_values;
    //double targetValue;
    //double smoothing;
};

class FaderRowComponent : public juce::Component
{
public:
    FaderRowComponent(std::unordered_map<int, std::unique_ptr<FaderComponent>>& mixerFaders)
    : faders(mixerFaders)
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

static float denormalize_slider_frequency(float value)
{
    float a = powf(value, 2.0f);
    float b = a * (20000.0f - 20.0f);
    return b + 20.0f;
}

static float normalize_slider_frequency(float frequency)
{
    return std::sqrt((frequency - 20.0f) / (20000.0f - 20.0f));
}

struct Frequency_Bounds_Widget : juce::Component
{
public:
    Frequency_Bounds_Widget()
    {
        
        minLabel.setJustificationType(juce::Justification::right);
        addAndMakeVisible(minLabel);
        
        maxLabel.setJustificationType(juce::Justification::left);
        addAndMakeVisible(maxLabel);
        
        frequencyRangeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        frequencyRangeSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
        frequencyRangeSlider.setRange(0.0f, 1.0f);

        auto update_labels = [this] (int min_freq, int max_freq)
        {
            auto min_text = juce::String(min_freq) + " Hz";
            auto max_text = juce::String(max_freq) + " Hz";
            minLabel.setText(min_text, juce::dontSendNotification);
            maxLabel.setText(max_text, juce::dontSendNotification);
        };

        frequencyRangeSlider.onValueChange = [this, update_labels]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_slider_frequency(minValue);
            float new_max_frequency = denormalize_slider_frequency(maxValue);
            update_labels((int)new_min_frequency, (int) new_max_frequency);
            if(on_mix_max_changed)
                on_mix_max_changed(new_min_frequency, new_max_frequency, false);
        };

        frequencyRangeSlider.onDragEnd = [this, update_labels]{
            float minValue = (float)frequencyRangeSlider.getMinValue();
            float maxValue = (float)frequencyRangeSlider.getMaxValue();
            float new_min_frequency = denormalize_slider_frequency(minValue);
            float new_max_frequency = denormalize_slider_frequency(maxValue);
            update_labels((int)new_min_frequency, (int) new_max_frequency);
            if(on_mix_max_changed)
                on_mix_max_changed(new_min_frequency, new_max_frequency, true);
        };
        addAndMakeVisible(frequencyRangeSlider);
    }

    void setMinAndMaxValues(float min_frequency, float max_frequency)
    {
        frequencyRangeSlider.setMinAndMaxValues(
            normalize_slider_frequency(min_frequency), 
            normalize_slider_frequency(max_frequency)
        );
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto min_label_bounds = bounds.removeFromLeft(70);
        auto max_label_bounds = bounds.removeFromRight(70);

        minLabel.setBounds(min_label_bounds);
        frequencyRangeSlider.setBounds(bounds);
        maxLabel.setBounds(max_label_bounds);
    }
    std::function < void(float, float, bool) > on_mix_max_changed;

private:
    juce::Label minLabel;
    juce::Label maxLabel;
    juce::Slider frequencyRangeSlider;
};

#endif //SHARED_H
