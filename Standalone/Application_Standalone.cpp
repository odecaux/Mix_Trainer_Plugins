static const juce::Identifier id_config_root = "configs";
static const juce::Identifier id_config = "config";
static const juce::Identifier id_config_title = "title";
static const juce::Identifier id_config_gain = "gain";
static const juce::Identifier id_config_quality = "q";
static const juce::Identifier id_config_window = "window";
static const juce::Identifier id_config_question_timeout_enabled = "question_timeout_enabled";
static const juce::Identifier id_config_question_timeout_ms = "question_timeout_ms";
static const juce::Identifier id_config_result_timeout_enabled = "result_timeout_enabled";
static const juce::Identifier id_config_result_timeout_ms = "result_timeout_ms";

static const juce::Identifier id_results_root = "results_history";
static const juce::Identifier id_result = "result";
static const juce::Identifier id_result_score = "score";
static const juce::Identifier id_result_timestamp = "timestamp";

Application_Standalone::Application_Standalone(juce::AudioFormatManager &formatManager, Main_Component *main_component)
:   player(formatManager), main_component(main_component)
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
        while (! stream->isExhausted()) // [3]
        {
            auto line = stream->readNextLine();
            auto audio_file_path = juce::File(line);
            if (!audio_file_path.existsAsFile())
            {
                DBG(line << " does not exist");
                continue;
            }
            auto * reader = formatManager.createReaderFor(audio_file_path);
            if (!reader)
            {
                DBG("invalid audio file " << line);
                continue;
            }
            
            Audio_File new_audio_file = {
                .file = audio_file_path,
                .title = audio_file_path.getFileNameWithoutExtension(),
                .loop_bounds = { 0, reader->lengthInSamples }
            };
            files.emplace_back(std::move(new_audio_file));
            delete reader;
        }
    }();

    //load config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("configs.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/configs.xml");
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
                .question_timeout_enabled = node.getProperty(id_config_question_timeout_enabled, false),
                .question_timeout_ms = node.getProperty(id_config_question_timeout_ms, -1),
                .result_timeout_enabled = node.getProperty(id_config_result_timeout_enabled, false),
                .result_timeout_ms = node.getProperty(id_config_result_timeout_ms, -1),
            };
            game_configs.push_back(config);
        }
    }();
    
    if (game_configs.empty())
    {
        game_configs = { 
            frequency_game_config_default("Default")
        };
    }
    jassert(!game_configs.empty());

    
    //load previous results
    [&] {
        auto file_RENAME = store_directory.getChildFile("results.xml");
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/results.xml");
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
            game_results_history.push_back(result);
        }
    }();

    if (game_configs.empty())
    {
        game_configs = { frequency_game_config_default("Default") };
    }

    toMainMenu();
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
        for (const Audio_File &audio_file : files)
        {
            *stream << audio_file.file.getFullPathName() << juce::newLine;
        }
    }();

    //save config list
    [&] {
        auto file_RENAME = store_directory.getChildFile("configs.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/configs.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_config_root };
        for (const FrequencyGame_Config& config : game_configs)
        {
            juce::ValueTree node = { id_config, {
                { id_config_title,  config.title },
                { id_config_gain, config.eq_gain },
                { id_config_quality, config.eq_quality },
                { id_config_window, config.initial_correct_answer_window },
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
        auto file_RENAME = store_directory.getChildFile("results.xml");
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/results.xml");
            return;
        }
        stream->setPosition(0);
        stream->truncate();
        juce::ValueTree root_node { id_results_root };
        for (const FrequencyGame_Results& result : game_results_history)
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

void Application_Standalone::toMainMenu()
{
    game_state.reset();
    auto main_menu_panel = std::make_unique < MainMenu_Panel > (
        [this] { toGameConfig(); },
        [this] { toFileSelector(); },
        [this] {}
    );
    if (files.empty())
    {
        main_menu_panel->game_button.setEnabled(false);
        main_menu_panel->game_button.setTooltip("Please add audio files");
    }
    main_component->changePanel(std::move(main_menu_panel));
}

void Application_Standalone::toFileSelector()
{
    assert(game_state == nullptr);
    auto file_selector_panel = std::make_unique < FileSelector_Panel > (player, files, [&] { toMainMenu(); } );
    main_component->changePanel(std::move(file_selector_panel));
}

void Application_Standalone::toGameConfig()
{
    assert(game_state == nullptr);
    auto config_panel = std::make_unique < Config_Panel > (game_configs, current_config_idx, [&] { toMainMenu(); }, [&] { toGame(); });
    main_component->changePanel(std::move(config_panel));
}

void Application_Standalone::toGame()
{
    game_state = frequency_game_state_init(game_configs[current_config_idx], files);
    if(game_state == nullptr)
        return;
    main_component->changePanel(nullptr);

    auto observer = [this] (const Effects &effects) { 
        if (effects.transition)
        {
            if (effects.transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < FrequencyGame_UI > (game_state.get(), game_io.get());
                game_ui = new_game_ui.get();
                main_component->changePanel(std::move(new_game_ui));
            }
            if(game_ui)
                frequency_game_ui_transitions(*game_ui, *effects.transition);
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
            game_results_history.push_back(*effects.results);
        }
        if (effects.ui)
        {
            assert(game_ui);
            frequency_game_ui_update(*game_ui, *effects.ui);
        }
    };

    auto debug_observer = [this] (const Effects &effects) {
        juce::ignoreUnused(effects);
    };
    
    game_io = frequency_game_io_init();

    auto on_quit = [this] { 
        game_io->timer.stopTimer();
        player.post_command( { .type = Audio_Command_Stop });
        toMainMenu();
    };
    game_io->on_quit = std::move(on_quit);

    frequency_game_add_observer(game_io.get(), std::move(observer));
    frequency_game_add_observer(game_io.get(), std::move(debug_observer));

    frequency_game_post_event(game_state.get(), game_io.get(), Event { .type = Event_Init });
    frequency_game_post_event(game_state.get(), game_io.get(), Event { .type = Event_Create_UI });
    game_io->timer.callback = [state = game_state.get(), io = game_io.get()] (juce::int64 timestamp) {
        frequency_game_post_event(state, io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
    };
    game_io->timer.startTimerHz(60);
}





//------------------------------------------------------------------------
FilePlayer::FilePlayer(juce::AudioFormatManager &formatManager)
:   formatManager(formatManager),
    dsp_callback(&transportSource)
{
    // audio setup
    readAheadThread.startThread ();

    audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
    
    dsp_callback.push_new_dsp_state(ChannelDSP_on());
    audioSourcePlayer.setSource (&dsp_callback);
    audioDeviceManager.addAudioCallback (&audioSourcePlayer);
}

FilePlayer::~FilePlayer()
{
    transportSource  .setSource (nullptr);
    audioSourcePlayer.setSource (nullptr);

    audioDeviceManager.removeAudioCallback (&audioSourcePlayer);
}

bool FilePlayer::loadFileIntoTransport (const juce::File& audio_file)
{
    // unload the previous file source and delete it..
    transportSource.stop();
    transportSource.setSource (nullptr);
    currentAudioFileSource.reset();

    const auto source = makeInputSource (audio_file);

    if (source == nullptr)
        return false;

    auto stream = juce::rawToUniquePtr (source->createInputStream());

    if (stream == nullptr)
        return false;

    auto reader = juce::rawToUniquePtr (formatManager.createReaderFor (std::move (stream)));

    if (reader == nullptr)
        return false;

    currentAudioFileSource = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);

    // ..and plug it into our transport source
    transportSource.setSource (currentAudioFileSource.get(),
                            32768,                   // tells it to buffer this many samples ahead
                            &readAheadThread,                 // this is the background thread to use for reading-ahead
                            currentAudioFileSource->getAudioFormatReader()->sampleRate);     // allows for sample rate correction
    transportSource.setLooping(true);
    return true;
}

Return_Value FilePlayer::post_command(Audio_Command command)
{
    switch (command.type)
    {
        case Audio_Command_Play :
        {
            DBG("Play");
            transportSource.start();
            transport_state.step = Transport_Playing;
        } break;
        case Audio_Command_Pause :
        {
            DBG("Pause");
            transportSource.stop();
            transport_state.step = Transport_Paused;
        } break;
        case Audio_Command_Stop :
        {
            DBG("Stop");
            transportSource.stop();
            transportSource.setPosition(0);
            transport_state.step = Transport_Stopped;
        } break;
        case Audio_Command_Seek :
        {
            DBG("Seek : "<<command.value_f);
            transportSource.setPosition(command.value_f);
        } break;
        case Audio_Command_Load :
        {
            DBG("Load : "<<command.value_file.getFileName());
            bool success = loadFileIntoTransport(command.value_file);
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
