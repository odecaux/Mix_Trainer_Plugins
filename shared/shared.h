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
    from round_trip{static_cast<from>(to_value)};
    if (round_trip != from_value)
        throw std::runtime_error("Naughty cast");
#endif
    return to_value;
}
//#define ArraySize(array) (sizeof((array)) / sizeof(*(array)))

static int random_positive_int(int max = INT_MAX)
{
    auto seed = juce::Random::getSystemRandom().nextInt(max);
    return seed;
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

using Channel_DSP_FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
using Channel_DSP_Gain       = juce::dsp::Gain<float>;
using Channel_DSP_Compressor = juce::dsp::Compressor<float>;
using Channel_DSP_Chain = juce::dsp::ProcessorChain<Channel_DSP_FilterBand, Channel_DSP_Compressor, Channel_DSP_Gain, Channel_DSP_Gain>;

void channel_dsp_update_chain(Channel_DSP_Chain *dsp_chain,
                              Channel_DSP_State state,
                              juce::CriticalSection *lock,
                              double sample_rate);

//------------------------------------------------------------------------
struct Channel_DSP_Callback : public juce::AudioSource
{
    Channel_DSP_Callback(juce::AudioSource* inputSource) : input_source(inputSource)
    {
        state = ChannelDSP_on();
    }
    
    virtual ~Channel_DSP_Callback() override = default;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        assert(bufferToFill.buffer != nullptr);
        input_source->getNextAudioBlock (bufferToFill);

        juce::ScopedNoDenormals noDenormals;

        {
            juce::dsp::AudioBlock<float> block(*bufferToFill.buffer, (size_t) bufferToFill.startSample);
            juce::dsp::ProcessContextReplacing<float> context (block);
            juce::ScopedLock processLock (lock);
            dsp_chain.process (context);
        }

        {
            juce::dsp::AudioBlock<float> block(*bufferToFill.buffer, (size_t) bufferToFill.startSample);
            juce::dsp::ProcessContextReplacing<float> context (block);
            master_volume.process(context);
        }
    }
    
    void prepareToPlay (int blockSize, double sampleRate) override
    {
        sample_rate = sampleRate;
        input_source->prepareToPlay (blockSize, sampleRate);

        dsp_chain.prepare({ sampleRate, (juce::uint32) blockSize, 2 }); //TODO always stereo ?
        channel_dsp_update_chain(&dsp_chain, state, &lock, sample_rate);
        master_volume.prepare({ sampleRate, (juce::uint32) blockSize, 2 });
    }

    void releaseResources() override
    {
        input_source->releaseResources();
    }

    void push_new_dsp_state(Channel_DSP_State new_state)
    {
        state = new_state;
        if(sample_rate != -1.0)
            channel_dsp_update_chain(&dsp_chain, state, &lock, sample_rate);
    }

    void push_master_volume_db(double master_volume_db)
    {
        master_volume.setGainDecibels((float)master_volume_db);
    }

    Channel_DSP_State state;
    Channel_DSP_Chain dsp_chain;
    juce::dsp::Gain<float> master_volume;

    double sample_rate = -1.0;
    juce::CriticalSection lock;
    juce::AudioSource *input_source;
};

struct Game_Channel
{
    int id;
    char name[128];
    float min_freq;
    float max_freq;
};

struct Daw_Channel
{
    int id;
    int assigned_game_channel_id;
    char name[128];
};

struct MuliTrack_Model;

using multitrack_observer_t = std::function < void(MuliTrack_Model*) >;

enum MultiTrack_Observers_ID : int
{
    MultiTrack_Observers_Debug = 0,
    MultiTrack_Observers_Broadcast,
    MultiTrack_Observers_Channel_Settings
};

struct MuliTrack_Model
{
    std::unordered_map<int, Game_Channel> game_channels;
    std::vector<int> order;
    std::unordered_map<int, Daw_Channel> daw_channels;
    std::unordered_map<int, multitrack_observer_t> observers;
    std::unordered_map<int, int> assigned_daw_track_count;
};

