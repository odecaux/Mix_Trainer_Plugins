struct Audio_File_List 
{   
    juce::AudioFormatManager &format_manager;
    std::vector<Audio_File> files = {};
    std::vector<bool> selected;
};


bool insert_file(Audio_File_List &audio_file_list, juce::File file, juce::AudioFormatManager &format_manager);
void remove_files(Audio_File_List &audio_file_list, const juce::SparseSet < int > & indices);
std::vector<Audio_File> get_selected_list(Audio_File_List &audio_file_list);

juce::String audio_file_list_serialize(const std::vector<Audio_File> &audio_file_list);
std::vector<Audio_File> audio_file_list_deserialize(juce::String xml_string);

//------------------------------------------------------------------------
struct FilePlayer : juce::ChangeListener 
{
    FilePlayer(juce::AudioFormatManager &formatManager);

    ~FilePlayer();

    bool load_file_into_transport (const Audio_File& audio_file);
    Return_Value post_command(Audio_Command command);
    void push_new_dsp_state(Channel_DSP_State new_dsp_state);

    juce::AudioFormatManager &format_manager;
    Transport_State transport_state;
    
    juce::AudioDeviceManager device_manager;
    juce::String output_device_name;
    juce::TimeSliceThread read_ahead_thread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer source_player;
    juce::AudioTransportSource transport_source;
    std::unique_ptr<juce::AudioFormatReaderSource> current_reader_source;
    Channel_DSP_Callback dsp_callback;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
};

class Application_Standalone
{
    public :
    
    Application_Standalone(juce::AudioFormatManager &formatManager, Main_Component *main_component);
    ~Application_Standalone();
    void to_main_menu();

    void to_audio_file_settings();
    void to_freq_game_settings();
    void to_frequency_game();
    void to_low_end_frequency_game();
    void to_comp_game_settings();
    void to_compressor_game();

    private :
    FilePlayer player;
    Audio_File_List audio_file_list;
    std::unique_ptr<FrequencyGame_IO> frequency_game_io;
    std::unique_ptr<CompressorGame_IO> compressor_game_io;
    Main_Component *main_component;
    FrequencyGame_UI *frequency_game_ui;
    CompressorGame_UI *compressor_game_ui;
    
    std::vector<FrequencyGame_Config> frequency_game_configs = {};
    size_t current_frequency_game_config_idx = 0;
    std::vector<FrequencyGame_Results> frequency_game_results_history = {};
    std::vector<CompressorGame_Config> compressor_game_configs = {};
    size_t current_compressor_game_config_idx = 0;
    std::vector<CompressorGame_Results> compressor_game_results_history = {};
};