
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
//#include "CompressorWidget.h"
#include "CompressorGame.h"

CompressorGame_Config compressor_game_config_default(juce::String name)
{
    return {
        .title = name,
        .threshold_values_db = { -24.0f, -18.0f, -12.0f, -6.0f, 0.0f },
        .ratio_values = { 1.0f, 3.0f, 6.0f, 10.0f },
        .attack_values = { 1.0f, 10.0f, 30.0f, 60.0f, 100.0f },
        .release_values = { 1.0f, 50.0f, 100.0f, 150.0f },
        .variant = Compressor_Game_Normal,
        .listens = 0,
        .timeout_ms = 0,
        .total_rounds = 10
    };
}

void compressor_game_ui_transitions(CompressorGame_UI *ui, Effect_Transition transition)
{
    juce::ignoreUnused(ui);
    if (transition.in_transition == GameStep_Begin)
    {
        ui->previewer_file_list.setVisible(true);
        ui->resized();
#if 0  
        ui->previewer_file_list.selection_changed_callback = 
            [fileList, selection_changed = std::move(update_ui)] (const std::vector<bool> & new_selection)
        {
            fileList->selected = new_selection;
            selection_changed(new_selection);
        };
        std::vector<juce::String> file_names{};
        for (uint32_t i = 0; fileList->files.size(); i++)
        {
            file_names.push_back(fileList->files[i].title);
        }
        list_comp.set_rows(file_names, fileList->selected);
#endif
    }
    if (transition.out_transition == GameStep_Begin)
    {
        ui->previewer_file_list.setVisible(false);
#if 0
        ui->compressor_widget = std::make_unique < CompressorWidget > ();
        ui->compressor_widget->onClick = [state = ui->game_state, io = ui->game_io] (int compressor) {
            Event event = {
                .type = Event_Click_Compressor,
                .value_i = compressor
            };
            compressor_game_post_event(state, io, event);
        };
        ui->addAndMakeVisible(*ui->compressor_widget);
#endif
        ui->resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
#if 0
        ui->compressor_widget.reset();
        ui->results_panel = std::make_unique < CompressorGame_Results_Panel > ();
        ui->addAndMakeVisible(*ui->results_panel);
        ui->resized();
#endif
    }
}

void compressor_game_ui_update(CompressorGame_UI *ui, Compressor_Game_Effect_UI *new_ui)
{
    compressor_game_ui_transitions(ui, new_ui->transition);

	game_ui_header_update(&ui->header, new_ui->header_center_text, new_ui->header_right_text);
    
    auto threshold_text = [values = new_ui->comp_widget.threshold_values_db] (double new_pos) {
        return juce::Decibels::toString(values[static_cast<size_t>(new_pos)], 0);
    };

    auto ratio_text = [values = new_ui->comp_widget.ratio_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]);
    };

    auto attack_text = [values = new_ui->comp_widget.attack_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]) + " ms";
    };

    auto release_text = [values = new_ui->comp_widget.release_values] (double new_pos) {
        return juce::String(values[static_cast<size_t>(new_pos)]) + " ms";
    };

    using bundle_t = std::tuple<TextSlider&, Widget_Interaction_Type, int, int, std::function < juce::String(double)> >;

    auto bundle = std::vector<bundle_t>{
        { ui->compressor_widget.threshold_slider, new_ui->comp_widget.threshold_visibility, new_ui->comp_widget.threshold_pos, static_cast<int>(new_ui->comp_widget.threshold_values_db.size()), std::move(threshold_text) },
        { ui->compressor_widget.ratio_slider, new_ui->comp_widget.ratio_visibility, new_ui->comp_widget.ratio_pos, static_cast<int>(new_ui->comp_widget.ratio_values.size()), std::move(ratio_text) },
        { ui->compressor_widget.attack_slider, new_ui->comp_widget.attack_visibility,  new_ui->comp_widget.attack_pos, static_cast<int>(new_ui->comp_widget.attack_values.size()), std::move(attack_text) },
        { ui->compressor_widget.release_slider, new_ui->comp_widget.release_visibility, new_ui->comp_widget.release_pos, static_cast<int>(new_ui->comp_widget.release_values.size()), std::move(release_text) }
    };

    //TODO rename range
    for (auto &[slider, visibility, position, range, text_from_value] : bundle)
    {
        slider.setVisible(true);
        if (visibility != Widget_Hiding)
        {
            slider.get_text_from_value = std::move(text_from_value);
            slider.setRange(0.0, (double)(range - 1), 1.0);
            slider.setValue(static_cast<double>(position), juce::dontSendNotification);
        }
        else
        {
            slider.get_text_from_value = [](auto ...) { return ""; };
            slider.setRange(0.0, 1.0, 1.0);
            slider.setValue(0.0, juce::dontSendNotification);
        }

        switch (visibility)
        {
            case Widget_Editing :
            {
                slider.setEnabled(true);
            } break;
            case Widget_Hiding :
            case Widget_Showing :
            {
                slider.setEnabled(false);
            } break;
        }
    }

    game_ui_bottom_update(&ui->bottom, true, new_ui->bottom_button_text, new_ui->mix_toggles, new_ui->bottom_button_event);
}

