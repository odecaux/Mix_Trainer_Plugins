
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "FrequencyWidget.h"
#include "FrequencyGame.h"

FrequencyGame_Config frequency_game_config_default(juce::String name)
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


void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition, int ui_target)
{
    if (transition.out_transition == GameStep_Begin)
    {
        if (ui_target == 0)
        {
            auto frequency_widget = std::make_unique<FrequencyWidget>();
            frequency_widget->onClick = [io = ui.game_io] (int frequency) {
                Event event = {
                    .type = Event_Click_Frequency,
                    .value_i = frequency
                };
                frequency_game_post_event(io, event);
            };
            ui.center_panel = std::move(frequency_widget);
        }
        else
        {
            auto text_input = std::make_unique<Frequency_Game_Text_Input>();
            text_input->onClick = [io = ui.game_io] (int frequency) {
                Event event = {
                    .type = Event_Click_Frequency,
                    .value_i = frequency
                };
                frequency_game_post_event(io, event);
            };
            ui.center_panel = std::move(text_input);
        }
        ui.addAndMakeVisible(*ui.center_panel);
        ui.resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
        assert(ui_target == 1);
        auto results_panel = std::make_unique < FrequencyGame_Results_Panel > ();
        ui.center_panel = std::move(results_panel);
        ui.addAndMakeVisible(*ui.center_panel);
        ui.resized();
    }
}

void frequency_game_ui_update(FrequencyGame_UI &ui, const Frequency_Game_Effect_UI &new_ui)
{
    frequency_game_ui_transitions(ui, new_ui.transition, new_ui.ui_target);
    game_ui_header_update(&ui.header, new_ui.header_center_text, new_ui.header_right_text);
    if (ui.center_panel)
    {
        switch (new_ui.ui_target)
        {
            case 0 :
            {
                auto *frequency_widget = dynamic_cast<FrequencyWidget*>(ui.center_panel.get());
                assert(frequency_widget);
                frequency_widget_update(frequency_widget, new_ui);
            } break;
            case 1 :
            {
                auto *result_panel = dynamic_cast<FrequencyGame_Results_Panel*>(ui.center_panel.get());
                assert(result_panel);
                result_panel->score_label.setText(juce::String("score : ") + juce::String(new_ui.results.score), juce::dontSendNotification);
            } break;
            case 2 :
            {
                auto *text_input = dynamic_cast<Frequency_Game_Text_Input*>(ui.center_panel.get());
                assert(text_input);
                text_input->text_input.setEnabled(!new_ui.freq_widget.is_cursor_locked);
                if (new_ui.freq_widget.display_target)
                {
                    text_input->text_input.setText(juce::String(new_ui.freq_widget.target_frequency));
                }
                if (new_ui.transition.in_transition == GameStep_Question)
                {
                    text_input->text_input.setText("");
                    text_input->text_input.grabKeyboardFocus();
                }
                if (new_ui.transition.out_transition == GameStep_Question)
                {
                    text_input->text_input.giveAwayKeyboardFocus();
                }
            };
        }
    }
    game_ui_bottom_update(&ui.bottom, new_ui.display_button, new_ui.button_text, new_ui.mix, new_ui.button_event);
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
        observer(effects);

    if (effects.quit)
    {
        io->on_quit();
    }
}

FrequencyGame_State frequency_game_state_init(FrequencyGame_Config config, 
                                              std::vector<Audio_File> files)
{
    assert(!files.empty());
    auto state = FrequencyGame_State {
        .files = std::move(files),
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
    int TEMP_answer_frequency = -1;

    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Click_Frequency :
        {
            check_answer = true;
            TEMP_answer_frequency = event.value_i;
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
                    if(state.lives > 0)
                        in_transition = GameStep_Result;
                    else
                        in_transition = GameStep_EndResults;
                }
            }
            else if (state.step == GameStep_Result && state.config.result_timeout_enabled)
            {
                if (state.current_timestamp >=
                    state.timestamp_start + state.config.result_timeout_ms)
                {
                    out_transition = GameStep_Result;
                    in_transition = GameStep_Question;
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
        auto clicked_ratio = normalize_frequency(TEMP_answer_frequency, state.config.min_f, state.config.num_octaves);
        auto target_ratio = normalize_frequency(state.target_frequency, state.config.min_f, state.config.num_octaves);
        auto distance = std::abs(clicked_ratio - target_ratio);
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
            
        if(state.lives > 0)
            in_transition = GameStep_Result;
        else
            in_transition = GameStep_EndResults;
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

            state.current_file_idx = -1;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            state.step = GameStep_Question;
            state.target_frequency = denormalize_frequency(juce::Random::getSystemRandom().nextFloat(), state.config.min_f, state.config.num_octaves);
    
            state.current_file_idx = random_positive_int((int)state.files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state.files[static_cast<size_t>(state.current_file_idx)] },
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
                effects.ui->header_right_text = juce::String("Score : ") + juce::String(state.score);
                effects.ui->ui_target = state.config.input == Frequency_Input_Widget ? 0 : 2;
                effects.ui->freq_widget.display_target = false;
                effects.ui->freq_widget.is_cursor_locked = false;
                effects.ui->freq_widget.display_window = true;
                effects.ui->freq_widget.correct_answer_window = state.correct_answer_window;

                effects.ui->header_center_text = juce::String("Lives : ") + juce::String(state.lives);
                effects.ui->display_button = false;
            } break;
            case GameStep_Result :
            {
                effects.ui->header_right_text = juce::String("Score : ") + juce::String(state.score);
        
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
                    effects.ui->freq_widget.locked_cursor_frequency = -1;
                }
                else 
                    jassertfalse;

                effects.ui->header_center_text = juce::String("Lives : ") + juce::String(state.lives);
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

void frequency_widget_update(FrequencyWidget *widget, const Frequency_Game_Effect_UI &new_ui)
{
    widget->display_target = new_ui.freq_widget.display_target;
    widget->target_frequency = new_ui.freq_widget.target_frequency;
    widget->is_cursor_locked = new_ui.freq_widget.is_cursor_locked;
    widget->locked_cursor_frequency = new_ui.freq_widget.locked_cursor_frequency;
    widget->display_window = new_ui.freq_widget.display_window;
    widget->correct_answer_window = new_ui.freq_widget.correct_answer_window;
    widget->min_f = new_ui.freq_widget.min_f;
    widget->num_octaves = new_ui.freq_widget.num_octaves;
    widget->repaint();
}