#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../shared/shared_ui.h"
#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "../Game/Frequency_Game.h"
#include "../Game/Compressor_Game.h"
#include "../Game/Compressor_Game_UI.h"
#include "../Game/Frequency_Game_UI.h"
#include "Application_Standalone.h"
#include "Standalone_UI.h"

static Audio_File audio_file_scan_length_and_max(Audio_File audio_file, juce::AudioFormatManager *format_manager)
{
    //TODO sanity check ?
    assert(audio_file.file.existsAsFile());
     
    //expensive though
    auto * reader_ptr = format_manager->createReaderFor(audio_file.file);
    auto reader = std::unique_ptr<juce::AudioFormatReader>(reader_ptr);
    if (reader == nullptr)
        return audio_file;

    std::vector<juce::Range < float> > max_by_channel{};
        
    max_by_channel.resize(reader->numChannels);
    reader->readMaxLevels(0, reader->lengthInSamples, max_by_channel.data(), checked_cast<int>(reader->numChannels));
        
    float max_level = 0.0f;
    for (juce::Range < float > channel_range : max_by_channel)
    {
        DBG(audio_file.file.getFileNameWithoutExtension() << " : " << channel_range.getStart() << " to " << channel_range.getEnd());
        max_level = std::max(max_level, std::abs(channel_range.getStart()));
        max_level = std::max(max_level, std::abs(channel_range.getEnd()));
    }
    if (max_level <= 0.0F)
        return audio_file;

    audio_file.max_level = max_level;
    double sample_rate = reader->sampleRate;
    int64_t file_length_ms = (int64_t)(reader->lengthInSamples * 1000.0 / sample_rate);
    audio_file.loop_bounds_ms = { 0, file_length_ms};
    audio_file.file_length_ms = file_length_ms;
    return audio_file;
}

bool insert_file(Audio_File_List *audio_file_list, juce::File file, juce::AudioFormatManager *format_manager)
{
    if (!file.existsAsFile())
    {
        DBG(file.getFullPathName() << " does not exist");
        return false;
    }
    int64_t hash = file.hashCode64();
    //can't have the same file twice
    if (audio_file_list->files.contains(hash))
        return false;

    Audio_File new_audio_file = {
        .file = file,
        .last_modification_time = file.getLastModificationTime(),
        .title = file.getFileNameWithoutExtension().toStdString(),
        .freq_bounds = { 20, 20000 },
        .hash = hash
    };
    new_audio_file = audio_file_scan_length_and_max(new_audio_file, format_manager);

    audio_file_list->files.emplace(hash, std::move(new_audio_file));
    audio_file_list->selected.emplace(hash, false);
    audio_file_list->order.emplace_back(hash);
    //TODO assert sizes, assert emplaces
    return true;
}

void remove_files(Audio_File_List *audio_file_list, std::vector<int> indices)
{
    for (int i = checked_cast<int>(indices.size()) - 1; i >= 0; i--)
    {   
        int idx = indices[i];
        //TODO assert
        uint64_t hash = audio_file_list->order[idx];
        {
            size_t count = audio_file_list->files.erase(hash);
            assert(count == 1);
        }
        {
            size_t count =  audio_file_list->selected.erase(hash);
            assert(count == 1);
        }
        {
            audio_file_list->order.erase(audio_file_list->order.begin() + idx);
        }
    }
}

    
std::vector<Audio_File> generate_list_of_selected_files(Audio_File_List *audio_file_list)
{
    assert(audio_file_list->files.size() == audio_file_list->selected.size());
    assert(audio_file_list->files.size() == audio_file_list->order.size());
    std::vector<Audio_File> selected_files;
    for (uint64_t hash : audio_file_list->order)
    {
        if(audio_file_list->selected.at(hash))
            selected_files.push_back(audio_file_list->files.at(hash));
    }
    return selected_files;
}

