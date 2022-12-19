
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


struct Frequency_Game_Effect_UI {
    struct {
        bool display_target;
        int target_frequency;
        bool is_cursor_locked;
        int locked_cursor_frequency;
        bool display_window;
        float correct_answer_window;
    } freq_widget;
    FrequencyGame_Results results;
    juce::String header_center_text;
    juce::String header_right_text;
    Mix mix;
    bool display_button;
    juce::String button_text;
    Event_Type button_event;
};



enum Frequency_Question_Type
{
    Frequency_Question_Free = 0,
    Frequency_Question_Timeout = 1,
    Frequency_Question_Rising = 2
};

struct FrequencyGame_Config
{
    juce::String title;
    //frequency
    float eq_gain;
    float eq_quality;
    float initial_correct_answer_window;
    //
    PreListen_Type prelisten_type;
    int prelisten_timeout_ms;

    Frequency_Question_Type question_type;
    int question_timeout_ms;

    bool result_timeout_enabled;
    int result_timeout_ms;
};
struct FrequencyGame_State
{
    GameStep step;
    int score;
    int lives;
    //frequency
    int target_frequency;
    float correct_answer_window;
    //
    int current_file_idx;
    std::vector<Audio_File> files;
    FrequencyGame_Config config;
    FrequencyGame_Results results;

    juce::int64 timestamp_start;
    juce::int64 current_timestamp;
};

struct Frequency_Game_Effects {
    int error;
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP_Single_Track > dsp;
    std::optional < Effect_Player > player;
    std::optional < Frequency_Game_Effect_UI > ui;
    std::optional < FrequencyGame_Results > results;
    bool quit;
    FrequencyGame_State new_state;
};

using frequency_game_observer_t = std::function<void(const Frequency_Game_Effects&)>;

struct FrequencyGame_IO
{
    FrequencyGame_State game_state;
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<frequency_game_observer_t> observers;
    std::function < void() > on_quit;
};


struct FrequencyGame_UI;


FrequencyGame_Config frequency_game_config_default(juce::String name);
FrequencyGame_State frequency_game_state_init(FrequencyGame_Config config, std::vector<Audio_File> files);
std::unique_ptr<FrequencyGame_IO> frequency_game_io_init(FrequencyGame_State);
void frequency_game_add_observer(FrequencyGame_IO *io, frequency_game_observer_t observer);

