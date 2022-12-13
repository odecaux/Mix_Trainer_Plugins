static const juce::Identifier id_config_root = "configs";
static const juce::Identifier id_config = "config";
static const juce::Identifier id_config_title = "title";
static const juce::Identifier id_config_gain = "gain";
static const juce::Identifier id_config_quality = "q";
static const juce::Identifier id_config_window = "window";

static const juce::Identifier id_config_prelisten_type = "prelisten_type";
static const juce::Identifier id_config_prelisten_timeout_ms = "prelisten_timeout_ms";

static const juce::Identifier id_config_question_timeout_enabled = "question_timeout_enabled";
static const juce::Identifier id_config_question_timeout_ms = "question_timeout_ms";

static const juce::Identifier id_config_result_timeout_enabled = "result_timeout_enabled";
static const juce::Identifier id_config_result_timeout_ms = "result_timeout_ms";

static const juce::Identifier id_results_root = "results_history";
static const juce::Identifier id_result = "result";
static const juce::Identifier id_result_score = "score";
static const juce::Identifier id_result_timestamp = "timestamp";

Application_Standalone::Application_Standalone(juce::AudioFormatManager &formatManager, Main_Component *mainComponent)
:   player(formatManager),
    audio_file_list { formatManager },
    main_component(mainComponent)
{
    juce::File app_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    DBG(app_data.getFullPathName());
    auto store_directory = app_data.getChildFile("MixTrainer");

    //load audio file list
    [&] {
        auto file_RENAME = store_directory.getChildFile("audio_file_list.txt");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/audio_file_list.txt");
            return;
        }
        //TODO performance : this loop is O^2/2
        while (! stream->isExhausted())
        {
            auto line = stream->readNextLine();
            auto audio_file_path = juce::File(line);
            audio_file_list.insert_file(audio_file_path);
        }
    }();

    //load config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("frequency_game_configs.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/frequency_game_configs.xml");
            return;
        }
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_config_root)
            return;
        for (int i = 0; i < root_node.getNumChildren(); i++)
        {
            juce::ValueTree node = root_node.getChild(i);
            if(node.getType() != id_config)
                continue;
            FrequencyGame_Config config = {
                .title = node.getProperty(id_config_title, ""),
                .eq_gain = node.getProperty(id_config_gain, -1.0f),
                .eq_quality = node.getProperty(id_config_quality, -1.0f),
                .initial_correct_answer_window = node.getProperty(id_config_window, -1.0f),

                .prelisten_type = (PreListen_Type)(int)node.getProperty(id_config_prelisten_type, (int)PreListen_None),
                .prelisten_timeout_ms = node.getProperty(id_config_prelisten_timeout_ms, -1),

                .question_timeout_enabled = node.getProperty(id_config_question_timeout_enabled, false),
                .question_timeout_ms = node.getProperty(id_config_question_timeout_ms, -1),

                .result_timeout_enabled = node.getProperty(id_config_result_timeout_enabled, false),
                .result_timeout_ms = node.getProperty(id_config_result_timeout_ms, -1),
            };
            frequency_game_configs.push_back(config);
        }
    }();
    
    if (frequency_game_configs.empty())
    {
        frequency_game_configs = { 
            frequency_game_config_default("Default")
        };
    }
    jassert(!frequency_game_configs.empty());

    
    //load previous results
    [&] {
        auto file_RENAME = store_directory.getChildFile("frequency_game_results.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/frequency_game_results.xml");
            return;
        }
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_results_root)
            return;
        for (int i = 0; i < root_node.getNumChildren(); i++)
        {
            juce::ValueTree node = root_node.getChild(i);
            if(node.getType() != id_result)
                continue;
            FrequencyGame_Results result = {
                .score = node.getProperty(id_result_score, "")
            };
            frequency_game_results_history.push_back(result);
        }
    }();

    
    //load config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("compressor_game_configs.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/compressor_game_configs.xml");
            return;
        }
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_config_root)
            return;
        for (int i = 0; i < root_node.getNumChildren(); i++)
        {
            juce::ValueTree node = root_node.getChild(i);
            if(node.getType() != id_config)
                continue;
            CompressorGame_Config config = {
            };
            compressor_game_configs.push_back(config);
        }
    }();
    
    if (compressor_game_configs.empty())
    {
        compressor_game_configs = { 
            compressor_game_config_default("Default")
        };
    }
    jassert(!compressor_game_configs.empty());

    
    //load previous results
    [&] {
        auto file_RENAME = store_directory.getChildFile("compressor_game_results.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/compressor_game_results.xml");
            return;
        }
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_results_root)
            return;
        for (int i = 0; i < root_node.getNumChildren(); i++)
        {
            juce::ValueTree node = root_node.getChild(i);
            if(node.getType() != id_result)
                continue;
            CompressorGame_Results result = {
                .score = node.getProperty(id_result_score, "")
            };
            compressor_game_results_history.push_back(result);
        }
    }();

    if (frequency_game_configs.empty())
    {
        frequency_game_configs = { frequency_game_config_default("Default") };
    }
    if (compressor_game_configs.empty())
    {
        compressor_game_configs = { compressor_game_config_default("Default") };
    }

    to_main_menu();
}

