
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
//#include "CompressorWidget.h"
#include "CompressorGame.h"

CompressorGame_Config compressor_game_config_default(juce::String name)
{
    return {
        .title = name,
        .prelisten_type = PreListen_None,
        .prelisten_timeout_ms = 1000,
        .question_timeout_enabled = false,
        .question_timeout_ms = 1000,
        .result_timeout_enabled = true,
        .result_timeout_ms = 1000
    };
}


void compressor_game_ui_transitions(CompressorGame_UI &ui, Effect_Transition transition)
{
    if (transition.out_transition == GameStep_Begin)
    {
#if 0
        ui.compressor_widget = std::make_unique < CompressorWidget > ();
        ui.compressor_widget->onClick = [state = ui.game_state, io = ui.game_io] (int compressor) {
            Event event = {
                .type = Event_Click_Compressor,
                .value_i = compressor
            };
            compressor_game_post_event(state, io, event);
        };
        ui.addAndMakeVisible(*ui.compressor_widget);
#endif
        ui.resized();
    }
    if (transition.in_transition == GameStep_EndResults)
    {
#if 0
        ui.compressor_widget.reset();
        ui.results_panel = std::make_unique < CompressorGame_Results_Panel > ();
        ui.addAndMakeVisible(*ui.results_panel);
#endif
        ui.resized();
    }
}

void compressor_game_ui_update(CompressorGame_UI &ui, const Compressor_Game_Effect_UI &new_ui)
{
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
#if 0
    if (ui.compressor_widget)
    {
        compressor_widget_update(ui.compressor_widget.get(), new_ui);
    }
    else if (ui.results_panel)
    {
        ui.results_panel->score_label.setText(juce::String("score : ") + juce::String(new_ui.results.score), juce::dontSendNotification);
    }
#endif
    game_ui_bottom_update(&ui.bottom, new_ui.display_button, new_ui.button_text, new_ui.mix, new_ui.button_event);
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
    for(auto &observer : io->observers)
        observer(effects);

    if (effects.quit)
    {
        io->on_quit();
    }
}

CompressorGame_State compressor_game_state_init(CompressorGame_Config config, 
                                                std::vector<Audio_File> files)
{
    assert(!files.empty());
    auto state = CompressorGame_State {
        .files = std::move(files),
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


    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
#if 0
        case Event_Click_Frequency :
        {
            if (state.step != GameStep_Question) return { .error = 1 };
            auto clicked_freq = event.value_i;
            auto clicked_ratio = normalize_compressor(clicked_freq);
            auto target_ratio = normalize_compressor(state.target_compressor);
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
        } break;
#endif
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
            if (state.step == GameStep_Question && state.config.question_timeout_enabled)
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
#if 0
            state.correct_answer_window = state.config.initial_correct_answer_window;
#endif
            state.current_file_idx = -1;
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            state.step = GameStep_Question;
#if 0
            state.target_compressor = denormalize_compressor(juce::Random::getSystemRandom().nextFloat());
#endif
            state.current_file_idx = juce::Random::getSystemRandom().nextInt((int)state.files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state.files[static_cast<size_t>(state.current_file_idx)].file },
                    { .type = Audio_Command_Play },
                }
            };
            
            if (state.config.question_timeout_enabled)
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
#if 0
        float compensation_gain = state.config.eq_gain > 1.0f ? 
            1.0f / state.config.eq_gain :
            1.0f;
        dsp.gain = compensation_gain;
#endif
        switch (state.step)
        {
            case GameStep_Begin :
            {
            } break;
            case GameStep_Question :
            case GameStep_Result :
            {
#if 0
                dsp.eq_bands[0].type = Filter_Peak;
                dsp.eq_bands[0].compressor = (float)state.target_compressor;
                dsp.eq_bands[0].gain = state.config.eq_gain;
                dsp.eq_bands[0].quality = state.config.eq_quality;
#endif
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
        Compressor_Game_Effect_UI effect_ui;
        switch (state.step)
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
#if 0
                effect_ui.freq_widget.display_target = false;
                effect_ui.freq_widget.is_cursor_locked = false;
                effect_ui.freq_widget.display_window = true;
                effect_ui.freq_widget.correct_answer_window = state.correct_answer_window;
#endif

                effect_ui.header_text = juce::String("Lives : ") + juce::String(state.lives);
                effect_ui.display_button = false;
            } break;
            case GameStep_Result :
            {
#if 0
                effect_ui.freq_widget.display_target = true;
                effect_ui.freq_widget.target_compressor = state.target_compressor;
                if (event.type == Event_Click_Compressor)
                {
                    effect_ui.freq_widget.is_cursor_locked = true;
                    effect_ui.freq_widget.locked_cursor_compressor = event.value_i;
                    effect_ui.freq_widget.display_window = true;
                    effect_ui.freq_widget.correct_answer_window = state.correct_answer_window;
                } 
                else if (event.type == Event_Timer_Tick)
                {
                    effect_ui.freq_widget.is_cursor_locked = true;
                    effect_ui.freq_widget.locked_cursor_compressor = -1;
                }
                else 
                    jassertfalse;
#endif

                effect_ui.header_text = juce::String("Lives : ") + juce::String(state.lives);
                effect_ui.display_button = true;
                effect_ui.button_text = "Next";
                effect_ui.button_event = Event_Click_Next;
            } break;
            case GameStep_EndResults :
            {
                effect_ui.results.score = state.score;
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
        effect_ui.score = state.score;
        effects.ui = effect_ui;
    }

    effects.new_state = state;
    return effects;
}

void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer)
{
    io->observers.push_back(std::move(observer));
}

#if 0
void compressor_widget_update(CompressorWidget *widget, const Compressor_Game_Effect_UI &new_ui)
{
    widget->display_target = new_ui.freq_widget.display_target;
    widget->target_compressor = new_ui.freq_widget.target_compressor;
    widget->is_cursor_locked = new_ui.freq_widget.is_cursor_locked;
    widget->locked_cursor_compressor = new_ui.freq_widget.locked_cursor_compressor;
    widget->display_window = new_ui.freq_widget.display_window;
    widget->correct_answer_window = new_ui.freq_widget.correct_answer_window;
    widget->repaint();
}
#endif
