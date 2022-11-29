constexpr int max_f = 12800;
constexpr int min_f = 50;
double power = 2.0;

int totalSections = 9;

int ratioToHz(float pos) {
    return int(min_f * std::pow(2, totalSections * pos));
}
float hzToRatio(int hz) {
    return float(std::log((double)hz / (double)min_f) / (totalSections * std::log(2.0)));
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
        return -1.0f;
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
            g.drawLine( { a, b } );
        };

        //outline
        g.setColour(juce::Colour(85, 85, 85));
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        
        auto mouse_position = getMouseXYRelative();
        auto mouse_ratio = ratioInRect(mouse_position, r);
        float cursor_ratio = is_cursor_locked ? 
            hzToRatio(locked_cursor_frequency) :
            mouse_ratio;
        int cursor_frequency = is_cursor_locked ? 
            locked_cursor_frequency : 
            positionToFrequency(mouse_position, r);
        juce::Line<int> cursor_line;


        g.setColour(juce::Colour(170, 170, 170));
        for (float line_freq : { 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f })
        {
            auto x = frequencyToX(line_freq, r);
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float>{ 30.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        if(cursor_frequency != -1.0f)
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
        if(display_target)
        {
            g.setColour(juce::Colours::black);
            auto x = frequencyToX(target_frequency, r);
            drawLineAt(0.0f, 0.0f, x);
        }
        //hover
#if 0
        if(cursor_frequency != -1.0f)
        {
            juce::Rectangle rect = { 200, 200 };
            rect.setCentre(mouse_position);
            g.setColour(juce::Colour(255, 240, 217));
            g.drawText(juce::String(cursor_frequency), rect, juce::Justification::centred, false);
        }
#endif
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

struct Effect_DSP {
    Channel_DSP_State dsp_state;
};

struct Effect_UI {
    //GameStep step; 
    
    bool display_target;
    int target_frequency;
    bool is_cursor_locked;
    int locked_cursor_frequency;
    bool display_window;
    float correct_answer_window;

    juce::String header_text;
    int score; 
    Mix mix;
    bool display_button;
    juce::String button_text;
    Event_Type button_event;
};

struct Effect_Player {
    Audio_Command command;
};


struct Effect_Timer {
    int timeout_ms;
    std::function<void()> callback;
};

struct Effects {
    std::optional < Effect_DSP> dsp;
    std::optional < Effect_Player > player;
    std::optional < Effect_UI > ui;
    bool quit;
    std::optional < Effect_Timer > timer;
};

using audio_observer_t = std::function<void(Effect_DSP)>;
using player_observer_t = std::function<void(Effect_Player)>;
using ui_observer_t = std::function<void(Effect_UI &)>;

struct FrequencyGame_State
{
    GameStep step;
    int score;
    int target_frequency;
    float correct_answer_window;
    std::vector<juce::File> file_list;
    std::vector<ui_observer_t> observers_ui;
    Timer timer;
    std::vector<audio_observer_t> observers_audio;
    std::vector<player_observer_t> observers_player;
};

struct FrequencyGame_UI;


void frequency_widget_update(FrequencyWidget *widget, Effect_UI &new_ui);
void frequency_game_ui_update(FrequencyGame_UI &ui, Effect_UI &new_ui);
Effects frequency_game_update(FrequencyGame_State *state, Event event);
void frequency_game_post_event(FrequencyGame_State *state, Event event);


struct FrequencyGame_UI : public juce::Component
{
    
    FrequencyGame_UI(FrequencyGame_State *state) 
    : state(state)
    {
        frequency_widget.onClick = [state] (int frequency) {
            Event event = {
                .type = Event_Click_Frequency,
                .value_i = frequency
            };
            frequency_game_post_event(state, event);
        };

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
        addAndMakeVisible(frequency_widget);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        auto bottom_height = 50;
        auto header_height = 20;

        auto header_bounds = bounds.withHeight(header_height);
        auto game_bounds = bounds.withTrimmedBottom(bottom_height).withTrimmedTop(header_height);
        auto bottom_bounds = bounds.withTrimmedTop(bounds.getHeight() - bottom_height);
        
        header.setBounds(header_bounds);
        frequency_widget.setBounds(game_bounds);
        bottom.setBounds(bottom_bounds);
    }

    FrequencyWidget frequency_widget;
    GameUI_Header header;
    GameUI_Bottom bottom;
    FrequencyGame_State *state;
};

void frequency_game_ui_update(FrequencyGame_UI &ui, Effect_UI &new_ui)
{
    game_ui_header_update(&ui.header, new_ui.header_text, new_ui.score);
    frequency_widget_update(&ui.frequency_widget, new_ui);
    game_ui_bottom_update(&ui.bottom, new_ui.button_text, new_ui.mix, new_ui.button_event);
}



void frequency_game_post_event(FrequencyGame_State *state, Event event)
{
    Effects effects = frequency_game_update(state, event);
    if (effects.dsp)
    {
        for(auto &observer : state->observers_audio)
            observer(*effects.dsp);
    }
    if (effects.ui)
    {
        for(auto &observer : state->observers_ui)
            observer(*effects.ui);
    }
    if (effects.timer)
    {
        jassert(!state->timer.isTimerRunning());
        state->timer.callback = std::move(effects.timer->callback); 
        state->timer.startTimer(effects.timer->timeout_ms);
    }
    if (effects.player)
    {
        for(auto &observer : state->observers_player)
            observer(*effects.player);
    }
    if (effects.quit)
    {
        //state->app->quitGame();
    }
}

std::unique_ptr<FrequencyGame_State> frequency_game_state_init(std::vector<juce::File> file_list)
{
    if(file_list.empty())
        return nullptr;
    auto state = FrequencyGame_State {
        .file_list = std::move(file_list)
    };
    return std::make_unique < FrequencyGame_State > (std::move(state));
}

Effects frequency_game_update(FrequencyGame_State *state, Event event)
{
    GameStep old_step = state->step;
    GameStep step = old_step;
    Transition transition = Transition_None;
    bool update_audio = false;
    bool update_ui = false;

    Effects effects = { 
        .dsp = std::nullopt, 
        .ui = std::nullopt, 
        .quit = false, 
        .timer = std::nullopt 
    };

    jassert(old_step != GameStep_Listening);
    jassert(old_step != GameStep_ShowingAnswer);

    switch (event.type) 
    {
        case Event_Init :
        {
            transition = Transition_To_Begin;
        } break;
        case Event_Click_Frequency :
        {
            jassert(old_step == GameStep_Editing);
            auto clicked_freq = event.value_i;
            auto clicked_ratio = hzToRatio(clicked_freq);
            auto target_ratio = hzToRatio(state->target_frequency);
            auto distance = std::abs(clicked_ratio - target_ratio);
            if (distance < state->correct_answer_window)
            {
                state->score++;
                state->correct_answer_window *= 0.95f;
            }

            transition = Transition_To_Answer;
        } break;
        case Event_Toggle_Input_Target :
        {
            jassert(old_step != GameStep_Begin);
            if(event.value_b && step == GameStep_Editing)
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
        } break;
        case Event_Timeout :
        {
            jassertfalse;
            //temp
        } break;
        case Event_Click_Begin :
        {
            jassert(old_step == GameStep_Begin);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Next :
        {
            jassert(old_step == GameStep_ShowingTruth);
            transition = Transition_To_Exercice;
        } break;
        case Event_Click_Back :
        {
            effects.quit = true;
        } break;
        case Event_Click_Quit :
        {
            jassertfalse;
            //unimplemented
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

    
    switch (transition)
    {
        case Transition_None : {
        }break;
        case Transition_To_Begin : {
            step = GameStep_Begin;
            state->score = 0;
            state->correct_answer_window = 0.15f;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Exercice : {
            step = GameStep_Editing;
            state->target_frequency = ratioToHz(juce::Random::getSystemRandom().nextFloat());
#if 0
            effects.timer = Effect_Timer {
                .timeout_ms = state->timeout_ms ,
                .callback = [state] {
                    frequency_game_post_event(state, Event { .type = Event_Timeout });
                }
            };
#endif
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_Answer : {
            step = GameStep_ShowingTruth;
            update_audio = true;
            update_ui = true;
        }break;
        case Transition_To_End_Result : {
        }break;
    }

    if (update_audio)
    {
        Channel_DSP_State dsp = {};
        effects.dsp = Effect_DSP { dsp };
    }

    if (update_ui)
    {
        Effect_UI effect_ui;
        switch (step)
        {
            case GameStep_Begin :
            {
                effect_ui.display_target = false;
                effect_ui.is_cursor_locked = false;
                effect_ui.display_window = false;

                effect_ui.header_text = "Ready ?";
                effect_ui.display_button = true;
                effect_ui.button_text = "Begin";
                effect_ui.button_event = Event_Click_Begin;
            } break;
            case GameStep_Listening :
            {
                jassertfalse;
            } break;
            case GameStep_Editing :
            {
                effect_ui.display_target = false;
                effect_ui.is_cursor_locked = false;
                effect_ui.display_window = true;
                effect_ui.correct_answer_window = state->correct_answer_window;

                effect_ui.header_text = "Find the boosted frequency";
                effect_ui.display_button = false;
            } break;
            case GameStep_ShowingTruth :
            {
                effect_ui.display_target = true;
                effect_ui.target_frequency = state->target_frequency;
                effect_ui.is_cursor_locked = true;
                jassert(event.type == Event_Click_Frequency);
                effect_ui.locked_cursor_frequency = event.value_i;
                effect_ui.display_window = true;
                effect_ui.correct_answer_window = state->correct_answer_window;

                effect_ui.header_text = "Results";
                effect_ui.display_button = true;
                effect_ui.button_text = "Next";
                effect_ui.button_event = Event_Click_Next;
            } break;
            case GameStep_ShowingAnswer : 
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

void frequency_game_add_ui_observer(FrequencyGame_State *state, ui_observer_t &&observer)
{
    state->observers_ui.push_back(std::move(observer));
}

void frequency_game_add_audio_observer(FrequencyGame_State *state, audio_observer_t &&observer)
{
    state->observers_audio.push_back(std::move(observer));
}

void frequency_game_add_player_observer(FrequencyGame_State *state, player_observer_t &&observer)
{
    state->observers_player.push_back(std::move(observer));
}

void frequency_widget_update(FrequencyWidget *widget, Effect_UI &new_ui)
{
    widget->display_target = new_ui.display_target;
    widget->target_frequency = new_ui.target_frequency;
    widget->is_cursor_locked = new_ui.is_cursor_locked;
    widget->locked_cursor_frequency = new_ui.locked_cursor_frequency;
    widget->display_window = new_ui.display_window;
    widget->correct_answer_window = new_ui.correct_answer_window;
    widget->repaint();
}