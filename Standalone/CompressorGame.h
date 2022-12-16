
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
    CompressorGame_Results results;
    juce::String header_center_text;
    juce::String header_right_text;
    //int score;
    struct {
        int threshold_pos;
        int ratio_pos;
        int attack_pos;
        int release_pos;
        
        std::vector < float > threshold_values_db;
        std::vector < float > ratio_values;
        std::vector < float > attack_values;
        std::vector < float > release_values;
    } comp_widget;
    Widget_Interaction_Type widget_visibility;
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
    std::vector < float > threshold_values_db;
    std::vector < float > ratio_values;
    std::vector < float > attack_values;
    std::vector < float > release_values;
    //
    Compressor_Game_Variant variant;
    int listens;
    int timeout_ms;
    int total_rounds;
};

static inline CompressorGame_Config compressor_game_config_validate(CompressorGame_Config config)
{
    auto sort_clamp_and_filter = [] (std::vector<float> &vec, float min, float max)
    {
        std::sort(vec.begin(), vec.end());
        auto clamping = [min, max] (float in)
        {
            return std::clamp(in, min, max);
        };
        std::transform(vec.begin(), vec.end(), vec.begin(), clamping);
        vec.erase( std::unique( vec.begin(), vec.end() ), vec.end() );
    };
    sort_clamp_and_filter(config.threshold_values_db, -90, 0);
    sort_clamp_and_filter(config.ratio_values, 1, 10);
    sort_clamp_and_filter(config.attack_values, 1, 1000);
    sort_clamp_and_filter(config.release_values, 1, 1000);
    return config;
}


struct CompressorGame_State
{
    GameStep step;
    Mix mix;
    int score;
    //compressor
    int target_threshold_pos;
    int target_ratio_pos;
    int target_attack_pos;
    int target_release_pos;

    int input_threshold_pos;
    int input_ratio_pos;
    int input_attack_pos;
    int input_release_pos;

    //
    int current_file_idx;
    std::vector<Audio_File> files;
    CompressorGame_Config config;
    CompressorGame_Results results;
    
    int remaining_listens;
    bool can_still_listen;
    juce::int64 timestamp_start;
    juce::int64 current_timestamp;
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

using compressor_game_observer_t = std::function<void(const Compressor_Game_Effects&)>;

struct CompressorGame_IO
{
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<compressor_game_observer_t> observers;
    std::function < void() > on_quit;
    CompressorGame_State game_state;
};

struct CompressorGame_UI;

CompressorGame_Config compressor_game_config_default(juce::String name);
CompressorGame_State compressor_game_state_init(CompressorGame_Config config, std::vector<Audio_File> files);
std::unique_ptr<CompressorGame_IO> compressor_game_io_init(CompressorGame_State state);
void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer);

void compressor_game_post_event(CompressorGame_IO *io, Event event);
Compressor_Game_Effects compressor_game_update(CompressorGame_State state, Event event);
void compressor_game_ui_transitions(CompressorGame_UI &ui, Effect_Transition transition);
void compressor_game_ui_update(CompressorGame_UI &ui, const Compressor_Game_Effect_UI &new_ui);
#if 0
void compressor_widget_update(CompressorWidget *widget, const Compressor_Game_Effect_UI &new_ui);
#endif


struct CompressorGame_UI : public juce::Component
{
    
    CompressorGame_UI(CompressorGame_IO *gameIO)
    : game_io(gameIO),
      previewer_file_list(false)
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
        
        auto setup_slider = [&] (TextSlider &slider, juce::Label &label, juce::String name, std::function<void(int)> onEdit) {
            label.setText(name, juce::NotificationType::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, slider.getTextBoxWidth(), 15);
            slider.setScrollWheelEnabled(true);
        
            slider.onValueChange = [&slider, onEdit = std::move(onEdit)] {
                onEdit((int)slider.getValue());
            };
            addAndMakeVisible(slider);
        };
        
