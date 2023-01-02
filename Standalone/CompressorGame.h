
struct CompressorGame_Results_Panel : public juce::Component
{
    CompressorGame_Results_Panel()
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

struct CompressorGame_Results
{
    int score;
    float analytics;
    //temps moyen ? distance moyenne ? stats ?
};

struct Compressor_Game_Effect_UI {
    Effect_Transition transition;
    CompressorGame_Results results;
    juce::String header_center_text;
    juce::String header_right_text;
    //int score;
    struct {
        Widget_Interaction_Type threshold_visibility;
        Widget_Interaction_Type ratio_visibility;
        Widget_Interaction_Type attack_visibility;
        Widget_Interaction_Type release_visibility;

        uint32_t threshold_pos;
        uint32_t ratio_pos;
        uint32_t attack_pos;
        uint32_t release_pos;
        
        std::vector < float > threshold_values_db;
        std::vector < float > ratio_values;
        std::vector < float > attack_values;
        std::vector < float > release_values;
    } comp_widget;
    Mix mix_toggles;
    juce::String bottom_button_text;
    Event_Type bottom_button_event;
};

enum Compressor_Game_Variant
{
    Compressor_Game_Normal,
    Compressor_Game_Tries,
    Compressor_Game_Timer
};

struct CompressorGame_Config
{
    juce::String title;
    //compressor
    bool threshold_active;
    bool ratio_active;
    bool attack_active;
    bool release_active;

    std::vector<float> threshold_values_db;
    std::vector<float> ratio_values;
    std::vector<float> attack_values;
    std::vector<float> release_values;
    //
    Compressor_Game_Variant variant;
    int listens;
    int timeout_ms;
    int total_rounds;
};

static void sort_clamp_and_filter(std::vector<float> *vec, float min, float max)
{
    std::sort(vec->begin(), vec->end());
    auto clamping = [min, max] (float in)
    {
        return std::clamp(in, min, max);
    };
    std::transform(vec->begin(), vec->end(), vec->begin(), clamping);
    vec->erase(std::unique(vec->begin(), vec->end()), vec->end());
}


static inline bool compressor_game_config_validate(CompressorGame_Config *config)
{
    sort_clamp_and_filter(&config->threshold_values_db, -90, 0);
    sort_clamp_and_filter(&config->ratio_values, 1, 10);
    sort_clamp_and_filter(&config->attack_values, 1, 1000);
    sort_clamp_and_filter(&config->release_values, 1, 1000);

    return config->threshold_active
        || config->ratio_active
        || config->attack_active
        || config->release_active;
}


struct CompressorGame_State
{
    GameStep step;
    Mix mix;
    int score;
    //compressor
    uint32_t target_threshold_pos;
    uint32_t target_ratio_pos;
    uint32_t target_attack_pos;
    uint32_t target_release_pos;

    uint32_t input_threshold_pos;
    uint32_t input_ratio_pos;
    uint32_t input_attack_pos;
    uint32_t input_release_pos;

    //
    uint32_t current_file_idx;
    std::vector<Audio_File> files;
    CompressorGame_Config config;
    CompressorGame_Results results;
    
    int remaining_listens;
    bool can_still_listen;
    int64_t timestamp_start;
    int64_t current_timestamp;
    int current_round;
};

struct Compressor_Game_Effects {
    int error;
    CompressorGame_State new_state;
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP_Single_Track > dsp;
    std::optional < Effect_Player > player;
    std::optional < Compressor_Game_Effect_UI > ui;
    std::optional < CompressorGame_Results > results;
    bool quit;
};

using compressor_game_observer_t = std::function<void(Compressor_Game_Effects*)>;

struct CompressorGame_IO
{
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<compressor_game_observer_t> observers;
    std::function < void() > on_quit;
    CompressorGame_State game_state;
};

struct CompressorGame_UI;
struct CompressorGame_Widget;

juce::String compressor_game_serialize(std::vector<CompressorGame_Config> *compressor_game_configs);
std::vector<CompressorGame_Config> compressor_game_deserialize(juce::String xml_string);

CompressorGame_Config compressor_game_config_default(juce::String name);
CompressorGame_State compressor_game_state_init(CompressorGame_Config config, std::vector<Audio_File> *files);
std::unique_ptr<CompressorGame_IO> compressor_game_io_init(CompressorGame_State state);
void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer);

