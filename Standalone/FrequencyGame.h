
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

    bool question_timeout_enabled;
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
class Frequency_Config_List : 
    public juce::Component,
public juce::ListBoxModel
{
   
public:
    Frequency_Config_List(std::vector<FrequencyGame_Config> &gameConfigs,
                          std::function<void(int)> onSelectionChanged) : 
        configs(gameConfigs),
    selected_row_changed_callback(std::move(onSelectionChanged))
    {
        file_list_component.setMultipleSelectionEnabled(false);
        file_list_component.setColour (juce::ListBox::outlineColourId, juce::Colours::grey);      // [2]
        file_list_component.setOutlineThickness (2);
        addAndMakeVisible(file_list_component);
        file_list_component.updateContent();

        editable_mouse_down_callback = [&] (int row_idx) {
            file_list_component.selectRow(row_idx);
        };
        editable_text_changed_callback = [&] (int row_idx, juce::String row_text) {
            configs[row_idx].title = row_text;
        };
        editable_create_row_callback = [&] (juce::String new_row_text) {
            configs.push_back(frequency_game_config_default(new_row_text));
            file_list_component.selectRow(checked_cast<int>(configs.size()) - 1);
            file_list_component.updateContent();
        };
    }

    virtual ~Frequency_Config_List() override = default;

    void resized() override
    {
        file_list_component.setBounds(getLocalBounds());
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (getNumRows() == 0)
        {
            auto r = getLocalBounds();
            g.setColour(juce::Colours::white);
            g.drawText("Create game config", r.toFloat(), juce::Justification::centred);
        }
    }

    int getNumRows() override { return (int)configs.size() + 1; }

    void paintListBoxItem (int row,
                           juce::Graphics& g,
                           int width, int height,
                           bool row_is_selected) override
    {
        if (row_is_selected && row < getNumRows())
        {
            g.setColour(juce::Colours::white);
            auto bounds = juce::Rectangle { 0, 0, width, height };
            g.drawRect(bounds);
        }
    }

    juce::Component *refreshComponentForRow (int row_number,
                                             bool,
                                             Component *existing_component) override
    {
        //assert (existingComponentToUpdate == nullptr || dynamic_cast<EditableTextCustomComponent*> (existingComponentToUpdate) != nullptr);
        //unused row
        if (row_number > checked_cast<int>(configs.size()))
        {
            if (existing_component != nullptr)
                delete existing_component;
            return nullptr;
        }
        else
        {
            List_Row_Label *label;

            if (existing_component != nullptr)
            {
                label = dynamic_cast<List_Row_Label*>(existing_component);
                assert(label != nullptr);
            }
            else
            {
                label = new List_Row_Label("Create new config",
                                           editable_mouse_down_callback,
                                           editable_text_changed_callback,
                                           editable_create_row_callback);
            }
            juce::String row_text = "";
            bool is_last_row = row_number == configs.size();
            if (!is_last_row)
            {
                row_text = configs[row_number].title;
            }
            label->update(row_number, row_text, is_last_row);
            return label;
        } 
    }
    
    void listBoxItemClicked (int, const juce::MouseEvent&) override 
    {
    }

    void listBoxItemDoubleClicked (int, const juce::MouseEvent &) override {}
    
    void deleteKeyPressed (int) override
    {
        auto selected_row = file_list_component.getSelectedRow();
        if(selected_row == -1 || selected_row == getNumRows() - 1) 
            return;
        configs.erase(configs.begin() + selected_row);
        auto row_to_select = selected_row == 0 ? 0 : selected_row - 1;
        file_list_component.selectRow(row_to_select);
        file_list_component.updateContent();
    }

    bool keyPressed (const juce::KeyPress &key) override
    {
        if (key == key.escapeKey)
        {
            file_list_component.deselectAllRows();
            return true;
        }
        return false;
    }

    void selectedRowsChanged (int last_row_selected) override
    {   
        if(last_row_selected != -1 && checked_cast<size_t>(last_row_selected) != configs.size())
            selected_row_changed_callback(last_row_selected);
    }

    void selectRow(int new_row)
    {
        file_list_component.selectRow(new_row);
    }

private:
    
    std::function < void(int) > editable_mouse_down_callback;
    std::function < void(int, juce::String) > editable_text_changed_callback;
    std::function < void(juce::String) > editable_create_row_callback;

    std::vector<FrequencyGame_Config> &configs;
    juce::ListBox file_list_component = { {}, this };
    std::function < void(int) > selected_row_changed_callback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Frequency_Config_List)
};


//------------------------------------------------------------------------
struct Frequency_Config_Panel : public juce::Component
{
    using slider_and_label_t = std::vector<std::tuple < juce::Slider&, juce::Label&, juce::Range<double>, double> >;
    using slider_and_toggle_t = std::vector<std::tuple < juce::Slider&, juce::ToggleButton&, juce::Range<double>, double> >;
    
    class Scroller : public juce::Component
    {
    public:
        Scroller(std::function<void(juce::Rectangle<int>)> onResize) 
        : on_resized_callback(std::move(onResize))
        {}

        void resized() override
        {
            on_resized_callback(getLocalBounds());
        }
    private:
        std::function < void(juce::Rectangle<int>) > on_resized_callback;
    };