static void debug_multitrack_model(MuliTrack_Model *model)
{    
    assert(model->game_channels.size() == model->order.size());

    DBG("Game Channels :");
    for (auto& [id, game_channel] : model->game_channels)
    {
        DBG(game_channel.name << ", " << game_channel.id % 100);
        DBG("-----> bound to " << model->assigned_daw_track_count.at(id) << " tracks");
    }
    assert(model->game_channels.size() == model->order.size());

    DBG("Order");
    for (auto channel_id : model->order)
    {
        DBG(model->game_channels.at(channel_id).name);
    }
    assert(model->game_channels.size() == model->order.size());
    
    DBG("Daw Channels :");
    for (auto& [id, daw_channel] : model->daw_channels)
    {
        DBG(daw_channel.name << ", " << daw_channel.id % 100);
        if (daw_channel.assigned_game_channel_id == -1)
        {
            DBG("-----> unassigned");
        }
        else
        {
            auto game_channel_it = model->game_channels.find(daw_channel.assigned_game_channel_id);
            if(game_channel_it != model->game_channels.end())
                DBG("-----> " << game_channel_it->second.name << ", " << daw_channel.assigned_game_channel_id % 100);
            else 
                DBG("-----> incorrect assignment : " << daw_channel.assigned_game_channel_id % 100);
        }
    }
    DBG("\n\n\n");
}