Application_Standalone::~Application_Standalone()
{
    juce::File app_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    DBG(app_data.getFullPathName());
    assert(app_data.exists() && app_data.isDirectory());
    auto store_directory = app_data.getChildFile("MixTrainer");
    //save audio file list
    [&] {
        auto file_RENAME = store_directory.getChildFile("audio_file_list.txt");;
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/audio_file_list.txt");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        for (const Audio_File &audio_file : audio_file_list.files)
        {
            *stream << audio_file.file.getFullPathName() << juce::newLine;
        }
    }();

    //save config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("frequency_game_configs.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/frequency_game_configs.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_config_root };
        for (const FrequencyGame_Config& config : frequency_game_configs)
        {
            juce::ValueTree node = { id_config, {
                { id_config_title,  config.title },
                { id_config_gain, config.eq_gain },
                { id_config_quality, config.eq_quality },
                { id_config_window, config.initial_correct_answer_window },

                { id_config_prelisten_type, config.prelisten_type },
                { id_config_prelisten_timeout_ms, config.prelisten_timeout_ms },

                { id_config_question_timeout_enabled, config.question_timeout_enabled },
                { id_config_question_timeout_ms, config.question_timeout_ms },

                { id_config_result_timeout_enabled, config.result_timeout_enabled },
                { id_config_result_timeout_ms, config.result_timeout_ms },
            }};
            root_node.addChild(node, -1, nullptr);
        }
        auto xml_string = root_node.toXmlString();
        *stream << xml_string;
    }();

    
    //save previous results
    [&] {
        auto file_RENAME = store_directory.getChildFile("frequency_game_results.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/frequency_game_results.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_results_root };
        for (const FrequencyGame_Results& result : frequency_game_results_history)
        {
            juce::ValueTree node = { id_result, {
                { id_result_score,  result.score }
            }};
            root_node.addChild(node, -1, nullptr);
        }
        auto xml_string = root_node.toXmlString();
        *stream << xml_string;
    }();


    
    //save config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("compressor_game_configs.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/compressor_game_configs.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_config_root };
        for (const CompressorGame_Config& config : compressor_game_configs)
        {
            juce::ValueTree node = { id_config, {
                { id_config_title,  config.title },
#if 0
                { id_config_gain, config.eq_gain },
                { id_config_quality, config.eq_quality },
                { id_config_window, config.initial_correct_answer_window },
                { id_config_prelisten_type, config.prelisten_type },
                { id_config_prelisten_timeout_ms, config.prelisten_timeout_ms },

                { id_config_question_timeout_enabled, config.question_timeout_enabled },
                { id_config_question_timeout_ms, config.question_timeout_ms },

                { id_config_result_timeout_enabled, config.result_timeout_enabled },
                { id_config_result_timeout_ms, config.result_timeout_ms },
#endif
            }};
            root_node.addChild(node, -1, nullptr);
        }
        auto xml_string = root_node.toXmlString();
        *stream << xml_string;
    }();

    
    //save previous results
    [&] {
        auto file_RENAME = store_directory.getChildFile("compressor_game_results.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/compressor_game_results.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_results_root };
        for (const CompressorGame_Results& result : compressor_game_results_history)
        {
            juce::ValueTree node = { id_result, {
                { id_result_score,  result.score }
            }};
            root_node.addChild(node, -1, nullptr);
        }
        auto xml_string = root_node.toXmlString();
        *stream << xml_string;
    }();
}