void frequency_game_post_event(FrequencyGame_IO *io, Event event);
Frequency_Game_Effects frequency_game_update(FrequencyGame_State state, Event event);
void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition);
void frequency_game_ui_update(FrequencyGame_UI &ui, const Frequency_Game_Effect_UI &new_ui);
void frequency_widget_update(FrequencyWidget *widget, const Frequency_Game_Effect_UI &new_ui);

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

        if(frequency_widget)
            frequency_widget->setBounds(game_bounds);
        else if(results_panel)
            results_panel->setBounds(game_bounds);
    }

    GameUI_Header header;
    std::unique_ptr<FrequencyWidget> frequency_widget;
    std::unique_ptr<FrequencyGame_Results_Panel> results_panel;
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

    Frequency_Config_Panel(std::vector<FrequencyGame_Config> &gameConfigs,
                 size_t &currentConfigIdx,
                 std::function < void() > onClickBack,
                 std::function < void() > onClickNext)
    :        configs(gameConfigs),
             current_config_idx(currentConfigIdx)
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
            eq_gain.setTextValueSuffix (" dB");
            eq_gain.onValueChange = [&] {
                configs[current_config_idx].eq_gain = juce::Decibels::decibelsToGain((float) eq_gain.getValue());
            };

            eq_quality.onValueChange = [&] {
                configs[current_config_idx].eq_quality = (float) eq_quality.getValue();
            };

            initial_correct_answer_window.onValueChange = [&] {
                configs[current_config_idx].initial_correct_answer_window = (float) initial_correct_answer_window.getValue();
            };


            prelisten_type.setEditableText(false);
            prelisten_type.setJustificationType(juce::Justification::left);
            prelisten_type.addItem("None", PreListen_None + 1);
            prelisten_type.addItem("Timeout", PreListen_Timeout + 1);
            prelisten_type.addItem("Free", PreListen_Free + 1);
         
            prelisten_type.onChange = [&] {
                PreListen_Type new_type = static_cast<PreListen_Type>(prelisten_type.getSelectedId() - 1);
                configs[current_config_idx].prelisten_type = new_type;
                prelisten_timeout_ms.setEnabled(new_type == PreListen_Timeout);
            };

            scroller.addAndMakeVisible(prelisten_type);
            scroller.addAndMakeVisible(prelisten_type_label);
            scroller.addAndMakeVisible(prelisten_timeout_ms);

            prelisten_timeout_ms.setTextValueSuffix(" ms");
            prelisten_timeout_ms.onValueChange = [&] {
                configs[current_config_idx].prelisten_timeout_ms = (int) prelisten_timeout_ms.getValue();
            };
            prelisten_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
            prelisten_timeout_ms.setScrollWheelEnabled(false);
            prelisten_timeout_ms.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            prelisten_timeout_ms.setRange( { 1000, 4000 }, 500);

            question_type.setEditableText(false);
            question_type.setJustificationType(juce::Justification::left);
            question_type.addItem("Free", Frequency_Question_Free + 1);
            question_type.addItem("Timeout", Frequency_Question_Timeout + 1);
            question_type.addItem("Rising", Frequency_Question_Rising + 1);
         
            question_type.onChange = [&] {
                Frequency_Question_Type new_type = static_cast<Frequency_Question_Type>(question_type.getSelectedId() - 1);
                configs[current_config_idx].question_type = new_type;
                question_timeout_ms.setEnabled(new_type != Frequency_Question_Free);
            };


            question_timeout_ms.setTextValueSuffix(" ms");
            question_timeout_ms.onValueChange = [&] {
                configs[current_config_idx].question_timeout_ms = (int) question_timeout_ms.getValue();
            };
            question_timeout_ms.setNumDecimalPlacesToDisplay(0);
           
            
            question_timeout_ms.setScrollWheelEnabled(false);
            question_timeout_ms.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            question_timeout_ms.setRange( { 1000, 4000 }, 500);

        
            scroller.addAndMakeVisible(question_type);
            scroller.addAndMakeVisible(question_type_label);
            scroller.addAndMakeVisible(question_timeout_ms);

#if 0
            question_timeout_enabled.onClick = [&] {
                bool new_toggle_state = question_timeout_enabled.getToggleState();
                configs[current_config_idx].question_timeout_enabled = new_toggle_state;
                question_timeout_ms.setEnabled(new_toggle_state);
            };