#if 0
std::vector<Audio_File> generate_ordered_list_of_files(Audio_File_List *audio_file_list)
{
    std::vector<Audio_File> files{};
    files.reserve(audio_file_list->order.size());
    for (uint32_t i = 0; i < audio_file_list->order.size(); i++)
    {
        uint64_t hash = audio_file_list->order[i];
        files.push_back(audio_file_list->files.at(hash));
    }
    return files;
}
#endif  

std::vector<std::string> generate_titles(Audio_File_List *audio_file_list)
{
    std::vector<std::string> titles{};
    titles.reserve(audio_file_list->order.size());
    for (uint32_t i = 0; i < audio_file_list->order.size(); i++)
    {
        uint64_t hash = audio_file_list->order[i];
        titles.push_back(audio_file_list->files.at(hash).title);
    }
    return titles;
}

static const juce::Identifier id_results_root = "results_history";
static const juce::Identifier id_result = "result";
static const juce::Identifier id_result_score = "score";
static const juce::Identifier id_result_timestamp = "timestamp";

static const juce::Identifier id_files_root = "audio_files";
static const juce::Identifier id_file = "file";
static const juce::Identifier id_file_name = "filename";
static const juce::Identifier id_file_last_modification_time = "last_modification_time";
static const juce::Identifier id_file_title = "title";
static const juce::Identifier id_file_loop_bounds_ms = "loop_bounds_ms";
static const juce::Identifier id_file_freq_bounds = "freq_bounds";
static const juce::Identifier id_file_max_level = "max_level";
static const juce::Identifier id_file_length_ms = "length_ms";


std::string audio_file_list_serialize(Audio_File_List *audio_file_list)
{
    juce::ValueTree root_node { id_files_root };
    for (uint64_t hash : audio_file_list->order)
    {
        const auto& audio_file = audio_file_list->files.at(hash);
        std::vector<int64_t> loop_bounds { audio_file.loop_bounds_ms.getStart(), audio_file.loop_bounds_ms.getEnd() };
        std::vector<uint32_t> freq_bounds { audio_file.freq_bounds.getStart(), audio_file.freq_bounds.getEnd() };
        juce::ValueTree node = { id_file, {
            { id_file_name,  audio_file.file.getFullPathName() },
            { id_file_last_modification_time, audio_file.last_modification_time.toMilliseconds() },
            { id_file_title, juce::String(audio_file.title) },
            { id_file_loop_bounds_ms, serialize_vector(loop_bounds) },
            { id_file_freq_bounds, serialize_vector(freq_bounds) },
            { id_file_max_level, audio_file.max_level },
            { id_file_length_ms, juce::int64(audio_file.file_length_ms) }
        }};
        root_node.addChild(node, -1, nullptr);
    }
    return root_node.toXmlString().toStdString();
}

std::vector<Audio_File> audio_file_list_deserialize(std::string xml_string)
{
    std::vector<Audio_File> audio_files{};
    juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
    if (root_node.getType() != id_files_root)
        return {};
    
    for (uint32_t i = 0; i < checked_cast<uint32_t>(root_node.getNumChildren()); i++)
    {
        juce::ValueTree node = root_node.getChild(i);
        if(node.getType() != id_file)
            continue;

        juce::String file_name = node.getProperty(id_file_name);
        auto file = juce::File{ file_name };
        if(!file.existsAsFile())
            continue;
        auto loop_bounds_ms = deserialize_vector<int64_t>(node.getProperty(id_file_loop_bounds_ms, ""));
        if(loop_bounds_ms.size() != 2)
            continue;
        auto freq_bounds = deserialize_vector<uint32_t>(node.getProperty(id_file_freq_bounds, ""));
        if(freq_bounds.size() != 2)
            continue;
        int64_t modification_time = (juce::int64)node.getProperty(id_file_last_modification_time, 0);
        Audio_File audio_file = {
            .file = file,
            .last_modification_time = juce::Time(modification_time),
            .title = node.getProperty(id_file_title, "").toString().toStdString(),
            .loop_bounds_ms = { loop_bounds_ms[0], loop_bounds_ms[1] },
            .freq_bounds = { freq_bounds[0], freq_bounds[1] },
            .max_level = node.getProperty(id_file_max_level, 1.0f),
            .file_length_ms = (juce::int64)node.getProperty(id_file_length_ms, 0),
        };
        audio_files.push_back(audio_file);
    }
    return audio_files;
}

