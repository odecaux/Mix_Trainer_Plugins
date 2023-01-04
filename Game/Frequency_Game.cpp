
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../shared/shared_ui.h"
#include "../Game/Game.h"
#include "../Game/Game_UI.h"
#include "Frequency_Game.h"
#include "Frequency_Game_UI.h"

FrequencyGame_Config frequency_game_config_default(std::string name)
{
    return {
        .title = name,

        .input = Frequency_Input_Widget,
        .eq_gain_db = 6.0f,
        .eq_quality = 0.7f,
        .initial_correct_answer_window = 0.15f,
        .min_f = 70,
        .num_octaves = 8,

        .prelisten_type = PreListen_None,
        .prelisten_timeout_ms = 1000,
        .question_type = Frequency_Question_Free,
        .question_timeout_ms = 1000,
        .result_timeout_enabled = true,
        .result_timeout_ms = 1000
    };
}

void frequency_game_post_event(FrequencyGame_IO *io, Event event)
{
    Frequency_Game_Effects effects = frequency_game_update(io->game_state, event);
    assert(effects.error == 0);
    {
        std::lock_guard lock { io->update_fn_mutex };
        io->game_state = effects.new_state;
    }
    assert(effects.error == 0);
    for(auto &observer : io->observers)
        observer(&effects);

    if (effects.quit)
    {
        io->on_quit();
    }
}

FrequencyGame_State frequency_game_state_init(FrequencyGame_Config config, 
                                              std::vector<Audio_File> *files)
{
    assert(!files->empty());
    auto state = FrequencyGame_State {
        .files = std::move(*files),
        .config = config,
        .timestamp_start = -1
    };

    return state;
}

std::unique_ptr<FrequencyGame_IO> frequency_game_io_init(FrequencyGame_State state)
{
    auto io = std::make_unique<FrequencyGame_IO>();
    io->game_state = state;
    return io;
}

