struct FrequencyGame_UI;
struct FrequencyWidget;

void frequency_game_ui_transitions(FrequencyGame_UI *ui, Effect_Transition transition, int ui_target);
void frequency_game_ui_update(FrequencyGame_UI *ui, Frequency_Game_Effect_UI *new_ui);
void frequency_widget_update(FrequencyWidget *widget, Frequency_Game_Effect_UI *new_ui);


static uint32_t denormalize_frequency(float ratio, uint32_t min_f, uint32_t num_octaves) 
{
    float exponent = ratio * static_cast<float>(num_octaves);
    return static_cast<uint32_t>(static_cast<float>(min_f) * powf(2, exponent));
}

static float normalize_frequency(uint32_t hz, uint32_t min_f, uint32_t num_octaves) 
{
    float a = logf(static_cast<float>(hz) / min_f) / logf(2.0f);
    float ratio = (a) / static_cast<float>(num_octaves);
    return ratio;
}

static float frequency_to_x(uint32_t frequency, juce::Rectangle<int> bounds, uint32_t min_f, uint32_t num_octaves)
{
    float ratio_x = normalize_frequency(frequency, min_f, num_octaves);
    return ratio_x * static_cast<float>(bounds.getWidth()) + static_cast<float>(bounds.getX());
}

static float normalize_position(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    return float(relative.x) / float(bounds.getWidth());
}