static void multitrack_model_broadcast_change(MuliTrack_Model *model, int observer_id_to_skip = -1)
{       
    assert(model->game_channels.size() == model->order.size());
    model->assigned_daw_track_count.clear();
    for (const auto& [id, game_channel] : model->game_channels)
    {
        model->assigned_daw_track_count.emplace(game_channel.id, 0);
    }
    for (const auto& [id, daw_channel] : model->daw_channels)
    {
        //if (daw_channel.assigned_game_channel_id == -1) continue;
        //TODO HACK, not sure if it is a proper fix
        auto it = model->assigned_daw_track_count.find(daw_channel.assigned_game_channel_id);
        if(it == model->assigned_daw_track_count.end())
            continue;
        it->second++;
    }
    for (auto &[id, observer] : model->observers)
    {    
        assert(model->game_channels.size() == model->order.size());
        if(id != observer_id_to_skip)
            observer(model);
        assert(model->game_channels.size() == model->order.size());
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

enum Widget_Interaction_Type {
    Widget_Editing,
    Widget_Hiding,
    Widget_Showing
};


class Continuous_Fader : public juce::Component
{
public:
    Continuous_Fader()
    {
        label.setText("Main Volume", juce::NotificationType::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setEditable(false);
        addAndMakeVisible(label);

        fader.get_text_from_value = [&] (double db) -> juce::String
        {
            if (db == -30.0)
                return "Muted";
            else
                return juce::Decibels::toString(db, 0);
        };

        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, true, fader.getTextBoxWidth(), 40);
        fader.setRange(-30.0, 0.0, 1);
        fader.setScrollWheelEnabled(true);

        fader.onValueChange = [this, &onMove = fader_moved_db_callback] {
            onMove(fader.getValue());
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

    void update(double new_value_db)
    {
        fader.setValue(new_value_db);
    }
    std::function < void(double) > fader_moved_db_callback;

private:
    juce::Label label;
    TextSlider fader;
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
            assert(this->DEBUG_step == Widget_Editing);
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
    
    void update(Widget_Interaction_Type new_step, int new_pos)
    {
        DEBUG_step = new_step;
        switch (new_step)
        {
            case Widget_Editing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(true);
                fader.setVisible(true);
            } break;
            case Widget_Hiding :
            {
                assert(new_pos == -1);
                //TODO ?
                //fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(false);
            } break;
            case Widget_Showing :
            {
                fader.setValue((double)new_pos, juce::dontSendNotification);
                fader.setEnabled(false);
                fader.setVisible(true);
            } break;
        }

        repaint();
    }
    
private:
    Widget_Interaction_Type DEBUG_step;
    juce::Label label;
    TextSlider fader;
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



class List_Row_Label  : public juce::Label
{
public:
    List_Row_Label (
        juce::String newRowText,
        const std::function < void(int) > &onMouseDown,
        const std::function < void(int, juce::String) > &onTextChanged,
        const std::function < void(juce::String) > &onCreateRow
    )
    : new_row_text(newRowText),
      mouse_down_callback(onMouseDown),
      text_changed_callback(onTextChanged),
      create_row_callback(onCreateRow)
    {
        // double click to edit the label text; single click handled below
        setEditable (false, true, true);
    }

    void mouseDown (const juce::MouseEvent& ) override
    {   
        mouse_down_callback(row);
        //juce::Label::mouseDown (event);
    }

    void editorShown (juce::TextEditor * new_editor) override
    {
        if(is_last_row)
            new_editor->setText("", juce::dontSendNotification);
    }

    void textWasEdited() override
    {
        bool input_is_empty = getText().isEmpty();

        //rename track
        if (!is_last_row)
        {
            if (input_is_empty)
            {
                setText(row_text, juce::dontSendNotification);
                return;
            }
            text_changed_callback(row, getText());
        }
        //insert new track
        else 
        {
                
            if (input_is_empty)
            {
                setText(new_row_text, juce::dontSendNotification);
                return;
            }
            is_last_row = false;
            create_row_callback(getText());
        }
    }

    void update(int new_row, juce::String new_text, bool new_is_last_row)
    {
        row = new_row;
        row_text = new_text;
        is_last_row = new_is_last_row;
        if (!is_last_row)
        {
            setJustificationType(juce::Justification::left);
            setText (row_text, juce::dontSendNotification);
            setColour(juce::Label::textColourId, juce::Colours::white);
        }
        else
        {
            setJustificationType(juce::Justification::centred);
            setText (new_row_text, juce::dontSendNotification);
            setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto& lf = getLookAndFeel();
        if (! dynamic_cast<juce::LookAndFeel_V4*>(&lf))
            lf.setColour (textColourId, juce::Colours::black);
    
        juce::Label::paint (g);
    }

    const std::function < void(int) > &mouse_down_callback;
    const std::function < void(int, juce::String) > &text_changed_callback;
    const std::function < void(juce::String) > &create_row_callback;

    int row;
    juce::String row_text;
    bool is_last_row;
private:
    juce::String new_row_text;
    juce::Colour textColour;
};


class Selection_List : public juce::Component,
public juce::ListBoxModel
{
public:
    explicit Selection_List(
                   std::vector<juce::String> initial_row_texts,
                   const std::vector<int> &initial_selection_indices,
                   bool multiple_selection_enabled,
                   std::function < void(const std::vector<int> &) > onSelectionChaged)
    :
      row_texts(std::move(initial_row_texts)),
      selection_changed_callback(std::move(onSelectionChaged))
    {
        list_comp.setModel(this);
        list_comp.setMultipleSelectionEnabled(multiple_selection_enabled);
        //list_comp.updateContent();
        for (int idx : initial_selection_indices)
        {
            list_comp.selectRow(idx, false, false);
        }
        //TODO slow
        addAndMakeVisible(list_comp);
    }

    int getNumRows() override
    {
        return checked_cast<int>(row_texts.size());
    }

    void paintListBoxItem (int rowNumber,
                           juce::Graphics &g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        auto bounds = juce::Rectangle { 0, 0, width, height };
        g.setColour(juce::Colours::white);
        g.drawText(row_texts[rowNumber], bounds, juce::Justification::centredLeft);
        if (rowIsSelected)
        {
            g.drawRect(bounds);
        }
    }

    void selectedRowsChanged(int) override
    {
        auto selected_rows = list_comp.getSelectedRows();
        std::vector<int> selected_indices{};
        for (auto i = 0; i < selected_rows.size(); i++)
        {
            int selected_idx = selected_rows[i];
            selected_indices.push_back(selected_idx);
        }
        //TODO slow ??? who cares ?
        selection_changed_callback(selected_indices);
    }

    void resized() override
    {
        list_comp.setBounds(getLocalBounds());
    }

    std::function < void(const std::vector<int> &) > selection_changed_callback;

private:
    std::vector<juce::String> row_texts;
    juce::ListBox list_comp;
};

#endif //SHARED_H