        auto onThresholdMoved = [io = this->game_io] (int new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 0,
                .value_i = new_pos
            };
            compressor_game_post_event(io, event);
        };

        setup_slider(threshold_slider, threshold_label, "Threshold", std::move(onThresholdMoved));
        
        auto onRatioMoved = [io = this->game_io] (int new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 1,
                .value_i = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(ratio_slider, ratio_label, "Ratio", std::move(onRatioMoved));
        
        auto onAttackMoved = [io = this->game_io] (int new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 2,
                .value_i = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(attack_slider, attack_label, "Attack", std::move(onAttackMoved));
        
        auto onReleaseMoved = [io = this->game_io] (int new_pos) {
            Event event = {
                .type = Event_Slider,
                .id = 3,
                .value_i = new_pos
            };
            compressor_game_post_event(io, event);
        };
        setup_slider(release_slider, release_label, "Release", std::move(onReleaseMoved));
        
        bottom.target_mix_button.setButtonText("Target settings");
        bottom.user_mix_button.setButtonText("Your settings");
        addAndMakeVisible(bottom);
    }
    
    void parentHierarchyChanged() override
    {
        auto * top_level_window = getTopLevelComponent();
        auto * resizable_window = dynamic_cast<juce::ResizableWindow*>(top_level_window);
        assert(resizable_window);
        auto min_height = 8 + header.getHeight() + bottom.getHeight() + game_bounds_dim.getHeight() + 40; //TODO hack ? why doesn't it work ?
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
        auto game_bounds = game_bounds_dim.withCentre(bounds.getCentre());
        if (previewer_file_list.isVisible())
        {
            game_bounds.setX(bounds.getRight() - game_bounds.getWidth());
            auto previewer_bounds = game_bounds;
            previewer_bounds.setX(0);
            previewer_bounds.setRight(game_bounds.getX());
            previewer_file_list.setBounds(previewer_bounds);
        }
        auto threshold_bounds = game_bounds.getProportion<float>( { 0.0f, 0.0f, 0.5f, 0.5f }).reduced(5);
        auto ratio_bounds = game_bounds.getProportion<float>( { 0.5f, 0.0f, 0.5f, 0.5f }).reduced(5);
        auto attack_bounds = game_bounds.getProportion<float>( { 0.0f, 0.5f, 0.5f, 0.5f }).reduced(5);
        auto release_bounds = game_bounds.getProportion<float>( { 0.5f, 0.5f, 0.5f, 0.5f }).reduced(5);

        threshold_label.setBounds(threshold_bounds.removeFromTop(15));
        ratio_label.setBounds(ratio_bounds.removeFromTop(15));
        attack_label.setBounds(attack_bounds.removeFromTop(15));
        release_label.setBounds(release_bounds.removeFromTop(15));
        
        threshold_slider.setBounds(threshold_bounds);
        ratio_slider.setBounds(ratio_bounds);
        attack_slider.setBounds(attack_bounds);
        release_slider.setBounds(release_bounds);
    }

    GameUI_Header header;
    Selection_List previewer_file_list;
    juce::Rectangle < int > game_bounds_dim { 0, 0, 200, 200 };
#if 0
    std::unique_ptr<CompressorWidget> compressor_widget;
    std::unique_ptr<CompressorGame_Results_Panel> results_panel;
#endif
    TextSlider threshold_slider;
    TextSlider ratio_slider;
    TextSlider attack_slider;
    TextSlider release_slider;
    
    juce::Label threshold_label;
    juce::Label ratio_label;
    juce::Label attack_label;
    juce::Label release_label;

#if 0
    std::vector < float > threshold_values;
    std::vector < float > ratio_values;
    std::vector < float > attack_values;
    std::vector < float > release_values;
#endif
    GameUI_Bottom bottom;
    CompressorGame_IO *game_io;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGame_UI)
};



//------------------------------------------------------------------------
struct Compressor_Config_Panel : public juce::Component
{
    using textedit_and_label_t = std::vector<std::tuple<juce::TextEditor&, juce::Label&>>;
    using textedit_and_toggle_t = std::vector<std::tuple<juce::TextEditor&, juce::ToggleButton&>>;

