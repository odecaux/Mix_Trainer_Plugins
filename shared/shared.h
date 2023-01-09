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

static uint32_t random_uint(uint32_t max = INT_MAX)
{
    int seed = juce::Random::getSystemRandom().nextInt(checked_cast<int>(max));
    return checked_cast<uint32_t>(seed);
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
    double gain_db;
    DSP_EQ_Band eq_bands[1];
    Compressor_DSP_State comp;
};


struct Audio_File
{
    bool is_valid;
    juce::File file;
    //juce::int64 hash_code;
    juce::Time last_modification_time;
    std::string title;
    juce::Range<int64_t> loop_bounds_ms;
    juce::Range<uint32_t> freq_bounds;
    float max_level;
    int64_t file_length_ms;
    int64_t hash;
};

Channel_DSP_State ChannelDSP_on();
Channel_DSP_State ChannelDSP_off();
Channel_DSP_State ChannelDSP_gain_db(double gain_db);
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
            normalization_gain.process(context);
            dsp_chain.process(context);
            master_gain.process(context);
        }
    }
    
    void prepareToPlay (int blockSize, double sampleRate) override
    {
        sample_rate = sampleRate;
        input_source->prepareToPlay (blockSize, sampleRate);

        normalization_gain.prepare({ sampleRate, (uint32_t) blockSize, 2 });
        dsp_chain.prepare({ sampleRate, (uint32_t) blockSize, 2 }); //TODO always stereo ?
        channel_dsp_update_chain(&dsp_chain, state, &lock, sample_rate);
        master_gain.prepare({ sampleRate, (uint32_t) blockSize, 2 });
    }

    void releaseResources() override
    {
        input_source->releaseResources();
    }

    void push_normalization_volume(float normalization_level)
    {
        normalization_gain.setGainLinear(normalization_level);
    }

    void push_new_dsp_state(Channel_DSP_State new_state)
    {
        state = new_state;
        if(sample_rate != -1.0)
            channel_dsp_update_chain(&dsp_chain, state, &lock, sample_rate);
    }

    void push_master_volume_db(double master_volume_db)
    {
        master_gain.setGainDecibels((float)master_volume_db);
    }

    Channel_DSP_State state;
    juce::dsp::Gain<float> normalization_gain;
    Channel_DSP_Chain dsp_chain;
    juce::dsp::Gain<float> master_gain;

    double sample_rate = -1.0;
    juce::CriticalSection lock;
    juce::AudioSource *input_source;
};

struct Game_Channel
{
    uint32_t id;
    char name[128];
    float min_freq;
    float max_freq;
};

struct Daw_Channel
{
    uint32_t id;
    int64_t assigned_game_channel_id;
    char name[128];
};

struct MuliTrack_Model;

using multitrack_observer_t = std::function < void(MuliTrack_Model*) >;

enum MultiTrack_Observers_ID : int
{
    MultiTrack_Observers_Debug = 0,
    MultiTrack_Observers_Broadcast,
    MultiTrack_Observers_Channel_Settings,
    MultiTrack_Observers_Main_Menu
};

struct MuliTrack_Model
{
    std::unordered_map<uint32_t, Game_Channel> game_channels;
    std::vector<int> order;
    std::unordered_map<int, Daw_Channel> daw_channels;
    std::unordered_map<int, multitrack_observer_t> observers;
    std::unordered_map<int, int> assigned_daw_track_count;
};

#if 0

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
            auto game_channel_it = model->game_channels.find(checked_cast<uint32_t>(daw_channel.assigned_game_channel_id));
            if(game_channel_it != model->game_channels.end())
                DBG("-----> " << game_channel_it->second.name << ", " << daw_channel.assigned_game_channel_id % 100);
            else 
                DBG("-----> incorrect assignment : " << daw_channel.assigned_game_channel_id % 100);
        }
    }
    DBG("\n\n\n");
}

#endif 

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
        if (daw_channel.assigned_game_channel_id == -1) continue;
        //TODO HACK, not sure if it is a proper fix
        auto it = model->assigned_daw_track_count.find(checked_cast<uint32_t>(daw_channel.assigned_game_channel_id));
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

enum Widget_Interaction_Type {
    Widget_Editing,
    Widget_Hiding,
    Widget_Showing
};


enum Transport_Step
{
    Transport_Stopped,
    Transport_Playing,
    Transport_Paused,
    Transport_Loading_Failed
};


enum Audio_Command_Type
{
    Audio_Command_Play,
    Audio_Command_Pause,
    Audio_Command_Stop,
    Audio_Command_Seek,
    Audio_Command_Update_Loop,
    Audio_Command_Load,
};

struct Audio_Command
{
    Audio_Command_Type type;
    float value_f;
    int64_t value_i64;
    int64_t loop_start_ms;
    int64_t loop_end_ms;
    Audio_File value_file;
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

template <typename Type>
Type string_to(juce::String);

template <typename Type>
std::vector<Type> deserialize_vector(juce::String str)
{
    juce::StringArray tokens = juce::StringArray::fromTokens(str, false);
           
    std::vector<Type> values{};
    for (const auto& token : tokens)
    {
        values.emplace_back(string_to<Type>(token));
    }
    return values;
}

template <typename Type>
juce::String serialize_vector(std::vector<Type> values)
{
    juce::String str{};
    for (const auto value : values)
    {
        str += juce::String(value) + " ";
    }
    str = str.dropLastCharacters(1);
    return str;
}

#endif //SHARED_H
