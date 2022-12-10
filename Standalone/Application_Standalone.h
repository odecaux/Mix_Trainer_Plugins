struct Audio_File_List 
{
    bool insert_file(juce::File file)
    {
        if (!file.existsAsFile())
        {
            DBG(file.getFullPathName() << " does not exist");
            return false;
        }
        //can't have the same file twice
        auto result = std::find_if(files.begin(), files.end(), [&] (const Audio_File &in) { return in.file == file; });
        if (result != files.end())
            return false;

        //expensive though
        auto * reader = format_manager.createReaderFor(file);
        if (reader == nullptr)
            return false;
         
        Audio_File new_audio_file = {
            .file = file,
            .title = file.getFileNameWithoutExtension(),
            .loop_bounds = { 0, reader->lengthInSamples }
        };
        files.emplace_back(std::move(new_audio_file));
        delete reader;
        return true;
    }

    void remove_files(const juce::SparseSet<int>& indices)
    {
        for (int i = static_cast<int>(files.size()); --i >= 0;)
        {   
            if(indices.contains(static_cast<int>(i)))
                files.erase(files.begin() + i);
        }
    }
    
    juce::AudioFormatManager &format_manager;
    std::vector<Audio_File> files;
};



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
    void to_low_end_frequency_game();
    void to_compressor_game();
    

    private :
    FilePlayer player;
    Audio_File_List audio_file_list;
    std::unique_ptr<FrequencyGame_IO> game_io;
    Main_Component *main_component;
    FrequencyGame_UI *frequency_game_ui;
    CompressorGame_UI *compressor_game_ui;
    
    std::vector<FrequencyGame_Config> game_configs = {};
    std::vector<FrequencyGame_Results> game_results_history = {};
    size_t current_config_idx = 0;
};