
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
    struct {
#if 0
        bool display_target;
        int target_compressor;
        bool is_cursor_locked;
        int locked_cursor_compressor;
        bool display_window;
        float correct_answer_window;
#endif
    } comp_widget;
    CompressorGame_Results results;
    juce::String header_text;
    int score;
    Mix mix;
    bool display_button;
    juce::String button_text;
    Event_Type button_event;
};

struct Compressor_Game_Effects {
    int error;
    std::optional < Effect_Transition> transition;
    std::optional < Effect_DSP_Single_Track > dsp;
    std::optional < Effect_Player > player;
    std::optional < Compressor_Game_Effect_UI > ui;
    std::optional < CompressorGame_Results > results;
    bool quit;
};
struct CompressorGame_Config
{
    juce::String title;
    //compressor
    //
    PreListen_Type prelisten_type;
    int prelisten_timeout_ms;

    bool question_timeout_enabled;
    int question_timeout_ms;

    bool result_timeout_enabled;
    int result_timeout_ms;
};

using compressor_game_observer_t = std::function<void(const Compressor_Game_Effects&)>;

struct CompressorGame_IO
{
    Timer timer;
    std::mutex update_fn_mutex;
    std::vector<compressor_game_observer_t> observers;
    std::function < void() > on_quit;
};

struct CompressorGame_State
{
    GameStep step;
    int score;
    int lives;
    //compressor
    int target_compressor;
    float correct_answer_window;
    //
    int current_file_idx;
    std::vector<Audio_File> files;
    CompressorGame_Config config;
    CompressorGame_Results results;

    juce::int64 timestamp_start;
    juce::int64 current_timestamp;
};

struct CompressorGame_UI;


CompressorGame_Config compressor_game_config_default(juce::String name);
std::unique_ptr<CompressorGame_State> compressor_game_state_init(CompressorGame_Config config, std::vector<Audio_File> files);
std::unique_ptr<CompressorGame_IO> compressor_game_io_init();
void compressor_game_add_observer(CompressorGame_IO *io, compressor_game_observer_t observer);

void compressor_game_post_event(CompressorGame_State *state, CompressorGame_IO *io, Event event);
Compressor_Game_Effects compressor_game_update(CompressorGame_State *state, Event event);
void compressor_game_ui_transitions(CompressorGame_UI &ui, Effect_Transition transition);
void compressor_game_ui_update(CompressorGame_UI &ui, const Compressor_Game_Effect_UI &new_ui);
#if 0
void compressor_widget_update(CompressorWidget *widget, const Compressor_Game_Effect_UI &new_ui);
#endif
struct CompressorGame_UI : public juce::Component
{
    
    CompressorGame_UI(
#if 0
        CompressorGame_State *game_state, 
        CompressorGame_IO *game_io
#endif
    ) 
#if 0
    : 
        game_io(game_io),
        game_state(game_state)
#endif
    {
#if 0
        bottom.onNextClicked = [game_state, game_io] (Event_Type e){
            Event event = {
                .type = e
            };
            compressor_game_post_event(game_state, game_io, event);
        };
        header.onBackClicked = [game_state, game_io] {
            Event event = {
                .type = Event_Click_Back
            };
            //compressor_game_post_event(game_state, game_io, event);
        };
        bottom.onToggleClicked = [game_state, game_io] (bool a){
            Event event = {
                .type = Event_Toggle_Input_Target,
                .value_b = a
            };
            compressor_game_post_event(game_state, game_io, event);
        };
#endif
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
#if 0
        if(compressor_widget)
            compressor_widget->setBounds(game_bounds);
        else if(results_panel)
            results_panel->setBounds(game_bounds);
#endif
        bottom.setBounds(bottom_bounds);
    }

    GameUI_Header header;
#if 0
    std::unique_ptr<CompressorWidget> compressor_widget;
    std::unique_ptr<CompressorGame_Results_Panel> results_panel;
#endif
    GameUI_Bottom bottom;
#if 0
    CompressorGame_IO *game_io;
    CompressorGame_State *game_state;
#endif
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorGame_UI)
};