Application_Standalone::Application_Standalone(juce::AudioFormatManager *formatManager, Main_Component *mainComponent)
:   player(formatManager),
    main_component(mainComponent)
{

    main_component->main_fader.fader_moved_db_callback = [&] (double new_gain_db)
    {
        player.dsp_callback.push_master_volume_db(new_gain_db);
    };
    main_component->main_fader.update(-6.0);

    juce::File app_data = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    DBG(app_data.getFullPathName());
    auto store_directory = app_data.getChildFile("MixTrainer");

    auto get_file_from_appdata = [&] (juce::String file_name) -> std::unique_ptr<juce::FileInputStream>
    {
        auto file_RENAME = store_directory.getChildFile(file_name);
        if (!file_RENAME.existsAsFile())
        {
            file_RENAME.create();
            return nullptr;
        }
        auto stream = file_RENAME.createInputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/" << file_name);
            return nullptr;
        }
        return stream;
    };

    //load audio file list
    [&] {
        auto stream = get_file_from_appdata("audio_files.xml");
        if (!stream) return;
        juce::String xml_string = stream->readString();
        auto file_vec = audio_file_list_deserialize(xml_string.toStdString());
        audio_file_list.files.reserve(file_vec.size());
        audio_file_list.selected.reserve(file_vec.size());
        audio_file_list.order.reserve(file_vec.size());
        for (auto& audio_file : file_vec)
        {
            uint64_t hash = audio_file.file.hashCode64();
            audio_file.hash = hash; //HACK or is it ? It's a cached value, right ?
            audio_file_list.files.emplace(hash, audio_file);
            audio_file_list.selected.emplace(hash, false);
            audio_file_list.order.emplace_back(hash);
        }
        for (auto& [hash, audio_file] : audio_file_list.files)
        {
            if (audio_file.file.getLastModificationTime().toMilliseconds() > audio_file.last_modification_time.toMilliseconds())
            {
                audio_file = audio_file_scan_length_and_max(audio_file, formatManager);
            }
        }
    }();

    //load frequency config list
    [&] {
        auto stream = get_file_from_appdata("frequency_game_configs.xml");
        if (!stream) return;
        juce::String xml_string = stream->readString();
        frequency_game_configs = frequency_game_deserialize(xml_string.toStdString());
    }();
    
    //load previous results
    [&] {
        auto stream = get_file_from_appdata("frequency_game_results.xml");
        if (!stream) return;
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_results_root)
            return;
        for (uint32_t i = 0; i < checked_cast<uint32_t>(root_node.getNumChildren()); i++)
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
    
    //load compressor config list
    [&] {
        auto stream = get_file_from_appdata("compressor_game_configs.xml");
        if (!stream) return;
        juce::String xml_string = stream->readString();
        compressor_game_configs = compressor_game_deserialize(xml_string.toStdString());
    }();
    

    //load previous results
    [&] {
        auto stream = get_file_from_appdata("compressor_game_results.xml");
        if (!stream) return;
        juce::String xml_string = stream->readString();
        juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
        if(root_node.getType() != id_results_root)
            return;
        for (uint32_t i = 0; i < checked_cast<uint32_t>(root_node.getNumChildren()); i++)
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

    auto get_file_stream_from_appdata = [&] (juce::String file_name) -> std::unique_ptr<juce::FileOutputStream>
    {
        auto file_RENAME = store_directory.getChildFile(file_name);
        auto stream = file_RENAME.createOutputStream();
        if (!stream->openedOk())
        {
            DBG("couldn't open %appdata%/MixTrainer/" << file_name);
            return nullptr;
        }
        stream->setPosition(0);
        stream->truncate();
        return stream;
    };

    //save audio file list
    [&] {
        std::unique_ptr<juce::FileOutputStream> stream = get_file_stream_from_appdata("audio_files.xml");
        if (!stream) return;
        std::string file_text = audio_file_list_serialize(&audio_file_list);
        *stream << juce::StringRef(file_text);
    }();

    //save frequency config list
    [&] {
        auto stream = get_file_stream_from_appdata("frequency_game_configs.xml");
        std::string file_text = frequency_game_serlialize(&frequency_game_configs);
        *stream << juce::StringRef(file_text);
    }();

    
    //save frequency results
    [&] {
        auto stream = get_file_stream_from_appdata("frequency_game_results.xml");
        if (!stream) return;
        juce::ValueTree root_node { id_results_root };
        for (const FrequencyGame_Results& result : frequency_game_results_history)
        {
            juce::ValueTree node = { id_result, {
                { id_result_score,  result.score }
            } };
            root_node.addChild(node, -1, nullptr);
        }
        auto xml_string = root_node.toXmlString();
        *stream << xml_string;
    }();

    //save compressor config list
    [&] {
        auto stream = get_file_stream_from_appdata("compressor_game_configs.xml");
        if (!stream) return;
        std::string file_text = compressor_game_serialize(&compressor_game_configs);
        *stream << juce::StringRef(file_text);
    }();
    
    //save previous results
    [&] {
        auto stream = get_file_stream_from_appdata("compressor_game_results.xml");
        if (!stream) return;
        juce::ValueTree root_node { id_results_root };
        for (uint32_t i = 0; i < compressor_game_results_history.size(); i++)
        {
            juce::ValueTree node = { id_result, {
                { id_result_score,  compressor_game_results_history[i].score }
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
        [this] { to_freq_game_settings(); },
        [this] { to_comp_game_settings(); },
        [this] { to_audio_file_settings(); },
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

void Application_Standalone::to_audio_file_settings()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    auto on_back_pressed = [&] {
        file_player_post_command(&player, { .type = Audio_Command_Stop });
        to_main_menu();
    };
    auto audio_file_settings_panel = std::make_unique<Audio_File_Settings_Panel>(&player, &audio_file_list,  std::move(on_back_pressed));
    main_component->changePanel(std::move(audio_file_settings_panel));
}

void Application_Standalone::to_freq_game_settings()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    auto to_main_menu = [&] { this->to_main_menu(); };
    auto to_selector = [&] {
        auto back_to_config = [&] { 
            to_freq_game_settings();
        };
        auto to_game = [&] { to_frequency_game(); };
        auto selector_panel = std::make_unique < Audio_File_Chooser > (
            std::move(back_to_config),
            std::move(to_game),
            &audio_file_list
        );
        main_component->changePanel(std::move(selector_panel));
    };

    auto config_panel = std::make_unique < Frequency_Config_Panel > (
        &frequency_game_configs, 
        &current_frequency_game_config_idx, 
        std::move(to_main_menu), 
        std::move(to_selector)
    );
    main_component->changePanel(std::move(config_panel));
}

void Application_Standalone::to_frequency_game()
{
    main_component->changePanel(nullptr);

    auto observer = [this] (Frequency_Game_Effects *effects) { 
        if (effects->transition)
        {
            if (effects->transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < FrequencyGame_UI > (frequency_game_io.get());
                frequency_game_ui = new_game_ui.get();
                main_component->changePanel(std::move(new_game_ui));
            }
        }
        if (effects->dsp)
        {
            file_player_push_dsp(&player, effects->dsp->dsp_state);
        }
        if (effects->player)
        {
            for (const auto& command : effects->player->commands)
                file_player_post_command(&player, command);
        }
        if (effects->results)
        {
            frequency_game_results_history.push_back(*effects->results);
        }
        if (effects->ui)
        {
            assert(frequency_game_ui);
            frequency_game_ui_update(frequency_game_ui, &(*effects->ui));
        }
    };

    auto debug_observer = [] (Frequency_Game_Effects *effects) {
        juce::ignoreUnused(effects);
    };
    
    auto selected_file_list = generate_list_of_selected_files(&audio_file_list);
    FrequencyGame_Config config = frequency_game_configs[current_frequency_game_config_idx];
    auto new_game_state = frequency_game_state_init(config, &selected_file_list);
    frequency_game_io = frequency_game_io_init(new_game_state);

    auto on_quit = [this] { 
        frequency_game_io->timer.stopTimer();
        file_player_post_command(&player, { .type = Audio_Command_Stop });
        to_main_menu();
    };
    frequency_game_io->on_quit = std::move(on_quit);

    frequency_game_add_observer(frequency_game_io.get(), std::move(observer));
    frequency_game_add_observer(frequency_game_io.get(), std::move(debug_observer));

    frequency_game_post_event(frequency_game_io.get(), Event { .type = Event_Init });
    frequency_game_post_event(frequency_game_io.get(), Event { .type = Event_Create_UI });
    frequency_game_io->timer.callback = [io = frequency_game_io.get()] (int64_t timestamp) {
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

void Application_Standalone::to_comp_game_settings()
{
    assert(frequency_game_io == nullptr);
    assert(compressor_game_io == nullptr);
    auto to_main_menu = [&] { this->to_main_menu(); };
    auto to_selector = [&] {
        auto back_to_config = [&] { 
            to_comp_game_settings();
        };
        auto to_game = [&] { to_compressor_game(); };
        auto selector_panel = std::make_unique < Audio_File_Chooser > (
            std::move(back_to_config),
            std::move(to_game),
            &audio_file_list
        );
        main_component->changePanel(std::move(selector_panel));
    };

    auto config_panel = std::make_unique < Compressor_Config_Panel > (
        &compressor_game_configs, 
        &current_compressor_game_config_idx, 
        std::move(to_main_menu), 
        std::move(to_selector)
    );
    main_component->changePanel(std::move(config_panel));
}

void Application_Standalone::to_compressor_game()
{
    main_component->changePanel(nullptr);

    auto observer = [this] (Compressor_Game_Effects *effects) { 
        if (effects->transition)
        {
            if (effects->transition->in_transition == GameStep_Begin)
            {
                auto new_game_ui = std::make_unique < CompressorGame_UI > (compressor_game_io.get());
                compressor_game_ui = new_game_ui.get();
                main_component->changePanel(std::move(new_game_ui));
            }
        }
        if (effects->dsp)
        {
            file_player_push_dsp(&player, effects->dsp->dsp_state);
        }
        if (effects->player)
        {
            for (const auto& command : effects->player->commands)
                file_player_post_command(&player, command);
        }
        if (effects->results)
        {
            compressor_game_results_history.push_back(*effects->results);
        }
        if (effects->ui)
        {
            assert(compressor_game_ui);
            compressor_game_ui_update(compressor_game_ui, &(*effects->ui));
        }
    };

    auto debug_observer = [] (Compressor_Game_Effects *effects) {
        juce::ignoreUnused(effects);
    };
    
    auto selected_file_list = generate_list_of_selected_files(&audio_file_list);
    CompressorGame_Config config = compressor_game_configs[current_compressor_game_config_idx];
    auto new_game_state = compressor_game_state_init(config, &selected_file_list);
    compressor_game_io = compressor_game_io_init(new_game_state);

    auto on_quit = [this] { 
        compressor_game_io->timer.stopTimer();
        file_player_post_command(&player, { .type = Audio_Command_Stop });
        to_main_menu();
    };
    compressor_game_io->on_quit = std::move(on_quit);

    compressor_game_add_observer(compressor_game_io.get(), std::move(observer));
    compressor_game_add_observer(compressor_game_io.get(), std::move(debug_observer));

    compressor_game_post_event(compressor_game_io.get(), Event { .type = Event_Init });
    compressor_game_post_event(compressor_game_io.get(), Event { .type = Event_Create_UI });
    compressor_game_io->timer.callback = [io = compressor_game_io.get()] (int64_t timestamp) {
        compressor_game_post_event(io, Event {.type = Event_Timer_Tick, .value_i64 = timestamp});
    };
    compressor_game_io->timer.startTimerHz(60);
}

//------------------------------------------------------------------------
File_Player::File_Player(juce::AudioFormatManager *formatManager)
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

File_Player::~File_Player()
{
    transport_source.setSource (nullptr);
    source_player.setSource (nullptr);

    device_manager.removeAudioCallback (&source_player);
}

bool file_player_load(File_Player *player, Audio_File *audio_file, int64_t *out_file_length_ms)
{
    player->transport_source.stop();
    player->transport_source.setSource(nullptr);
    player->current_reader_source.reset();

    const auto source = makeInputSource(audio_file->file);

    if (source == nullptr)
        return false;

    auto stream = juce::rawToUniquePtr(source->createInputStream());

    if (stream == nullptr)
        return false;

    auto reader = juce::rawToUniquePtr(player->format_manager->createReaderFor(std::move(stream)));

    if (reader == nullptr)
        return false;

    player->dsp_callback.push_normalization_volume(1.0f / audio_file->max_level);

    player->current_reader_source = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    player->current_reader_source->setLooping(false);

    player->transport_source.setSource(player->current_reader_source.get(),
                                       32768,
                                       &player->read_ahead_thread,
                                       player->current_reader_source->getAudioFormatReader()->sampleRate);
    player->transport_source.setLoopBounds(audio_file->loop_bounds_ms.getStart(), audio_file->loop_bounds_ms.getEnd());
    player->transport_source.setPosition((double)audio_file->loop_bounds_ms.getStart() / 1000.0);

    *out_file_length_ms = (int64_t)(player->transport_source.getLengthInSeconds() * 1000.0);
    return true;
}

File_Player_State file_player_post_command(File_Player *player, Audio_Command command)
{
    switch (command.type)
    {
        case Audio_Command_Play :
        {
            player->transport_source.start();
            player->player_state.step = Transport_Playing;
        } break;
        case Audio_Command_Pause :
        {
            DBG("Pause");
            player->transport_source.stop();
            player->player_state.step = Transport_Paused;
        } break;
        case Audio_Command_Stop :
        {
            player->transport_source.stop();
            player->transport_source.setPosition(0);
            player->player_state.step = Transport_Stopped;
        } break;
        case Audio_Command_Seek :
        {
            assert(command.value_i64 >= 0 && command.value_i64 < player->player_state.file_length_ms);
            player->transport_source.setPosition((double)command.value_i64 / 1000.0);
        } break;
        case Audio_Command_Update_Loop :
        {
            //TODO loop can't be zero length
            assert(command.loop_start_ms >= 0 && command.loop_start_ms <= player->player_state.file_length_ms);
            assert(command.loop_end_ms >= 0 && command.loop_end_ms <= player->player_state.file_length_ms);
            assert(command.loop_start_ms <= command.loop_end_ms);
            player->player_state.loop_start_ms = command.loop_start_ms;
            player->player_state.loop_end_ms = command.loop_end_ms;
            player->transport_source.setLoopBounds(command.loop_start_ms, command.loop_end_ms);

        } break;
        case Audio_Command_Load :
        {
            int64_t file_lentgh_ms;
            bool success = file_player_load(player, &command.value_file, &file_lentgh_ms);
            if (success)
            {
                player->player_state.file_length_ms = file_lentgh_ms;
                player->player_state.playing_file_hash = command.value_file.hash;
                player->player_state.step = Transport_Stopped;
                player->player_state.loop_start_ms = command.value_file.loop_bounds_ms.getStart();
                player->player_state.loop_end_ms = command.value_file.loop_bounds_ms.getEnd();
            }
            else
            {
                player->player_state.step = Transport_Loading_Failed;
            }
        } break;
    }
    return player->player_state;
}

void file_player_push_dsp(File_Player *player, Channel_DSP_State new_dsp_state)
{
    player->dsp_callback.push_new_dsp_state(new_dsp_state);
}

void File_Player::changeListenerCallback(juce::ChangeBroadcaster* source)
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

File_Player_State file_player_query_state(File_Player *player)
{
    return player->player_state;
}