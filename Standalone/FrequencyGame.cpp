
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
#include "FrequencyWidget.h"
#include "FrequencyGame.h"

FrequencyGame_Config frequency_game_config_default(juce::String name)
{
    return {
        .title = name,
        .eq_gain = 4.0f,
        .eq_quality = 0.7f,
        .initial_correct_answer_window = 0.15f,
        .result_timeout_enabled = true,
        .result_timeout_ms = 1000
    };
}


void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition)
{
    if (transition.out_transition == GameStep_Begin)
    {
        ui.frequency_widget = std::make_unique < FrequencyWidget > ();
        ui.frequency_widget->onClick = [state = ui.game_state, io = ui.game_io] (int frequency) {
            Event event = {
                .type = Event_Click_Frequency,
                .value_i = frequency
            };
            frequency_game_post_event(state, io, event);
        };
        ui.addAndMakeVisible(*ui.frequency_widget);
        ui.resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
        ui.frequency_widget.reset();
        ui.results_panel = std::make_unique < FrequencyGame_Results_Panel > ();
        ui.addAndMakeVisible(*ui.results_panel);
        ui.resized();
    }
}

void frequency_game_ui_update(FrequencyGame_UI &ui, const Effect_UI &new_ui)
{
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
    if (ui.frequency_widget)
    {
        frequency_widget_update(ui.frequency_widget.get(), new_ui);
    }
    else if (ui.results_panel)
    {
        ui.results_panel->score_label.setText(juce::String("score : ") + juce::String(new_ui.results.score), juce::dontSendNotification);
    }
    game_ui_bottom_update(&ui.bottom, new_ui.display_button, new_ui.button_text, new_ui.mix, new_ui.button_event);
}

void frequency_game_post_event(FrequencyGame_State *state, FrequencyGame_IO *io, Event event)
{
    Effects effects;
    {
        std::lock_guard lock { io->update_fn_mutex };
        effects = frequency_game_update(state, event);
    }
    for(auto &observer : io->observers)
        observer(effects);

    if (effects.quit)
    {
        io->on_quit();
    }
}

std::unique_ptr<FrequencyGame_State> frequency_game_state_init(FrequencyGame_Config config, 
                                                               std::vector<Audio_File> files)
{
    assert(!files.empty());
    auto state = FrequencyGame_State {
        .files = std::move(files),
        .config = config,
        .timestamp_start = -1
    };

    return std::make_unique < FrequencyGame_State > (std::move(state));
}

std::unique_ptr<FrequencyGame_IO> frequency_game_io_init()
{
    return std::make_unique<FrequencyGame_IO>();
}