void compressor_game_post_event(CompressorGame_IO *io, Event event)
{
    Compressor_Game_Effects effects = compressor_game_update(io->game_state, event);
    assert(effects.error == 0);
    {
        std::lock_guard lock { io->update_fn_mutex };
        io->game_state = effects.new_state;
    }
    assert(effects.error == 0);
    for (auto i = 0; i < io->observers.size(); i++)
    {
        io->observers[i](&effects);
    }

    if (effects.quit)
    {
        io->on_quit();
    }
}

CompressorGame_State compressor_game_state_init(CompressorGame_Config config, 
                                                std::vector<Audio_File> *files)
{
    assert(!files->empty());
    auto state = CompressorGame_State {
        .files = std::move(*files),
        .config = config,
        .timestamp_start = -1
    };
    return state;
}

std::unique_ptr<CompressorGame_IO> compressor_game_io_init(CompressorGame_State state)
{
    auto io = std::make_unique<CompressorGame_IO>();
    io->game_state = state;
    return io;
}

Compressor_Game_Effects compressor_game_update(CompressorGame_State state, Event event)
{
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;

    Compressor_Game_Effects effects = { 
        .error = 0,
        .transition = std::nullopt,
        .dsp = std::nullopt, 
        .player = std::nullopt, 
        .ui = std::nullopt, 
        .quit = false, 
    };

    bool done_listening = false;
    bool check_answer = false;

    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Slider : 
        {
            assert(state.step == GameStep_Begin || state.step == GameStep_Question);

            switch (event.id) 
            {
                case 0 : {
                    assert(state.config.threshold_active || state.step == GameStep_Begin);
                    state.input_threshold_pos = event.value_i;
                } break;
                case 1 : {
                    assert(state.config.ratio_active || state.step == GameStep_Begin);
                    state.input_ratio_pos = event.value_i;
                } break;
                case 2 : {
                    assert(state.config.attack_active || state.step == GameStep_Begin);
                    state.input_attack_pos = event.value_i;
                } break;
                case 3 : {
                    assert(state.config.release_active || state.step == GameStep_Begin);
                    state.input_release_pos = event.value_i;
                } break;
            }
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : 
        {
            if (state.step == GameStep_Question)
            {
                if(state.config.variant == Compressor_Game_Timer) assert(false);
                if (!state.can_still_listen) assert(false);
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) assert(false);
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) assert(false);
                    switch (state.config.variant)
                    {
                        case Compressor_Game_Normal : 
                        {
                            state.mix = Mix_User;
                        } break;
                        case Compressor_Game_Timer : 
                        {
                            jassertfalse;
                        } break;
                        case Compressor_Game_Tries : 
                        {
                            state.remaining_listens--;
                            if(state.remaining_listens == 0)
                                done_listening = true;
                            else 
                                state.mix = Mix_User;
                        } break;
                    }
                }
                else jassertfalse;
            }
            else if (state.step == GameStep_Result)
            {
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) assert(false);
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) assert(false);
                    state.mix = Mix_User;
                }
                else jassertfalse;
            }
            else jassertfalse;
            update_audio = true;
            update_ui = true;
        } break;
        case Event_Timer_Tick :
        {
            state.current_timestamp = event.value_i64;

            if (state.step == GameStep_Question 
                && state.config.variant == Compressor_Game_Timer
                && state.mix == Mix_Target)
            {
                if (state.current_timestamp >= state.timestamp_start + state.config.timeout_ms)
                {
                    done_listening = true;
                }
            }
            else
            {
                if(state.timestamp_start != -1) assert(false);
            }
        } break;
        case Event_Click_Begin :
        {
            if(state.step != GameStep_Begin) assert(false);
            if(state.mix != Mix_Hidden) assert(false);
            if (state.target_ratio_pos != -1 ||
                state.target_threshold_pos != -1 ||
                state.target_attack_pos != -1 ||
                state.target_release_pos != -1)
            {
                assert(false);
            }
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Click_Answer :
        {
            check_answer = true;
        } break;
        case Event_Click_Next :
        {
            if (state.step != GameStep_Result) assert(false);
            out_transition = GameStep_Result;
            if(state.current_round == state.config.total_rounds)
                in_transition = GameStep_EndResults;
            else
                in_transition = GameStep_Question;
        } break;
        case Event_Click_Back :
        {
            effects.quit = true;
        } break;
        case Event_Click_Quit :
        {
            effects.quit = true;
        } break;
        case Event_Create_UI :
        {
            update_ui = true;
        } break;
        case Event_Destroy_UI :
        {
        } break; 
        case Event_Click_Frequency :
        case Event_Click_Track :
        case Event_Toggle_Track :
        case Event_Click_Done_Listening :
        {
            jassertfalse;
        } break;
    }
    
    if (check_answer)
    {
        if(state.step != GameStep_Question) assert(false);

        int points_awarded = 0;

        if (state.config.threshold_active && state.target_threshold_pos == state.input_threshold_pos)
            points_awarded++;

        if (state.config.ratio_active && state.target_ratio_pos == state.input_ratio_pos) 
            points_awarded++;

        if (state.config.attack_active && state.target_attack_pos == state.input_attack_pos) 
            points_awarded++;

        if (state.config.release_active && state.target_release_pos == state.input_release_pos) 
            points_awarded++;

        state.score += points_awarded;
        out_transition = GameStep_Question;
        in_transition = GameStep_Result;
    }
    
    if (done_listening)
    {
        if(state.step != GameStep_Question) assert(false);
        if(state.mix != Mix_Target) assert(false);
        state.mix = Mix_User;
        state.timestamp_start = -1;

        state.can_still_listen = false;
        update_audio = true;
        update_ui = true;
    }

    if (in_transition != GameStep_None || out_transition != GameStep_None)
    {
        effects.transition = {
            .in_transition = in_transition,
            .out_transition = out_transition
        };
    }

    switch (out_transition)
    {
        case GameStep_None :
        {
        } break;
        case GameStep_Begin :
        {
        } break;
        case GameStep_Question :
        {
            state.timestamp_start = -1;
        } break;
        case GameStep_Result :
        {
            state.timestamp_start = -1;
        } break;
        case GameStep_EndResults :
        {
            jassertfalse;
        } break;
    }
    
    switch (in_transition)
    {
        case GameStep_None : 
        {
        }break;
        case GameStep_Begin : {
            state.step = GameStep_Begin;
            state.score = 0;
            state.mix = Mix_Hidden;

            state.target_threshold_pos = -1;
            state.target_ratio_pos = -1;
            state.target_attack_pos = -1;
            state.target_release_pos = -1;
            
            state.input_threshold_pos = static_cast<int>(state.config.threshold_values_db.size()) - 1;
            state.input_ratio_pos = 0;
            state.input_attack_pos = 0;
            state.input_release_pos = 0;

            state.current_file_idx = -1;
            state.current_round = 0;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            state.step = GameStep_Question;
            state.mix = Mix_Target;
            state.can_still_listen = true;
            state.current_round++;

            state.target_threshold_pos = random_positive_int(static_cast<int>(state.config.threshold_values_db.size()));
            state.target_ratio_pos = random_positive_int(static_cast<int>(state.config.ratio_values.size()));
            state.target_attack_pos = random_positive_int(static_cast<int>(state.config.attack_values.size()));
            state.target_release_pos = random_positive_int(static_cast<int>(state.config.release_values.size()));

            {
                //TODO
                if (state.config.threshold_active)
                    state.input_threshold_pos = static_cast<int>(state.config.threshold_values_db.size()) - 1;
                else
                    state.input_threshold_pos = state.target_threshold_pos;

                if (state.config.ratio_active)
                    state.input_ratio_pos = 0;
                else
                    state.input_ratio_pos = state.target_ratio_pos;

                if (state.config.attack_active)
                    state.input_attack_pos = 0;
                else
                    state.input_attack_pos = state.target_attack_pos;

                if (state.config.release_active)
                    state.input_release_pos = 0;
                else
                    state.input_release_pos = state.target_release_pos;
            }

            state.current_file_idx = random_positive_int((int)state.files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state.files[static_cast<size_t>(state.current_file_idx)] },
                    { .type = Audio_Command_Play },
                }
            };
            
            switch (state.config.variant)
            {
                case Compressor_Game_Normal : {
                } break;
                case Compressor_Game_Timer : {
                    state.timestamp_start = state.current_timestamp;
                } break;
                case Compressor_Game_Tries : {
                    state.remaining_listens = state.config.listens;
                } break;
            }

            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            state.step = GameStep_Result;
            state.mix = Mix_Target;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        {
            state.step = GameStep_EndResults;
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Stop },
                }
            };
            state.results = {
                .score = state.score,
                .analytics = juce::Random::getSystemRandom().nextFloat()
            };
            effects.results = state.results;
            update_audio = true;
            update_ui = true;
        }break;
    }
    
    int threshold_pos;
    int ratio_pos;
    int attack_pos;
    int release_pos;
    if (state.step == GameStep_Begin || state.mix == Mix_User)
    {
        threshold_pos = state.input_threshold_pos;
        ratio_pos = state.input_ratio_pos;
        attack_pos = state.input_attack_pos;
        release_pos = state.input_release_pos;
    }
    else
    {
        threshold_pos = state.target_threshold_pos;
        ratio_pos = state.target_ratio_pos;
        attack_pos = state.target_attack_pos;
        release_pos = state.target_release_pos;
    }

    if (update_audio)
    {
        Channel_DSP_State dsp = ChannelDSP_on();

        switch (state.step)
        {
            case GameStep_Begin :
            {
            } break;
            case GameStep_Question :
            case GameStep_Result :
            {
                dsp.comp = {
                    .is_on = true,
                    .threshold_gain = state.config.threshold_values_db[threshold_pos],
                    .ratio = state.config.ratio_values[ratio_pos],
                    .attack = state.config.attack_values[attack_pos],
                    .release = state.config.release_values[release_pos],
                    .makeup_gain = 1.0f,
                };
            } break;
            case GameStep_EndResults :
            {
            } break;
            case GameStep_None :
            {
                jassertfalse;
            } break;
        }
        effects.dsp = Effect_DSP_Single_Track { dsp };
    }

    if (update_ui)
    {
        effects.ui = Compressor_Game_Effect_UI
        {
            .transition = {
                .in_transition = in_transition,
                .out_transition = out_transition
            },
            .comp_widget = {
                .threshold_pos = threshold_pos,
                .ratio_pos = ratio_pos,
                .attack_pos = attack_pos,
                .release_pos = release_pos,

                .threshold_values_db = state.config.threshold_values_db,
                .ratio_values = state.config.ratio_values,
                .attack_values = state.config.attack_values,
                .release_values = state.config.release_values
            }
        };
        if (state.step != GameStep_EndResults)
        {
            Widget_Interaction_Type active_visibility = gameStepToFaderStep(state.step, state.mix);
            Widget_Interaction_Type inactive_visibility = state.step == GameStep_Begin ? Widget_Editing : Widget_Showing;

            if (state.config.threshold_active)
                effects.ui->comp_widget.threshold_visibility = active_visibility;
            else
                effects.ui->comp_widget.threshold_visibility = inactive_visibility;

            if (state.config.ratio_active)
                effects.ui->comp_widget.ratio_visibility = active_visibility;
            else
                effects.ui->comp_widget.ratio_visibility = inactive_visibility;

            if (state.config.attack_active)
                effects.ui->comp_widget.attack_visibility = active_visibility;
            else
                effects.ui->comp_widget.attack_visibility = inactive_visibility;

            if (state.config.release_active)
                effects.ui->comp_widget.release_visibility = active_visibility;
            else
                effects.ui->comp_widget.release_visibility = inactive_visibility;
        }
        
        switch (state.config.variant)
        {
            case Compressor_Game_Normal : {
                effects.ui->mix_toggles = state.mix;
            } break;
            case Compressor_Game_Timer : {
                effects.ui->mix_toggles = Mix_Hidden;
            } break;
            case Compressor_Game_Tries : {
                if (state.step == GameStep_Question && state.remaining_listens == 0)
                    effects.ui->mix_toggles = Mix_Hidden;
                else
                    effects.ui->mix_toggles = state.mix;
            } break;
        }

        switch (state.step)
        {
            case GameStep_Begin :
            {
                effects.ui->header_center_text = "Ready ?";
                effects.ui->bottom_button_text = "Begin";
                effects.ui->bottom_button_event = Event_Click_Begin;
            } break;
            case GameStep_Question :
            {
                effects.ui->header_center_text = "Listen and reproduce";
                effects.ui->header_right_text = "Round " + juce::String(state.current_round) + " / " + juce::String(state.config.total_rounds);
                
                switch (state.config.variant)
                {
                    case Compressor_Game_Normal :
                    {
                        effects.ui->bottom_button_text = "Validate";
                        effects.ui->bottom_button_event = Event_Click_Answer;
                    } break;
                    case Compressor_Game_Timer :
                    {
                        if (state.mix == Mix_Target)
                        {
                            effects.ui->bottom_button_text = "Go";
                            effects.ui->bottom_button_event = Event_Click_Done_Listening;
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->bottom_button_text = "Validate";
                            effects.ui->bottom_button_event = Event_Click_Answer;
                        }
                        else jassertfalse;
                    } break;
                    case Compressor_Game_Tries :
                    {
                        if (state.mix == Mix_Target)
                        {
                            if (!state.can_still_listen) assert(false);
                            effects.ui->header_center_text = juce::String("remaining listens : ") + juce::String(state.remaining_listens);
                        }
                        else if (state.mix == Mix_User)
                        {
                            effects.ui->header_center_text = "Reproduce the target mix";
                        }
                        else jassertfalse;
                        effects.ui->bottom_button_text = "Validate";
                        effects.ui->bottom_button_event = Event_Click_Answer;
                    } break;
                }
            } break;
            case GameStep_Result :
            {
                effects.ui->header_center_text = juce::String("Score : ") + juce::String(state.score);
                effects.ui->header_right_text = "Round " + juce::String(state.current_round) + " / " + juce::String(state.config.total_rounds);
                effects.ui->bottom_button_text = "Next";
                effects.ui->bottom_button_event = Event_Click_Next;
            } break;
            case GameStep_EndResults :
            {
                effects.ui->results.score = state.score;
                effects.ui->header_center_text = "Results";
                effects.ui->header_right_text = "";
                effects.ui->bottom_button_text = "Quit";
                effects.ui->bottom_button_event = Event_Click_Quit;
            } break;
            case GameStep_None :
            {
                jassertfalse;
            } break;
        }
    }

    effects.new_state = state;
    return effects;
}

