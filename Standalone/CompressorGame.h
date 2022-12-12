
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
    juce::String header_text;
    int score;
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
    FaderStep fader_step;
    Mix mix;
    bool display_button;
    juce::String button_text;
    Event_Type button_event;
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
};


struct CompressorGame_State
{
    GameStep step;
    Mix mix;
    int score;
    int lives;
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
    
    CompressorGame_UI(CompressorGame_IO *gameIO) : game_io(gameIO)
    {
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

        addAndMakeVisible(bottom);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds().reduced(4);
        auto bottom_height = 50;

        auto header_bounds = bounds.removeFromTop(game_ui_header_height);
        auto bottom_bounds = bounds.removeFromBottom(bottom_height);
        auto game_bounds = juce::Rectangle < int > (200, 200); 
        game_bounds.setCentre(bounds.getCentre());

        header.setBounds(header_bounds);
        
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

        bottom.setBounds(bottom_bounds);
    }

    GameUI_Header header;
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
