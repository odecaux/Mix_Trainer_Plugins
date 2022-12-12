
#include "../shared/pch.h"
#include "../shared/shared.h"
#include "../Game/Game.h"
//#include "CompressorWidget.h"
#include "CompressorGame.h"

CompressorGame_Config compressor_game_config_default(juce::String name)
{
    return {
        .title = name,
        .threshold_values_db = { -20.0f, -16.0f, -12.0f, -8.0f, -4.0f, 0.0f },
        .ratio_values = { 1.0f, 3.0f, 6.0f, 10.0f },
        .attack_values = { 1.0f, 10.0f, 30.0f, 60.0f, 100.0f },
        .release_values = { 1.0f, 50.0f, 100.0f, 150.0f },
        .variant = Compressor_Game_Normal,
        .listens = 0,
        .timeout_ms = 0,
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
    
    //ui.threshold_values = new_ui.comp_widget.threshold_values_db;
    ui.threshold_slider.setValue(static_cast<double>(new_ui.comp_widget.threshold_pos), juce::dontSendNotification);
    ui.threshold_slider.setRange(0.0, (double)((int)new_ui.comp_widget.threshold_values_db.size() - 1), 1.0);
    //ui.threshold_label.setText(juce::Decibels::toString(new_ui.comp_widget.threshold_values_db[new_ui.comp_widget.threshold_pos]), juce::dontSendNotification);
    ui.threshold_slider.get_text_from_value = [=] (double new_pos)
    {
        return juce::Decibels::toString(new_ui.comp_widget.threshold_values_db[static_cast<size_t>(new_pos)], 0);
    };
    //ui.ratio_values = new_ui.comp_widget.ratio_values;
    ui.ratio_slider.setValue(static_cast<double>(new_ui.comp_widget.ratio_pos), juce::dontSendNotification);
    ui.ratio_slider.setRange(0.0, (double)((int)new_ui.comp_widget.ratio_values.size() - 1), 1.0);
    //ui.ratio_label.setText(juce::String(new_ui.comp_widget.ratio_values[new_ui.comp_widget.ratio_pos]), juce::dontSendNotification);
    ui.ratio_slider.get_text_from_value = [=] (double new_pos)
    {
        return juce::String(new_ui.comp_widget.ratio_values[static_cast<size_t>(new_pos)]);
    };

    //ui.attack_values = new_ui.comp_widget.attack_values;
    ui.attack_slider.setValue(static_cast<double>(new_ui.comp_widget.attack_pos), juce::dontSendNotification);
    ui.attack_slider.setRange(0.0, (double)((int)new_ui.comp_widget.attack_values.size() - 1), 1.0);
    //ui.attack_label.setText(juce::String(new_ui.comp_widget.attack_values[new_ui.comp_widget.attack_pos]) + " ms", juce::dontSendNotification);
    ui.attack_slider.get_text_from_value = [=] (double new_pos)
    {
        return juce::String(new_ui.comp_widget.attack_values[static_cast<size_t>(new_pos)]) + " ms";
    };

    //ui.release_values = new_ui.comp_widget.release_values;
    ui.release_slider.setValue(static_cast<double>(new_ui.comp_widget.release_pos), juce::dontSendNotification);
    ui.release_slider.setRange(0.0, (double)((int)new_ui.comp_widget.release_values.size() - 1), 1.0);
    //ui.release_label.setText(juce::String(new_ui.comp_widget.release_values[new_ui.comp_widget.release_pos]) + " ms", juce::dontSendNotification);
    ui.release_slider.get_text_from_value = [=] (double new_pos)
    {
        return juce::String(new_ui.comp_widget.release_values[static_cast<size_t>(new_pos)]) + " ms";
    };

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

    bool done_listening = false;

    switch (event.type) 
    {
        case Event_Init :
        {
            in_transition = GameStep_Begin;
        } break;
        case Event_Slider : 
        {
            switch (event.id) 
            {
                case 0 : {
                    state.input_threshold_pos = event.value_i;
                } break;
                case 1 : {
                    state.input_ratio_pos = event.value_i;
                } break;
                case 2 : {
                    state.input_attack_pos = event.value_i;
                } break;
                case 3 : {
                    state.input_release_pos = event.value_i;
                } break;
            }
            update_audio = true;
        } break;
        case Event_Toggle_Input_Target : 
        {
            if (state.step == GameStep_Question)
            {
                if(state.config.variant == Compressor_Game_Timer) return { .error = 1 };
                if (!state.can_still_listen) return { .error = 1 };
                if (state.mix == Mix_User)
                {
                    if(!event.value_b) return { .error = 1 };
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) return { .error = 1 };
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
                    if(!event.value_b) return { .error = 1 };
                    state.mix = Mix_Target;
                }
                else if (state.mix == Mix_Target)
                {
                    if(event.value_b) return { .error = 1 };
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
                if(state.timestamp_start != -1) return { .error = 1 };
            }
        } break;
        case Event_Click_Begin :
        {
            if(state.step != GameStep_Begin) return { .error = 1 };
            if(state.mix != Mix_Hidden) return { .error = 1 };
            if (state.target_ratio_pos != -1 ||
                state.target_threshold_pos != -1 ||
                state.target_attack_pos != -1 ||
                state.target_release_pos != -1)
            {
                return { .error = 1 };
            }
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
        case Event_Click_Answer :
        case Event_Click_Done_Listening :
        {
            jassertfalse;
        } break;
    }
    
    
    if (done_listening)
    {
        if(state.step != GameStep_Question) return { .error = 1 };
        if(state.mix != Mix_Target) return { .error = 1 };
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
            state.lives = 5;
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
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Question : {
            state.step = GameStep_Question;

            state.target_threshold_pos = random_int(static_cast<int>(state.config.threshold_values_db.size()));
            state.target_ratio_pos = random_int(static_cast<int>(state.config.ratio_values.size()));
            state.target_attack_pos = random_int(static_cast<int>(state.config.attack_values.size()));
            state.target_release_pos = random_int(static_cast<int>(state.config.release_values.size()));

            //TODO
            state.input_threshold_pos = static_cast<int>(state.config.threshold_values_db.size()) - 1;
            state.input_ratio_pos = 0;
            state.input_attack_pos = 0;
            state.input_release_pos = 0;

            state.current_file_idx = random_int((int)state.files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state.files[static_cast<size_t>(state.current_file_idx)].file },
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

#if 0
            if (state.config.question_timeout_enabled)
            {
                state.timestamp_start = state.current_timestamp;
            }
            else
            {
                if (state.timestamp_start != -1) return { .error = 1 };
            }
#endif
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            state.step = GameStep_Result;
            state.mix = Mix_Target;
#if 0
            if (state.config.result_timeout_enabled)
            {
                state.timestamp_start = state.current_timestamp;
            }
            else
            {
                if (state.timestamp_start != -1) return { .error = 1 };
            }
#endif
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_EndResults : 
        {
#if 0
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
#endif
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
                float threshold_gain = juce::Decibels::decibelsToGain(state.config.threshold_values_db[threshold_pos]);
                dsp.comp = {
                    .is_on = true,
                    .threshold_gain = threshold_gain,
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
        Compressor_Game_Effect_UI effect_ui;
        effect_ui.comp_widget = {
            .threshold_pos = threshold_pos,
            .ratio_pos = ratio_pos,
            .attack_pos = attack_pos,
            .release_pos = release_pos,
            .threshold_values_db = state.config.threshold_values_db,
            .ratio_values = state.config.ratio_values,
            .attack_values = state.config.attack_values,
            .release_values = state.config.release_values
        };

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