void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer)
{
    io->observers.push_back(std::move(observer));
}

static const juce::Identifier id_config_root = "configs";
static const juce::Identifier id_config = "config";
static const juce::Identifier id_config_title = "title";

static const juce::Identifier id_config_variant = "variant";

static const juce::Identifier id_config_total_rounds = "total_rounds";
static const juce::Identifier id_config_listen_count = "listen_count";

static const juce::Identifier id_config_thresholds = "thresholds";
static const juce::Identifier id_config_ratios = "ratios";
static const juce::Identifier id_config_attacks = "attacks";
static const juce::Identifier id_config_releases = "releases";

static const juce::Identifier id_config_threshold_active = "threshold_active";
static const juce::Identifier id_config_ratio_active = "ratio_active";
static const juce::Identifier id_config_attack_active = "attack_active";
static const juce::Identifier id_config_release_active = "release_active";

static const juce::Identifier id_config_prelisten_type = "prelisten_type";
static const juce::Identifier id_config_prelisten_timeout_ms = "prelisten_timeout_ms";

static const juce::Identifier id_config_question_type = "question_type";
static const juce::Identifier id_config_question_timeout_ms = "question_timeout_ms";

static const juce::Identifier id_config_result_timeout_enabled = "result_timeout_enabled";
static const juce::Identifier id_config_result_timeout_ms = "result_timeout_ms";