Frequency_Game_Effects frequency_game_update(FrequencyGame_State state, Event event)
{
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;

    Frequency_Game_Effects effects = { 
        .error = 0,
        .transition = std::nullopt,
        .dsp = std::nullopt, 
        .player = std::nullopt, 
        .ui = std::nullopt, 
        .quit = false, 
    };

    bool check_answer = false;
    uint32_t TEMP_answer_frequency = 0;

    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Click_Frequency :
        {
            check_answer = true;
            TEMP_answer_frequency = event.value_u;
        } break;
        case Event_Toggle_Input_Target :
        {
#if 0
            if (state.step != GameStep_Begin) return { .error = 1 };
            if(event.value_b && step == GameStep_Question)
            {
                step = GameStep_Listening;
            }
            else if(event.value_b && step == GameStep_ShowingAnswer)
            {
                step = GameStep_ShowingTruth;
            }
            else if(!event.value_b && step == GameStep_Listening)
            {
                step = GameStep_Editing;
            }
            else if(!event.value_b && step == GameStep_ShowingTruth){
                step = GameStep_ShowingAnswer;
            }
            update_audio = true;
            update_ui = true;
#endif
            jassertfalse;
        } break;
        case Event_Timer_Tick :
        {
            state.current_timestamp = event.value_i64;
            if (state.step == GameStep_Question && state.config.question_type != Frequency_Question_Free)
            {
                if (state.current_timestamp >=
                    state.timestamp_start + state.config.question_timeout_ms)
                {
                    state.lives--;
            
                    out_transition = GameStep_Question;
                    in_transition = GameStep_Result;
                }
            }
            else if (state.step == GameStep_Result && state.config.result_timeout_enabled)
            {
                if (state.current_timestamp >=
                    state.timestamp_start + state.config.result_timeout_ms)
                {
                    
                    out_transition = GameStep_Result;
                    if(state.lives > 0)
                        in_transition = GameStep_Question;
                    else
                        in_transition = GameStep_EndResults;
                }
            }
            else
            {
                if (state.timestamp_start != -1) return { .error = 1 };
            }
            if (state.step == GameStep_Question && state.config.question_type == Frequency_Question_Rising)
                update_audio = true;
        } break;
        case Event_Click_Begin :
        {
            if (state.step != GameStep_Begin) return { .error = 1 };
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Click_Next :
        {
            if (state.step != GameStep_Result) return { .error = 1 };
            out_transition = GameStep_Result;
            if(state.lives > 0)
                in_transition = GameStep_Question;
            else
                in_transition = GameStep_EndResults;
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
        case Event_Click_Track :
        case Event_Toggle_Track :
        case Event_Slider :
        case Event_Click_Answer :
        case Event_Click_Done_Listening :
        {
            jassertfalse;
        } break;
    }

    if (check_answer)
    {
        if (state.step != GameStep_Question) return { .error = 1 };
        float clicked_ratio = normalize_frequency(TEMP_answer_frequency, state.config.min_f, state.config.num_octaves);
        float target_ratio = normalize_frequency(state.target_frequency, state.config.min_f, state.config.num_octaves);
        float distance = std::abs(clicked_ratio - target_ratio);
        if (distance < state.correct_answer_window)
        {
            int points_scored = int((1.0f - distance) * 100.0f);
            state.score += points_scored;
            state.correct_answer_window *= 0.95f;
        }
        else
        {
            state.lives--;
        }
            
        in_transition = GameStep_Result;
        out_transition = GameStep_Question;
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
            state.lives = 5;

            state.correct_answer_window = state.config.initial_correct_answer_window;

            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            state.step = GameStep_Question;
            state.target_frequency = denormalize_frequency(juce::Random::getSystemRandom().nextFloat(), state.config.min_f, state.config.num_octaves);
    
            state.current_file_idx = random_uint(checked_cast<uint32_t>(state.files.size()));
            auto & file = state.files[static_cast<size_t>(state.current_file_idx)];
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = file},
                    { .type = Audio_Command_Play },
                }
            };
            
            if (state.config.question_type != Frequency_Question_Free)
            {
                state.timestamp_start = state.current_timestamp;
            }
            else
            {
                if (state.timestamp_start != -1) return { .error = 1 };
            }
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            state.step = GameStep_Result;
            if (state.config.result_timeout_enabled)
            {
                state.timestamp_start = state.current_timestamp;
            }
            else
            {
                if (state.timestamp_start != -1) return { .error = 1 };
            }
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

    if (update_audio)
    {
        Channel_DSP_State dsp = ChannelDSP_on();

        float compensation_gain_db = state.config.eq_gain_db > 0.0f ? 
            - state.config.eq_gain_db :
            0.0f;
        dsp.gain_db = compensation_gain_db;

        switch (state.step)
        {
            case GameStep_Begin :
            {
            } break;
            case GameStep_Question :
            {
                float ratio = 1.0f;
                if (state.config.question_type == Frequency_Question_Rising)
                {
                    ratio = float(state.current_timestamp - state.timestamp_start) / float(state.config.question_timeout_ms);
                    assert(ratio >= 0.0f && ratio < 1.0f);
                    DBG("ratio " << ratio);
                }
                dsp.eq_bands[0] = eq_band_peak((float)state.target_frequency, state.config.eq_quality, juce::Decibels::decibelsToGain(state.config.eq_gain_db * ratio));
            } break;
            case GameStep_Result :
            {
                dsp.eq_bands[0] = eq_band_peak((float)state.target_frequency, state.config.eq_quality, juce::Decibels::decibelsToGain(state.config.eq_gain_db));
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
        effects.ui = Frequency_Game_Effect_UI{};
        effects.ui->transition = {
            .in_transition = in_transition,
            .out_transition = out_transition
        };

        effects.ui->freq_widget.min_f = state.config.min_f;
        effects.ui->freq_widget.num_octaves = state.config.num_octaves;
        switch (state.step)
        {
            case GameStep_Begin :
            {
                effects.ui->header_center_text = "Ready ?";
                effects.ui->display_button = true;
                effects.ui->button_text = "Begin";
                effects.ui->button_event = Event_Click_Begin;
            } break;
            case GameStep_Question :
            {
                effects.ui->header_right_text = "Score : " + std::to_string(state.score);
                effects.ui->ui_target = state.config.input == Frequency_Input_Widget ? 0 : 2;
                effects.ui->freq_widget.display_target = false;
                effects.ui->freq_widget.is_cursor_locked = false;
                effects.ui->freq_widget.display_window = true;
                effects.ui->freq_widget.correct_answer_window = state.correct_answer_window;

                effects.ui->header_center_text = "Lives : " + std::to_string(state.lives);
                effects.ui->display_button = false;
            } break;
            case GameStep_Result :
            {
                effects.ui->header_right_text = "Score : " + std::to_string(state.score);
        
                effects.ui->ui_target = state.config.input == Frequency_Input_Widget ? 0 : 2;
                effects.ui->freq_widget.display_target = true;
                effects.ui->freq_widget.target_frequency = state.target_frequency;
                //TODO remove all that ! should not depend on the event, but on some kind of state 
                //some kind of result, I guess
                if (event.type == Event_Click_Frequency)
                {
                    effects.ui->freq_widget.is_cursor_locked = true;
                    assert(check_answer);
                    effects.ui->freq_widget.locked_cursor_frequency = TEMP_answer_frequency;
                    effects.ui->freq_widget.display_window = true;
                    effects.ui->freq_widget.correct_answer_window = state.correct_answer_window;
                } 
                else if (event.type == Event_Timer_Tick)
                {
                    effects.ui->freq_widget.is_cursor_locked = true;
                    effects.ui->freq_widget.locked_cursor_frequency = 0;
                }
                else 
                    jassertfalse;

                effects.ui->header_center_text = "Lives : " + std::to_string(state.lives);
                effects.ui->display_button = true;
                effects.ui->button_text = "Next";
                effects.ui->button_event = Event_Click_Next;
            } break;
            case GameStep_EndResults :
            {
                effects.ui->ui_target = 1;
                effects.ui->results.score = state.score;
                effects.ui->header_center_text = "Results";
                effects.ui->display_button = true;
                effects.ui->button_text = "Quit";
                effects.ui->button_event = Event_Click_Quit;
            } break;
            case GameStep_None :
            {
                jassertfalse;
            } break;
        }

        effects.ui->mix = Mix_Hidden;
        //effects.ui->score = state.score;
    }
    
    effects.new_state = state;
    return effects;
}

void frequency_game_add_observer(FrequencyGame_IO *io, frequency_game_observer_t observer)
{
    io->observers.push_back(std::move(observer));
}

void frequency_widget_update(FrequencyWidget *widget, Frequency_Game_Effect_UI *new_ui)
{
    widget->display_target = new_ui->freq_widget.display_target;
    widget->target_frequency = new_ui->freq_widget.target_frequency;
    widget->is_cursor_locked = new_ui->freq_widget.is_cursor_locked;
    widget->locked_cursor_frequency = new_ui->freq_widget.locked_cursor_frequency;
    widget->display_window = new_ui->freq_widget.display_window;
    widget->correct_answer_window = new_ui->freq_widget.correct_answer_window;
    widget->min_frequency = new_ui->freq_widget.min_f;
    widget->num_octaves = new_ui->freq_widget.num_octaves;
    widget->repaint();
}

static const juce::Identifier id_config_root = "configs";
static const juce::Identifier id_config = "config";
static const juce::Identifier id_config_title = "title";

static const juce::Identifier id_config_variant = "variant";

static const juce::Identifier id_config_total_rounds = "total_rounds";
static const juce::Identifier id_config_listen_count = "listen_count";

static const juce::Identifier id_config_input = "input";
static const juce::Identifier id_config_gain = "gain";
static const juce::Identifier id_config_quality = "q";
static const juce::Identifier id_config_window = "window";
static const juce::Identifier id_config_min_f = "min_f";
static const juce::Identifier id_config_num_octaves = "num_octaves";

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

std::string frequency_game_serlialize(std::vector<FrequencyGame_Config> *frequency_game_configs)
{
    juce::ValueTree root_node { id_config_root };
    for (uint32_t i = 0; i < frequency_game_configs->size(); i++)
    {
        auto *config = &frequency_game_configs->at(i);
        juce::ValueTree node = { id_config, {
            { id_config_title,  juce::String(config->title) },

            { id_config_input, config->input },
            { id_config_gain, config->eq_gain_db },
            { id_config_quality, config->eq_quality },
            { id_config_window, config->initial_correct_answer_window },
            { id_config_min_f, checked_cast<int>(config->min_f) },
            { id_config_num_octaves, checked_cast<int>(config->num_octaves) },

            { id_config_prelisten_type, config->prelisten_type },
            { id_config_prelisten_timeout_ms, config->prelisten_timeout_ms },

            { id_config_question_type, config->question_type },
            { id_config_question_timeout_ms, config->question_timeout_ms },

            { id_config_result_timeout_enabled, config->result_timeout_enabled },
            { id_config_result_timeout_ms, config->result_timeout_ms },
        } };
        root_node.addChild(node, -1, nullptr);
    }
    return root_node.toXmlString().toStdString();
}


std::vector<FrequencyGame_Config> frequency_game_deserialize(std::string xml_string)
{
    std::vector<FrequencyGame_Config> frequency_game_configs{};

    juce::ValueTree root_node = juce::ValueTree::fromXml(xml_string);
    if (root_node.getType() != id_config_root)
        return {};
    for (uint32_t i = 0; i < checked_cast<uint32_t>(root_node.getNumChildren()); i++)
    {
        juce::ValueTree node = root_node.getChild(i);
        if(node.getType() != id_config)
            continue;
        FrequencyGame_Config config = {
            .title = node.getProperty(id_config_title, "").toString().toStdString(),

            .input = (Frequency_Input)(int)node.getProperty(id_config_input, (int)Frequency_Input_Widget),
            .eq_gain_db = node.getProperty(id_config_gain, 0.0f),
            .eq_quality = node.getProperty(id_config_quality, -1.0f),
            .initial_correct_answer_window = node.getProperty(id_config_window, -1.0f),
            .min_f = checked_cast<uint32_t>((int)node.getProperty(id_config_min_f, 0)),
            .num_octaves = checked_cast<uint32_t>((int)node.getProperty(id_config_num_octaves, 0)),

            .prelisten_type = (PreListen_Type)(int)node.getProperty(id_config_prelisten_type, (int)PreListen_None),
            .prelisten_timeout_ms = node.getProperty(id_config_prelisten_timeout_ms, -1),

            .question_type = (Frequency_Question_Type)(int)node.getProperty(id_config_question_type, false),
            .question_timeout_ms = node.getProperty(id_config_question_timeout_ms, -1),

            .result_timeout_enabled = node.getProperty(id_config_result_timeout_enabled, false),
            .result_timeout_ms = node.getProperty(id_config_result_timeout_ms, -1),
        };
        frequency_game_configs.push_back(config);
    }
    return frequency_game_configs;
}