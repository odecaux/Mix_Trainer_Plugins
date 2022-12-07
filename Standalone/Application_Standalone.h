
//------------------------------------------------------------------------
struct FilePlayer {
    FilePlayer(juce::AudioFormatManager &formatManager);

    ~FilePlayer();

    bool loadFileIntoTransport (const juce::File& audio_file);

    Return_Value post_command(Audio_Command command);

    void push_new_dsp_state(Channel_DSP_State new_dsp_state);

    juce::AudioFormatManager &formatManager;
    Transport_State transport_state;
    
    juce::AudioDeviceManager audioDeviceManager;
    juce::TimeSliceThread readAheadThread  { "audio file preview" };
    
    //juce::URL currentAudioFile;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFileSource;
    Channel_DSP_Callback dsp_callback;
};

class Application_Standalone
{
    public :
    
    Application_Standalone(juce::AudioFormatManager &formatManager, Main_Component *main_component);
    ~Application_Standalone();
    void toMainMenu();
    void toFileSelector();
    void toGameConfig();
    void toGame();


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