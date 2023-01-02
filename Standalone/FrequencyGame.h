
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
    Effect_Transition transition;
    int ui_target;
    struct {
        bool display_target;
        uint32_t target_frequency;
        bool is_cursor_locked;
        uint32_t locked_cursor_frequency;
        bool display_window;
        float correct_answer_window;
        uint32_t min_f;
        uint32_t num_octaves;
    } freq_widget;
    FrequencyGame_Results results;
    std::string header_center_text;
    std::string header_right_text;
    Mix mix;
    bool display_button;
    std::string button_text;
    Event_Type button_event;
};


enum Frequency_Input
{
    Frequency_Input_Widget,
    Frequency_Input_Text
};

enum Frequency_Question_Type
{
    Frequency_Question_Free = 0,
    Frequency_Question_Timeout = 1,
    Frequency_Question_Rising = 2
};

struct FrequencyGame_Config
{
    std::string title;
    //frequency
    Frequency_Input input;
    float eq_gain_db;
    float eq_quality;
    float initial_correct_answer_window;
    uint32_t min_f;
    uint32_t num_octaves;
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
    uint32_t target_frequency;
    float correct_answer_window;
    //
    uint32_t current_file_idx;
    std::vector<Audio_File> files;
    FrequencyGame_Config config;
    FrequencyGame_Results results;

    int64_t timestamp_start;
    int64_t current_timestamp;
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

using frequency_game_observer_t = std::function<void(Frequency_Game_Effects*)>;

struct FrequencyGame_IO
{
    FrequencyGame_State game_state;
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<frequency_game_observer_t> observers;
    std::function < void() > on_quit;
};


struct FrequencyGame_UI;

std::string frequency_game_serlialize(std::vector<FrequencyGame_Config> *frequency_game_configs);
std::vector<FrequencyGame_Config> frequency_game_deserialize(std::string xml_string);


FrequencyGame_Config frequency_game_config_default(std::string name);
FrequencyGame_State frequency_game_state_init(FrequencyGame_Config config, std::vector<Audio_File> *files);
std::unique_ptr<FrequencyGame_IO> frequency_game_io_init(FrequencyGame_State);
void frequency_game_add_observer(FrequencyGame_IO *io, frequency_game_observer_t observer);

void frequency_game_post_event(FrequencyGame_IO *io, Event event);
Frequency_Game_Effects frequency_game_update(FrequencyGame_State state, Event event);
void frequency_game_ui_transitions(FrequencyGame_UI *ui, Effect_Transition transition, int ui_target);
void frequency_game_ui_update(FrequencyGame_UI *ui, Frequency_Game_Effect_UI *new_ui);
void frequency_widget_update(FrequencyWidget *widget, Frequency_Game_Effect_UI *new_ui);

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
            config_list_comp.selected_channel_changed_callback = [&](int config_idx) {
                selectConfig(config_idx);
            };
        
            config_list_comp.create_channel_callback = [&, configs_to_names](std::string new_config_name) {
                configs->push_back(frequency_game_config_default(new_config_name));
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.delete_channel_callback = [&, configs_to_names](int row_to_delete) {
                configs->erase(configs->begin() + row_to_delete);
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.rename_channel_callback = [&, configs_to_names](int row_idx, std::string new_config_name) {
                (*configs)[row_idx].title = new_config_name;
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.customization_point = [&](int, List_Row_Label*) {
            };

            config_list_comp.insert_row_text = "Create new config";
            config_list_comp.update(configs_to_names(configs));
            config_list_comp.select_row(static_cast<int>(*current_config_idx));
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
    Insertable_List config_list_comp;
    
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