void compressor_game_post_event(CompressorGame_IO *io, Event event);
Compressor_Game_Effects compressor_game_update(CompressorGame_State state, Event event);
void compressor_game_ui_transitions(CompressorGame_UI *ui, Effect_Transition transition);
void compressor_game_ui_update(CompressorGame_UI *ui, Compressor_Game_Effect_UI *new_ui);
void compressor_widget_update(CompressorGame_Widget *widget, Compressor_Game_Effect_UI *new_ui);


struct CompressorGame_Widget : public juce::Component
{
    CompressorGame_Widget(CompressorGame_IO *gameIO)
    : game_io(gameIO)
    {
        auto setup_slider = [&] (TextSlider &slider, juce::Label &label, juce::String name, std::function<void(uint32_t)> onEdit) {
            label.setText(name, juce::NotificationType::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, slider.getTextBoxWidth(), 15);
            slider.setScrollWheelEnabled(true);
        
            slider.onValueChange = [&slider, onEdit = std::move(onEdit)] {
                onEdit((uint32_t)slider.getValue());
            };
            addAndMakeVisible(slider);
        };
        
        auto onThresholdMoved = [io = this->game_io] (uint32_t new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 0,
                .value_u = new_pos
            };
            compressor_game_post_event(io, event);
        };

        setup_slider(threshold_slider, threshold_label, "Threshold", std::move(onThresholdMoved));
        
        auto onRatioMoved = [io = this->game_io] (uint32_t new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 1,
                .value_u = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(ratio_slider, ratio_label, "Ratio", std::move(onRatioMoved));
        
        auto onAttackMoved = [io = this->game_io] (uint32_t new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 2,
                .value_u = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(attack_slider, attack_label, "Attack", std::move(onAttackMoved));
        
        auto onReleaseMoved = [io = this->game_io] (uint32_t new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 3,
                .value_u = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(release_slider, release_label, "Release", std::move(onReleaseMoved));
        
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto threshold_bounds = bounds.getProportion<float>( { 0.0f, 0.0f, 0.5f, 0.5f }).reduced(5);
        auto ratio_bounds = bounds.getProportion<float>( { 0.5f, 0.0f, 0.5f, 0.5f }).reduced(5);
        auto attack_bounds = bounds.getProportion<float>( { 0.0f, 0.5f, 0.5f, 0.5f }).reduced(5);
        auto release_bounds = bounds.getProportion<float>( { 0.5f, 0.5f, 0.5f, 0.5f }).reduced(5);

        threshold_label.setBounds(threshold_bounds.removeFromTop(15));
        ratio_label.setBounds(ratio_bounds.removeFromTop(15));
        attack_label.setBounds(attack_bounds.removeFromTop(15));
        release_label.setBounds(release_bounds.removeFromTop(15));
        
        threshold_slider.setBounds(threshold_bounds);
        ratio_slider.setBounds(ratio_bounds);
        attack_slider.setBounds(attack_bounds);
        release_slider.setBounds(release_bounds);
    }
    
    TextSlider threshold_slider;
    TextSlider ratio_slider;
    TextSlider attack_slider;
    TextSlider release_slider;
    
    juce::Label threshold_label;
    juce::Label ratio_label;
    juce::Label attack_label;
    juce::Label release_label;

    CompressorGame_IO *game_io;
};

struct CompressorGame_UI : public juce::Component
{
    
    CompressorGame_UI(CompressorGame_IO *gameIO)
    : previewer_file_list(false),
      compressor_widget(gameIO),
      game_io(gameIO)
    {
        addChildComponent(previewer_file_list);

        bottom.onNextClicked = [io = this->game_io] (Event_Type e) {
            Event event = {
                .type = e
            };
            compressor_game_post_event(io, event);
        };
        header.onBackClicked = [io = this->game_io] {
            Event event = {
                .type = Event_Click_Back
            };
            compressor_game_post_event(io, event);
        };
        bottom.onToggleClicked = [io = this->game_io] (bool a) {
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            compressor_game_post_event(io, event);
        };
        addAndMakeVisible(header);
        
        compressor_widget.setSize(200, 200);
        addAndMakeVisible(compressor_widget);
        
        bottom.target_mix_button.setButtonText("Target settings");
        bottom.user_mix_button.setButtonText("Your settings");
        addAndMakeVisible(bottom);
    }
    
    void parentHierarchyChanged() override
    {
        auto * top_level_window = getTopLevelComponent();
        auto * resizable_window = dynamic_cast<juce::ResizableWindow*>(top_level_window);
        assert(resizable_window);
        auto min_height = 8 + header.getHeight() + bottom.getHeight() + compressor_widget.getHeight() + 40; //TODO hack ? why doesn't it work ?
        resizable_window->setResizeLimits (400, min_height, 
                                           10000, 10000);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds().reduced(4);

        auto header_bounds = bounds.removeFromTop(header.getHeight());
        header.setBounds(header_bounds);

        auto bottom_bounds = bounds.removeFromBottom(bottom.getHeight());
        bottom.setBounds(bottom_bounds);

        //TODO temporary, should not depend on internal state
        compressor_widget.setCentrePosition(bounds.getCentre());
        if (previewer_file_list.isVisible())
        {
            auto game_bounds = compressor_widget.getBounds();
            auto previewer_bounds = game_bounds.withX(bounds.getRight() - game_bounds.getWidth());
            previewer_bounds.setX(0);
            previewer_bounds.setRight(game_bounds.getX());
            previewer_file_list.setBounds(previewer_bounds);
        }
    }

    GameUI_Header header;
    Selection_List previewer_file_list;

    CompressorGame_Widget compressor_widget;
    //std::unique_ptr<CompressorGame_Results_Panel> results_panel;

    GameUI_Bottom bottom;
    CompressorGame_IO *game_io;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGame_UI)
};



//------------------------------------------------------------------------
struct Compressor_Config_Panel : public juce::Component
{
    using textedit_and_toggle_t = std::vector<std::tuple<juce::TextEditor&, juce::ToggleButton&>>;