void Application_Standalone::to_main_menu()
{
    frequency_game_io.reset();
    compressor_game_io.reset();
    auto main_menu_panel = std::make_unique < MainMenu_Panel > (
        [this] { to_game_config(); },
        [this] { to_compressor_game(); },
        [this] { to_file_selector(); },
        [] {}
    );
    if (audio_file_list.files.empty())
    {
        main_menu_panel->frequency_game_button.setEnabled(false);
        main_menu_panel->frequency_game_button.setTooltip("Please add audio files");
        main_menu_panel->compressor_game_button.setEnabled(false);
        main_menu_panel->compressor_game_button.setTooltip("Please add audio files");
    }
    main_component->changePanel(std::move(main_menu_panel));
}

void Application_Standalone::to_file_selector()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    auto file_selector_panel = std::make_unique < FileSelector_Panel > (player, audio_file_list, [&] { to_main_menu(); } );
    main_component->changePanel(std::move(file_selector_panel));
}

void Application_Standalone::to_game_config()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    auto config_panel = std::make_unique < Config_Panel > (frequency_game_configs, current_config_idx, [&] { to_main_menu(); }, [&] { to_frequency_game(); });
    main_component->changePanel(std::move(config_panel));
}

void Application_Standalone::to_frequency_game()
{
    main_component->changePanel(nullptr);

    auto observer = [this] (const Frequency_Game_Effects &effects) { 
        if (effects.transition)
        {
            if (effects.transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < FrequencyGame_UI > (frequency_game_io.get());
                frequency_game_ui = new_game_ui.get();
                main_component->changePanel(std::move(new_game_ui));
            }
            if(frequency_game_ui)
                frequency_game_ui_transitions(*frequency_game_ui, *effects.transition);
        }
        if (effects.dsp)
        {
            player.push_new_dsp_state(effects.dsp->dsp_state);
        }
        if (effects.player)
        {
            for (const auto& command : effects.player->commands)
                player.post_command(command);
        }
        if (effects.results)
        {
            frequency_game_results_history.push_back(*effects.results);
        }
        if (effects.ui)
        {
            assert(frequency_game_ui);
            frequency_game_ui_update(*frequency_game_ui, *effects.ui);
        }
    };

    auto debug_observer = [] (const Frequency_Game_Effects &effects) {
        juce::ignoreUnused(effects);
    };
    
    auto new_game_state = frequency_game_state_init(frequency_game_configs[current_config_idx], audio_file_list.files);
    frequency_game_io = frequency_game_io_init(new_game_state);

    auto on_quit = [this] { 
        frequency_game_io->timer.stopTimer();
        player.post_command( { .type = Audio_Command_Stop });
        to_main_menu();
    };
    frequency_game_io->on_quit = std::move(on_quit);

    frequency_game_add_observer(frequency_game_io.get(), std::move(observer));
    frequency_game_add_observer(frequency_game_io.get(), std::move(debug_observer));

    frequency_game_post_event(frequency_game_io.get(), Event { .type = Event_Init });
    frequency_game_post_event(frequency_game_io.get(), Event { .type = Event_Create_UI });
    frequency_game_io->timer.callback = [io = frequency_game_io.get()] (juce::int64 timestamp) {
        frequency_game_post_event(io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
    };
    frequency_game_io->timer.startTimerHz(60);
}


void Application_Standalone::to_low_end_frequency_game()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    //main_component->changePanel(std::move(config_panel));
}

