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
std::pair<std::vector<std::string>, std::vector<bool>> generate_titles_and_selection_lists(Audio_File_List *audio_file_list);

std::string audio_file_list_serialize(Audio_File_List *audio_file_list);
std::vector<Audio_File> audio_file_list_deserialize(std::string xml_string);

struct File_Player_State
{
    Transport_Step step;
    int64_t playing_file_hash;
    int64_t num_samples;
    int64_t start_sample;
    int64_t end_sample;
};

class Looping_Transport_Source : public juce::AudioTransportSource
{
public:
    void setLoopBounds(int64_t start_sample, int64_t end_sample)
    {
        temp_start = start_sample;
        temp_end = end_sample;
        write_flag = true;
    }
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& buffer) override
    {
        bool expected = true;
        if (write_flag.compare_exchange_strong(expected, false))
        {
            start_sample_position = temp_start;
            end_sample_position = temp_end;
        }

        if (!isPlaying())
        {
            juce::AudioTransportSource::getNextAudioBlock(buffer);
            return;
        }
        assert(start_sample_position >= 0);
        assert(end_sample_position >= 0);
        assert(end_sample_position <= getTotalLength());
        assert(end_sample_position <= getTotalLength());
        assert(start_sample_position <= end_sample_position);

        int64_t current_position = getNextReadPosition();

        if (current_position > end_sample_position)
        {
            setNextReadPosition(start_sample_position);
            current_position = start_sample_position;
        }
            
        if (current_position + buffer.numSamples <= end_sample_position)
        {
            juce::AudioTransportSource::getNextAudioBlock(buffer);
            return;
        }
        int64_t pre_loop_samples = end_sample_position - current_position;
        auto pre_loop_buffer = buffer;
        pre_loop_buffer.numSamples = checked_cast<int>(pre_loop_samples);
        juce::AudioTransportSource::getNextAudioBlock(pre_loop_buffer);

        setNextReadPosition(start_sample_position);
            
        int64_t post_loop_samples = buffer.numSamples - pre_loop_samples;
        auto post_loop_buffer = buffer;
        post_loop_buffer.startSample = buffer.startSample + checked_cast<int>(pre_loop_samples);
        post_loop_buffer.numSamples = buffer.numSamples - checked_cast<int>(pre_loop_samples);
        juce::AudioTransportSource::getNextAudioBlock(post_loop_buffer);
    }
private:

    int64_t start_sample_position = -1;
    int64_t end_sample_position = -1;

    int64_t temp_start;
    int64_t temp_end;
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