static const juce::Identifier id_results_root = "results_history";
static const juce::Identifier id_result = "result";
static const juce::Identifier id_result_score = "score";
static const juce::Identifier id_result_timestamp = "timestamp";

juce::String compressor_game_serialize(std::vector<CompressorGame_Config> *compressor_game_configs)
{ 
    juce::ValueTree root_node { id_config_root };
    for (uint32_t i = 0; i < compressor_game_configs->size(); i++)
    {
        auto *config = &compressor_game_configs->at(i);
        juce::ValueTree node = { id_config, {
            { id_config_title,  config->title },
                
            { id_config_threshold_active, config->threshold_active },
            { id_config_ratio_active, config->ratio_active },
            { id_config_attack_active, config->attack_active },
            { id_config_release_active, config->release_active },

            { id_config_thresholds, serialize_vector<float>(config->threshold_values_db) },
            { id_config_ratios, serialize_vector<float>(config->ratio_values) },
            { id_config_attacks, serialize_vector<float>(config->attack_values) },
            { id_config_releases, serialize_vector<float>(config->release_values) },

            { id_config_variant, (int) config->variant },
            { id_config_listen_count, config->listens },
            { id_config_question_timeout_ms, config->timeout_ms },
            { id_config_total_rounds, config->total_rounds }
        }};
        root_node.addChild(node, -1, nullptr);
    }
    return root_node.toXmlString();
}