void Application_Standalone::to_compressor_game()
{
    main_component->changePanel(nullptr);

    auto observer = [this] (const Compressor_Game_Effects &effects) { 
        if (effects.transition)
        {
            if (effects.transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < CompressorGame_UI > (compressor_game_io.get());
                compressor_game_ui = new_game_ui.get();
                main_component->changePanel(std::move(new_game_ui));
            }
            if(compressor_game_ui)
                compressor_game_ui_transitions(*compressor_game_ui, *effects.transition);
        }
        if (effects.dsp)
        {
            player.push_new_dsp_state(effects.dsp->dsp_state);
        }
        if (effects.player)
        {
            for (const auto& command : effects.player->commands)
                player.post_command(command);
        }
        if (effects.results)
        {
            compressor_game_results_history.push_back(*effects.results);
        }
        if (effects.ui)
        {
            assert(compressor_game_ui);
            compressor_game_ui_update(*compressor_game_ui, *effects.ui);
        }
    };

    auto debug_observer = [] (const Compressor_Game_Effects &effects) {
        juce::ignoreUnused(effects);
    };
    
    auto new_game_state = compressor_game_state_init(compressor_game_config_default("test"), audio_file_list.files);
    compressor_game_io = compressor_game_io_init(new_game_state);

    auto on_quit = [this] { 
        compressor_game_io->timer.stopTimer();
        player.post_command( { .type = Audio_Command_Stop });
        to_main_menu();
    };
    compressor_game_io->on_quit = std::move(on_quit);

    compressor_game_add_observer(compressor_game_io.get(), std::move(observer));
    compressor_game_add_observer(compressor_game_io.get(), std::move(debug_observer));

    compressor_game_post_event(compressor_game_io.get(), Event { .type = Event_Init });
    compressor_game_post_event(compressor_game_io.get(), Event { .type = Event_Create_UI });
    compressor_game_io->timer.callback = [io = compressor_game_io.get()] (juce::int64 timestamp) {
        compressor_game_post_event(io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
    };
    compressor_game_io->timer.startTimerHz(60);
}

//------------------------------------------------------------------------
FilePlayer::FilePlayer(juce::AudioFormatManager &formatManager)
:   format_manager(formatManager),
    dsp_callback(&transport_source)
{
    // audio setup
    read_ahead_thread.startThread ();
    
    dsp_callback.push_new_dsp_state(ChannelDSP_on());
    source_player.setSource (&dsp_callback);

    auto result = device_manager.initialiseWithDefaultDevices(0, 2);
    if (result.isEmpty())
    {
        device_manager.addAudioCallback (&source_player);
        output_device_name = device_manager.getAudioDeviceSetup().outputDeviceName;
    }
    device_manager.addChangeListener(this);
}

FilePlayer::~FilePlayer()
{
    transport_source  .setSource (nullptr);
    source_player.setSource (nullptr);

    device_manager.removeAudioCallback (&source_player);
}

bool FilePlayer::load_file_into_transport (const juce::File& audio_file)
{
    // unload the previous file source and delete it..
    transport_source.stop();
    transport_source.setSource (nullptr);
    current_reader_source.reset();

    const auto source = makeInputSource (audio_file);

    if (source == nullptr)
        return false;

    auto stream = juce::rawToUniquePtr (source->createInputStream());

    if (stream == nullptr)
        return false;

    auto reader = juce::rawToUniquePtr (format_manager.createReaderFor (std::move (stream)));

    if (reader == nullptr)
        return false;

    current_reader_source = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);
    current_reader_source->setLooping(true);

    // ..and plug it into our transport source
    transport_source.setSource (current_reader_source.get(),
                            32768,                   // tells it to buffer this many samples ahead
                            &read_ahead_thread,                 // this is the background thread to use for reading-ahead
                            current_reader_source->getAudioFormatReader()->sampleRate);     // allows for sample rate correction
    return true;
}

Return_Value FilePlayer::post_command(Audio_Command command)
{
    switch (command.type)
    {
        case Audio_Command_Play :
        {
            DBG("Play");
            transport_source.start();
            transport_state.step = Transport_Playing;
        } break;
        case Audio_Command_Pause :
        {
            DBG("Pause");
            transport_source.stop();
            transport_state.step = Transport_Paused;
        } break;
        case Audio_Command_Stop :
        {
            DBG("Stop");
            transport_source.stop();
            transport_source.setPosition(0);
            transport_state.step = Transport_Stopped;
        } break;
        case Audio_Command_Seek :
        {
            DBG("Seek : "<<command.value_f);
            transport_source.setPosition(command.value_f);
        } break;
        case Audio_Command_Load :
        {
            DBG("Load : "<<command.value_file.getFileName());
            bool success = load_file_into_transport(command.value_file);
            transport_state.step = Transport_Stopped;
            return { .value_b = success };
        } break;
    }
    return { .value_b = true };
}

void FilePlayer::push_new_dsp_state(Channel_DSP_State new_dsp_state)
{
    dsp_callback.push_new_dsp_state(new_dsp_state);
}

void FilePlayer::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if(source != &device_manager)
        return;

    auto *io_device_type = device_manager.getCurrentDeviceTypeObject();
    auto device_names = io_device_type->getDeviceNames();
    if(device_names.size() == 0)
        return;
    auto default_device_idx = io_device_type->getDefaultDeviceIndex(false);
    auto default_device_name = device_names[default_device_idx];
    if(default_device_name == output_device_name)
        return;
    device_manager.removeChangeListener(this);
    auto result = device_manager.initialiseWithDefaultDevices(0, 2);
    if (result.isEmpty())
    {
        device_manager.addAudioCallback (&source_player);
        output_device_name = device_manager.getAudioDeviceSetup().outputDeviceName;
    }
    device_manager.addChangeListener(this);
}