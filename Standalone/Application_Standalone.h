#include <atomic>

struct Audio_File_List 
{   
    std::unordered_map<int64_t, Audio_File> files;
    std::unordered_map<int64_t, bool> selected;
    std::vector<int64_t> order;
};


bool insert_file(Audio_File_List *audio_file_list, juce::File file, juce::AudioFormatManager *format_manager);
void remove_files(Audio_File_List *audio_file_list, std::vector<int> indices);
std::vector<Audio_File> generate_list_of_selected_files(Audio_File_List *audio_file_list);
std::vector<Audio_File> generate_ordered_list_of_files(Audio_File_List *audio_file_list);
std::vector<std::string> generate_titles(Audio_File_List *audio_file_list);

std::string audio_file_list_serialize(Audio_File_List *audio_file_list);
std::vector<Audio_File> audio_file_list_deserialize(std::string xml_string);

struct File_Player_State
{
    Transport_Step step;
    int64_t playing_file_hash;
    int64_t file_length_ms;
    int64_t loop_start_ms;
    int64_t loop_end_ms;
};

class Looping_Transport_Source : public juce::AudioTransportSource
{
public:
    void setLoopBounds(int64_t loop_start_ms, int64_t loop_end_ms)
    {
        temp_start = loop_start_ms;
        temp_end = loop_end_ms;
        write_flag = true;
    }

    
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        playback_sample_rate = newSampleRate;
        juce::AudioTransportSource::prepareToPlay(samplesPerBlockExpected, newSampleRate);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& buffer) override
    {
        bool expected = true;
        if (write_flag.compare_exchange_strong(expected, false))
        {
            internal_start = temp_start;
            internal_end = temp_end;
        }

        if (!isPlaying())
        {
            juce::AudioTransportSource::getNextAudioBlock(buffer);
            return;
        }
        int64_t length_ms = (int64_t)(getLengthInSeconds() * 1000.0);
        assert(internal_start >= 0);
        assert(internal_end >= 0);
        assert(internal_end <= length_ms);
        assert(internal_start <= internal_end);

        int64_t current_position_ms = (int64_t)(getCurrentPosition() * 1000.0);
        
        if (current_position_ms > internal_end)
        {
            
            setPosition(double(internal_start) / 1000.0);
            current_position_ms = internal_start;
        }
        
        if (current_position_ms + ((double)buffer.numSamples * 1000.0 / playback_sample_rate) <= internal_end)
        {
            juce::AudioTransportSource::getNextAudioBlock(buffer);
            return;
        }

        int64_t end_of_loop_samples = (int64_t)((internal_end - current_position_ms) * playback_sample_rate / 1000.0);
        auto temp_buffer = buffer;
        temp_buffer.numSamples = checked_cast<int>(end_of_loop_samples);
        juce::AudioTransportSource::getNextAudioBlock(temp_buffer);

        setPosition((double)internal_start / 1000.0);
            
        int64_t beginning_of_loop_samples = buffer.numSamples - end_of_loop_samples;
        temp_buffer.startSample = buffer.startSample + checked_cast<int>(beginning_of_loop_samples);
        temp_buffer.numSamples = buffer.numSamples - checked_cast<int>(beginning_of_loop_samples);
        juce::AudioTransportSource::getNextAudioBlock(temp_buffer);
    }
private:

    int64_t internal_start = -1;
    int64_t internal_end = -1;

    int64_t temp_start;
    int64_t temp_end;
    double playback_sample_rate;
    std::atomic<bool> write_flag { false } ;
};

//------------------------------------------------------------------------
struct File_Player : juce::ChangeListener 
{
    File_Player(juce::AudioFormatManager *formatManager);

    ~File_Player();

    juce::AudioFormatManager *format_manager;
    File_Player_State player_state;
    
    juce::AudioDeviceManager device_manager;
    juce::String output_device_name;
    juce::TimeSliceThread read_ahead_thread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer source_player;
    Looping_Transport_Source transport_source;
    std::unique_ptr<juce::AudioFormatReaderSource> current_reader_source;
    Channel_DSP_Callback dsp_callback;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
};


bool file_player_load(File_Player *player, Audio_File *audio_file, int64_t *out_num_samples);
File_Player_State file_player_post_command(File_Player *player, Audio_Command command);
void file_player_push_dsp(File_Player *player, Channel_DSP_State new_dsp_state);
File_Player_State file_player_query_state(File_Player *player);

class Main_Component;

class Application_Standalone
{
    public :
    
    Application_Standalone(juce::AudioFormatManager* formatManager, Main_Component *main_component);
    ~Application_Standalone();

    void to_main_menu();
    void to_audio_file_settings();
    void to_freq_game_settings();
    void to_frequency_game();
    void to_low_end_frequency_game();
    void to_comp_game_settings();
    void to_compressor_game();

    private :
    File_Player player;
    Audio_File_List audio_file_list;
    Main_Component *main_component;
    
    std::unique_ptr<FrequencyGame_IO> frequency_game_io;
    std::vector<FrequencyGame_Config> frequency_game_configs = {};
    size_t current_frequency_game_config_idx = 0;
    std::vector<FrequencyGame_Results> frequency_game_results_history = {};
    FrequencyGame_UI *frequency_game_ui;

    std::unique_ptr<CompressorGame_IO> compressor_game_io;
    std::vector<CompressorGame_Config> compressor_game_configs = {};
    size_t current_compressor_game_config_idx = 0;
    std::vector<CompressorGame_Results> compressor_game_results_history = {};
    CompressorGame_UI *compressor_game_ui;
};