std::vector<CompressorGame_Config> compressor_game_deserialize(juce::String xml_string)
{
    std::vector<CompressorGame_Config> compressor_game_configs{};
    juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
    if (root_node.getType() != id_config_root)
        return {};
    for (int i = 0; i < root_node.getNumChildren(); i++)
    {
        juce::ValueTree node = root_node.getChild(i);
        if(node.getType() != id_config)
            continue;

        CompressorGame_Config config = {
            .title = node.getProperty(id_config_title, ""),

            .threshold_active = node.getProperty(id_config_threshold_active, true),
            .ratio_active = node.getProperty(id_config_ratio_active, true),
            .attack_active = node.getProperty(id_config_attack_active, true),
            .release_active = node.getProperty(id_config_release_active, true),

            .threshold_values_db = deserialize_vector<float>(node.getProperty(id_config_thresholds, "")),
            .ratio_values = deserialize_vector<float>(node.getProperty(id_config_ratios, "")),
            .attack_values = deserialize_vector<float>(node.getProperty(id_config_attacks, "")),
            .release_values = deserialize_vector<float>(node.getProperty(id_config_releases, "")),

            .variant = (Compressor_Game_Variant)(int)node.getProperty(id_config_variant, 0),
            .listens = node.getProperty(id_config_listen_count, 0),
            .timeout_ms = node.getProperty(id_config_question_timeout_ms, 0),
            .total_rounds = node.getProperty(id_config_total_rounds, 0),
        };
        compressor_game_configs.push_back(config);
    }
    return compressor_game_configs;
}