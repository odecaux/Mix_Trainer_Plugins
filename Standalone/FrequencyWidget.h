constexpr int max_f = 12800;
constexpr int min_f = 50;

int totalSections = 9;

int ratioToHz(float ratio) {
    double exponent = 0.5 + (double)ratio * double(totalSections - 1);
    return int(min_f * std::pow(2, exponent));
}
float hzToRatio(int hz) {
    double a = std::log((double)hz / (double)min_f) / std::log(2.0);
    double ratio = (a - 0.5) / double(totalSections - 1);
    return (float)ratio;
}

float frequencyToX(int frequency, juce::Rectangle<int> bounds)
{
    //float ratio_x = std::powf((frequency - min_f) / (max_f - min_f), 1.0f / power);
    float ratio_x = hzToRatio(frequency);
    return ratio_x * bounds.getWidth() + bounds.getX();
}

float ratioInRect(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    return float(relative.x) / float(bounds.getWidth());
}

int positionToFrequency(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    auto ratio = ratioInRect(position, bounds);
    if(ratio == -1.0f)
        return -1;
    return ratioToHz(ratio);
}

struct FrequencyWidget : public juce::Component 
{
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds();

        auto drawLineAt = [&] (float marginTop, float marginBottom, float x) {
            auto a = juce::Point<float>(x, (float)r.getY() + marginTop);
            auto b = juce::Point<float>(x, (float)r.getBottom() - marginBottom);
            g.drawLine( { a, b });
        };

        //outline
        g.setColour(juce::Colour(85, 85, 85));
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        
        //frequency guide lines
        g.setColour(juce::Colour(170, 170, 170));
        for (int line_freq : { 100, 200, 400, 800, 1600, 3200, 6400, 12800 })
        {
            auto x = frequencyToX(line_freq, r);
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float> { 40.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        
        {
            auto mouse_position = getMouseXYRelative();
            auto mouse_ratio = ratioInRect(mouse_position, r);
            float cursor_ratio = is_cursor_locked ?
                hzToRatio(locked_cursor_frequency) :
                mouse_ratio;
            int cursor_frequency = is_cursor_locked ?
                locked_cursor_frequency :
                positionToFrequency(mouse_position, r);
            juce::Line<int> cursor_line;
    
            //cursor
            if (cursor_frequency != -1.0f)
            {
                float window_right = std::min(1.0f, cursor_ratio + correct_answer_window);
                float window_left = std::max(0.0f, cursor_ratio - correct_answer_window);
                auto window_right_x = r.getX() + r.getWidth() * window_right;
                auto window_left_x = r.getX() + r.getWidth() * window_left;
    
                auto a = juce::Point<float>(window_left_x, (float)r.getY());
                auto b = juce::Point<float>(window_right_x, (float)r.getBottom());
                auto window_rect = juce::Rectangle<float> { a, b };
                if (display_window)
                {
                    g.setColour(juce::Colours::yellow);
                    g.setOpacity(0.5f);
                    g.fillRect(window_rect);
                }
                g.setOpacity(1.0f);
                g.setColour(juce::Colours::black);
                auto cursor_x = frequencyToX(cursor_frequency, r);
                drawLineAt(0.0f, 0.0f, cursor_x);
    
                juce::Rectangle text_bounds = { 200, 200 };
                text_bounds.setCentre(window_rect.getCentre().toInt());
                g.setColour(juce::Colour(255, 240, 217));
                g.drawText(juce::String(cursor_frequency), text_bounds, juce::Justification::centred, false);
            }
            
            //target
            if (display_target)
            {
                g.setColour(juce::Colours::black);
                auto x = frequencyToX(target_frequency, r);
                drawLineAt(0.0f, 0.0f, x);
            }
    #if 0
            //cursor text
            if (cursor_frequency != -1.0f)
            {
                juce::Rectangle rect = { 200, 200 };
                rect.setCentre(mouse_position);
                g.setColour(juce::Colour(255, 240, 217));
                g.drawText(juce::String(cursor_frequency), rect, juce::Justification::centred, false);
            }
    #endif
    
        }
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(r.toFloat(), 8.0f, 2.0f);
    }

    
private:
    void mouseMove (const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        //static_assert(false); //mouse_move
        repaint();
    }
    
    
    void mouseDrag (const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        //static_assert(false); //mouse_move ?
        repaint();
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        auto clicked_freq = positionToFrequency(getMouseXYRelative(), getLocalBounds());
        onClick(clicked_freq);
    }

public:
    bool display_target;
    int target_frequency;
    bool is_cursor_locked;
    int locked_cursor_frequency;
    bool display_window;
    float correct_answer_window;
    std::function < void(int) > onClick;
};

struct FrequencyGame_Results_Panel : public juce::Component
{
    FrequencyGame_Results_Panel()
    {
        score_label.setSize(100, 50);
        addAndMakeVisible(score_label);
    }

