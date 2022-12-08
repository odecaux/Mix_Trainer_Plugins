
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

enum PreListen_Type
{
   PreListen_None = 0,
   PreListen_Timeout = 1,
   PreListen_Free = 2
};

struct FrequencyGame_Config
{
    juce::String title;
    float eq_gain;
    float eq_quality;
    float initial_correct_answer_window;
    PreListen_Type prelisten_type;
    int prelisten_timeout_ms;
    bool question_timeout_enabled;
    int question_timeout_ms;
    bool result_timeout_enabled;
    int result_timeout_ms;
};


struct Audio_File
{
    juce::File file;
    juce::String title;
    juce::Range<juce::int64> loop_bounds;
    juce::Range<int> freq_bounds;
};

using observer_t = std::function<void(const Effects &)>;

struct FrequencyGame_IO
{
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<observer_t> observers;
    std::function < void() > on_quit;
};

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
    juce::int64 current_timestamp;
};

struct FrequencyGame_UI;


FrequencyGame_Config frequency_game_config_default(juce::String name);
std::unique_ptr<FrequencyGame_State> frequency_game_state_init(FrequencyGame_Config config, std::vector<Audio_File> files);
std::unique_ptr<FrequencyGame_IO> frequency_game_io_init();
void frequency_game_add_observer(FrequencyGame_IO *io, observer_t observer);

void frequency_game_post_event(FrequencyGame_State *state, FrequencyGame_IO *io, Event event);
Effects frequency_game_update(FrequencyGame_State *state, Event event);
void frequency_game_ui_transitions(FrequencyGame_UI &ui, Effect_Transition transition);
void frequency_game_ui_update(FrequencyGame_UI &ui, const Effect_UI &new_ui);
void frequency_widget_update(FrequencyWidget *widget, const Effect_UI &new_ui);

struct FrequencyGame_UI : public juce::Component
{
    
    FrequencyGame_UI(FrequencyGame_State *gameState, FrequencyGame_IO *gameIO) : 
        game_io(gameIO),
    game_state(gameState)
    {
        bottom.onNextClicked = [this] (Event_Type e){
            Event event = {
                .type = e
            };
            frequency_game_post_event(game_state, game_io, event);
        };
        header.onBackClicked = [this] {
            Event event = {
                .type = Event_Click_Back
            };
            frequency_game_post_event(game_state, game_io, event);
        };
        bottom.onToggleClicked = [this] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            frequency_game_post_event(game_state, game_io, event);
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
    FrequencyGame_IO *game_io;
    FrequencyGame_State *game_state;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGame_UI)
};