static uint32_t x_to_frequency(juce::Point<int> position, juce::Rectangle<int> bounds, uint32_t  min_f, uint32_t num_octaves)
{
    auto ratio = normalize_position(position, bounds);
    if(ratio == -1.0f)
        return 0;
    return denormalize_frequency(ratio, min_f, num_octaves);
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
        for (int line_freq : { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800 })
        {
            auto x = frequency_to_x(line_freq, r, min_frequency, num_octaves);
            if (x <= 0)
                continue;
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float> { 40.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        
        {
            auto mouse_position = getMouseXYRelative();
            auto mouse_ratio = normalize_position(mouse_position, r);
            float cursor_ratio = is_cursor_locked ?
                normalize_frequency(locked_cursor_frequency, min_frequency, num_octaves) :
                mouse_ratio;
            uint32_t cursor_frequency = is_cursor_locked ?
                locked_cursor_frequency :
                x_to_frequency(mouse_position, r, min_frequency, num_octaves);
            
            //juce::Line<int> cursor_line;
    
            //cursor
            if (cursor_frequency != 0)
            {
                float window_left = std::max(0.0f, cursor_ratio - correct_answer_window);
                float window_right = std::min(1.0f, cursor_ratio + correct_answer_window);
                float window_left_x = static_cast<float>(r.getX()) + static_cast<float>(r.getWidth()) * window_left;
                float window_right_x = static_cast<float>(r.getX()) + static_cast<float>(r.getWidth()) * window_right;
    
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
                auto cursor_x = frequency_to_x(cursor_frequency, r, min_frequency, num_octaves);
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
                auto x = frequency_to_x(target_frequency, r, min_frequency, num_octaves);
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
        auto clicked_freq = x_to_frequency(getMouseXYRelative(), getLocalBounds(), min_frequency, num_octaves);
        onClick(clicked_freq);
    }

public:
    bool display_target;
    uint32_t target_frequency;
    bool is_cursor_locked;
    uint32_t locked_cursor_frequency;
    bool display_window;
    float correct_answer_window;
    uint32_t min_frequency;
    uint32_t num_octaves;
    std::function < void(uint32_t) > onClick;
};


struct FrequencyGame_UI : public juce::Component
{
    
    FrequencyGame_UI(FrequencyGame_IO *gameIO) : 
        game_io(gameIO)
    {
        bottom.onNextClicked = [this] (Event_Type e){
            Event event = {
                .type = e
            };
            frequency_game_post_event(game_io, event);
        };
        header.onBackClicked = [this] {
            Event event = {
                .type = Event_Click_Back
            };
            frequency_game_post_event(game_io, event);
        };
        bottom.onToggleClicked = [this] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            frequency_game_post_event(game_io, event);
        };
        addAndMakeVisible(header);
        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds().reduced(4);

        auto header_bounds = bounds.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        auto bottom_bounds = bounds.removeFromBottom(bottom.getHeight());
        bottom.setBounds(bottom_bounds);
        
        auto game_bounds = bounds.withTrimmedTop(4).withTrimmedBottom(4);

        if(center_panel)
            center_panel->setBounds(game_bounds);
    }

    GameUI_Header header;
    std::unique_ptr<juce::Component> center_panel;
    GameUI_Bottom bottom;
    FrequencyGame_IO *game_io;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGame_UI)
};


//------------------------------------------------------------------------
struct Frequency_Config_Panel : public juce::Component
{
    using slider_and_label_t = std::vector<std::tuple<juce::Slider&, juce::Label&, juce::Range < double>, double> >;
    using slider_and_toggle_t = std::vector<std::tuple<juce::Slider&, juce::ToggleButton&, juce::Range < double>, double> >;

    Frequency_Config_Panel(std::vector<FrequencyGame_Config> *gameConfigs,
                           size_t *currentConfigIdx,
                           std::function<void()> onClickBack,
                           std::function<void()> onClickNext)
    :        configs(gameConfigs),
             current_config_idx(currentConfigIdx),
             config_list_freq("Create new config")
    {
        //Header
        {
            header.onBackClicked = [click = std::move(onClickBack)] {
                click();
            };
            game_ui_header_update(&header, "Config", {});
            addAndMakeVisible(header);
        }

        {
            
            mode.setEditableText(false);
            mode.setJustificationType(juce::Justification::left);
            mode.addItem("Normal", PreListen_None + 1);
            mode.addItem("Bass", PreListen_Timeout + 1);
            mode.onChange = [&] {
                updateConfig();
            };
            scroller.addAndMakeVisible(mode);
            scroller.addAndMakeVisible(mode_label);
            
            input.setEditableText(false);
            input.setJustificationType(juce::Justification::left);
            input.addItem("Normal", Frequency_Input_Widget + 1);
            input.addItem("Text", Frequency_Input_Text + 1);
            input.onChange = [&] {
                updateConfig();
            };
            scroller.addAndMakeVisible(input);
            scroller.addAndMakeVisible(input_label);

            eq_gain.setTextValueSuffix (" dB");
            eq_gain.onValueChange = [&] {
                updateConfig();
            };

            eq_quality.onValueChange = [&] {
                updateConfig();
            };

            initial_correct_answer_window.onValueChange = [&] {
                updateConfig();
            };


            prelisten_type.setEditableText(false);
            prelisten_type.setJustificationType(juce::Justification::left);
            prelisten_type.addItem("None", PreListen_None + 1);
            prelisten_type.addItem("Timeout", PreListen_Timeout + 1);
            prelisten_type.addItem("Free", PreListen_Free + 1);
         
            prelisten_type.onChange = [&] {
                updateConfig();
            };

            scroller.addAndMakeVisible(prelisten_type);
            scroller.addAndMakeVisible(prelisten_type_label);
            scroller.addAndMakeVisible(prelisten_timeout_ms);

            prelisten_timeout_ms.setTextValueSuffix(" ms");
            prelisten_timeout_ms.onValueChange = [&] {
                updateConfig();
            };
            prelisten_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
            prelisten_timeout_ms.setScrollWheelEnabled(false);
            prelisten_timeout_ms.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            prelisten_timeout_ms.setRange( { 1000, 4000 }, 500);

            question_type.setEditableText(false);
            question_type.setJustificationType(juce::Justification::left);
            question_type.addItem("Free", Frequency_Question_Free + 1);
            question_type.addItem("Timeout", Frequency_Question_Timeout + 1);
            question_type.addItem("Rising Gain", Frequency_Question_Rising + 1);
         
            question_type.onChange = [&] {
                updateConfig();
            };


            question_timeout_ms.setTextValueSuffix(" ms");
            question_timeout_ms.onValueChange = [&] {
                updateConfig();
            };
            question_timeout_ms.setNumDecimalPlacesToDisplay(0);
           
            
            question_timeout_ms.setScrollWheelEnabled(false);
            question_timeout_ms.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            question_timeout_ms.setRange( { 1000, 10000 }, 500);

        
            scroller.addAndMakeVisible(question_type);
            scroller.addAndMakeVisible(question_type_label);
            scroller.addAndMakeVisible(question_timeout_ms);

            result_timeout_ms.setTextValueSuffix(" ms");
            result_timeout_ms.onValueChange = [&] {
                updateConfig();
            };
            result_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
            result_timeout_enabled.onClick = [&] {
                updateConfig();
            };

        
            slider_and_label_t slider_and_label = {
                { eq_gain, eq_gain_label, { -15.0, 15.0 }, 3.0 },
                { eq_quality, eq_quality_label, { 0.5, 8.0 }, 0.1 },
                { initial_correct_answer_window, initial_correct_answer_window_label, { 0.01, 0.4 }, 0.01 }
            };

            for (auto &[slider, label, range, interval] : slider_and_label)
            {
                slider.setScrollWheelEnabled(false);
                slider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
                slider.setRange(range, interval);

                label.setBorderSize(juce::BorderSize < int > { 0 });
                label.setJustificationType(juce::Justification::left);
            
                scroller.addAndMakeVisible(slider);
                scroller.addAndMakeVisible(label);
            }

            slider_and_toggle_t slider_and_toggle = {
                { result_timeout_ms , result_timeout_enabled, { 1000, 4000 }, 500 }
            };

        
            for (auto &[slider, toggle, range, interval] : slider_and_toggle)
            {
                slider.setScrollWheelEnabled(false);
                slider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
                slider.setRange(range, interval);

                //toggle.setBorderSize( juce::BorderSize<int>{ 0 });
                //toggle.setJustificationType(juce::Justification::left);
            
                scroller.addAndMakeVisible(slider);
                scroller.addAndMakeVisible(toggle);
            }

            viewport.setScrollBarsShown(true, false);
            viewport.setViewedComponent(&scroller, false);
            addAndMakeVisible(viewport);
        }

        //Next Button
        {
            nextButton.onClick = [&, onClickNext = std::move(onClickNext)] {
                updateConfig();
                onClickNext();
            };
            addAndMakeVisible(nextButton);
        }
        
        //List
        {
            
            auto configs_to_names = [] (std::vector<FrequencyGame_Config> *configs) {
                std::vector<std::string> config_names{};
                config_names.resize(configs->size());
                auto projection = [] (const FrequencyGame_Config& config) { return config.title; };
                std::transform(configs->begin(), configs->end(), config_names.begin(), projection);
                return config_names;
            };
            config_list_freq.selected_channel_changed_callback = [&](int config_idx) {
                selectConfig(config_idx);
            };
        
            config_list_freq.create_channel_callback = [&, configs_to_names](std::string new_config_name) {
                configs->push_back(frequency_game_config_default(new_config_name));
                config_list_freq.update(configs_to_names(configs));
            };

            config_list_freq.delete_channel_callback = [&, configs_to_names](int row_to_delete) {
                if(configs->size() == 1)
                    return;
                configs->erase(configs->begin() + row_to_delete);
                config_list_freq.update(configs_to_names(configs));
            };

            config_list_freq.rename_channel_callback = [&, configs_to_names](int row_idx, std::string new_config_name) {
                (*configs)[row_idx].title = new_config_name;
                config_list_freq.update(configs_to_names(configs));
            };

            config_list_freq.customization_point = [&](int, List_Row_Label*) {
            };

            config_list_freq.insertion_row_text = "Create new config";
            config_list_freq.update(configs_to_names(configs));
            config_list_freq.select_row(static_cast<int>(*current_config_idx));
            addAndMakeVisible(config_list_freq);
        }
    }

    void resized()
    {
        auto r = getLocalBounds().reduced (4);

        auto header_bounds = r.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        auto bottom_bounds = r.removeFromBottom(50);
        auto left_bounds = r.getProportion<float>( { .0f, .0f, 0.4f, 1.0f }).withTrimmedRight(5);
        auto right_bounds = r.getProportion<float>( { 0.4f, .0f, 0.6f, 1.0f }).withTrimmedLeft(5);

        config_list_freq.setBounds(left_bounds);

        auto scroller_height = onResizeScroller(right_bounds.getWidth());
        scroller.setSize(right_bounds.getWidth(), scroller_height);
        viewport.setBounds(right_bounds);

        auto button_bounds = bottom_bounds.withSizeKeepingCentre(100, 50);
        nextButton.setBounds(button_bounds);
    }

    void updateConfig()
    {
        FrequencyGame_Config *current_config = &configs->at(*current_config_idx);

        if (mode.getSelectedId() == 1)
        {
            current_config->min_f = 70;
            current_config->num_octaves = 8;
        }
        else
        {
            current_config->min_f = 35;
            current_config->num_octaves = 2;
        }
    
        Frequency_Input input_type = static_cast<Frequency_Input>(input.getSelectedId() - 1);
        current_config->input = input_type;
        
        current_config->eq_gain_db = (float)eq_gain.getValue();
        current_config->eq_quality = (float) eq_quality.getValue();
        current_config->initial_correct_answer_window = (float) initial_correct_answer_window.getValue();
        
        PreListen_Type new_prelisten_type = static_cast<PreListen_Type>(prelisten_type.getSelectedId() - 1);
        current_config->prelisten_type = new_prelisten_type;
        prelisten_timeout_ms.setEnabled(new_prelisten_type == PreListen_Timeout);

        current_config->prelisten_timeout_ms = (int) prelisten_timeout_ms.getValue();
        
        Frequency_Question_Type new_question_type = static_cast<Frequency_Question_Type>(question_type.getSelectedId() - 1);
        current_config->question_type = new_question_type;
        question_timeout_ms.setEnabled(new_question_type != Frequency_Question_Free);
        
        current_config->question_timeout_ms = (int) question_timeout_ms.getValue();
        current_config->result_timeout_ms = (int) result_timeout_ms.getValue();

        bool new_toggle_state = result_timeout_enabled.getToggleState();
        current_config->result_timeout_enabled = new_toggle_state;
        result_timeout_ms.setEnabled(new_toggle_state);
        
    }

    void selectConfig(size_t new_config_idx)
    {
        assert(new_config_idx < configs->size());
        *current_config_idx = new_config_idx;
        FrequencyGame_Config *current_config = &configs->at(*current_config_idx);

        if (current_config->min_f == 35)
            mode.setSelectedId(2);
        else
            mode.setSelectedId(1);
        
        input.setSelectedId(static_cast<int>(current_config->input) + 1);

        eq_gain.setValue(current_config->eq_gain_db);
        eq_quality.setValue(current_config->eq_quality);
        initial_correct_answer_window.setValue(current_config->initial_correct_answer_window);
        
        prelisten_timeout_ms.setValue(static_cast<double>(current_config->prelisten_timeout_ms));
        prelisten_timeout_ms.setEnabled(current_config->prelisten_type == PreListen_Timeout);
        prelisten_type.setSelectedId(current_config->prelisten_type + 1);

        question_timeout_ms.setValue(static_cast<double>(current_config->question_timeout_ms));
        question_timeout_ms.setEnabled(current_config->question_type != Frequency_Question_Free);
        question_type.setSelectedId(current_config->question_type + 1);

        result_timeout_ms.setValue(static_cast<double>(current_config->result_timeout_ms));
        result_timeout_ms.setEnabled(current_config->result_timeout_enabled);
        result_timeout_enabled.setToggleState(current_config->result_timeout_enabled, juce::dontSendNotification);
    }
    
    int onResizeScroller(int width)
    {
        //scroller_bounds = scroller_bounds.reduced(4);
        int total_height = 0;

        auto bounds = [&](int height) {
            auto new_bounds = juce::Rectangle < int > {
                0, total_height, width, height
            };
            total_height += height;
            return new_bounds;
        };

        int label_height = 35;
        int slider_height = 35;

        auto mode_bounds = bounds(label_height);
        mode_label.setBounds(mode_bounds.removeFromLeft(80));
        mode.setBounds(mode_bounds.removeFromLeft(80).reduced(0, 4));

        auto input_bounds = bounds(label_height);
        input_label.setBounds(input_bounds.removeFromLeft(80));
        input.setBounds(input_bounds.removeFromLeft(80).reduced(0, 4));

        eq_gain_label.setBounds(bounds(label_height));
        eq_gain.setBounds(bounds(slider_height));
        
        eq_quality_label.setBounds(bounds(label_height));
        eq_quality.setBounds(bounds(slider_height));

        initial_correct_answer_window_label.setBounds(bounds(label_height));
        initial_correct_answer_window.setBounds(bounds(slider_height));
        
        auto prelisten_top_bounds = bounds(label_height);
        prelisten_type_label.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_type.setBounds(prelisten_top_bounds.removeFromLeft(80).reduced(0, 4));
        prelisten_timeout_ms.setBounds(bounds(slider_height));

        auto question_top_bounds = bounds(label_height);
        question_type_label.setBounds(question_top_bounds.removeFromLeft(80));
        question_type.setBounds(question_top_bounds.removeFromLeft(80).reduced(0, 4));
        question_timeout_ms.setBounds(bounds(slider_height));

        result_timeout_enabled.setBounds(bounds(label_height));
        result_timeout_ms.setBounds(bounds(slider_height));

        return total_height;
    }

    std::vector<FrequencyGame_Config> *configs;
    size_t *current_config_idx;
    Insertable_List config_list_freq;
    
    GameUI_Header header;
    
    juce::ComboBox mode;
    juce::Label mode_label { {}, "Mode : " };

    juce::ComboBox input;
    juce::Label input_label { {}, "Input : " };

    juce::Slider eq_gain;
    juce::Label eq_gain_label { {}, "Gain"};
    
    juce::Slider eq_quality;
    juce::Label eq_quality_label { {}, "Q" };
    
    juce::Slider initial_correct_answer_window;
    juce::Label initial_correct_answer_window_label { {}, "Initial answer window" };
    
    juce::ComboBox prelisten_type;
    juce::Label prelisten_type_label { {}, "Pre-Listen : " };
    juce::Slider prelisten_timeout_ms;

    juce::ComboBox question_type;
    juce::Label question_type_label { {}, "Question : " };
    juce::Slider question_timeout_ms;

    juce::ToggleButton result_timeout_enabled { "Post answer timeout" };
    juce::Slider result_timeout_ms;

    juce::Viewport viewport;
    juce::Component scroller;
   
    juce::TextButton nextButton { "Next" };
   
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Frequency_Config_Panel)
};

struct Frequency_Game_Text_Input : public juce::Component
{
    Frequency_Game_Text_Input()
    {
        text_input.setSize(100, 30);
        text_input.setInputRestrictions(5, "0123456789");
        addAndMakeVisible(text_input);
        addAndMakeVisible(label);
        text_input.onReturnKey = [&] {
            auto text = text_input.getText();
            if(text.isEmpty())
                return;
            auto freq = text.getIntValue();
            onClick(freq);
        };
    }

    void resized() override
    {
        text_input.setCentrePosition(getLocalBounds().getCentre());

    }

    std::function<void(int)> onClick;
    juce::TextEditor text_input;
    juce::Label label;
};