    void resized()
    {
        auto r = getLocalBounds();
        score_label.setCentrePosition(r.getCentre());
    }
    juce::Label score_label;
};

struct FrequencyGame_Results
{
    int score;
    float analytics;
    //temps moyen ? distance moyenne ? stats ?
};

struct Effect_Transition {
    GameStep in_transition;
    GameStep out_transition;
};

struct Effect_DSP {
    Channel_DSP_State dsp_state;
};

struct Effect_UI {
    struct {
        bool display_target;
        int target_frequency;
        bool is_cursor_locked;
        int locked_cursor_frequency;
        bool display_window;
        float correct_answer_window;
    } freq_widget;
    FrequencyGame_Results results;
    juce::String header_text;
    int score;
    Mix mix;
    bool display_button;
    juce::String button_text;
    Event_Type button_event;
};

struct Effect_Player {
    std::vector<Audio_Command> commands;
};

struct Effects {
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP > dsp;
    std::optional < Effect_Player > player;
    std::optional < Effect_UI > ui;
    std::optional < FrequencyGame_Results > results;
    bool quit;
};

struct FrequencyGame_Config
{
    juce::String title;
    float eq_gain;
    float eq_quality;
    float initial_correct_answer_window;
    bool next_question_timeout_enabled;
    int next_question_timeout_ms;
};

FrequencyGame_Config frequency_game_config_default(juce::String name)
{
    return {
        .title = name,
        .eq_gain = 4.0f,
        .eq_quality = 0.7f,
        .initial_correct_answer_window = 0.15f,
        .next_question_timeout_enabled = true,
        .next_question_timeout_ms = 1000
    };
}

struct Audio_File
{
    juce::File file;
    juce::String title;
    juce::Range<juce::int64> loop_bounds;
    juce::Range<int> freq_bounds;
};

using observer_t = std::function<void(const Effects &)>;

struct FrequencyGame_State
{
    GameStep step;
    int score;
    int lives;
    int target_frequency;
    float correct_answer_window;
    int current_file_idx;
    std::vector<Audio_File> files;
    FrequencyGame_Config config;
    FrequencyGame_Results results;

    juce::int64 timestamp_start;
    std::unique_ptr<std::mutex> update_fn_mutex;
    Timer timer;
    std::vector<observer_t> observers;
    std::function < void() > on_quit;
};

struct FrequencyGame_UI;


void frequency_widget_update(FrequencyWidget *widget, const Effect_UI &new_ui);
void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition);
void frequency_game_ui_update(FrequencyGame_UI &ui, const Effect_UI &new_ui);
Effects frequency_game_update(FrequencyGame_State *state, Event event);
void frequency_game_post_event(FrequencyGame_State *state, Event event);


struct FrequencyGame_UI : public juce::Component
{
    
    FrequencyGame_UI(FrequencyGame_State *state) 
    : state(state)
    {
        bottom.onNextClicked = [state] (Event_Type e){
            Event event = {
                .type = e
            };
            frequency_game_post_event(state, event);
        };
        header.onBackClicked = [state] {
            Event event = {
                .type = Event_Click_Back
            };
            frequency_game_post_event(state, event);
        };
        bottom.onToggleClicked = [state] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            frequency_game_post_event(state, event);
        };
        addAndMakeVisible(header);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds().reduced(4);
        auto bottom_height = 50;

