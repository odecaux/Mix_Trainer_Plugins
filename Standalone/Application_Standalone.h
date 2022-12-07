
//------------------------------------------------------------------------
struct FilePlayer {
    FilePlayer(juce::AudioFormatManager &formatManager);

    ~FilePlayer();

    bool load_file_into_transport (const juce::File& audio_file);
    Return_Value post_command(Audio_Command command);
    void push_new_dsp_state(Channel_DSP_State new_dsp_state);

    juce::AudioFormatManager &format_manager;
    Transport_State transport_state;
    
    juce::AudioDeviceManager device_manager;
    juce::TimeSliceThread read_ahead_thread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer source_player;
    juce::AudioTransportSource transport_source;
    std::unique_ptr<juce::AudioFormatReaderSource> current_reader_source;
    Channel_DSP_Callback dsp_callback;
};

class Application_Standalone
{
    public :
    
    Application_Standalone(juce::AudioFormatManager &formatManager, Main_Component *main_component);
    ~Application_Standalone();
    void to_main_menu();
    void to_file_selector();
    void to_game_config();
    void to_frequency_game();


    private :
    FilePlayer player;
    std::unique_ptr<FrequencyGame_IO> game_io;
    std::unique_ptr<FrequencyGame_State> game_state;
    Main_Component *main_component;
    FrequencyGame_UI *game_ui;
    

    std::vector<Audio_File> files;
    std::vector<FrequencyGame_Config> game_configs = {};
    std::vector<FrequencyGame_Results> game_results_history = {};
    int current_config_idx = 0;
};