    Compressor_Config_Panel(std::vector<CompressorGame_Config> &gameConfigs,
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
            thresholds.onReturnKey = [&] {
                juce::StringArray tokens = juce::StringArray::fromTokens(thresholds.getText(), false);
                if (tokens.isEmpty())
                {
                    selectConfig(current_config_idx);
                    return;
                }
                CompressorGame_Config &current_config = configs[current_config_idx];            
                current_config.threshold_values_db.clear();
                for (const auto& token : tokens)
                {
                    current_config.threshold_values_db.emplace_back(token.getFloatValue());
                }
                current_config = compressor_game_config_validate(current_config);
                selectConfig(current_config_idx);
            };
            
            ratios.onReturnKey = [&] {
                juce::StringArray tokens = juce::StringArray::fromTokens(ratios.getText(), false);
                if (tokens.isEmpty())
                {
                    selectConfig(current_config_idx);
                    return;
                }
                CompressorGame_Config &current_config = configs[current_config_idx];            
                current_config.ratio_values.clear();
                for (const auto& token : tokens)
                {
                    current_config.ratio_values.emplace_back(token.getFloatValue());
                }
                current_config = compressor_game_config_validate(current_config);
                selectConfig(current_config_idx);
            };

            
            attacks.onReturnKey = [&] {
                juce::StringArray tokens = juce::StringArray::fromTokens(attacks.getText(), false);
                if (tokens.isEmpty())
                {
                    selectConfig(current_config_idx);
                    return;
                }
                CompressorGame_Config &current_config = configs[current_config_idx];            
                current_config.attack_values.clear();
                for (const auto& token : tokens)
                {
                    current_config.attack_values.emplace_back(token.getFloatValue());
                }
                current_config = compressor_game_config_validate(current_config);
                selectConfig(current_config_idx);
            };

            releases.onReturnKey = [&] {
                juce::StringArray tokens = juce::StringArray::fromTokens(releases.getText(), false);
                if (tokens.isEmpty())
                {
                    selectConfig(current_config_idx);
                    return;
                }
                CompressorGame_Config &current_config = configs[current_config_idx];            
                current_config.release_values.clear();
                for (const auto& token : tokens)
                {
                    current_config.release_values.emplace_back(token.getFloatValue());
                }
                current_config = compressor_game_config_validate(current_config);
                selectConfig(current_config_idx);
            };
        
            textedit_and_label_t textedit_and_label = {
                { thresholds, thresholds_label },
                { ratios, ratios_label },
                { attacks, attacks_label },
                { releases, releases_label }
            };

            for (auto &[textedit, label] : textedit_and_label)
            {
                textedit.setMultiLine(false);
                textedit.onEscapeKey = [&] {
                    selectConfig(current_config_idx);
                };
                //textedit.setScrollWheelEnabled(false);
                //textedit.setTextBoxStyle(juce::Slider::TextBoxLeft, true, 50, 20);
                //textedit.setRange(range, interval);

                label.setBorderSize(juce::BorderSize < int > { 0 });
                label.setJustificationType(juce::Justification::left);
            
                scroller.addAndMakeVisible(textedit);
                scroller.addAndMakeVisible(label);
            }

            textedit_and_toggle_t textedit_and_toggle = {
               // { question_timeout_ms , result_timeout_enabled, { 1000, 4000 }, 500 },
               // { result_timeout_ms , question_timeout_enabled, { 1000, 4000 }, 500 }
            };

        
            for (auto &[textedit, toggle] : textedit_and_toggle)
            {
                textedit.setMultiLine(false);
                textedit.setJustification(juce::Justification::centredLeft);
                //toggle.setBorderSize( juce::BorderSize<int>{ 0 });
                //toggle.setJustificationType(juce::Justification::left);
            
                scroller.addAndMakeVisible(textedit);
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
            
            auto configs_to_names = [] (const std::vector<CompressorGame_Config>& configs) {
                std::vector<juce::String> config_names{};
                config_names.resize(configs.size());
                auto projection = [] (const CompressorGame_Config& config) { 
                    return config.title; 
                };
                std::transform(configs.begin(), configs.end(), config_names.begin(), projection);
                return config_names;
            };
            config_list_comp.selected_channel_changed_callback = [&](int config_idx) {
                selectConfig(config_idx);
            };
        
            config_list_comp.create_channel_callback = [&, configs_to_names](juce::String new_config_name) {
                configs.push_back(compressor_game_config_default(new_config_name));
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
        CompressorGame_Config &current_config = configs[current_config_idx];
      
        {
            juce::String thresholds_str{};
            for (const float threshold : current_config.threshold_values_db)
            {
                thresholds_str += juce::String(threshold) + " ";
            }
            thresholds_str = thresholds_str.dropLastCharacters(1);
            thresholds.setText(thresholds_str, juce::dontSendNotification);
        }
        
        {
            juce::String ratios_str{};
            for (const float ratio : current_config.ratio_values)
            {
                ratios_str += juce::String(ratio) + " ";
            }
            ratios_str = ratios_str.dropLastCharacters(1);
            ratios.setText(ratios_str, juce::dontSendNotification);
        }
        
        {
            juce::String attacks_str{};
            for (const float attack : current_config.attack_values)
            {
                attacks_str += juce::String(attack) + " ";
            }
            attacks_str = attacks_str.dropLastCharacters(1);
            attacks.setText(attacks_str, juce::dontSendNotification);
        }
        
        {
            juce::String releases_str{};
            for (const float release : current_config.release_values)
            {
                releases_str += juce::String(release) + " ";
            }
            releases_str = releases_str.dropLastCharacters(1);
            releases.setText(releases_str, juce::dontSendNotification);
        }
#if 0
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
#endif
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

    std::vector<CompressorGame_Config> &configs;
    size_t &current_config_idx;
    Insertable_List config_list_comp;
    
    GameUI_Header header;

    juce::TextEditor thresholds;
    juce::Label thresholds_label { {}, "Thresholds" };

    juce::TextEditor ratios;
    juce::Label ratios_label { {}, "Ratios" };
    
    juce::TextEditor attacks;
    juce::Label attacks_label { {}, "Attacks" };

    juce::TextEditor releases;
    juce::Label releases_label { {}, "Releases" };
    
#if 0
    juce::ComboBox prelisten_type;
    juce::Label prelisten_type_label { {}, "Pre-Listen : " };
    juce::Slider prelisten_timeout_ms;

    juce::ToggleButton question_timeout_enabled { "Question timeout" };
    juce::Slider question_timeout_ms;

    juce::ToggleButton result_timeout_enabled { "Post answer timeout" };
    juce::Slider result_timeout_ms;
#endif
    juce::Viewport viewport;
    juce::Component scroller;
   
    juce::TextButton nextButton { "Next" };
   
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Compressor_Config_Panel)
};