Effects frequency_game_update(FrequencyGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    GameStep in_transition = GameStep_None;
    GameStep out_transition = GameStep_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
        .dsp = std::nullopt, 
        .player = std::nullopt, 
        .ui = std::nullopt, 
        .quit = false, 
    };


    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Click_Frequency :
        {
            assert(old_step == GameStep_Question);
            auto clicked_freq = event.value_i;
            auto clicked_ratio = normalize_frequency(clicked_freq);
            auto target_ratio = normalize_frequency(state->target_frequency);
            auto distance = std::abs(clicked_ratio - target_ratio);
            if (distance < state->correct_answer_window)
            {
                int points_scored = int((1.0f - distance) * 100.0f);
                state->score += points_scored;
                state->correct_answer_window *= 0.95f;
            }
            else
            {
                state->lives--;
            }
            
            if(state->lives > 0)
                in_transition = GameStep_Result;
            else
                in_transition = GameStep_EndResults;
            out_transition = GameStep_Question;
        } break;
        case Event_Toggle_Input_Target :
        {
#if 0
            assert(old_step != GameStep_Begin);
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
            state->current_timestamp = event.value_i64;
            if (step == GameStep_Question && state->config.question_timeout_enabled)
            {
                if (state->current_timestamp >=
                    state->timestamp_start + state->config.question_timeout_ms)
                {
                    state->lives--;
            
                    out_transition = GameStep_Question;
                    if(state->lives > 0)
                        in_transition = GameStep_Result;
                    else
                        in_transition = GameStep_EndResults;
                }
            }
            else if (step == GameStep_Result && state->config.result_timeout_enabled)
            {
                if (state->current_timestamp >=
                    state->timestamp_start + state->config.result_timeout_ms)
                {
                    out_transition = GameStep_Result;
                    in_transition = GameStep_Question;
                }
            }
            else
            {
                assert(state->timestamp_start == -1);
            }
        } break;
        case Event_Click_Begin :
        {
            assert(old_step == GameStep_Begin);
            out_transition = GameStep_Begin;
            in_transition = GameStep_Question;
        } break;
        case Event_Click_Next :
        {
            assert(old_step == GameStep_Result);
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
        case Event_Channel_Create :
        case Event_Channel_Delete :
        case Event_Channel_Rename_From_Track :
        case Event_Channel_Rename_From_UI :
        case Event_Change_Frequency_Range :
        {
            jassertfalse;
        } break;
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
            state->timestamp_start = -1;
        } break;
        case GameStep_Result :
        {
            state->timestamp_start = -1;
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
            step = GameStep_Begin;
            state->score = 0;
            state->lives = 5;
            state->correct_answer_window = state->config.initial_correct_answer_window;
            state->current_file_idx = -1;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            step = GameStep_Question;
            state->target_frequency = denormalize_frequency(juce::Random::getSystemRandom().nextFloat());
    
            state->current_file_idx = juce::Random::getSystemRandom().nextInt((int)state->files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state->files[state->current_file_idx].file },
                    { .type = Audio_Command_Play },
                }
            };
            
            if (state->config.question_timeout_enabled)
            {
                state->timestamp_start = state->current_timestamp;
            }
            else 
                assert(state->timestamp_start == -1);
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            step = GameStep_Result;
            if (state->config.result_timeout_enabled)
            {
                state->timestamp_start = state->current_timestamp;
            }
            else 
                assert(state->timestamp_start == -1);
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        {
            step = GameStep_EndResults;
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Stop },
                }
            };
            state->results = {
                .score = state->score,
                .analytics = juce::Random::getSystemRandom().nextFloat()
            };
            effects.results = state->results;
            update_audio = true;
            update_ui = true;
        }break;
    }

    if (update_audio)
    {
        Channel_DSP_State dsp = ChannelDSP_on();
        
        float compensation_gain = state->config.eq_gain > 1.0f ? 
            1.0f / state->config.eq_gain :
            1.0f;
        dsp.gain = compensation_gain;

        switch (step)
        {
            case GameStep_Begin :
            {
            } break;
            case GameStep_Question :
            case GameStep_Result :
            {
                dsp.bands[0].type = Filter_Peak;
                dsp.bands[0].frequency = (float)state->target_frequency;
                dsp.bands[0].gain = state->config.eq_gain;
                dsp.bands[0].quality = state->config.eq_quality;
            } break;
            case GameStep_EndResults :
            {
            } break;
            case GameStep_None :
            {
                jassertfalse;
            } break;
        }
        effects.dsp = Effect_DSP { dsp };
    }

    if (update_ui)
    {
        Effect_UI effect_ui;
        switch (step)
        {
            case GameStep_Begin :
            {
                effect_ui.header_text = "Ready ?";
                effect_ui.display_button = true;
                effect_ui.button_text = "Begin";
                effect_ui.button_event = Event_Click_Begin;
            } break;
            case GameStep_Question :
            {
                effect_ui.freq_widget.display_target = false;
                effect_ui.freq_widget.is_cursor_locked = false;
                effect_ui.freq_widget.display_window = true;
                effect_ui.freq_widget.correct_answer_window = state->correct_answer_window;

                effect_ui.header_text = juce::String("Lives : ") + juce::String(state->lives);
                effect_ui.display_button = false;
            } break;
            case GameStep_Result :
            {
                effect_ui.freq_widget.display_target = true;
                effect_ui.freq_widget.target_frequency = state->target_frequency;
                if (event.type == Event_Click_Frequency)
                {
                    effect_ui.freq_widget.is_cursor_locked = true;
                    effect_ui.freq_widget.locked_cursor_frequency = event.value_i;
                    effect_ui.freq_widget.display_window = true;
                    effect_ui.freq_widget.correct_answer_window = state->correct_answer_window;
                } 
                else if (event.type == Event_Timer_Tick)
                {
                    effect_ui.freq_widget.is_cursor_locked = true;
                    effect_ui.freq_widget.locked_cursor_frequency = -1;
                }
                else 
                    jassertfalse;

                effect_ui.header_text = juce::String("Lives : ") + juce::String(state->lives);
                effect_ui.display_button = true;
                effect_ui.button_text = "Next";
                effect_ui.button_event = Event_Click_Next;
            } break;
            case GameStep_EndResults :
            {
                effect_ui.results.score = state->score;
                effect_ui.header_text = "Results";
                effect_ui.display_button = true;
                effect_ui.button_text = "Quit";
                effect_ui.button_event = Event_Click_Quit;
            } break;
            case GameStep_None :
            {
                jassertfalse;
            } break;
        }

        effect_ui.mix = Mix_Hidden;
        effect_ui.score = state->score;
        effects.ui = effect_ui;
    }

    state->step = step;
    return effects;
}

void frequency_game_add_observer(FrequencyGame_IO *io, observer_t observer)
{
    io->observers.push_back(std::move(observer));
}

void frequency_widget_update(FrequencyWidget *widget, const Effect_UI &new_ui)
{
    widget->display_target = new_ui.freq_widget.display_target;
    widget->target_frequency = new_ui.freq_widget.target_frequency;
    widget->is_cursor_locked = new_ui.freq_widget.is_cursor_locked;
    widget->locked_cursor_frequency = new_ui.freq_widget.locked_cursor_frequency;
    widget->display_window = new_ui.freq_widget.display_window;
    widget->correct_answer_window = new_ui.freq_widget.correct_answer_window;
    widget->repaint();
}