    Frequency_Config_Panel(std::vector<FrequencyGame_Config> &gameConfigs,
                 size_t &currentConfigIdx,
                 std::function < void() > onClickBack,
                 std::function < void() > onClickNext)
    :        configs(gameConfigs),
             current_config_idx(currentConfigIdx),
             config_list_comp { configs, [&] (int idx) { selectConfig(idx); } }
    {
        {
            header.onBackClicked = [click = std::move(onClickBack)] {
                click();
            };
            game_ui_header_update(&header, "Config", {});
            addAndMakeVisible(header);
        }
        
        addAndMakeVisible(config_list_comp);

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
        prelisten_type.addItem("Free", PreListen_Free  + 1);
         
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
            configs[current_config_idx].prelisten_timeout_ms =(int) prelisten_timeout_ms.getValue();
        };
        prelisten_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
        prelisten_timeout_ms.setScrollWheelEnabled(false);
        prelisten_timeout_ms.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
        prelisten_timeout_ms.setRange({1000, 4000}, 500);

        question_timeout_ms.setTextValueSuffix(" ms");
        question_timeout_ms.onValueChange = [&] {
            configs[current_config_idx].question_timeout_ms =(int) question_timeout_ms.getValue();
        };
        question_timeout_ms.setNumDecimalPlacesToDisplay(0);
        
        question_timeout_enabled.onClick = [&] {
            bool new_toggle_state = question_timeout_enabled.getToggleState();
            configs[current_config_idx].question_timeout_enabled = new_toggle_state;
            question_timeout_ms.setEnabled(new_toggle_state);
        };

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
            { eq_gain, eq_gain_label, { -15.0, 15.0 }, 3.0}, 
            { eq_quality, eq_quality_label, { 0.5, 4 }, 0.1 }, 
            { initial_correct_answer_window, initial_correct_answer_window_label, { 0.01, 0.4 }, 0.01 }
        };

        for (auto &[slider, label, range, interval] : slider_and_label)
        {
            slider.setScrollWheelEnabled(false);
            slider.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
            slider.setRange(range, interval);

            label.setBorderSize( juce::BorderSize<int>{ 0 });
            label.setJustificationType(juce::Justification::left);
            
            scroller.addAndMakeVisible(slider);
            scroller.addAndMakeVisible(label);
        }

        slider_and_toggle_t slider_and_toggle = {
            { question_timeout_ms , result_timeout_enabled, { 1000, 4000 }, 500 },
            { result_timeout_ms , question_timeout_enabled, { 1000, 4000 }, 500 }
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

        scroller.setSize(0, 6 * 60);
        viewport.setScrollBarsShown(true, false);
        viewport.setViewedComponent(&scroller, false);
        addAndMakeVisible(viewport);

        {
            nextButton.onClick = [onClickNext = std::move(onClickNext)] {
                onClickNext();
            };
            addAndMakeVisible(nextButton);
        }

        config_list_comp.selectRow(static_cast<int>(current_config_idx));
        //selectConfig(current_config_idx);
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
        scroller.setSize(right_bounds.getWidth(), scroller.getHeight());
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
        question_timeout_ms.setEnabled(current_config.question_timeout_enabled);
        question_timeout_enabled.setToggleState(current_config.question_timeout_enabled, juce::dontSendNotification);
        
        result_timeout_ms.setValue(static_cast<double>(current_config.result_timeout_ms));
        result_timeout_ms.setEnabled(current_config.result_timeout_enabled);
        result_timeout_enabled.setToggleState(current_config.result_timeout_enabled, juce::dontSendNotification);
    }
    
    void onResizeScroller(juce::Rectangle<int> scroller_bounds)
    {
        scroller_bounds = scroller_bounds.reduced(4);
        auto height = scroller_bounds.getHeight();
        auto param_height = height / 6;

        auto same_bounds = [&] {
            return scroller_bounds.removeFromTop(param_height / 2);
        };

        eq_gain_label.setBounds(same_bounds());
        eq_gain.setBounds(same_bounds());
        
        eq_quality_label.setBounds(same_bounds());
        eq_quality.setBounds(same_bounds());

        initial_correct_answer_window_label.setBounds(same_bounds());
        initial_correct_answer_window.setBounds(same_bounds());
        
        auto prelisten_top_bounds = same_bounds();
        prelisten_type_label.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_type.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_timeout_ms.setBounds(same_bounds());

        question_timeout_enabled.setBounds(same_bounds());
        question_timeout_ms.setBounds(same_bounds());

        result_timeout_enabled.setBounds(same_bounds());
        result_timeout_ms.setBounds(same_bounds());
    }

    std::vector<FrequencyGame_Config> &configs;
    size_t &current_config_idx;
    Frequency_Config_List config_list_comp;
    
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

    juce::ToggleButton question_timeout_enabled { "Question timeout" };
    juce::Slider question_timeout_ms;

    juce::ToggleButton result_timeout_enabled { "Post answer timeout" };
    juce::Slider result_timeout_ms;

    juce::Viewport viewport;
    Scroller scroller { [this] (auto bounds) { onResizeScroller(bounds); } };
   
    juce::TextButton nextButton { "Next" };
   
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Frequency_Config_Panel)
};