        auto header_bounds = bounds.removeFromTop(game_ui_header_height);
        auto bottom_bounds = bounds.removeFromBottom(bottom_height);
        auto game_bounds = bounds.withTrimmedTop(4).withTrimmedBottom(4);

        header.setBounds(header_bounds);
        if(frequency_widget)
            frequency_widget->setBounds(game_bounds);
        else if(results_panel)
            results_panel->setBounds(game_bounds);
        bottom.setBounds(bottom_bounds);
    }

    GameUI_Header header;
    std::unique_ptr<FrequencyWidget> frequency_widget;
    std::unique_ptr<FrequencyGame_Results_Panel> results_panel;
    GameUI_Bottom bottom;
    FrequencyGame_State *state;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGame_UI)
};

void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition)
{
    if (transition.out_transition == GameStep_Begin)
    {
        ui.frequency_widget = std::make_unique < FrequencyWidget > ();
        ui.frequency_widget->onClick = [state = ui.state] (int frequency) {
            Event event = {
                .type = Event_Click_Frequency,
                .value_i = frequency
            };
            frequency_game_post_event(state, event);
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

void frequency_game_post_event(FrequencyGame_State *state, Event event)
{
    Effects effects;
    {
        std::lock_guard lock { *state->update_fn_mutex };
        effects = frequency_game_update(state, event);
    }
    for(auto &observer : state->observers)
        observer(effects);

    if (effects.quit)
    {
        state->timer.stopTimer();
        state->on_quit();
    }
}

std::unique_ptr<FrequencyGame_State> frequency_game_state_init(FrequencyGame_Config config, 
                                                               std::vector<Audio_File> files,
                                                               std::function<void()> on_quit)
{
    assert(!files.empty());
    auto state = FrequencyGame_State {
        .files = std::move(files),
        .config = config,
        .timestamp_start = -1,
        .update_fn_mutex = std::make_unique<std::mutex>(),
        .on_quit = std::move(on_quit)
    };

    auto state_ptr = std::make_unique < FrequencyGame_State > (std::move(state));
    state_ptr->timer.callback = [state = state_ptr.get()] {
        frequency_game_post_event(state, Event {.type = Event_Timer_Tick});
    };
    state_ptr->timer.startTimerHz(60);
    return state_ptr;
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
            auto clicked_ratio = hzToRatio(clicked_freq);
            auto target_ratio = hzToRatio(state->target_frequency);
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
            if (step == GameStep_Result)
            {
                auto current_time_ms = juce::Time::currentTimeMillis();
                if (current_time_ms >=
                    state->timestamp_start + state->config.next_question_timeout_ms)
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
            state->target_frequency = ratioToHz(juce::Random::getSystemRandom().nextFloat());
    
            state->current_file_idx = juce::Random::getSystemRandom().nextInt((int)state->files.size());
            effects.player = Effect_Player {
                .commands = { 
                    { .type = Audio_Command_Load, .value_file = state->files[state->current_file_idx].file },
                    { .type = Audio_Command_Play },
                }
            };
            
            update_audio = true;
            update_ui = true;
        }break;
        case GameStep_Result : 
        {
            step = GameStep_Result;
            auto current_time_ms = juce::Time::currentTimeMillis();
            state->timestamp_start = current_time_ms;
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
                effect_ui.freq_widget.is_cursor_locked = true;
                assert(event.type == Event_Click_Frequency);
                effect_ui.freq_widget.locked_cursor_frequency = event.value_i;
                effect_ui.freq_widget.display_window = true;
                effect_ui.freq_widget.correct_answer_window = state->correct_answer_window;

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
        }

        effect_ui.mix = Mix_Hidden;
        effect_ui.score = state->score;
        effects.ui = effect_ui;
    }

    state->step = step;
    return effects;
}

void frequency_game_add_observer(FrequencyGame_State *state, observer_t &&observer)
{
    state->observers.push_back(std::move(observer));
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