#endif

            result_timeout_ms.setTextValueSuffix(" ms");
            result_timeout_ms.onValueChange = [&] {
                configs[current_config_idx].result_timeout_ms = (int) result_timeout_ms.getValue();
            };
            result_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
            result_timeout_enabled.onClick = [&] {
                bool new_toggle_state = result_timeout_enabled.getToggleState();
                configs[current_config_idx].result_timeout_enabled = new_toggle_state;
                result_timeout_ms.setEnabled(new_toggle_state);
            };

        
            slider_and_label_t slider_and_label = {
                { eq_gain, eq_gain_label, { -15.0, 15.0 }, 3.0 },
                { eq_quality, eq_quality_label, { 0.5, 4 }, 0.1 },
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
            nextButton.onClick = [onClickNext = std::move(onClickNext)] {
                onClickNext();
            };
            addAndMakeVisible(nextButton);
        }
        
        //List
        {
            
            auto configs_to_names = [] (const std::vector<FrequencyGame_Config>& configs) {
                std::vector<juce::String> config_names{};
                config_names.resize(configs.size());
                auto projection = [] (const FrequencyGame_Config& config) { return config.title; };
                std::transform(configs.begin(), configs.end(), config_names.begin(), projection);
                return config_names;
            };
            config_list_comp.selected_channel_changed_callback = [&](int config_idx) {
                selectConfig(config_idx);
            };
        
            config_list_comp.create_channel_callback = [&, configs_to_names](juce::String new_config_name) {
                configs.push_back(frequency_game_config_default(new_config_name));
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.delete_channel_callback = [&, configs_to_names](int row_to_delete) {
                configs.erase(configs.begin() + row_to_delete);
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.rename_channel_callback = [&, configs_to_names](int row_idx, juce::String new_config_name) {
                configs[row_idx].title = new_config_name;
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.customization_point = [&](int, List_Row_Label*) {
            };

            config_list_comp.insert_row_text = "Create new config";
            config_list_comp.update(configs_to_names(configs));
            config_list_comp.select_row(static_cast<int>(current_config_idx));
            addAndMakeVisible(config_list_comp);
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

        config_list_comp.setBounds(left_bounds);

        auto scroller_height = onResizeScroller(right_bounds.getWidth());
        scroller.setSize(right_bounds.getWidth(), scroller_height);
        viewport.setBounds(right_bounds);

        auto button_bounds = bottom_bounds.withSizeKeepingCentre(100, 50);
        nextButton.setBounds(button_bounds);
    }

    void selectConfig(size_t new_config_idx)
    {
        assert(new_config_idx < configs.size());
        current_config_idx = new_config_idx;
        FrequencyGame_Config &current_config = configs[current_config_idx];
        float gain_db = juce::Decibels::gainToDecibels(current_config.eq_gain);
        eq_gain.setValue(gain_db);
        eq_quality.setValue(current_config.eq_quality);
        initial_correct_answer_window.setValue(current_config.initial_correct_answer_window);
        
        prelisten_timeout_ms.setValue(static_cast<double>(current_config.prelisten_timeout_ms));
        prelisten_timeout_ms.setEnabled(current_config.prelisten_type == PreListen_Timeout);
        prelisten_type.setSelectedId(current_config.prelisten_type + 1);

        question_timeout_ms.setValue(static_cast<double>(current_config.question_timeout_ms));
        question_timeout_ms.setEnabled(current_config.question_type != Frequency_Question_Free);
        question_type.setSelectedId(current_config.question_type + 1);

        result_timeout_ms.setValue(static_cast<double>(current_config.result_timeout_ms));
        result_timeout_ms.setEnabled(current_config.result_timeout_enabled);
        result_timeout_enabled.setToggleState(current_config.result_timeout_enabled, juce::dontSendNotification);
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

        eq_gain_label.setBounds(bounds(label_height));
        eq_gain.setBounds(bounds(slider_height));
        
        eq_quality_label.setBounds(bounds(label_height));
        eq_quality.setBounds(bounds(slider_height));

        initial_correct_answer_window_label.setBounds(bounds(label_height));
        initial_correct_answer_window.setBounds(bounds(slider_height));
        
        auto prelisten_top_bounds = bounds(label_height);
        prelisten_type_label.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_type.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_timeout_ms.setBounds(bounds(slider_height));

        auto question_top_bounds = bounds(label_height);
        question_type_label.setBounds(question_top_bounds.removeFromLeft(80));
        question_type.setBounds(question_top_bounds.removeFromLeft(80));
        question_timeout_ms.setBounds(bounds(slider_height));

        result_timeout_enabled.setBounds(bounds(label_height));
        result_timeout_ms.setBounds(bounds(slider_height));

        return total_height;
    }

    std::vector<FrequencyGame_Config> &configs;
    size_t &current_config_idx;
    Insertable_List config_list_comp;
    
    GameUI_Header header;

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