    Compressor_Config_Panel(std::vector<CompressorGame_Config> *gameConfigs,
                           size_t *currentConfigIdx,
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

        scroller.setWantsKeyboardFocus(true);

        {
            auto update = [&] {
                updateConfig();
            };
            thresholds_label.onStateChange = update;
            thresholds.setInputRestrictions(0, "0123456789,. -");
            thresholds.onReturnKey = update;
            thresholds.onEscapeKey = update;
            thresholds.onFocusLost = update;
            
            ratios_label.onStateChange = update;
            ratios.setInputRestrictions(0, "0123456789,. ");
            ratios.onReturnKey = update;
            ratios.onEscapeKey = update;
            ratios.onFocusLost = update;
            
            attacks_label.onStateChange = update;
            attacks.setInputRestrictions(0, "0123456789,. ");
            attacks.onReturnKey = update;
            attacks.onEscapeKey = update;
            attacks.onFocusLost = update;

            releases_label.onStateChange = update;
            releases.setInputRestrictions(0, "0123456789,. ");
            releases.onReturnKey = update;
            releases.onEscapeKey = update;
            releases.onFocusLost = update;
        
            textedit_and_toggle_t textedit_and_label = {
                { thresholds, thresholds_label },
                { ratios, ratios_label },
                { attacks, attacks_label },
                { releases, releases_label }
            };

            for (auto &[textedit, label] : textedit_and_label)
            {
                textedit.setMultiLine(false);

                scroller.addAndMakeVisible(textedit);
                scroller.addAndMakeVisible(label);
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
            
            auto configs_to_names = [] (std::vector<CompressorGame_Config> *configs) {
                std::vector<juce::String> config_names{};
                config_names.resize(configs->size());
                auto projection = [] (const CompressorGame_Config& config) { 
                    return config.title; 
                };
                std::transform(configs->begin(), configs->end(), config_names.begin(), projection);
                return config_names;
            };
            config_list_comp.selected_channel_changed_callback = [&](int config_idx) {
                selectConfig(config_idx);
            };
        
            config_list_comp.create_channel_callback = [&, configs_to_names](juce::String new_config_name) {
                configs->push_back(compressor_game_config_default(new_config_name));
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.delete_channel_callback = [&, configs_to_names](int row_to_delete) {
                configs->erase(configs->begin() + row_to_delete);
                config_list_comp.update(configs_to_names(configs));
            };

            config_list_comp.rename_channel_callback = [&, configs_to_names](int row_idx, juce::String new_config_name) {
                configs->at(row_idx).title = new_config_name;
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
        CompressorGame_Config *current_config = &configs->at(*current_config_idx);
        
        current_config->threshold_active = thresholds_label.getToggleState();
        auto thresholds_str = thresholds.getText();
        if (!thresholds_str.isEmpty())
        {
            current_config->threshold_values_db = deserialize_vector<float>(thresholds_str);
        }
    
        current_config->ratio_active = ratios_label.getToggleState();
        auto ratios_str = ratios.getText();
        if (!ratios_str.isEmpty())
        {
            current_config->ratio_values = deserialize_vector<float>(ratios_str);
        }

        current_config->attack_active = attacks_label.getToggleState();
        auto attacks_str = attacks.getText();
        if (!attacks_str.isEmpty())
        {
            current_config->attack_values = deserialize_vector<float>(attacks_str);
        }
        
        current_config->release_active = releases_label.getToggleState();
        auto releases_str = releases.getText();
        if (!releases_str.isEmpty())
        {
            current_config->release_values = deserialize_vector<float>(releases_str);
        }

        bool is_config_valid = compressor_game_config_validate(current_config);
        nextButton.setEnabled(is_config_valid);

        selectConfig(*current_config_idx);
    }

    void selectConfig(size_t new_config_idx)
    {
        assert(new_config_idx < configs->size());
        *current_config_idx = new_config_idx;
        CompressorGame_Config *current_config = &configs->at(*current_config_idx);
      
        thresholds_label.setToggleState(current_config->threshold_active, juce::dontSendNotification);
        ratios_label.setToggleState(current_config->ratio_active, juce::dontSendNotification);
        attacks_label.setToggleState(current_config->attack_active, juce::dontSendNotification);
        releases_label.setToggleState(current_config->release_active, juce::dontSendNotification);

        thresholds.setText(serialize_vector<float>(current_config->threshold_values_db), juce::dontSendNotification);
        ratios.setText(serialize_vector<float>(current_config->ratio_values), juce::dontSendNotification);
        attacks.setText(serialize_vector<float>(current_config->attack_values), juce::dontSendNotification);
        releases.setText(serialize_vector<float>(current_config->release_values), juce::dontSendNotification);
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
        int textedit_height = 25;

        thresholds_label.setBounds(bounds(label_height));
        thresholds.setBounds(bounds(textedit_height));
        
        ratios_label.setBounds(bounds(label_height));
        ratios.setBounds(bounds(textedit_height));
        
        attacks_label.setBounds(bounds(label_height));
        attacks.setBounds(bounds(textedit_height));
        
        releases_label.setBounds(bounds(label_height));
        releases.setBounds(bounds(textedit_height));
        
#if 0
        auto prelisten_top_bounds = bounds();
        prelisten_type_label.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_type.setBounds(prelisten_top_bounds.removeFromLeft(80));
        prelisten_timeout_ms.setBounds(bounds());
#endif
        return total_height;
    }

    std::vector<CompressorGame_Config> *configs;
    size_t *current_config_idx;
    Insertable_List config_list_comp;
    
    GameUI_Header header;

    juce::TextEditor thresholds;
    juce::ToggleButton thresholds_label { "Thresholds" };

    juce::TextEditor ratios;
    juce::ToggleButton ratios_label { "Ratios" };
    
    juce::TextEditor attacks;
    juce::ToggleButton attacks_label { "Attacks" };

    juce::TextEditor releases;
    juce::ToggleButton releases_label { "Releases" };
    
    juce::Viewport viewport;
    juce::Component scroller;
   
    juce::TextButton nextButton { "Next" };
   
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Compressor_Config_Panel)
};